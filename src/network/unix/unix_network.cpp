#include "network/network.h"

#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "base/base.h"

struct UnixConnection {
	struct sockaddr_in address;
	i32 id;
};

struct UnixSocket {
	i32 descriptor;

	UnixConnection* connections;
	u32 connections_len;

	// Lookup table into connections. Indices into this table are connection_ids.
	i32 id_to_connection[MAX_CLIENTS];
};

Network::Socket* platform_init_server_socket(Arena* arena)
{
	Network::Socket* platform_socket = (Network::Socket*)arena_alloc(arena, sizeof(Network::Socket));
	platform_socket->type = Network::SocketType::Server;
	platform_socket->backend = arena_alloc(arena, sizeof(UnixSocket));

	UnixSocket* sock = (UnixSocket*)platform_socket->backend;
	sock->connections = (UnixConnection*)arena_alloc(arena, sizeof(UnixConnection) * MAX_CLIENTS);
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

Network::Socket* platform_init_client_socket(Arena* arena, char* ip_string)
{
	Network::Socket* platform_socket = (Network::Socket*)arena_alloc(arena, sizeof(Network::Socket));
	platform_socket->type = Network::SocketType::Client;
	platform_socket->backend = arena_alloc(arena, sizeof(UnixSocket));

	UnixSocket* sock = (UnixSocket*)platform_socket->backend;
	sock->connections = (UnixConnection*)arena_alloc(arena, sizeof(UnixConnection));
	sock->connections_len = 1;
	sock->id_to_connection[0] = 0;

	if((sock->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
		panic();
	}

	UnixConnection* server = &sock->connections[0];
	memset(&server->address, 0, sizeof(struct sockaddr_in));

	server->address.sin_family = AF_INET;
	server->address.sin_port = htons(8080);
	if(ip_string[0] == '\0') {
		server->address.sin_addr.s_addr = INADDR_ANY;
	} else {
		if(inet_pton(AF_INET, ip_string, &server->address.sin_addr) <= 0) {
			panic();
		}
	}

#if NETWORK_SIM_MODE == true
	platform_socket->sim_packets_len = 0;
#endif

	return platform_socket;
}

i32 platform_add_connection(Network::Socket* socket, void* address)
{
	struct sockaddr_in* addr = (struct sockaddr_in*)address;
	UnixSocket* sock = (UnixSocket*)socket->backend;

	for(u32 i = 0; i < MAX_CLIENTS; i++) {
		if(sock->id_to_connection[i] == -1) {
			sock->id_to_connection[i] = sock->connections_len;

			UnixConnection* new_connection = &sock->connections[sock->connections_len];
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

void platform_free_connection(Network::Socket* socket, i32 connection_id)
{
	UnixSocket* sock = (UnixSocket*)socket->backend;
	if(sock->id_to_connection[connection_id] != sock->connections_len - 1) {
		UnixConnection replacement = sock->connections[sock->connections_len - 1];
		sock->connections[sock->id_to_connection[connection_id]] = replacement;
		sock->id_to_connection[replacement.id] = sock->id_to_connection[connection_id];
	}
	sock->connections_len--;
	sock->id_to_connection[connection_id] = -1;
	printf("Freed connection. Connections len: %i\n", sock->connections_len);
}

// This exists so that we can reuse it's logic in NETWORK_SIM_MODE.
void xlib_send_packet(Network::Socket* socket, i32 connection_id, void* packet, u32 size)
{
	UnixSocket* sock = (UnixSocket*)socket->backend;

	UnixConnection* connection = &sock->connections[sock->id_to_connection[connection_id]];
	sendto(sock->descriptor, packet, size, MSG_CONFIRM, 
		(struct sockaddr*)&connection->address, sizeof(struct sockaddr_in));
}

#if NETWORK_SIM_MODE == true

void platform_update_sim_mode(Network::Socket* socket, f32 dt) 
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

void platform_send_packet(Network::Socket* socket, i32 connection_id, void* packet, u32 size) {
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

Network::Packet* platform_receive_packets(Network::Socket* socket, Arena* arena) {
	UnixSocket* sock = (UnixSocket*)socket->backend;

	//printf("Receive packets (connections len: %u)\n", sock->connections_len);

	Network::Packet* head = nullptr;
	Network::Packet** current_node = &head;

	i32 count = 0;
	while(true) {
		count++;

		struct sockaddr_in sender_address;
		socklen_t len = sizeof(struct sockaddr_in);

		char packet[MAX_PACKET_BYTES];
		i32 packet_size = recvfrom(sock->descriptor, packet, MAX_PACKET_BYTES, 
			MSG_WAITALL, (struct sockaddr*)&sender_address, &len);

		// -1 means there are no more packets.
		if(packet_size != -1) { 
			if(*current_node != nullptr) {
				(*current_node)->next = (Network::Packet*)arena_alloc(arena, sizeof(Network::Packet));
				current_node = &(*current_node)->next;
			} else {
				*current_node = (Network::Packet*)arena_alloc(arena, sizeof(Network::Packet));
			}
			(*current_node)->next = nullptr;

			Network::Packet* node = *current_node;
			node->size = packet_size;
			node->data = (void*)arena_alloc(arena, packet_size);

			if(socket->type == Network::SocketType::Server) {
				// Compare against other connections.
				bool connection_already_exists = false;
				for(u32 i = 0; i < sock->connections_len; i++) {
					UnixConnection* existing_connection = &sock->connections[i];
					struct sockaddr_in* existing_address = &existing_connection->address;

					if(existing_address->sin_addr.s_addr == sender_address.sin_addr.s_addr && existing_address->sin_port == sender_address.sin_port) {
						node->connection_id = sock->connections[i].id;
						connection_already_exists = true;
					}
				}
				if(!connection_already_exists) {
					node->connection_id = platform_add_connection(socket, &sender_address);
				}
			} else { // socket->type == Network::SocketType::Client
				node->connection_id = 0;
			}
			memcpy(node->data, packet, packet_size);
		} else {
			return head;
		}
	}

	panic();
}
