/* 
 * (https://github.com/siruidops/ip-info_c).
 * Copyright (c) 2021 uidops.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <errno.h> /* system error numbers. */
#include <string.h> /* string operations. */
#include <stdio.h> /* standard buffered input/output. */
#include <stdlib.h> /* standard library definitions. */
#include <unistd.h> /* standard symbolic constants and types. */
#include <sys/socket.h> /* main sockets header. */
#include <netdb.h> /* definitions for network database operations. */
#include <arpa/inet.h> /* definitions for internet operations. */
#include <err.h> /* formatted error messages. */
#include "jsmn.h" /* JSON parser/tokenizer. */

#define host "ip-api.com" /* api address for get data. */

#define KNRM  "\x1B[0m" /* normal */
#define KGRN  "\x1B[32m" /* green */
#define KBLU  "\x1B[34m" /* blue */
#define KMAG  "\x1B[35m" /* magenta */

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }

    return -1;
}

char *get_page(int s, char *ip) {
    char *temp, *msg, *ww, buf[0x1000+10];
    int numlinem, k=0, bytes;
    
    const char *format = "GET /json/%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (X11; Linux i686; rv:85.0) Gecko/20100101 Firefox/85.0.\r\n\r\n"; /* http header for send. */
    int send_handler = asprintf(&msg, format, ip, host);

    send_handler = send(s, msg, strlen(msg), 0); /* Send header. */

    sleep(1); /* Sleep 1sec for fix some bug. */
    bytes = recvfrom(s, buf, 0x1000, 0, 0, 0); /* Recivie data. */
    if (bytes == 0) errx(1, "no data recivied.");
    buf[bytes] = '\0';

    ww = malloc(sizeof(buf)*15); 
    ww = strcpy(ww, buf);
    temp = malloc(sizeof(ww)*15);;

    /* Separate the header from the data. */
    char *content = strstr(ww, "\r\n\r\n");
    if (content == NULL) errx(1, "no header found.");
    content += 4;

    return content;
    
}

int main(int argc, char **argv) {
    int i, r, socket_handler = -1;
    char *ip;
    struct addrinfo hints, *res, *res0;
    jsmn_parser p;
    jsmntok_t t[128]; 

    /* Get ip or host. */
    if (argc < 2) {
        ip = "";
            
    } else {
        ip = argv[1];
    }
    
    // set hints to 0.
    memset(&hints, 0, sizeof(hints));


    hints.ai_family = PF_UNSPEC; /* Accept any protocol family.*/
    hints.ai_socktype = SOCK_STREAM; /* IPv4 */

    int addr = getaddrinfo(host, "http", &hints, &res0); /* convert domain names, hostnames, and IP addresses between human-readable text representations */

    for (res = res0; res; res = res -> ai_next) {
        socket_handler = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol); /* Create socket.*/

        if (connect (socket_handler, res -> ai_addr, res -> ai_addrlen) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            close(socket_handler);
            exit(EXIT_FAILURE);
        }

        break;
    }

    if (socket_handler != -1) {
        char *source = get_page(socket_handler, ip); /* Send header and get data. */
        
        jsmn_init(&p);
        r = jsmn_parse(&p, source, strlen(source), t, sizeof(t) / sizeof(t[0]));
        
        if (r < 0) {
            fprintf(stderr, "Failed to parse JSON\n");
            printf("%s", source);
            
            return EXIT_FAILURE;
        }

        if (r < 1 || t[0].type != JSMN_OBJECT) {
            fprintf(stderr, "Object expected\n");
            printf("%s", source);

            return EXIT_FAILURE;
        }

        for (i = 1; i < r; i++) {
            if (jsoneq(source, &t[i], "status") == 0) {
                printf("%s- %sStatus%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++;
            } else if (jsoneq(source, &t[i], "country") == 0) {
                printf("%s* %sCountry%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++;
            } else if (jsoneq(source, &t[i], "city") == 0) {
                printf("%s- %sCity%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++;
            } else if (jsoneq(source, &t[i], "timezone") == 0) {
                printf("%s* %sTimezone%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++;
            } else if (jsoneq(source, &t[i], "isp") == 0) {
                printf("%s- %sISP%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++; 
            } else if (jsoneq(source, &t[i], "query") == 0) {
                printf("%s* %sIP%s:%s %.*s%s\n", KMAG, KBLU, KMAG, KGRN, t[i + 1].end - t[i + 1].start, source + t[i + 1].start, KNRM);
                i++;
            } else {
                NULL;
            }
        }
    }

    freeaddrinfo(res0); /* The freeaddrinfo() function frees the memory that was allocated for the dynamically allocated linked list res. */
    
    return EXIT_SUCCESS;
}