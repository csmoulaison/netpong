#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

// TODO: < NEXT: it would be good to thin out the platform layer as much as possible,
// accreting its functionality to this. Specifically, the server should keep
// track of storing incoming connections.
// The platform layer is strictly for abstracting the platform details.
 
// TIME TO IMPLEMENT:
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
	float ready_timeout_countdown;
};

struct Server {
	PlatformSocket* socket;
	ServerConnection connections[2];

	i32 frame;
	World world;
};

void server_reset_game(Server* server)
{
	server->frame = 0;
	for(u8 i = 0; i < 2; i++) {
		ServerConnection* connection = &server->connections[i];
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			connection->inputs[j].frame = -1;
			connection->inputs[j].input = {};
		}
	}
	world_init(&server->world);
}

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->socket = platform_init_server_socket(arena);
	for(u8 i = 0; i < 2; i++) {
		ServerConnection* connection = &server->connections[i];
		connection->state = SERVER_CONNECTION_OPEN;
	}
	server_reset_game(server);
	return server;
}

// Responds to incoming network requests to join the game as a client, sending
// back acceptance packets and setting the relevant connections to the PENDING
// state, where we will wait until we receive a counter acknowledgement from the
// new client.
void server_handle_connection_request(Server* server, i8 connection_id)
{
	if(connection_id > 1) {
		platform_free_connection(server->socket, connection_id);
		printf("Server: A client requested a connection (%u), but the game is full.\n", connection_id);
	}

	ServerConnection* client = &server->connections[connection_id];
	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
	if(client->state == SERVER_CONNECTION_OPEN) {
		client->state = SERVER_CONNECTION_PENDING;
		printf("Accepting client join, adding client: %d\n", connection_id);
	}

	ServerAcceptConnectionPacket accept_packet;
	accept_packet.header.type = SERVER_PACKET_ACCEPT_CONNECTION;
	accept_packet.client_id = connection_id;
	platform_send_packet(server->socket, connection_id, (void*)&accept_packet, sizeof(accept_packet));

}

// Sets clients to the ACTIVE state in response to join acknowledgement packets,
// which are sent to us in response to acceptance packets we sent when handling
// connection requests.
// Once both connections are in the ACTIVE state, the server update function
// will use the active game codepath.
void server_handle_client_ready(Server* server, i8 connection_id)
{
	assert(connection_id <= 1);

	ServerConnection* client = &server->connections[connection_id];
	if(client->state == SERVER_CONNECTION_PENDING) {
		client->state = SERVER_CONNECTION_ACTIVE;
		printf("Received client acknowledgement, setting client connected: %d\n", connection_id);
	}

	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
}

// Stores newly recieved input in the relevant client's input buffer. These
// buffered inputs are pulled from once every frame in the server active update
// function.
void server_handle_client_input(Server* server, i8 connection_id, ClientInputPacket* client_input)
{
	ServerConnection* client = &server->connections[connection_id];

	i32 frame_delta = client_input->latest_frame - client_input->oldest_frame;
	//printf("Server: Frame delta: %i for client %i.\n", frame_delta, connection_id);
	for(i32 i = 0; i <= frame_delta; i++) {
		i32 input_frame = client_input->latest_frame - frame_delta + i;
		ClientInput* buffer_input = &client->inputs[input_frame % INPUT_BUFFER_SIZE];
		if(buffer_input->frame == input_frame) {
			continue;
		}

		buffer_input->frame = input_frame;

		if(client_input->input_moves_up[i]) {
			buffer_input->input.move_up = 1.0f;
		} else {
			buffer_input->input.move_up = 0.0f;
		}
		if(client_input->input_moves_down[i]) {
			buffer_input->input.move_down = 1.0f;
		} else {
			buffer_input->input.move_down = 0.0f;
		}
	}
}

