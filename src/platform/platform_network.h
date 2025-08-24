#ifndef platform_network_h_INCLUDED
#define platform_network_h_INCLUDED

#include "base/base.h"

#define MAX_CLIENTS 8
#define MAX_PAYLOAD_PACKETS 64
#define MAX_PACKET_BYTES 2048

struct PlatformSocket {
	void* backend;
};

struct PlatformPacket {
	PlatformPacket* next;

	char* data;
	u32 data_size;

	i8 connection_id;
};

struct PlatformPayload {
	PlatformPacket* head;
};

// Opens a UDP socket for the host server.
PlatformSocket* platform_init_server_socket(Arena* arena);

// Opens a UDP socket for clients.
PlatformSocket* platform_init_client_socket(Arena* arena);

// Sends a packet to given connection. Assumes the connection exists.
void platform_send_packet(PlatformSocket* socket, i8 connection_id, const char* packet);

// Pull all packets received since the previous call of this function.
// 
// The packets are stored in the passed memory arena, so the arena needs to be
// freed or it's a memory leak.
PlatformPayload platform_receive_packets(PlatformSocket* socket, Arena* arena);

#endif // platform_network_h_INCLUDED
