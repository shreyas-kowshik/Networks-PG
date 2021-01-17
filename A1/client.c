/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) // run as ./client hostname filname
{
	int sockfd, numbytes;  
	// char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 3) {
	    fprintf(stderr,"usage: client hostname filename\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		// direct connection does not seem possible for UDP sockets
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	char msg[100];
	char buf[100];
	int sent_filname=0;

	FILE *ptr;

	while(1) {
		if(!sent_filname) {
			if(sendto(sockfd, argv[2], strlen(argv[2]) + 1, 0, p->ai_addr, p->ai_addrlen) == -1) {
				perror("send");
				exit(1);
			}
			sent_filname = 1;
		}
		else {
			if(numbytes = recvfrom(sockfd, buf, 100, 0, p->ai_addr, &p->ai_addrlen) == -1) {
				perror("send");
				exit(1);
			}
			
			printf("client> %s\n", buf);
			int t = strcmp(buf, "HELLO\n");
			printf("t : %d\n", t);
			if(t == 0) {
				printf("IN");
				ptr = fopen("client_file.txt", "a");
				printf("OUT");
			}
			else if(strcmp(buf, "END\n") == 0) {
				fclose(ptr);
				break;
			}
			else {
				fputs(buf, ptr);
			}
		}
	}


	close(sockfd);

	return 0;
}