void server_process_packets(Server* server)
{
	// TODO: this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(16000);
	PlatformPayload payload = platform_receive_packets(server->socket, &packet_arena);
	PlatformPacket* packet = payload.head;

	while(packet != nullptr) {
		i8 connection_id = packet->connection_id;
		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;

		switch(header->type) {
			case CLIENT_PACKET_REQUEST_CONNECTION:
				server_handle_connection_request(server, connection_id); break;
			case CLIENT_PACKET_READY_TO_START:
				server_handle_client_ready(server, connection_id); break;
			case CLIENT_PACKET_INPUT:
				server_handle_client_input(server, connection_id, (ClientInputPacket*)packet->data); break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

// TODO: In general, it's probably best to process packets and then handle them
// here. More centralized and decoupled from the network flow.
//
// It's the event queue from Quake, let's be honest, let's not mince words,
// let's not break balls.
void server_update_idle(Server* server, float dt)
{
	for(i32 i = 0; i < 2; i++) {
		ServerConnection* client = &server->connections[i];
		if(client->state == SERVER_CONNECTION_PENDING) {
			printf("client %i pending\n", i);
		} else if(client->state == SERVER_CONNECTION_ACTIVE) {
			printf("client %i active\n", i);
		} else if(client->state == SERVER_CONNECTION_OPEN) {
			printf("client %i open\n", i);
		}
	}
	
	server->frame = 0;
	for(i32 i = 0; i < 2; i++) {
		ServerConnection* client = &server->connections[i];
		if(client->state != SERVER_CONNECTION_ACTIVE) {
			continue;
		}
		
		client->ready_timeout_countdown -= dt;
		if(client->ready_timeout_countdown <= 0.0f) {
			ServerDisconnectPacket disconnect_packet;
			disconnect_packet.header.type = SERVER_PACKET_DISCONNECT;
			platform_send_packet(server->socket, i, &disconnect_packet, sizeof(disconnect_packet));

			platform_free_connection(server->socket, i);
			client->state = SERVER_CONNECTION_OPEN;
			printf("Freed connection %u.\n", i);
		}
	}
}

void server_update_active(Server* server, float delta_time)
{
	for(u8 i = 0; i < 2; i++) {
		ServerConnection* client = &server->connections[i];

		i32 latest_frame_received = -1;
		for(i32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			if(latest_frame_received < client->inputs[j].frame) {
				latest_frame_received = client->inputs[j].frame;
			}
		}

		// If we haven't received any input yet, keep telling the client that the game
		// has started, and break from the loop here.
		if(latest_frame_received == -1) {
			ServerStartGamePacket start_packet;
			start_packet.header.type = SERVER_PACKET_START_GAME;
			platform_send_packet(server->socket, i, (void*)&start_packet, sizeof(start_packet));
			printf("Server: Sent start game packet to client%u.\n", i);
			continue;
		// Client speed up if the server is too far ahead, client slow down if the
		// server is too far behind.
		// TODO: We want the thresholds to be based on current average ping to this client.
		} else if(latest_frame_received - server->frame < 3) {
			ServerSpeedUpPacket speed_packet;
			speed_packet.header.type = SERVER_PACKET_SPEED_UP;
			//speed_packet.frame = server->frame;
			platform_send_packet(server->socket, i, &speed_packet, sizeof(speed_packet));
		} else if(latest_frame_received - server->frame > 6) {
			ServerSlowDownPacket slow_packet;
			slow_packet.header.type = SERVER_PACKET_SLOW_DOWN;
			//slow_packet.frame = server->frame;
			platform_send_packet(server->socket, i, &slow_packet, sizeof(slow_packet));
		}

		// Optimally, we want to use the client input that coincides with the frame
		// the server is about to simulate, but if we don't have that frame, we will
		// use the last received input in the buffer.
		i32 effective_input_frame = server->frame;
		if(latest_frame_received < server->frame) {
			effective_input_frame = latest_frame_received;
		} else {
			while(client->inputs[effective_input_frame % INPUT_BUFFER_SIZE].frame != effective_input_frame) {
				effective_input_frame--;

				// We have already exited the loop if we haven't received any frames yet,
				// so this shouldn't fire.
				assert(effective_input_frame >= 0);
			}
		}
		if(effective_input_frame != server->frame) {
			printf("Server: Missed a packet from client %u.\n", i);
		}

		// Client timeout. We haven't received any inputs from the client for
		// INPUT_BUFFER_SIZE frames.
		// TODO: We want to decouple this from the queue size, really. 
		if(server->frame - effective_input_frame > INPUT_BUFFER_SIZE) {
			ServerDisconnectPacket disconnect_packet;
			disconnect_packet.header.type = SERVER_PACKET_DISCONNECT;
			platform_send_packet(server->socket, i, &disconnect_packet, sizeof(disconnect_packet));

			platform_free_connection(server->socket, i);
			server->connections[i].state = SERVER_CONNECTION_OPEN;
			printf("Freed connection %u.\n", i);

			i32 other_id = 0;
			if(i == 0) {
				other_id = 1;
			}

			// NOW: What if the other connection is already open? Right now, this client
			// timeout only happens when both connections are active. We will need to
			// expect some other continuous packet from the clients in order to make this
			// work properly.
			ServerEndGamePacket end_packet;
			end_packet.header.type = SERVER_PACKET_END_GAME;
			platform_send_packet(server->socket, other_id, &end_packet, sizeof(end_packet));
			printf("Sent end game packet to %u.\n", other_id);

			server_reset_game(server);
			return;
		}

		server->world.player_inputs[i] = client->inputs[effective_input_frame % INPUT_BUFFER_SIZE].input;
	}

	// Simulate using client inputs.
	world_simulate(&server->world, delta_time);

	// Reset the game if the ball has exited the play area.
	// 
	// TODO: This is game logic, once we decouple this from the server, it will
	// have to find it's way somewhere else.
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

	ServerWorldUpdatePacket update_packet = {};
	update_packet.header.type = SERVER_PACKET_WORLD_UPDATE;
	update_packet.frame = server->frame;
	update_packet.world = server->world;

	for(u8 i = 0; i < 2; i++) {
		platform_send_packet(server->socket, i, &update_packet, sizeof(update_packet));
	}
	server->frame++;
}

void server_update(Server* server, float delta_time)
{
	server_process_packets(server);

#if NETWORK_SIM_MODE
	platform_update_sim_mode(server->socket, delta_time);
#endif

	if(server->connections[0].state == SERVER_CONNECTION_ACTIVE 
	&& server->connections[1].state == SERVER_CONNECTION_ACTIVE) {
		server_update_active(server, delta_time);
	} else {
		server_update_idle(server, delta_time);
	}
}

bool server_close_requested(Server* server)
{
	return false;
}
