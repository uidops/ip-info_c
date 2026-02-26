/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, uidops
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This program requires libmaxminddb and the GeoLite2 dataset files to be present
 * under the `dataset/` directory next to the project's root. This file makes
 * MaxMind usage mandatory: the program will attempt to open
 *    dataset/GeoLite2-City.mmdb
 *    dataset/GeoLite2-ASN.mmdb
 *
 * If you don't have libmaxminddb installed, install the development package for
 * your platform and link with -lmaxminddb. The Makefile should be updated to
 * add -lmaxminddb. json-c is also used for parsing the ipify JSON response.
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <json-c/json.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <maxminddb.h>

#define HOST_IPIFY "api.ipify.org"
#define IPIFY_PATH  "/?format=json"

#define DATASET_CITY "dataset/GeoLite2-City.mmdb"
#define DATASET_ASN  "dataset/GeoLite2-ASN.mmdb"

#define KNRM  "\x1B[0m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"

static long response_code(const char *resp);
static char *fetch_url(const char *host, const char *path);
static char *fetch_public_ip(void);
static char *mmdb_entry_data_strdup(MMDB_entry_data_s *d);
static int lookup_city_mmdb(const char *city_db_path, const char *ip, char **country, char **city, char **timezone);
static char *lookup_isp_mmdb(const char *asn_db_path, const char *ip);
/* Resolve a hostname to an IPv4 address string (heap-allocated, caller must free).
 * If the input is already an IPv4 literal, returns a strdup of it. Returns NULL on failure.
 */
static char *resolve_host_to_ip(const char *host);

/* send a simple HTTP GET over IPv4 and return body (caller frees) */
char *fetch_url(const char *host, const char *path)
{
    if (!host || !path) return NULL;

    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    int sockfd = -1;
    char *req = NULL;
    char respbuf[32768];
    size_t total = 0;
    int retries = 0;
    struct timespec timeout = {0, 100000000L};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0)
        return NULL;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return NULL;

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return NULL;
    }

    const char fmt[] = "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: ip-info-c/1.0\r\nConnection: close\r\n\r\n";
    size_t rlen = strlen(fmt) + strlen(host) + strlen(path) + 16;
    req = calloc(1, rlen);
    if (!req) { close(sockfd); return NULL; }
    snprintf(req, rlen, fmt, path, host);

    ssize_t s = send(sockfd, req, strlen(req), 0);
    free(req);
    if (s == -1) { close(sockfd); return NULL; }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    while (total < sizeof(respbuf) - 1) {
        ssize_t r = recv(sockfd, respbuf + total, sizeof(respbuf) - 1 - total, 0);
        if (r == -1) {
            if (retries++ > 40) break;
            nanosleep(&timeout, NULL);
            continue;
        } else if (r == 0) {
            break;
        } else {
            total += (size_t)r;
            retries = 0;
        }
    }

    if (total == 0) { close(sockfd); return NULL; }

    respbuf[total] = '\0';

    long status = response_code(respbuf);
    if (status != 200) { close(sockfd); return NULL; }

    char *hdr = strstr(respbuf, "\r\n\r\n");
    if (!hdr) { close(sockfd); return NULL; }
    hdr += 4;
    char *body = strdup(hdr);
    close(sockfd);
    return body;
}

/* parse HTTP status code */
long response_code(const char *resp)
{
    if (!resp) return -1;
    char *tmp = strdup(resp);
    if (!tmp) return -1;
    char *saveptr = NULL;
    char *tok = strtok_r(tmp, " ", &saveptr);
    if (!tok) { free(tmp); return -1; }
    tok = strtok_r(NULL, " ", &saveptr);
    if (!tok) { free(tmp); return -1; }
    long code = strtol(tok, NULL, 10);
    free(tmp);
    if (code <= 0) return -1;
    return code;
}

