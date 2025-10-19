#ifndef network_h_INCLUDED
#define network_h_INCLUDED

#include "base/base.h"

#define MAX_CLIENTS 8
#define MAX_PAYLOAD_PACKETS 64
#define MAX_PACKET_BYTES 2048

#define PLATFORM_SOCKET_CLIENT_BIT 1 << 0
#define PLATFORM_SOCKET_SERVER_BIT 1 << 1

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
#endif

namespace Network {

#if NETWORK_SIM_MODE
	struct SimPacket {
		f32 countdown;
		i32 connection_id;
		void* packet;
		u32 size;
	};
#endif

	enum class SocketType {
		Client,
		Server
	};

	struct Socket {
		SocketType type;
		void* backend;

	#if NETWORK_SIM_MODE
		SimPacket sim_packets[NETWORK_SIM_PACKET_BUFFER_SIZE];
		i32 sim_packets_len;
	#endif
	};

	struct Packet {
		Packet* next;

		i32 connection_id;
		void* data;
		u32 size;
	};
}

Network::Socket* platform_init_server_socket(Arena* arena);
Network::Socket* platform_init_client_socket(Arena* arena, char* ip_string);
i32 platform_add_connection(Network::Socket* socket, void* address);
void platform_free_connection(Network::Socket* socket, i32 connection_id);
void platform_send_packet(Network::Socket* socket, i32 connection_id, void* packet, u32 size);
Network::Packet* platform_receive_packets(Network::Socket* socket, Arena* arena);

#endif
