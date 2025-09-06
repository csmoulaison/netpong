#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

struct ClientInput {
	// So that we can tell if a certain frame has been received yet.
	i32 frame;
	PlayerInput input;
};

struct ConnectedClient {
	ClientInput inputs[INPUT_BUFFER_SIZE];
};

struct Server {
	PlatformSocket* socket;

	World world;

	// Represents the current simulation frame, the number of frames elapsed since
	// the server session was initialized.
	//
	// INPUT_BUFFER_SIZE % Server.frame = The client input associated with the
	// current simulation frame.
	i32 frame;
	i32 latest_frame_received;

	// TODO - more than one connected client.
	ConnectedClient connected_clients[MAX_CLIENTS];
	u32 connected_clients_len;
};

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->socket = platform_init_server_socket(arena);
	server->frame = 0;
	server->latest_frame_received = 0;

	server->connected_clients_len = 0;
	for(u8 i = 0; i < MAX_CLIENTS; i++) {
		ConnectedClient* client;
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			client->inputs[j].frame = -1;
			client->inputs[j].input = {};
		}
	}

	world_init(&server->world);

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
		ClientInput* buffer_input;

		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				server_acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;
				server_acknowledge_packet.client_id = connection_id;

				// NOW - which frame should we send back? We want the client to start out
				// ahead, I suppose.
				server_acknowledge_packet.frame = server->frame;
				assert(connection_id <= server->connected_clients_len);

				if(connection_id == server->connected_clients_len) {
					server->connected_clients_len++;
				}
				 
				platform_send_packet(server->socket, connection_id, (void*)&server_acknowledge_packet, sizeof(ServerJoinAcknowledgePacket));
				printf("Acknowledging client join.\n");
				break;
			case CLIENT_PACKET_INPUT:
				// NOW - It should be noted that the order is currently correct, and we are
				// still having issues. Figure out why that would be.
				input_packet = (ClientInputPacket*)packet->data;
				client = &server->connected_clients[header->client_id];

				if(server->latest_frame_received < input_packet->header.frame) {
					server->latest_frame_received = input_packet->header.frame;
				}

				buffer_input = &client->inputs[input_packet->header.frame % INPUT_BUFFER_SIZE];
				buffer_input->frame = input_packet->header.frame;
				buffer_input->input.move_up = input_packet->input_move_up;
				buffer_input->input.move_down = input_packet->input_move_down;

				if(input_packet->header.frame - server->frame > INPUT_SLOWDOWN_THRESHOLD) {
					printf("Getting packets past input slowdown threshold (%i).\n", input_packet->header.frame - server->frame);

					ServerSlowDownPacket slow_packet;
					slow_packet.header.type = SERVER_PACKET_SLOW_DOWN;
					slow_packet.header.frame = server->frame;
					platform_send_packet(server->socket, header->client_id, &slow_packet, sizeof(ServerSlowDownPacket));
				}
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
	platform_update_sim_mode(server->socket, delta_time);

	if(server->connected_clients_len > 0) {
		// NOW - Hacky for now to make sure we've got some inputs.
		if(server->latest_frame_received < 2) {
			return;
		}

		for(u8 i = 0; i < server->connected_clients_len; i++) {
			ConnectedClient* client = &server->connected_clients[i];

			i32 last_input_frame = server->frame;
			while(client->inputs[last_input_frame % INPUT_BUFFER_SIZE].frame != last_input_frame) {
				last_input_frame--;
				if(last_input_frame <= 0) {
					printf("Server: No inputs yet from client %u.\n", i);
					last_input_frame = 0;
					break;
				}

				// NOW - This is a client timeout. We should just disconnect the client here.
				if(server->frame - last_input_frame > INPUT_BUFFER_SIZE) {
					panic();
				}
			}
			server->world.player_inputs[i] = client->inputs[last_input_frame % INPUT_BUFFER_SIZE].input;

			// Missed a client packet.
			if(last_input_frame != server->frame) {
				printf("Server: Missed a packet from client %u.\n", i);

				ServerSpeedUpPacket speed_packet;
				speed_packet.header.type = SERVER_PACKET_SPEED_UP;
				speed_packet.header.frame = server->frame;
				platform_send_packet(server->socket, i, &speed_packet, sizeof(ServerSpeedUpPacket));
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
		update_packet.header.frame = server->frame;
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