/* fetch public IP from ipify.org (JSON) */
char *fetch_public_ip(void)
{
    char *body = fetch_url(HOST_IPIFY, IPIFY_PATH);
    if (!body) return NULL;

    struct json_object *obj = json_tokener_parse(body);
    free(body);
    if (!obj) return NULL;

    struct json_object *ipobj = json_object_object_get(obj, "ip");
    const char *ipstr = ipobj ? json_object_get_string(ipobj) : NULL;
    char *ret = ipstr ? strdup(ipstr) : NULL;
    json_object_put(obj);
    return ret;
}

/* duplicate MMDB string entry_data into heap (caller frees) */
char *mmdb_entry_data_strdup(MMDB_entry_data_s *d)
{
    if (!d || !d->has_data) return NULL;
    if (d->type != MMDB_DATA_TYPE_UTF8_STRING) return NULL;
    size_t n = (size_t)d->data_size;
    char *s = malloc(n + 1);
    if (!s) return NULL;
    memcpy(s, d->utf8_string, n);
    s[n] = '\0';
    return s;
}

/* Resolve a hostname to an IPv4 address string (heap-allocated, caller must free).
 * If the input is already an IPv4 literal, returns a strdup of it. Returns NULL on failure.
 * This prefers the first IPv4 address returned by getaddrinfo.
 */
static char *
resolve_host_to_ip(const char *host)
{
    if (!host) return NULL;

    /* Quick check: if host looks like an IPv4 literal, accept it */
    struct in_addr in;
    if (inet_pton(AF_INET, host, &in) == 1) {
        return strdup(host);
    }

    struct addrinfo hints, *res = NULL, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* prefer IPv4 */
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        return NULL;
    }

    char buf[INET_ADDRSTRLEN];
    char *ret = NULL;
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
            if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf)) != NULL) {
                ret = strdup(buf);
                break;
            }
        }
    }

    freeaddrinfo(res);
    return ret;
}

/* Lookup city/country/timezone in provided City DB. Returns 0 on success; outputs heap strings in country/city/timezone (caller frees). */
int lookup_city_mmdb(const char *city_db_path, const char *ip, char **country, char **city, char **timezone)
{
    if (!city_db_path || !ip || !country || !city || !timezone) return -1;

    *country = NULL;
    *city = NULL;
    *timezone = NULL;

    MMDB_s mmdb;
    int status = MMDB_open(city_db_path, MMDB_MODE_MMAP, &mmdb);
    if (status != MMDB_SUCCESS) {
        return -1;
    }

    int gai_error = 0, mmdb_error = 0;
    MMDB_lookup_result_s result = MMDB_lookup_string(&mmdb, ip, &gai_error, &mmdb_error);
    if (!result.found_entry) {
        MMDB_close(&mmdb);
        return -1;
    }

    MMDB_entry_data_s entry;
    if (MMDB_get_value(&result.entry, &entry, "country", "names", "en", NULL) == MMDB_SUCCESS && entry.has_data) {
        *country = mmdb_entry_data_strdup(&entry);
    }
    if (MMDB_get_value(&result.entry, &entry, "city", "names", "en", NULL) == MMDB_SUCCESS && entry.has_data) {
        *city = mmdb_entry_data_strdup(&entry);
    }
    if (MMDB_get_value(&result.entry, &entry, "location", "time_zone", NULL) == MMDB_SUCCESS && entry.has_data) {
        *timezone = mmdb_entry_data_strdup(&entry);
    }

    MMDB_close(&mmdb);
    return ((*country || *city || *timezone) ? 0 : -1);
}

