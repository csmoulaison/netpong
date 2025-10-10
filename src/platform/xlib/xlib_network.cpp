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

struct XlibConnection {
	struct sockaddr_in address;
	i32 id;
};

struct XlibSocket {
	i32 descriptor;

	XlibConnection* connections;
	u32 connections_len;

	// Lookup table into connections. Indices into this table are connection_ids.
	i32 id_to_connection[MAX_CLIENTS];
};

// TODO: I think we are probably moving towards a world where server/client
// sockets aren't distinguished on the platform level. All those distinctions
// should probably live on the other side of the pond.
PlatformSocket* platform_init_server_socket(Arena* arena)
{
	PlatformSocket* platform_socket = (PlatformSocket*)arena_alloc(arena, sizeof(PlatformSocket));
	platform_socket->type = SOCKET_TYPE_SERVER;
	platform_socket->backend = arena_alloc(arena, sizeof(XlibSocket));

	XlibSocket* sock = (XlibSocket*)platform_socket->backend;
	sock->connections = (XlibConnection*)arena_alloc(arena, sizeof(XlibConnection) * MAX_CLIENTS);
	sock->connections_len = 0;
	for(i32 i = 0; i < MAX_CLIENTS; i++) {
		sock->id_to_connection[i] = -1;
	}

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

#if NETWORK_SIM_MODE == true
	platform_socket->sim_packets_len = 0;
#endif

	return platform_socket;
}

PlatformSocket* platform_init_client_socket(Arena* arena, char* ip_string)
{
	PlatformSocket* platform_socket = (PlatformSocket*)arena_alloc(arena, sizeof(PlatformSocket));
	platform_socket->type = SOCKET_TYPE_CLIENT;
	platform_socket->backend = arena_alloc(arena, sizeof(XlibSocket));

	XlibSocket* sock = (XlibSocket*)platform_socket->backend;
	sock->connections = (XlibConnection*)arena_alloc(arena, sizeof(XlibConnection));
	sock->connections_len = 1;
	sock->id_to_connection[0] = 0;

	if((sock->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
		panic();
	}

	XlibConnection* server = &sock->connections[0];
	memset(&server->address, 0, sizeof(struct sockaddr_in));

	server->address.sin_family = AF_INET;
	server->address.sin_port = htons(8080);
	if(ip_string == nullptr) {
		server->address.sin_addr.s_addr = INADDR_ANY;
	} else {
		if(inet_pton(AF_INET, ip_string, &server->address.sin_addr) <= 0) {
			panic();
		}
	}

	return platform_socket;
}

i32 platform_add_connection(PlatformSocket* socket, void* address)
{
	struct sockaddr_in* addr = (struct sockaddr_in*)address;
	XlibSocket* sock = (XlibSocket*)socket->backend;

	for(u32 i = 0; i < MAX_CLIENTS; i++) {
		if(sock->id_to_connection[i] == -1) {
			sock->id_to_connection[i] = sock->connections_len;

			XlibConnection* new_connection = &sock->connections[sock->connections_len];
			new_connection->address = (struct sockaddr_in)*addr;
			new_connection->id = i;

			sock->connections_len++;

			printf("Server: Add client connection %u.\n", i);
			return i;
		}
	}

	printf("No free connections!\n");
	panic();
	return -1;
}

void platform_free_connection(PlatformSocket* socket, i32 connection_id)
{
	XlibSocket* sock = (XlibSocket*)socket->backend;
	if(sock->id_to_connection[connection_id] != sock->connections_len - 1) {
		XlibConnection replacement = sock->connections[sock->connections_len - 1];
		sock->connections[sock->id_to_connection[connection_id]] = replacement;
		sock->id_to_connection[replacement.id] = sock->id_to_connection[connection_id];
	}
	sock->connections_len--;
	sock->id_to_connection[connection_id] = -1;
}

// This exists so that we can reuse it's logic in NETWORK_SIM_MODE.
void xlib_send_packet(PlatformSocket* socket, i32 connection_id, void* packet, u32 size)
{
	XlibSocket* sock = (XlibSocket*)socket->backend;

	XlibConnection* connection = &sock->connections[sock->id_to_connection[connection_id]];
	sendto(sock->descriptor, packet, size, MSG_CONFIRM, 
		(struct sockaddr*)&connection->address, sizeof(struct sockaddr_in));
}

#if NETWORK_SIM_MODE == true

void platform_update_sim_mode(PlatformSocket* socket, f32 dt) 
{
	for(i32 i = 0; i < socket->sim_packets_len; i++) {
		SimPacket* sim_packet = &socket->sim_packets[i];
		sim_packet->countdown -= dt;
		if(sim_packet->countdown <= 0.0f) {
			xlib_send_packet(socket, sim_packet->connection_id, sim_packet->packet, sim_packet->size);

			if(i != socket->sim_packets_len - 1) {
				memcpy(sim_packet, &socket->sim_packets[socket->sim_packets_len - 1], sizeof(SimPacket));
				i--;
			}
			socket->sim_packets_len--;
		}
	}
}

#endif

void platform_send_packet(PlatformSocket* socket, i32 connection_id, void* packet, u32 size) {
#if NETWORK_SIM_MODE == true
	if(random_f32() < NETWORK_SIM_PACKET_LOSS_CHANCE) {
		//printf("Artificial packet loss.\n");
		return;
	}

	assert(socket->sim_packets_len + 1 < NETWORK_SIM_PACKET_BUFFER_SIZE);
	SimPacket* sim_packet = &socket->sim_packets[socket->sim_packets_len];
	socket->sim_packets_len++;

	sim_packet->countdown = random_f32() * NETWORK_SIM_LATENCY_AVERAGE_SECONDS + random_f32() * NETWORK_SIM_LATENCY_VARIANCE_SECONDS;
	sim_packet->connection_id = connection_id;
	sim_packet->size = size;
	sim_packet->packet = malloc(size);
	memcpy(sim_packet->packet, packet, size);

	return;
#endif

	xlib_send_packet(socket, connection_id, packet, size);
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
				for(u32 i = 0; i < sock->connections_len; i++) {
					XlibConnection* existing_connection = &sock->connections[i];
					struct sockaddr_in* existing_address = &existing_connection->address;

					if(existing_address->sin_addr.s_addr == sender_address.sin_addr.s_addr && existing_address->sin_port == sender_address.sin_port) {
						packet->connection_id = sock->connections[i].id;
						connection_already_exists = true;
					}
				}
				if(!connection_already_exists) {
					packet->connection_id = platform_add_connection(socket, &sender_address);
				}
			} else { // socket->type == SOCKET_TYPE_CLIENT
				packet->connection_id = 0;
			}
			memcpy(packet->data, data, data_size);
		} else {
			return payload;
		}
	}

	panic();
}

#endif // xlib_network_h_INCLUDED
