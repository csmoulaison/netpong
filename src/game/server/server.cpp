#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

// TODO: It is time to implement:
// - Visual smoothing for mispredictions
//   - NOW: We have two things in flight at the moment: visual smoothing and
//     input attenuation. Both are disabled at the moment and we are seeing an
//     interesting jitter issue on only 1 client. This indicates there's some
//     unintended asymmetry going on here, presumably related to rollback?
//     1. Determine which client this is happening to.
//        - ANSWER: It's client id 1 (player 2, right paddle).
//        - NOTE: It's jittering both players on that client, and the
//        movement is smooth on the other client. WHY??
//     2. Brainstorm reasons this could happen and test them.
//        - NOTE: It still happens with NETWORK_SIM_MODE turned off.
//     3. It would probably be helpful to start logging some of the possible
//        issues here.
// - Disconnection and time out on both client and server side
// - Local multiplayer
// - Local bot
// - AND cleanup/comb for issues line by line, including packet serialization
// 
// THEN:
// - Some sound?
// - Debug text stuff
// - Menu with mode selection, etc
// 
// FROM THERE (NEW PROJECT):
// - Cylinders on flat plane -> then terrain
// - 3d with mouse look and WASD movement
// - Radial collision handling, try bounce/direct block, idk
//
// AND THEN:
// - Auto machine gun, not subject to client-side prediction. Must be resolved
//   client side on a delay, unlike other clients.
// - Physics scheme for mech stuff.

// TODO: Things we want to be able to test/log/handle:
// 1. Differing packet loss rates.
// 2. Differing packet latency/latency variance.
// 3. Client connections created and destroyed in various orders, mid-game.
// 4. Many missed client packets.
// 5. Clients running behind server due to slow down packets not being reversed
//    in time.
// 6. Clients running far ahead of server due to speed up packets not being
//    reversed in time.

struct ClientInput {
	// So that we can tell if a certain frame has been received yet.
	i32 frame;
	PlayerInput input;
};

struct ServerConnection {
	ClientInput inputs[INPUT_BUFFER_SIZE];
};

struct Server {
	PlatformSocket* socket;
	ServerConnection connections[MAX_CLIENTS];
	u8 connections_len;

	// Represents the current simulation frame, the number of frames elapsed since
	// the server session was initialized.
	// INPUT_BUFFER_SIZE % Server.frame = The client input associated with the
	// current simulation frame.
	i32 frame;
	World world;
};

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->socket = platform_init_server_socket(arena);
	server->frame = 0;

	server->connections_len = 0;
	for(u8 i = 0; i < MAX_CLIENTS; i++) {
		ServerConnection* client = &server->connections[i];
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
	// TODO: this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(server->socket, &packet_arena);
	PlatformPacket* packet = payload.head;

	// TODO: Make lookup table from connection_id to connected_client index, or
	// come up with some other solution for clients being disconnected.
	
	while(packet != nullptr) {
		i8 connection_id = packet->connection_id;
		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;

		ServerJoinAcknowledgePacket server_acknowledge_packet;
		ClientInputPacket* input_packet;
		ServerConnection* client;
		ClientInput* buffer_input;

		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				assert(connection_id <= server->connections_len);

				server_acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;
				server_acknowledge_packet.client_id = connection_id;
				server_acknowledge_packet.frame = server->frame;
				server_acknowledge_packet.world_state = server->world;
				platform_send_packet(server->socket, connection_id, (void*)&server_acknowledge_packet, sizeof(ServerJoinAcknowledgePacket));

				if(connection_id == server->connections_len) {
					printf("Acknowledging client join, adding client: %d\n", connection_id);
					server->connections_len++;
				}
				break;
			case CLIENT_PACKET_INPUT:
				input_packet = (ClientInputPacket*)packet->data;
				client = &server->connections[header->client_id];

				buffer_input = &client->inputs[input_packet->header.frame % INPUT_BUFFER_SIZE];
				buffer_input->frame = input_packet->header.frame;

				if(input_packet->input_move_up) {
					buffer_input->input.move_up = 1.0f;
				} else {
					buffer_input->input.move_up = 0.0f;
				}
				if(input_packet->input_move_down) {
					buffer_input->input.move_down = 1.0f;
				} else {
					buffer_input->input.move_down = 0.0f;
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

#if NETWORK_SIM_MODE
	platform_update_sim_mode(server->socket, delta_time);
#endif

	for(i8 i = 0; i < server->connections_len; i++) {
		ServerConnection* client = &server->connections[i];
		i32 latest_frame = 0;

		for(i32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			if(latest_frame < client->inputs[j].frame) {
				latest_frame = client->inputs[j].frame;
			}
		}

		// TODO: We want these thresholds to be set by measuring the ping for each
		// individual client.
		if(latest_frame - server->frame < 3) {
			ServerSpeedUpPacket speed_packet;
			speed_packet.header.type = SERVER_PACKET_SPEED_UP;
			speed_packet.header.frame = server->frame;
			platform_send_packet(server->socket, i, &speed_packet, sizeof(ServerSpeedUpPacket));
		} else if(latest_frame - server->frame > 5) {
			ServerSlowDownPacket slow_packet;
			slow_packet.header.type = SERVER_PACKET_SLOW_DOWN;
			slow_packet.header.frame = server->frame;
			platform_send_packet(server->socket, i, &slow_packet, sizeof(ServerSlowDownPacket));
		}
	}

	if(server->connections_len > 0) {
		for(u8 i = 0; i < server->connections_len; i++) {
			ServerConnection* client = &server->connections[i];

			i32 last_input_frame = server->frame;
			while(client->inputs[last_input_frame % INPUT_BUFFER_SIZE].frame != last_input_frame) {
				last_input_frame--;
				if(last_input_frame <= 0) {
					//printf("Server: No inputs yet from client %u.\n", i);
					last_input_frame = 0;
					break;
				}
			}

			// TODO: This is a client timeout. We should just disconnect the client here.
			if(last_input_frame != 0 && server->frame - last_input_frame > INPUT_BUFFER_SIZE) {
				panic();
			}

			server->world.player_inputs[i] = client->inputs[last_input_frame % INPUT_BUFFER_SIZE].input;

			if(last_input_frame != server->frame) {
				printf("Server: Missed a packet from client %u.\n", i);
			}
		}

		world_simulate(&server->world, delta_time);

		// TODO - Proper restart.
		if(server->world.ball_position[0] < -1.5f
		|| server->world.ball_position[0] > 1.5f) {
			server->world.ball_position[0] = 0.0f;
			server->world.ball_position[1] = 0.0f;
			server->world.ball_velocity[0] = 0.7f;
			server->world.ball_velocity[1] = 0.35f;
			server->world.countdown_to_start = START_COUNTDOWN_SECONDS;
		}

		ServerStateUpdatePacket update_packet = {};
		update_packet.header.type = SERVER_PACKET_STATE_UPDATE;
		update_packet.header.frame = server->frame;
		update_packet.world_state = server->world;

		for(u8 i = 0; i < server->connections_len; i++) {
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
