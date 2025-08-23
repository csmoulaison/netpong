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

// NOW - implement these
PlatformSocket platform_init_server_socket()
{
	i32 sockfd;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		panic();
	}

	struct sockaddr_in servaddr;
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

PlatformSocket platform_init_client_socket()
{
	return PlatformSocket{};
}

#endif // xlib_network_h_INCLUDED
