#define CLIENT_STATE_DISCONNECTED 0
#define CLIENT_STATE_CONNECTING 1
#define CLIENT_STATE_CONNECTED 2

struct Client {
	u8 state;
	u8 id;
	PlatformSocket* socket;
};

Client* client_init(Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));
	client->state = CLIENT_STATE_DISCONNECTED;
	return client;
}

void client_update(Client* client) 
{
	switch(client->state) {
		case CLIENT_STATE_DISCONNECTED:
			ClientJoinPacket join_packet;
			join_packet.header.type = CLIENT_PACKET_JOIN;
			break;
		case CLIENT_STATE_CONNECTING:
			break;
		case CLIENT_STATE_CONNECTED:
			break;
		default: break;
	}
}

void client_process_packets(Client* client, MatchStateBuffer* match_state_buffer)
{
	// NOW - this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(client->socket, &packet_arena);
	PlatformPacket* packet = payload.head;
	
	while(packet != nullptr) {
		ServerPacketHeader* header = (ServerPacketHeader*)packet->data;

		ServerJoinAcknowledgePacket* acknowledge_packet;
		ServerUpdatePacket* update_packet;
		switch(header->type) {
			case SERVER_PACKET_JOIN_ACKNOWLEDGE:
				acknowledge_packet = (ServerJoinAcknowledgePacket*)packet->data;
				client->id = acknowledge_packet->client_id;
				client->state = CLIENT_STATE_CONNECTED;
				break;
			case SERVER_PACKET_UPDATE:
				update_packet = (ServerUpdatePacket*)packet->data;
				break;
			default: break;
		}
	}
	arena_destroy(&packet_arena);
}
