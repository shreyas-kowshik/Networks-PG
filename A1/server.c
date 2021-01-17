#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // Port on which the server is run

// get sockaddr, IPv4 or IPv6
// A wrapper function for cases that use this sort of structure
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
	/*
	Steps :
	getaddrinfo()
	socket()
	bind()
	send/recv
	close()
	*/
	int sockfd; // create a socket file descriptor

	int yes = 1; // Used as a parameter to reuse a port in case it has been not cleaned up by the kernel from a previous process
	char s[INET6_ADDRSTRLEN]; // to store and print IP Addresses
	int rv; // just used to hold a return value

	struct addrinfo hints, *servinfo, *p;

	/* Populate hints with whatever information you want */
	memset(&hints, 0, sizeof hints); // make sure this struct is empty
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
	hints.ai_flags = AI_PASSIVE; // use my IP. This tells getaddrinfo() to use address of local host for socket str

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); // gai_strerror will take int and return string
		return 1;
	}

	// loop through all the results of the possible structaddr addresses for the given IP and PORT above
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd); // CLose since bind was not possible
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	// UDP sockets do not listen
	struct sockaddr_storage their_addr;
	socklen_t addrlen;
	addrlen = sizeof their_addr;

	char msg[100];
	char buf[100];
	int numbytes;
	int recv_file=0;
	FILE *ptr;

	while(1) {
		if(!recv_file) {
			// When received, the address structs are automatically filled, which can then be used to send information
			if(numbytes = recvfrom(sockfd, buf, 100, 0, (struct sockaddr*)&their_addr, &addrlen) == -1) {
				perror("recv");
				exit(1);
			}
			recv_file = 1;
			ptr = fopen(buf, "r");
			if(ptr == NULL) {
				printf("FILE NOT FOUND\n");
				exit(1);
			}
		}

		else {
			size_t read, len;
			char *line = NULL;
			read = getline(&line, &len, ptr);
			if(numbytes = sendto(sockfd, line, len, 0, (struct sockaddr*)&their_addr, addrlen) == -1) {
				perror("send");
				exit(1);
			}
			
			if(strcmp(line, "END\n") == 0) {
				fclose(ptr);
				break;
			}
		}
	}

	close(sockfd);

	return 0;
}