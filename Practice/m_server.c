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
#define BACKLOG 10

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
	listen()
	accept()
	send/recv
	close()
	*/
	int sockfd; // create a socket file descriptor

	int yes = 1; // Used as a parameter to reuse a port in case it has been not cleaned up by the kernel from a previous process
	char s[INET6_ADDRSTRLEN]; // to store and print IP Addresses
	int rv; // just used to hold a return value

	// addrinfo structures : These need to be used while opening/closing/connecting and so on with sockets
	// hints : Specifies what sort of IP-Protocol, Socket-Type, IP-Address do we want.
	// Only those address structs compatible with hints is returned
	// servinfo : Used to get struct values returned by the getaddrinfo() call
	// For the given set of parameters in hints, there can be different possible structs returned
	// For instance IPv4 and IPv6 are different structs that can be returned
	// We will use this servinfo to open sockets with different protocols at the particular address
	// p is just used to iterate over servinfo for then creating sockets
	struct addrinfo hints, *servinfo, *p;

	/* Populate hints with whatever information you want */
	memset(&hints, 0, sizeof hints); // make sure this struct is empty
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // use my IP. This tells getaddrinfo() to use address of local host for socket str

	// Get address structures for current IP Address on pc
	// getaddrinfo() requires a hostname, PORT, hints, res (linked list) which is populated
	// This also does DNS name lookup for you
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); // gai_strerror will take int and return string
		return 1;
	}

	// loop through all the results of the possible structaddr addresses for the given IP and PORT above
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// Create a socket
		// int socket(int domain, int type, int protocol);
		// domain : Type of IP_Version, can be PF_INET6/PF_INET(Or even PF<=>AF above)
		// type : SOCK_STREAM/SOCK_DGRAM
		// protocol : 0 : Choose proper protocol of given type; getprotobyname() to look up "tcp" or "udp"
		// This also populates the socket file-descriptor we created above
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		// Bind the socket to a port on the IP Address
		// This is for listening at that port
		// Mostly only relevant for the server side

		// Before binding, some corner-case checking
		/* Sometimes, you might notice, you try to rerun a server and bind() fails, claiming “Address already in use.”
		   A little bit of a socket that was connected is still hanging around in the kernel, and it’s hogging the port.
		   You can either wait for it to clear (a minute or so), or add code to your program allowing it to reuse the port.
		   This piece of code does exactly the reusing of the port.
		*/
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		// Call bind
		// int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
		// sockfd is the socket file descriptor returned by socket() . 
		// my_addr is a pointer to a struct sockaddr that contains information about your address, namely, port
		// and IP address. 
		// addrlen is the length in bytes of that address.
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd); // CLose since bind was not possible
			perror("server: bind");
			continue;
		}

		break; // if connection was successful, we break here. Depending on program requirement, this may not be necessary
	}

	freeaddrinfo(servinfo); // free the linked-list once done. This is no longer required. `p` is the thing now. Again depends on logic

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	// We now have a socket bound to a port. We need to listen for connections and accept them.
	// Listen on the particualr port for incoming connections
	// int listen(int sockfd, int backlog);
	/*
	sockfd is the usual socket file descriptor from the socket() system call. backlog is the number of connecions
	allowed on the incoming queue. What does that mean? Well, incoming connections are going to wait in this queue
	until you accept() them (see below) and this is the limit on how many can queue up.
	*/
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
		}

	printf("server: waiting for connections...\n");

	// Accept the incoming connections
	/*
	someone far far away will try to connect() to your machine on a port that you are listen() ing on. 
	Their connection will be queued up waiting to be accept() ed. You call accept() and you tell it to get the pending connection. 
	It’ll return to you a brand new socket file descriptor to use for this single connection! That’s right, suddenly you have two
	socket file descriptors for the price of one! The original one is still listening for more new connections,
	and the newly created one is finally ready to send() and recv().
	*/
	struct sockaddr_storage their_addr; // For connector's socket address information
	socklen_t sin_size; // to store size of the above structure
	int new_fd;

	// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	// sockfd is the listen() ing socket descriptor.
	// addr will usually be a pointer to a local struct sockaddr_storage . 
	// This is where the information about the incoming connection will go (and with it you can determine 
	// which host is calling you from which port).
	// addrlen is a local integer variable that should be set to sizeof(struct sockaddr_storage) before its address
	// is passed to accept() . accept() will not put more than that many bytes into addr.
	// If it puts fewer in, it’ll change the value of addrlen to reflect that.

	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
		perror("accept");
		exit(0);
	}

	// Ready to communicate on new_fd as a socket descriptor to send/recv data
	// The sockfd descriptor will continue listening on its port for incoming connections
	// If only one connection is required in an application, you can close sockfd above
	// That will stop its listening

	// Print information about the connection
	inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
	printf("server: got connection from %s\n", s);

	// We assume a single connection case for now
	close(sockfd); // No need to listen further if connection has been made

	// Send and Receive data
	/*
	send/recv : For Stream sockets
	sendto/recvfrom : Datagram sockets

	int send(int sockfd, const void *msg, int len, int flags);
	sockfd is the socket descriptor you want to send data to (whether it’s the one returned by socket() or the
	one you got with accept() ). msg is a pointer to the data you want to send, and len is the length of that data
	in bytes. Just set flags to 0.

	send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
	See, sometimes you tell it to send a whole gob of data and it just can’t handle it. It’ll fire off as much of the
	data as it can, and trust you to send the rest later. Remember, if the value returned by send() doesn’t match
	the value in len , it’s up to you to send the rest of the string.
	Again, -1 is returned on error, and errno is set to the error number.

	int recv(int sockfd, void *buf, int len, int flags);
	sockfd is the socket descriptor to read from, buf is the buffer to read the information into, len is the maximum
	length of the buffer, and flags can again be set to 0 . (See the recv() man page for flag information.)
	recv() returns the number of bytes actually read into the buffer, or -1 on error (with errno set, accordingly).
	Wait! recv() can return 0 . This can mean only one thing: the remote side has closed the connection on you!
	A return value of 0 is recv() ’s way of letting you know this has occurred.
	*/

	char msg[100];
	char buf[100];
	int numbytes;

	while(1) {
		printf("server>");
		scanf("%[^\n]%*c", msg);
		if (send(new_fd, msg, strlen(msg), 0) == -1)
			perror("send");

		if ((numbytes = recv(new_fd, buf, 100-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		else {
			printf("client> %s\n", buf);
		}
	}



	return 0;
}