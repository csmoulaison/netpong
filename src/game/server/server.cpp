#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

#define MAX_SERVER_EVENTS 256

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

enum ServerEventType {
	SERVER_EVENT_CONNECTION_REQUEST,
	SERVER_EVENT_CLIENT_READY_TO_START,
	SERVER_EVENT_CLIENT_INPUT
};

struct ServerEvent {
	ServerEventType type;
	i32 client_id;
    union {
		ClientInput client_input;
	};
};

enum ServerSlotState {
	SERVER_CONNECTION_OPEN,
	SERVER_CONNECTION_PENDING,
	SERVER_CONNECTION_ACTIVE
};

struct ServerSlot {
	ServerSlotState state;
	// absolute frame % INPUT_BUFFER_SIZE = frame index
	ClientInput inputs[INPUT_BUFFER_SIZE];
	f32 ready_timeout_countdown;
};

struct Server {
	PlatformSocket* socket;
	ServerSlot slots[2];

	ServerEvent events[MAX_SERVER_EVENTS];
	u32 events_len;

	i32 frame;
	World world;
};

void server_push_event(Server* server, ServerEvent event) {
	server->events[server->events_len] = event;
	server->events_len++;
}

void server_reset_game(Server* server)
{
	server->frame = 0;
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* connection = &server->slots[i];
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			connection->inputs[j].frame = -1;
			connection->inputs[j].input = {};
		}
	}
	world_init(&server->world);
}

Server* server_init(Arena* arena, bool accept_remote_connections)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));

	if(accept_remote_connections) {
		server->socket = platform_init_server_socket(arena);
	} else {
		server->socket = nullptr;
	}

	for(u8 i = 0; i < 2; i++) {
		ServerSlot* connection = &server->slots[i];
		connection->state = SERVER_CONNECTION_OPEN;
	}

	server_reset_game(server);
	return server;
}

// Responds to incoming network requests to join the game as a client, sending
// back acceptance packets and setting the relevant slots to the PENDING
// state, where we will wait until we receive a counter acknowledgement from the
// new client.
void server_handle_connection_request(Server* server, i8 slot_id)
{
	if(slot_id > 1) {
		platform_free_connection(server->socket, slot_id);
		printf("Server: A client requested a connection (%u), but the game is full.\n", slot_id);
	}

	ServerSlot* client = &server->slots[slot_id];
	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
	if(client->state == SERVER_CONNECTION_OPEN) {
		client->state = SERVER_CONNECTION_PENDING;
		printf("Accepting client join, adding client: %d\n", slot_id);
	}

	ServerAcceptConnectionMessage accept_message;
	accept_message.type = SERVER_MESSAGE_ACCEPT_CONNECTION;
	accept_message.client_id = slot_id;
	platform_send_packet(server->socket, slot_id, (void*)&accept_message, sizeof(accept_message));

}

// Sets clients to the ACTIVE state in response to join acknowledgement packets,
// which are sent to us in response to acceptance packets we sent when handling
// connection requests.
// Once both slots are in the ACTIVE state, the server update function
// will use the active game codepath.
void server_handle_client_ready(Server* server, i8 slot_id)
{
	assert(slot_id <= 1);

	ServerSlot* client = &server->slots[slot_id];
	if(client->state == SERVER_CONNECTION_PENDING) {
		client->state = SERVER_CONNECTION_ACTIVE;
		printf("Received client acknowledgement, setting client connected: %d\n", slot_id);
	}

	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
}

// Stores newly recieved input in the relevant client's input buffer. These
// buffered inputs are pulled from once every frame in the server active update
// function.
void server_handle_client_input_message(Server* server, i8 slot_id, ClientInputMessage* client_input)
{
	ServerSlot* client = &server->slots[slot_id];

	i32 frame_delta = client_input->latest_frame - client_input->oldest_frame;
	for(i32 i = 0; i <= frame_delta; i++) {
		i32 input_frame = client_input->latest_frame - frame_delta + i;
		ClientInput* buffer_input = &client->inputs[input_frame % INPUT_BUFFER_SIZE];
		if(buffer_input->frame == input_frame) {
			continue;
		}

		ClientInput event_input;
		event_input.frame = input_frame;
		if(client_input->input_moves_up[i]) {
			event_input.input.move_up = 1.0f;
		} else {
			event_input.input.move_up = 0.0f;
		}
		if(client_input->input_moves_down[i]) {
			event_input.input.move_down = 1.0f;
		} else {
			event_input.input.move_down = 0.0f;
		}

		server_push_event(server, (ServerEvent){ 
			.type = SERVER_EVENT_CLIENT_INPUT, 
			.client_id = slot_id,
			.client_input = event_input
		});
	}
}

void server_handle_client_input(Server* server, i8 slot_id, ClientInput* client_input) 
{
	ServerSlot* client = &server->slots[slot_id];
	ClientInput* buffer_input = &client->inputs[client_input->frame % INPUT_BUFFER_SIZE];
	*buffer_input = *client_input;
}

