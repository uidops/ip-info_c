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
#include <json-c/json.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HOST "ip-api.com"

#define KNRM  "\x1B[0m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"

char *get_page(int s, char *ip) {
	int		 i;
	char		*msg = calloc(sizeof(char), 1024);
	char		*ww, buf[1024];

	const char		*format = "GET /json/%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: " 
		"Mozilla/5.0 (X11; Linux i686; rv:85.0) Gecko/20100101 Firefox/85.0.\r\n\r\n";

	if (!sprintf(msg, format, ip, HOST))
		errx(EXIT_FAILURE, "sprintf() faild");

	i = send(s, msg, strlen(msg), 0);
	if (i == -1)
		err(EXIT_FAILURE, "send()");
	
	fcntl(s, F_SETFL, O_NONBLOCK);
	while (1) {
		i = recv(s, buf, 1024, 0);
		if (i == -1)
			usleep(100000);
		else
			break;
	}


	if (!i)
		errx(EXIT_FAILURE, "no data recivied.");

	buf[i] = '\0';

	ww = calloc(sizeof(char), strlen(buf)); 
	ww = strcpy(ww, buf);

	char		*content = strstr(ww, "\r\n\r\n");
	if (!content)
		errx(EXIT_FAILURE, "no header found.");

	content += 4;
	content = strdup(content);
	
	free(ww);
	free(msg);
	return content;

}

int main(int argc, char *argv[]) {
	int		 sockfd = -1;
	char		*ip;

	const char		*status, *country, *city, *timezone, *isp, *query;

	struct hostent *h_addr;
	struct sockaddr_in sock_addr;

	struct json_object *obj;
	enum json_tokener_error error;

	if (argc < 2) ip = "";
	else ip = argv[1];

	memset(&sock_addr, 0x00, sizeof(struct sockaddr_in));

	h_addr = gethostbyname(HOST);
	if (h_addr == (struct hostent *)0x00)
		err(EXIT_FAILURE, "gethostbyname()");


	
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(80);
	sock_addr.sin_addr.s_addr = *((unsigned long *)h_addr->h_addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		err(EXIT_FAILURE, "socket()");

	if (connect(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
		close(sockfd);
		err(EXIT_FAILURE, "connect()");	
	}

	char		*buffer = get_page(sockfd, ip);
	
	obj = json_tokener_parse_verbose(buffer, &error);
	if (error != json_tokener_success) errx(EXIT_FAILURE, "Buffer not json.");
	status = json_object_get_string(json_object_object_get(obj, "status"));
	printf("%s- %sStatus%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, status, KNRM);
	country = json_object_get_string(json_object_object_get(obj, "country"));
	printf("%s* %sCountry%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, country, KNRM);
	city = json_object_get_string(json_object_object_get(obj, "city"));
	printf("%s- %sCity%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, city, KNRM);
	timezone = json_object_get_string(json_object_object_get(obj, "timezone"));
	printf("%s- %sTimezone%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, timezone, KNRM);
	isp = json_object_get_string(json_object_object_get(obj, "isp"));
	printf("%s- %sISP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, isp, KNRM);
	query = json_object_get_string(json_object_object_get(obj, "query"));
	printf("%s- %sIP%s:%s %s%s\n", KMAG, KBLU, KMAG, KGRN, query, KNRM);
	
	free(buffer);
	close(sockfd);
	return EXIT_SUCCESS;
}