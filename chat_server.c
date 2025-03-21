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
#include <poll.h>


#define BACKLOG 10
#define PORT "8080" // the port users will be connecting to



void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) //IPv4
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}	


int main(void)
{
	int socket_file_descriptor, new_fd, ret; //listen on socket, new connection on new_fd
	struct addrinfo hints, *server_info, *p;
	struct sockaddr_storage connectors_address;
	socklen_t sin_size; //TODO: what is this?
	struct	sigaction sa;
	
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //whatever ip version we can get
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; //this means to use my IP

	//step1: get address info 
	//we are using our own ip address "::1", port 8080, and we are storing the
	//results in server_info which will be a linked list of addrinfo structs

	if (ret = getaddrinfo(NULL , PORT, &hints, &server_info) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return 1;
	}

	//if we get a linked list from getaddrinfo, we should loop through the nodes
	//and connect to the first one that we can
	for (p = server_info; p != NULL; p = p->ai_next)
	{
		//open socket
		if((socket_file_descriptor = socket(p->ai_family, p->ai_socktype,
		 	p->ai_protocol)) == -1)
		{
			perror("server: socket\n");
			continue;
		}

		/* this code allows for quick reuse of the program. I dont want to use this until I am sure it is necessary
		if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, 
			sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		*/
		//bind to socket
		if (bind(socket_file_descriptor, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socket_file_descriptor);
			perror("server: bind");
			continue;
		}
		break;
	}
	
	freeaddrinfo(server_info);

	if(p == NULL)
	{		
		//no connection successfully made/ did not bind
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}	
	

	if (listen(socket_file_descriptor, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}


	printf("waiting for connections\n");

	new_fd = accept(socket_file_descriptor, 
	(struct sockaddr *)&connectors_address, &sin_size);
	
	struct pollfd fds[2] =
	{
		{
			0, //stdin
			POLLIN,
			0 //no flags
		},
		{
			new_fd, //client
			POLLIN,
			0 //no flags
		}
	};

	sin_size = sizeof connectors_address;
	nfds_t nfds = new_fd - 1;


	if (new_fd == -1)
	{	
		perror("accept");
		exit(1);
	}	
	inet_ntop(connectors_address.ss_family,
		get_in_addr((struct sockaddr *)&connectors_address),
		s, sizeof s);
	
	while(1) //this is the accept() loop
	{
		char buf[256] = { 0 };
		int ready = poll(&fds, nfds, 0);
		if (ready)
		{

			if (fds[0].revents & POLLIN)
			{
				read(0, buf, 255);
				if (buf[0] == 'q')
				{
					break;
				}
				buf[255] = '\0';
				send(new_fd, buf, 256, 0);
			}
			else if (fds[1].revents & POLLIN)
			{
				if (recv(new_fd, buf, 256, 0) == 0)
				{
					break;
				}
				printf("\t\tclient: %s\n", buf);
			}

		}

		
		/*	
		if (!fork()) //child returns zero
		{
			close(socket_file_descriptor); //child does not need listener
			if (send(new_fd, "Hello World!", 13, 0) == -1)
			{
				perror("send");
			}
			close(new_fd);
			//exit(0);
		}
		close(new_fd);
		*/	
	
	}
	
	
	return 0;


}
