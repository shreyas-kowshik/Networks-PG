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

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

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

		/*
		Connect to an IP and a PORT using the socket
		The current process also has a socket but its port is not that important for this process perse
		So on calling connect, this socket is automatically bound to one port that this runs on
		This information is available to the server once it accepts the connection

		int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
		sockfd is our friendly neighborhood socket file descriptor, as returned by the socket() call, serv_addr is
		a struct sockaddr containing the destination port and IP address, and addrlen is the length in bytes of
		the server address structure.
		*/
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	char msg[100];
	while(1) {
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		else {
			buf[numbytes] = '\0';
			printf("server> '%s'\n",buf);
		}

		printf("client>");
		scanf("%[^\n]%*c", msg);
		if (send(sockfd, msg, strlen(msg), 0) == -1)
			perror("send");
	}


	close(sockfd);

	return 0;
}

