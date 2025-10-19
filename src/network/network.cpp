#include "network/network.h"

namespace Network {
	Network::Socket* init_server_socket(Arena* arena) {
		return platform_init_server_socket(arena);
	}

	Network::Socket* init_client_socket(Arena* arena, char* ip_string) {
		return platform_init_client_socket(arena, ip_string);
	}

	i32 add_connection(Network::Socket* socket, void* address) {
		return platform_add_connection(socket, address);
	}

	void free_connection(Network::Socket* socket, i32 connection_id) {
		platform_free_connection(socket, connection_id);
	}

	void send_packet(Network::Socket* socket, i32 connection_id, void* packet, u32 size) {
		platform_send_packet(socket, connection_id, packet, size);	
	}

	Network::Packet* receive_packets(Network::Socket* socket, Arena* arena) {
		return platform_receive_packets(socket, arena);	
	}
}
