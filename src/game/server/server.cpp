#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

// NOW: < THIS: packets
// 
// After: it would be good to thin out the platform layer as much as possible,
// accreting its functionality to this. Specifically, the server should keep
// track of storing incoming connections.
// The platform layer is strictly for abstracting the platform details.
 
// TIME TO IMPLEMENT:
// - Disconnection and time out on both client and server side.
//   - Add another phase for clients to be in where are waiting for both players
//     to exist.
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
	i32 frame;
	PlayerInput input;
};

enum ServerConnectionState {
	SERVER_CONNECTION_OPEN,
	SERVER_CONNECTION_PENDING,
	SERVER_CONNECTION_ACTIVE
};

struct ServerConnection {
	ServerConnectionState state;
	// absolute frame % INPUT_BUFFER_SIZE = frame index
	ClientInput inputs[INPUT_BUFFER_SIZE];
};

struct Server {
	PlatformSocket* socket;
	ServerConnection connections[2];

	i32 frame;
	World world;
};

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));

	server->state = SERVER_IDLE;
	server->socket = platform_init_server_socket(arena);
	server->frame = 0;

	for(u8 i = 0; i < 2; i++) {
		ServerConnection* connection = &server->connections[i];
		connection->state = SERVER_CONNECTION_OPEN;
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			connection->inputs[j].frame = -1;
			connection->inputs[j].input = {};
		}
	}

	world_init(&server->world);
	return server;
}

void server_handle_connection_request(Server* server, i8 connection_id)
{
	if(connection_id > 1) {
		platform_free_connection(server->socket, connection_id);
		printf("Server: A client requested a connection (%u), but the game is full.\n", connection_id);
	}

	if(server->connections[connection_id].state == SERVER_CONNECTION_OPEN) {
		server->connections[connection_id].state = SERVER_CONNECTION_PENDING;
		printf("Accepting client join, adding client: %d\n", connection_id);
	}

	ServerAcceptConnectionPacket accept_packet;
	accept_packet.header.type = SERVER_PACKET_ACCEPT_CONNECTION;
	accept_packet.client_id = connection_id;
	platform_send_packet(server->socket, connection_id, (void*)&accept_packet, sizeof(ServerAcceptConnectionPacket));

}

void server_handle_join_acknowledge(Server* server, i8 connection_id)
{
	assert(connection_id <= 1);

	if(server->connections[connection_id].state == SERVER_CONNECTION_PENDING) {
		server->connections[connection_id].state = SERVER_CONNECTION_ACTIVE;
		printf("Received client acknowledgement, setting client connected: %d\n", connection_id);

	}
}

void server_handle_client_input(Server* server)
{
	ClientInputPacket* input_packet = (ClientInputPacket*)packet->data;
	ServerConnection* client = &server->connections[header->client_id];

	ClientInput* buffer_input = &client->inputs[input_packet->header.frame % INPUT_BUFFER_SIZE];
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
}

void server_process_packets(Server* server)
{
	// TODO: this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(server->socket, &packet_arena);
	PlatformPacket* packet = payload.head;

	while(packet != nullptr) {
		i8 connection_id = packet->connection_id;

		switch(header->type) {
			case CLIENT_PACKET_CONNECTION_REQUEST:
				server_handle_connection_request(server, connection_id); break;
			case CLIENT_PACKET_JOIN_ACKNOWLEDGE:
				server_handle_join_acknowledge(server, connection_id); break;
			case CLIENT_PACKET_INPUT:
				server_handle_client_input(server); break;
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

	if(server->connections[0].state == SERVER_CONNECTION_ACTIVE 
	&& server->connections[1].state == SERVER_CONNECTION_ACTIVE) {
		for(u8 i = 0; i < 2; i++) {
			// NOW: Moved this up from a loop that previously existed outside of this one.
			// Is it in any way redundant with below?
			// REFERENCE MARK: YUPEE
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

			// NOW: Start game packet if we haven't received anything from this client yet.
			if(?) {
				ServerStartGamePacket start_packet;
				start_packet.header.type = SERVER_PACKET_ACCEPT_CONNECTION;
				start_packet.client_id = i;
				platform_send_packet(server->socket, i, (void*)&start_packet, sizeof(ServerStartGamePacket));
			}

			// REFERENCE MARK: YUPEE
			i32 last_input_frame = server->frame;
			while(client->inputs[last_input_frame % INPUT_BUFFER_SIZE].frame != last_input_frame) {
				last_input_frame--;
				if(last_input_frame <= 0) {
					//printf("Server: No inputs yet from client %u.\n", i);
					last_input_frame = 0;
					break;
				}
			}

			if(last_input_frame != 0 && server->frame - last_input_frame > INPUT_BUFFER_SIZE) {
				// NOW: This is the client timeout. Is it working?
				ServerDisconnectPacket disconnect_packet;
				disconnect_packet.header.type = SERVER_PACKET_DISCONNECT;
				disconnect_packet.header.frame = 0;
				platform_send_packet(server->socket, i, &disconnect_packet, sizeof(ServerDisconnectPacket));

				platform_free_connection(server->socket, i);
				server->connections[i].connected = false;
				printf("Freed connection %u.\n", i);
			}

			server->world.player_inputs[i] = client->inputs[last_input_frame % INPUT_BUFFER_SIZE].input;

			if(last_input_frame != server->frame) {
				printf("Server: Missed a packet from client %u.\n", i);
			}
		}

		world_simulate(&server->world, delta_time);

		if(server->world.ball_position[0] < -1.5f
		|| server->world.ball_position[0] > 1.5f) {
			// TODO: Should be world_init. Anything wrong with that?
			server->world.ball_position[0] = 0.0f;
			server->world.ball_position[1] = 0.0f;
			server->world.ball_velocity[0] = 0.7f;
			server->world.ball_velocity[1] = 0.35f;
			server->world.paddle_positions[0] = 0.0f;
			server->world.paddle_positions[1] = 0.0f;
			server->world.countdown_to_start = START_COUNTDOWN_SECONDS;
		}

		ServerStateUpdatePacket update_packet = {};
		update_packet.header.type = SERVER_PACKET_STATE_UPDATE;
		update_packet.header.frame = server->frame;
		update_packet.world_state = server->world;

		for(u8 i = 0; i < 2; i++) {
			if(server->connections[i].connected) {
				platform_send_packet(server->socket, i, &update_packet, sizeof(ServerStateUpdatePacket));
			}
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
