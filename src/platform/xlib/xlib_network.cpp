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

struct XlibSocket {
	i32 descriptor;

	i8 connections_len;
	struct sockaddr_in* connections;
};

PlatformSocket* platform_init_server_socket(Arena* arena)
{
	PlatformSocket* platform_socket = (PlatformSocket*)arena_alloc(arena, sizeof(PlatformSocket));
	platform_socket->type = SOCKET_TYPE_SERVER;
	platform_socket->backend = arena_alloc(arena, sizeof(XlibSocket));

	XlibSocket* sock = (XlibSocket*)platform_socket->backend;
	sock->connections = (struct sockaddr_in*)arena_alloc(arena, sizeof(struct sockaddr_in) * MAX_CLIENTS);
	sock->connections_len = 0;
	
	if((sock->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
		panic();
	}

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(struct sockaddr_in));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(8080);

	if(bind(sock->descriptor, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		panic();
	}

	return platform_socket;
}

PlatformSocket* platform_init_client_socket(Arena* arena)
{
	PlatformSocket* platform_socket = (PlatformSocket*)arena_alloc(arena, sizeof(PlatformSocket));
	platform_socket->type = SOCKET_TYPE_CLIENT;
	platform_socket->backend = arena_alloc(arena, sizeof(XlibSocket));

	XlibSocket* sock = (XlibSocket*)platform_socket->backend;
	sock->connections = (struct sockaddr_in*)arena_alloc(arena, sizeof(struct sockaddr_in));
	sock->connections_len = 1;

	if((sock->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
		panic();
	}

	struct sockaddr_in* server_address = &sock->connections[0];
	memset(server_address, 0, sizeof(struct sockaddr_in));

	server_address->sin_family = AF_INET;
	server_address->sin_addr.s_addr = INADDR_ANY;
	server_address->sin_port = htons(8080);

	return platform_socket;
}

void platform_send_packet(PlatformSocket* socket, i8 connection_id, void* packet, u32 size) {
	XlibSocket* sock = (XlibSocket*)socket->backend;

	assert(connection_id < sock->connections_len);

	struct sockaddr_in* server_address = &sock->connections[connection_id];
	//printf("Packet sent: size %i, address %i %i %i\n", size, server_address->sin_family, server_address->sin_addr.s_addr, server_address->sin_port);

	sendto(sock->descriptor, packet, size, MSG_CONFIRM, 
		(struct sockaddr*)&sock->connections[connection_id], sizeof(struct sockaddr_in));
}

PlatformPayload platform_receive_packets(PlatformSocket* socket, Arena* arena) {
	XlibSocket* sock = (XlibSocket*)socket->backend;

	PlatformPayload payload;
	payload.head = nullptr;
	PlatformPacket** current_packet = &payload.head;

	while(true) {
		struct sockaddr_in sender_address;
		socklen_t len = sizeof(struct sockaddr_in);

		char data[MAX_PACKET_BYTES];
		i32 data_size = recvfrom(sock->descriptor, data, MAX_PACKET_BYTES, 
			MSG_WAITALL, (struct sockaddr*)&sender_address, &len);

		// -1 means there are no more packets.
		if(data_size != -1) { 
			//printf("Packet received: size %i, address %i %i %i\n", data_size, sender_address.sin_family, sender_address.sin_addr.s_addr, sender_address.sin_port);

			if(*current_packet != nullptr) {
				(*current_packet)->next = (PlatformPacket*)arena_alloc(arena, sizeof(PlatformPacket));
				current_packet = &(*current_packet)->next;
			} else {
				*current_packet = (PlatformPacket*)arena_alloc(arena, sizeof(PlatformPacket));
			}
			(*current_packet)->next = nullptr;

			PlatformPacket* packet = *current_packet;
			packet->data_size = data_size;
			packet->data = (char*)arena_alloc(arena, data_size);

			if(socket->type == SOCKET_TYPE_SERVER) {
				// Compare against other connections.
				bool connection_already_exists = false;
				for(u8 i = 0; i < sock->connections_len; i++) {
					struct sockaddr_in* existing_address = &sock->connections[i];
					bool ip_match = (memcmp(&existing_address->sin_addr.s_addr, &sender_address.sin_addr.s_addr, sizeof(existing_address->sin_addr.s_addr)) == 0);
					bool port_match = (memcmp(&existing_address->sin_port, &sender_address.sin_port, sizeof(existing_address->sin_port)) == 0);

					if(existing_address->sin_addr.s_addr == sender_address.sin_addr.s_addr &&
			        existing_address->sin_port == sender_address.sin_port) {
						packet->connection_id = i;
						connection_already_exists = true;
					}
				}
				if(!connection_already_exists) {
					// TODO - Reject if too many connections.
					packet->connection_id = sock->connections_len;
					sock->connections[sock->connections_len] = sender_address;
					sock->connections_len++;
					printf("add connection\n");
				}
			} else {
				packet->connection_id = 0;
			}
			memcpy(packet->data, data, data_size);
		} else {
			//if((*current_packet) != nullptr) {
			//	(*current_packet)->next = nullptr;
			//}
			return payload;
		}
	}

	panic();
}

#endif // xlib_network_h_INCLUDED
