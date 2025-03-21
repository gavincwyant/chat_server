#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#define PORT "8080"
#define MAX_DATA_SIZE 100

void print_struct(struct addrinfo *info)
{
	printf("canon name: %s\n", info->ai_canonname);
	printf("socket type: %d\n", info->ai_socktype);
	printf("ai_protocol: %d\n", info->ai_protocol);
	printf("ai_family: %d\n", info->ai_family);
	//printf("ai_addr: %d\n", info->ai_addr);
	printf("ai_addrlen: %d\n", info->ai_addrlen);
}

void * get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int error, socket_file_descriptor;
	//buffer for type of ip version
	char ip_string[INET6_ADDRSTRLEN];
	//buffer for message from server
	char buf[MAX_DATA_SIZE];
	struct addrinfo hints, *info, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//grabbing address info for a user input (or loopback) ip
	error = getaddrinfo(NULL, PORT, &hints, &info);

	if (error != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		return 2;
	}
	for (p = info; p != NULL; p = p->ai_next)
	{
		void *address;
		char *ip_version;

		if (p->ai_family == AF_INET)
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			address = &(ipv4->sin_addr);
			ip_version = "IPv4";
		}
		else
		{
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			address = &(ipv6->sin6_addr);
			ip_version = "IPv6";	
		}

		//converting ip address to string
		inet_ntop(p->ai_family, address, ip_string, sizeof(ip_string));
		//printf("%s: %s\n", ip_version, ip_string);
		//print_struct(p);

		//asking the os for a socket with the specifications returned by getaddrinfo()
		socket_file_descriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socket_file_descriptor == -1)
		{
			printf("socket_fd error\n");
			continue;
		}

		if (connect(socket_file_descriptor, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socket_file_descriptor);
			printf("connect error\n");
			continue;
		}
		//if we make if here, we successfully connected
		break;	
	}
	if (p == NULL)
	{
		printf("we failed to connect\n");
	}
	//we have what we want in p!
	printf("we are making it to freeaddrinfo()\n");
	freeaddrinfo(info);


	//inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	//	ip_string, sizeof ip_string);
	printf("client: connecting to %s\n", ip_string);

	struct pollfd fds[2] =
	{
		{
			0, //stdin
			POLLIN,
			0
		},
		{
			socket_file_descriptor, //server fd I think
			POLLIN,
			0
		}
	};
	nfds_t nfds = socket_file_descriptor - 1;
	for (;;)
	{
		char buf[256];
		//we will poll to check if data should be sent (stdin ready)
		//or if data should be received (server fd ready)
		int ready = poll(&fds, nfds, 50000);
		if (ready)
		{

		if (fds[0].revents & POLLIN)
		{
			read(0, buf, 255);
			buf[255] = '\0';
			if (buf[0] == 'q')
			{
				break;
			}
			else
			{
				send(socket_file_descriptor, buf, 256, 0);
			}
		}
		else if (fds[1].revents & POLLIN)
		{
			int ret = recv(socket_file_descriptor, buf, 256, 0);
			if (ret == 0 || ret == -1)
			{
				break;
			}
			buf[ret] = '\0';
			printf("\t\tserver: %s\n",ret, buf);
		}
		/*
			int numbytes;
			if ((numbytes = recv(socket_file_descriptor, buf, MAX_DATA_SIZE-1, 0))
				 == -1)
			{
				//perror("recv");
				read(0, buf, 255);
				send(socket_file_descriptor, buf, 256, 0);
			}
			else
			{
				buf[numbytes] = '\0';
				printf("client: received: '%s'\n", buf);

			}
		*/

		}


	}

	close(socket_file_descriptor);
	return 0;
}

