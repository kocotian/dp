#ifndef HTTP_H
#define HTTP_H

#define _XOPEN_SOURCE 700

/*
 * http.h
 * simple and small http library for C99
 * version 0.2
 * creator: kocotian
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MAX_REQUEST_LEN
#define MAX_REQUEST_LEN 1024
#endif

size_t httpGET(char *hostname, unsigned short port, char *prepath, char *path, char **buffer);
int getResponseStatus(char *response, size_t responseSize);
int parseResponseLine(char *response, char *value, char **buffer);
char *truncateHeader(char *response);
int getHeaderLength(char *response);

size_t
httpGET(char *hostname, unsigned short port, char *prepath, char *path, char **buffer)
{
	char tmpbuf[BUFSIZ];
	struct hostent *hostent;
	struct sockaddr_in sockaddr_in;
	in_addr_t in_addr;
	int sockfd;
	char *request_template = "GET %s%s HTTP/1.0\r\nHost: %s\r\n\r\n";
	char request[MAX_REQUEST_LEN];
	int request_length;
	int byteswrote = 0;
	long long int totalbytes = 0;
	int writeiter = 0;
	request_length = snprintf(request, MAX_REQUEST_LEN, request_template,
			prepath, path, hostname);
	if ((hostent = gethostbyname(hostname)) == NULL) {
		fprintf(stderr, "gethostbyname(%s) error", hostname);
		return 1;
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket opening error");
		return 1;
	}
	in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent -> h_addr_list)));
	sockaddr_in.sin_port          = htons(port);
	sockaddr_in.sin_addr.s_addr   = in_addr;
	sockaddr_in.sin_family        = AF_INET;
	if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) < 0) {
		perror("socket connecting error");
		return 1;
	}
	if ((write(sockfd, request, request_length)) < 0) {
		perror("sending request error");
		return 1;
	}
	*buffer = calloc(BUFSIZ, 1);
	while ((byteswrote = read(sockfd, tmpbuf, BUFSIZ)) > 0) {
		totalbytes += byteswrote;
		strncat(*buffer, tmpbuf, byteswrote);
		*buffer = realloc(*buffer, BUFSIZ * (++writeiter + 1));
	}
	close(sockfd);
	return totalbytes;
}

int
getResponseStatus(char *response, size_t responseSize)
{
	char *responsecopy = malloc(responseSize);
	char *status; int ret;
	strncpy(responsecopy, response, responseSize);
	status = strtok(responsecopy, " ");
	status = strtok(NULL, " ");
	ret = atoi(status);
	free(responsecopy);
	return ret;
}

int
parseResponseLine(char *response, char *value, char **buffer)
{
	char *tmpbuf = strtok(response, "\r");
	if (!strcmp(value, "HTTP")) {
		*buffer = tmpbuf;
		return 0;
	}
	while (strcmp(tmpbuf, "\n")) {
		tmpbuf = strtok(NULL, "\r");
		unsigned short iter = -1;
		char allOk = 1;
		while (value[++iter]) {
			if (value[iter] != tmpbuf[iter + 1]) {
				allOk = 0;
				break;
			}
			else allOk = 1;
		}
		if (allOk) {
			iter += 2;
			int i2 = 1;
			*buffer = malloc(i2);
			while (tmpbuf[iter]) {
				(*buffer)[i2 - 1] = tmpbuf[++iter];
				*buffer = realloc(*buffer, ++i2);
			}
			(*buffer)[i2 - 2] = 0;
			return 0;
		}
	}
	return -1;
}

char *
truncateHeader(char *response)
{
	return strstr(response, "\r\n\r\n") + 4;
}

int
getHeaderLength(char *response)
{
	return strstr(response, "\r\n\r\n") - response;
}

#endif
