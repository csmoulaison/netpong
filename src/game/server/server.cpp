#define MAX_BUFFERED_INPUTS 64

/* TODO - Need a scheme for client local server.
enum ServerType {
	SERVER_TYPE_LOCAL,
	SERVER_TYPE_REMOTE
}; */

struct ConnectedClient {
	PlayerInput input_buffer[MAX_BUFFERED_INPUTS];
	u8 input_buffer_len;
};

struct Server {
	PlatformSocket* socket;

	World world;
	u32 frame;

	// TODO - more than one connected client.
	ConnectedClient connected_clients[MAX_CLIENTS];
	u32 connected_clients_len;
};

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->socket = platform_init_server_socket(arena);
	server->frame = 0;

	world_init(&server->world);
	server->connected_clients_len = 0;

	return server;
}

void server_update(Server* server)
{

}

// NOW - this is sort of just the server update function right now. Clean up!
void server_process_packets(Server* server)
{
	// TODO - this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(server->socket, &packet_arena);
	PlatformPacket* packet = payload.head;

	// TODO - Make lookup table from connection_id to connected_client index, or
	// come up with some other solution for clients being disconnected.
	
	while(packet != nullptr) {
		i8 connection_id = packet->connection_id;

		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;

		ServerJoinAcknowledgePacket server_acknowledge_packet;
		ClientInputPacket* input_packet;

		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				server_acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;
				server_acknowledge_packet.client_id = connection_id;
				server_acknowledge_packet.frame_number = server->frame;

				assert(connection_id <= server->connected_clients_len);
				if(connection_id == server->connected_clients_len) {
					server->connected_clients_len++;
				}
				 
				platform_send_packet(server->socket, connection_id, (void*)&server_acknowledge_packet, sizeof(ServerJoinAcknowledgePacket));
				printf("Acknowledging client join.\n");
				break;
			case CLIENT_PACKET_INPUT:
				input_packet = (ClientInputPacket*)packet;
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);

	if(server->connected_clients_len == 0) {
		server->frame = 0;
	} else {
		server->frame++;
	}
}

bool server_close_requested(Server* server)
{
	return false;
}
