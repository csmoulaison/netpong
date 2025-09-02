#define INPUT_QUEUE_SIZE 64
#define BUFFERED_INPUTS_TARGET 3

/* TODO - Need a scheme for client local server.
enum ServerType {
	SERVER_TYPE_LOCAL,
	SERVER_TYPE_REMOTE
}; */

struct ConnectedClient {
	// NOW - not queue, buffer
	PlayerInput input_queue[INPUT_QUEUE_SIZE];
	i8 input_queue_front;
	i8 input_queue_back;
	i8 input_queue_len;
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

	for(u8 i = 0; i < MAX_CLIENTS; i++) {
		ConnectedClient* client;
		client->input_queue_front = 0;
		client->input_queue_back = 0;
		client->input_queue_len = 0;
	}
	server->connected_clients_len = 0;

	return server;
}

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
		ConnectedClient* client;
		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				server_acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;
				server_acknowledge_packet.client_id = connection_id;
				// NOW - server->frame + 1 gives us the right thing. That's simply because the pulling from the
				// input queue doesn't in any way compare the actual frames against the client frames. Much better
				// will be to again, change the input buffer to a ring buffer that keeps track of the oldest frame,
				// just like on the client. 
				server_acknowledge_packet.frame_number = server->frame + 1;
				assert(connection_id <= server->connected_clients_len);

				if(connection_id == server->connected_clients_len) {
					server->connected_clients_len++;
				}
				 
				platform_send_packet(server->socket, connection_id, (void*)&server_acknowledge_packet, sizeof(ServerJoinAcknowledgePacket));
				printf("Acknowledging client join.\n");
				break;
			case CLIENT_PACKET_INPUT:
				// NOW - Duh, we are assuming these are coming in order, etc. A simple
				// queueing procedure is just totally wrong for this. We will instead need
				// to have the head and tails of this thing be based on something more like
				// it is on the client side. Figure out what the last input we care about is
				// and disregard the rest.
				// 
				// If you ever lose track of what you're doing here, just imagine the packets
				// all coming in out of order, and filling the holes.
				// 
				// AH, this is why Overwatch talks about telling the client to speed up in
				// the event of a missed packet, instead of in the event of a big/small
				// buffer. It's because the hole in the buffer is your only rigorous way of
				// defining the need for more packets in the buffer. Rewatch to figure out
				// how they describe the need for the client to slow down the sending of
				// packets.

				// NOW - It should be noted that the order is currently correct, and we are
				// still having issues. Figure out why that would be.
				input_packet = (ClientInputPacket*)packet->data;
				client = &server->connected_clients[header->client_id];

				//printf("client frame: %u\n", header->frame_number);

				if(input_packet->input_move_up) {
					printf("Server: Client input received (Client %u, Server %u)\n", header->frame_number, server->frame);
				}

				assert(client->input_queue_len < INPUT_QUEUE_SIZE);
				client->input_queue[client->input_queue_back] = PlayerInput { 
					.move_up = input_packet->input_move_up, 
					.move_down = input_packet->input_move_down
				};
				client->input_queue_len++;
				client->input_queue_back = (client->input_queue_back + 1) % INPUT_QUEUE_SIZE;
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);

}

void server_update(Server* server, float delta_time)
{
	server_process_packets(server);

	if(server->connected_clients_len > 0) {
		for(u8 i = 0; i < server->connected_clients_len; i++) {
			ConnectedClient* client = &server->connected_clients[i];

			if(client->input_queue_len > BUFFERED_INPUTS_TARGET) {
				ServerSlowDownPacket slow_packet;
				slow_packet.header.type = SERVER_PACKET_SLOW_DOWN;
				slow_packet.header.frame_number = server->frame;
				platform_send_packet(server->socket, i, &slow_packet, sizeof(ServerSlowDownPacket));
			} else if(client->input_queue_len < BUFFERED_INPUTS_TARGET) {
				ServerSpeedUpPacket speed_packet;
				speed_packet.header.type = SERVER_PACKET_SPEED_UP;
				speed_packet.header.frame_number = server->frame;
				platform_send_packet(server->socket, i, &speed_packet, sizeof(ServerSpeedUpPacket));
			}

			if(client->input_queue_len > 0) {
				server->world.player_inputs[i] = client->input_queue[client->input_queue_front];
				if(client->input_queue_len > 1) {
					client->input_queue_front = (client->input_queue_front + 1) % INPUT_QUEUE_SIZE;
					client->input_queue_len--;
				} else {
					//printf("Warning: only 1 input in queue for player %u. (%u)\n", i, server->frame);
				}
			} else {
				server->world.player_inputs[i] = {0};
			}
		}

		float prev_paddle_pos = server->world.paddle_positions[0];
		world_simulate(&server->world, delta_time);
		if(server->world.paddle_positions[0] > prev_paddle_pos) {
			printf("Server: Paddle  UP  (Frame %u)\n", server->frame);
		} else if(server->world.paddle_positions[0] < prev_paddle_pos) {
			printf("Server: Paddle DOWN (Frame %u)\n", server->frame);
		}

		ServerStateUpdatePacket update_packet = {};
		update_packet.header.type = SERVER_PACKET_STATE_UPDATE;
		update_packet.header.frame_number = server->frame;
		update_packet.world_state = server->world;

		for(u8 i = 0; i < server->connected_clients_len; i++) {
			platform_send_packet(server->socket, i, &update_packet, sizeof(ServerStateUpdatePacket));
		}

		server->frame++;
	} else {
		server->frame = 0;
	}
}


bool server_close_requested(Server* server)
{
	return false;
}