void server_receive_messages(Server* server)
{
	// TODO: Allocate arena from existing arena.
	Arena packet_arena = arena_create(16000);
	PlatformPacket* packet = platform_receive_packets(server->socket, &packet_arena);

	while(packet != nullptr) {
		u8 type = *(u8*)packet->data;
		void* data = packet->data;
		switch(type) {
			case CLIENT_MESSAGE_REQUEST_CONNECTION:
				server_push_event(server, (ServerEvent){ .type = SERVER_EVENT_CONNECTION_REQUEST, .client_id = packet->connection_id });
				break;
			case CLIENT_MESSAGE_READY_TO_START:
				server_push_event(server, (ServerEvent){ .type = SERVER_EVENT_CLIENT_READY_TO_START, .client_id = packet->connection_id });
				break;
			case CLIENT_MESSAGE_INPUT:
				server_handle_client_input_message(server, packet->connection_id, (ClientInputMessage*)packet->data);
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

void server_update_idle(Server* server, f32 dt)
{
	server->frame = 0;
	for(i32 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];
		if(client->state != SERVER_CONNECTION_ACTIVE) {
			continue;
		}
		
		client->ready_timeout_countdown -= dt;
		if(client->ready_timeout_countdown <= 0.0f) {
			ServerDisconnectMessage disconnect_message;
			disconnect_message.type = SERVER_MESSAGE_DISCONNECT;
			platform_send_packet(server->socket, i, &disconnect_message, sizeof(disconnect_message));

			platform_free_connection(server->socket, i);
			client->state = SERVER_CONNECTION_OPEN;
			printf("Freed connection %u.\n", i);
		}
	}
}

void server_update_active(Server* server, f32 delta_time)
{
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];

		i32 latest_frame_received = -1;
		for(i32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			if(latest_frame_received < client->inputs[j].frame) {
				latest_frame_received = client->inputs[j].frame;
			}
		}

		// If we haven't received any input yet, keep telling the client that the game
		// has started, and break from the loop here.
		if(latest_frame_received == -1) {
			ServerStartGameMessage start_message;
			start_message.type = SERVER_MESSAGE_START_GAME;
			platform_send_packet(server->socket, i, (void*)&start_message, sizeof(start_message));
			printf("Server: Sent start game packet to client%u.\n", i);
			continue;
		// Client speed up if the server is too far ahead, client slow down if the
		// server is too far behind.
		// TODO: We want the thresholds to be based on current average ping to this client.
		} else if(latest_frame_received - server->frame < 3) {
			ServerSpeedUpMessage speed_message;
			speed_message.type = SERVER_MESSAGE_SPEED_UP;
			//speed_message.frame = server->frame;
			platform_send_packet(server->socket, i, &speed_message, sizeof(speed_message));
		} else if(latest_frame_received - server->frame > 6) {
			ServerSlowDownMessage slow_message;
			slow_message.type = SERVER_MESSAGE_SLOW_DOWN;
			//slow_message.frame = server->frame;
			platform_send_packet(server->socket, i, &slow_message, sizeof(slow_message));
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
			ServerDisconnectMessage disconnect_message;
			disconnect_message.type = SERVER_MESSAGE_DISCONNECT;
			platform_send_packet(server->socket, i, &disconnect_message, sizeof(disconnect_message));

			platform_free_connection(server->socket, i);
			server->slots[i].state = SERVER_CONNECTION_OPEN;
			printf("Freed connection %u.\n", i);

			i32 other_id = 0;
			if(i == 0) {
				other_id = 1;
			}

			ServerEndGameMessage end_message;
			end_message.type = SERVER_MESSAGE_END_GAME;
			platform_send_packet(server->socket, other_id, &end_message, sizeof(end_message));
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

	ServerWorldUpdateMessage update_message = {};
	update_message.type = SERVER_MESSAGE_WORLD_UPDATE;
	update_message.frame = server->frame;
	update_message.world = server->world;

	for(u8 i = 0; i < 2; i++) {
		platform_send_packet(server->socket, i, &update_message, sizeof(update_message));
	}
	server->frame++;
}

void server_process_events(Server* server)
{
	for(u32 i = 0; i < server->events_len; i++) {
		ServerEvent* event = &server->events[i];
		switch(event->type) {
			case SERVER_EVENT_CONNECTION_REQUEST:
				server_handle_connection_request(server, event->client_id);
				break;
			case SERVER_EVENT_CLIENT_READY_TO_START:
				server_handle_client_ready(server, event->client_id);
				break;
			case SERVER_EVENT_CLIENT_INPUT:
				server_handle_client_input(server, event->client_id, &event->client_input); 
				break;
			default: break;
		}
	}
	server->events_len = 0;
}

void server_update(Server* server, f32 delta_time)
{
	if(server->socket != nullptr) {
		server_receive_messages(server);
	}
	server_process_events(server);

#if NETWORK_SIM_MODE
	platform_update_sim_mode(server->socket, delta_time);
#endif

	if(server->slots[0].state == SERVER_CONNECTION_ACTIVE 
	&& server->slots[1].state == SERVER_CONNECTION_ACTIVE) {
		server_update_active(server, delta_time);
	} else {
		server_update_idle(server, delta_time);
	}
}

bool server_close_requested(Server* server)
{
	return false;
}
