/*
* BSD 2-Clause License
*
* Copyright (c) 2021, uidops
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
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <json-c/json.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __USER_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif
#include <string.h>
#include <unistd.h>

#define HOST "ipwho.is"

#define KNRM  "\x1B[0m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"

long	response_code(const char *);
char	*get_page(int, char *);

int
main(int argc, char **argv)
{
	int sockfd = -1;
	char *ip;

	int success;
	const char *country;
	const char *city;
	const char *the_timezone;
	const char *isp;
	const char *query;

	struct json_object *connection_obj;
	struct json_object *timezone_obj;

	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_in sock_addr;

	const struct json_object *obj;
	enum json_tokener_error error;

	if (argc < 2)
		ip = "";
	else
		ip = *(argv + 1);

	memset(&sock_addr, 0x00, sizeof(struct sockaddr_in));
	memset(&hints, 0x00, sizeof(struct addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(HOST, NULL, &hints, &res))
		errx(EXIT_FAILURE, "getaddrinfo(): an error occurred");

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(80);
	sock_addr.sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(res);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		err(EXIT_FAILURE, "socket()");

	if (connect(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
		close(sockfd);
		err(EXIT_FAILURE, "connect()");
	}

	char *buffer = get_page(sockfd, ip);

	obj = json_tokener_parse_verbose(buffer, &error);
	if (error != json_tokener_success)
		errx(EXIT_FAILURE, "Buffer not json.");

	success = json_object_get_boolean(json_object_object_get(obj, "success"));
	printf("%s- %sStatus%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, success ? "Success" : "Falied", KNRM);

	country = json_object_get_string(json_object_object_get(obj, "country"));
	printf("%s* %sCountry%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, country, KNRM);

	city = json_object_get_string(json_object_object_get(obj, "city"));
	printf("%s- %sCity%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, city, KNRM);

	timezone_obj = json_object_object_get(obj, "timezone");
	the_timezone = json_object_get_string(json_object_object_get(timezone_obj, "id"));
	printf("%s* %sTimezone%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, the_timezone, KNRM);

	connection_obj = json_object_object_get(obj, "connection");
	isp = json_object_get_string(json_object_object_get(connection_obj, "isp"));
	printf("%s- %sISP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, isp, KNRM);

	query = json_object_get_string(json_object_object_get(obj, "ip"));
	printf("%s* %sIP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, query, KNRM);

	free(buffer);
	close(sockfd);

	return EXIT_SUCCESS;
}

long
response_code(const char *s)
{
	if (s == NULL)
		return -1;

	char *last;
	char *d = strdup(s);
	const char *str = strtok_r(d, " ", &last);
	if (str == NULL)
		goto return_error;

	str = strtok_r(NULL, " ", &last);
	if (str == NULL)
		goto return_error;

	long code = (int) strtol(str, (char **)NULL, 10);
	if (code == LONG_MAX || code == LONG_MIN)
		goto return_error;

	free(d);
	return code;

	return_error:
		free(d);
		return -1;
}

char *
get_page(int sockfd, char *ip)
{
	ssize_t i;
	const char format[] = "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: "
			"Mozilla/5.0 (X11; Linux i686; rv:85.0) Gecko/20100101 Firefox/85.0.\r\nConnection: close\r\n\r\n";
	size_t length = sizeof(format) + sizeof(HOST) + strlen(ip);
	char *msg = calloc(sizeof(char), length+1);
	char buf[4096];
	size_t total = 0;
	int retries = 0;


	if (!snprintf(msg, length, format, ip, HOST))
		errx(EXIT_FAILURE, "sprintf() faild");

	i = send(sockfd, msg, strlen(msg), 0);
	if (i == -1)
		err(EXIT_FAILURE, "send()");

	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	struct timespec timeout = {0, 100000000L};
	while (total < sizeof(buf) - 1) {
		i = recv(sockfd, buf + total, sizeof(buf) - 1 - total, 0);
		if (i == -1) {
			if (retries++ > 20)
				break;
			nanosleep(&timeout, NULL);
		} else if (i == 0) {
			break;
		} else {
			total += i;
			retries = 0;
		}
	}


	if (!total)
		errx(EXIT_FAILURE, "no data recivied.");

	buf[total] = '\0';

	long status_code = response_code(buf);
	if (status_code != 200)
		errx(EXIT_FAILURE, "response code %ld", status_code);

	char *content = strstr(buf, "\r\n\r\n");
	if (!content)
		errx(EXIT_FAILURE, "no header found.");

	content += 4;
	content = strdup(content);

	free(msg);

	return content;
}
