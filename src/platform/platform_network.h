#ifndef platform_network_h_INCLUDED
#define platform_network_h_INCLUDED

#include "base/base.h"

#define MAX_CLIENTS 8
#define MAX_PAYLOAD_PACKETS 64
#define MAX_PACKET_BYTES 2048

#define NETWORK_SIM_MODE false

#if NETWORK_SIM_MODE

#define NETWORK_SIM_PACKET_BUFFER_SIZE 2048
#define NETWORK_SIM_PACKET_LOSS_CHANCE 0.01f
// TODO - In order to be correct about this, we really need to spawn a seperate
// thread which counts this stuff down on its own. The way we process ticks with
// an accumulator means the actual time before the packet is sent is only
// loosely correlated with the numbers here.
#define NETWORK_SIM_LATENCY_AVERAGE_SECONDS 0.04f
#define NETWORK_SIM_LATENCY_VARIANCE_SECONDS 0.01f

struct SimPacket {
	f32 countdown;
	i32 connection_id;
	void* packet;
	u32 size;
};

#endif

enum SocketType {
	SOCKET_TYPE_CLIENT,
	SOCKET_TYPE_SERVER
};

struct PlatformSocket {
	SocketType type;
	void* backend;

#if NETWORK_SIM_MODE
	SimPacket sim_packets[NETWORK_SIM_PACKET_BUFFER_SIZE];
	i32 sim_packets_len;
#endif
};

// NOW: < THIS: This way of doing things makes it a little bit awkward in the
// sense that all of this needs to be transmitted over the network, at least the
// type does. I suppose we will want to fold all of this down into a void* create
// a serialize function for the header that can be composed with the main packet
// serialization stuff.
// 
// NOW: Rename to PlatformPacket once we are transitioned.
struct PlatformPacket {
	PlatformPacket* next;

	i32 connection_id;
	void* data;
	u32 size;
};

// Opens a UDP socket for the host server.
PlatformSocket* platform_init_server_socket(Arena* arena);

// Opens a UDP socket for clients.
PlatformSocket* platform_init_client_socket(Arena* arena, char* ip_string);

// Returns the connection id.
i32 platform_add_connection(PlatformSocket* socket, void* address);

// Opens a connection id to be used for another IP. Called after a client
// disconnect or to reject a first-time connection, for instance.
void platform_free_connection(PlatformSocket* socket, i32 connection_id);

// Sends a network packet to given connection. Assumes the connection exists.
void platform_send_packet(PlatformSocket* socket, i32 connection_id, void* packet, u32 size);

// Pull all packets received since the previous call of this function. The
// packets are allocated to the given memory arena, so remember to free it.
PlatformPacket* platform_receive_packets(PlatformSocket* socket, Arena* arena);

#if NETWORK_SIM_MODE

void platform_update_sim_mode(PlatformSocket* socket, f32 dt);

#endif

#endif // platform_network_h_INCLUDED