/* Lookup ASN/ISP in provided ASN DB. Returns heap-allocated string or NULL. */
char *lookup_isp_mmdb(const char *asn_db_path, const char *ip)
{
    if (!asn_db_path || !ip) return NULL;

    MMDB_s mmdb;
    if (MMDB_open(asn_db_path, MMDB_MODE_MMAP, &mmdb) != MMDB_SUCCESS) {
        return NULL;
    }

    int gai_error = 0, mmdb_error = 0;
    MMDB_lookup_result_s result = MMDB_lookup_string(&mmdb, ip, &gai_error, &mmdb_error);
    if (!result.found_entry) {
        MMDB_close(&mmdb);
        return NULL;
    }

    MMDB_entry_data_s entry;
    char *isp = NULL;

    if (MMDB_get_value(&result.entry, &entry, "autonomous_system_organization", NULL) == MMDB_SUCCESS && entry.has_data) {
        isp = mmdb_entry_data_strdup(&entry);
    }

    if (!isp) {
        if (MMDB_get_value(&result.entry, &entry, "autonomous_system_number", NULL) == MMDB_SUCCESS && entry.has_data) {
            if (entry.type == MMDB_DATA_TYPE_UINT32 || entry.type == MMDB_DATA_TYPE_UINT16 || entry.type == MMDB_DATA_TYPE_UINT64) {
                uint64_t asn = entry.uint64;
                char buf[64];
                snprintf(buf, sizeof(buf), "AS%llu", (unsigned long long)asn);
                isp = strdup(buf);
            } else if (entry.type == MMDB_DATA_TYPE_UTF8_STRING || entry.type == MMDB_DATA_TYPE_UTF8_STRING) {
                char *s = mmdb_entry_data_strdup(&entry);
                if (s) {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "AS%s", s);
                    isp = strdup(buf);
                    free(s);
                }
            }
        }
    }

    MMDB_close(&mmdb);
    return isp;
}

int main(int argc, char **argv)
{
    char *ip = NULL;
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        /* Resolve host to an IPv4 address (or accept IPv4 literal) before lookup */
        ip = resolve_host_to_ip(argv[1]);
        if (!ip) {
            errx(EXIT_FAILURE, "Failed to resolve host '%s' to an IP address", argv[1]);
        }
    } else {
        ip = fetch_public_ip();
        if (!ip) {
            errx(EXIT_FAILURE, "Failed to obtain public IP (ipify request failed)");
        }
    }

    /* Always use dataset files in dataset/ (required) */
    const char *city_db = DATASET_CITY;
    const char *asn_db  = DATASET_ASN;

    /* Ensure DB files are present and readable */
    if (access(city_db, R_OK) != 0) {
        errx(EXIT_FAILURE, "City DB not found or unreadable at '%s'. Please install GeoLite2-City.mmdb into dataset/", city_db);
    }
    if (access(asn_db, R_OK) != 0) {
        errx(EXIT_FAILURE, "ASN DB not found or unreadable at '%s'. Please install GeoLite2-ASN.mmdb into dataset/", asn_db);
    }

    char *country = NULL, *city = NULL, *timezone = NULL, *isp = NULL;

    int city_ok = lookup_city_mmdb(city_db, ip, &country, &city, &timezone);
    isp = lookup_isp_mmdb(asn_db, ip);

    /* Determine overall success: if either city lookup or isp lookup succeeded */
    int success = (city_ok == 0) || (isp != NULL);

    printf("%s- %sStatus%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, success ? "Success" : "Failed", KNRM);

    if (country && strlen(country))
        printf("%s* %sCountry%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, country, KNRM);
    else
        printf("%s* %sCountry%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, "Unknown", KNRM);

    if (city && strlen(city))
        printf("%s- %sCity%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, city, KNRM);
    else
        printf("%s- %sCity%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, "Unknown", KNRM);

    if (timezone && strlen(timezone))
        printf("%s* %sTimezone%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, timezone, KNRM);
    else
        printf("%s* %sTimezone%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, "Unknown", KNRM);

    if (isp && strlen(isp))
        printf("%s- %sISP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, isp, KNRM);
    else
        printf("%s- %sISP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, "Unavailable", KNRM);

    if (ip)
        printf("%s* %sIP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, ip, KNRM);
    else
        printf("%s* %sIP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, "Unknown", KNRM);

    free(country);
    free(city);
    free(timezone);
    free(isp);
    free(ip);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
