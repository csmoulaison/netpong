#ifndef xlib_network_h_INCLUDED
#define xlib_network_h_INCLUDED

#include "platform/platform_network.h"

#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

PlatformSocket platform_init_server_socket()
{
	i32 sockfd;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		panic();
	}

	struct sockaddr_in servaddr;
	// NOW - um, memset 0? after this works, just init to 0 with {0} and see if it
	// works.
	memset(&servaddr, 0, sizeof(servaddr));

	struct sockaddr_in cliaddr;
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(8080);

	if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		panic();
	}

	char buffer[256];
	socklen_t len = sizeof(cliaddr);;
	i32 n = recvfrom(sockfd, (char*)buffer, 256, MSG_WAITALL, (struct sockaddr*)&cliaddr, &len);
	buffer[n] = '\0';

	printf("client: %s\n", buffer);

	const char* msg = "Hello from the server!";
	sendto(sockfd, msg, strlen(msg), MSG_CONFIRM, (struct sockaddr*)&cliaddr, len);

	printf("Hello message sent!\n");

	return PlatformSocket{};
}

// NOW - implement these
PlatformSocket platform_init_client_socket()
{
	i32 sockfd;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		panic();
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8080);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	socklen_t len;

	const char* msg = "Hello from the client!";
	sendto(sockfd, msg, strlen(msg), MSG_CONFIRM, (struct sockaddr*)&servaddr, sizeof(servaddr));
	printf("Hello message sent!\n");

	char buffer[256];
	i32 n = recvfrom(sockfd, buffer, 256, MSG_WAITALL, (struct sockaddr*)&servaddr, &len);
	buffer[n] = '\0';
	printf("Server: %s\n", buffer);

	close(sockfd);
	return PlatformSocket{};
}

#endif // xlib_network_h_INCLUDED
