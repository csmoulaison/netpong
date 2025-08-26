#define SERVER_TYPE_LOCAL 0
#define SERVER_TYPE_REMOTE 1

#define SERVER_STATE_IDLE 0
#define SERVER_STATE_LOBBY 1
#define SERVER_STATE_GAME 1

struct ServerLocal {
};

struct ServerRemote {
	PlatformSocket* socket;
};

struct Server {
	u8 type;
	u8 state;

	MatchState match;
	bool close_requested;

	union {
		ServerLocal local;
		ServerRemote remote;
	};
};

Server* server_init(u8 type, Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->type = type;
	server->state = SERVER_STATE_IDLE;

	switch(type)
	{
		case SERVER_TYPE_LOCAL:
			break;
		case SERVER_TYPE_REMOTE:
			server->remote.socket = platform_init_server_socket(arena);
			break;
		default: break;
	}

	return server;
}

bool server_close_requested(Server* server)
{
	return false;
}

void server_process_packets(Server* server)
{
	// NOW - this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(server->remote.socket, &packet_arena);
	PlatformPacket* packet = payload.head;
	
	while(packet != nullptr) {
		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;
		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				ServerJoinAcknowledgePacket acknowledge_packet;
				acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;

				platform_send_packet(server->remote.socket, 0, (void*)&acknowledge_packet);
				break;
			case CLIENT_PACKET_UPDATE:
				break;
			default: break;
		}
	}
	arena_destroy(&packet_arena);
}
