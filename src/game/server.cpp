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
	union {
		i32 connection_id;
		i32 client_id;
	};
    union {
		ClientInput client_input;
	};
};

enum ServerSlotState {
	SERVER_SLOT_OPEN,
	SERVER_SLOT_PENDING,
	SERVER_SLOT_ACTIVE
};

// TODO: If we are making something with a larger number of players,
// differentiate types by array rather than with an enum.
enum ServerPlayerType {
	SERVER_PLAYER_LOCAL,
	SERVER_PLAYER_REMOTE,
	SERVER_PLAYER_BOT
};

struct ServerSlot {
	ServerSlotState state;
	ServerPlayerType type;
	ClientInput inputs[INPUT_BUFFER_SIZE];
	f32 ready_timeout_countdown;
	i32 connection_id;
};

struct Server {
	Network::Socket* socket;
	ServerSlot slots[2];
	i32 connection_to_client_ids[2];

	ServerEvent events[MAX_SERVER_EVENTS];
	u32 events_len;

	i32 frame;
	World world;
};

bool server_is_active(Server* server) 
{
	return server->slots[0].state == SERVER_SLOT_ACTIVE && server->slots[1].state == SERVER_SLOT_ACTIVE;
}

void server_push_event(Server* server, ServerEvent event) 
{
	server->events[server->events_len] = event;
	server->events_len++;
}

void server_reset_game(Server* server)
{
	server->frame = 0;
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* slot = &server->slots[i];
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			slot->inputs[j].frame = -1;
			slot->inputs[j].input = {};
		}
	}
	world_init(&server->world);
}

Server* server_init(Arena* arena, bool accept_client_connections)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));

	if(accept_client_connections) {
		server->socket = Network::init_server_socket(arena);
	} else {
		server->socket = nullptr;
	}

	for(u8 i = 0; i < 2; i++) {
		server->slots[i].state = SERVER_SLOT_OPEN;
		server->connection_to_client_ids[i] = -1;
	}

	server_reset_game(server);
	return server;
}

// TODO: Probably make these two return success or id, but that's not really
// useful for our current circumstance.
void server_add_local_player(Server* server)
{
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];
		if(client->state == SERVER_SLOT_OPEN) {
			client->state = SERVER_SLOT_ACTIVE;
			client->type = SERVER_PLAYER_LOCAL;
			return;
		}
	}
}

void server_add_bot(Server* server)
{
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];
		if(client->state == SERVER_SLOT_OPEN) {
			client->state = SERVER_SLOT_ACTIVE;
			client->type = SERVER_PLAYER_BOT;
			return;
		}
	}
}

// Responds to incoming network requests to join the game as a client, sending
// back acceptance packets and setting the relevant slots to the PENDING
// state, where we will wait until we receive a counter acknowledgement from the
// new client.
void server_handle_connection_request(Server* server, i32 connection_id)
{
	ServerSlot* client;
	i32 client_id;

	if(connection_id > 1) {
		printf("Server: Rejecting connection because the connection_id is > 1.\n");
		goto reject_connection_request;
	}

	client_id = -1;
	if(server->connection_to_client_ids[connection_id] != -1) {
		client_id = server->connection_to_client_ids[connection_id];
	} else {
		for(i8 i = 0; i < 2; i++) {
			ServerSlot* client  = &server->slots[i];
			if(client->state == SERVER_SLOT_OPEN) {
				client_id = i;
				break;
			}
		}
	}

	if(client_id == -1) {
		printf("Server: Rejecting connection because there are no open slots.\n");
		goto reject_connection_request;
	}

	client = &server->slots[client_id];
	client->state = SERVER_SLOT_PENDING;
	client->type = SERVER_PLAYER_REMOTE;
	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
	client->connection_id = connection_id;
	server->connection_to_client_ids[connection_id] = client_id;
	printf("Server: Accepting client join, adding client: %d\n", client_id);

	ServerAcceptConnectionMessage accept_message;
	accept_message.type = SERVER_MESSAGE_ACCEPT_CONNECTION;
	accept_message.client_id = client_id;
	Network::send_packet(server->socket, connection_id, (void*)&accept_message, sizeof(accept_message));
	return;

reject_connection_request:
	Network::free_connection(server->socket, connection_id);
	printf("Server: A client requested a connection (connection %u), but the game is full.\n", connection_id);
}

// Sets clients to the ACTIVE state in response to join acknowledgement packets,
// which are sent to us in response to acceptance packets we sent when handling
// connection requests.
// Once both slots are in the ACTIVE state, the server update function
// will use the active game codepath.
void server_handle_client_ready(Server* server, i32 client_id)
{
	ServerSlot* client = &server->slots[client_id];
	if(client->state == SERVER_SLOT_PENDING) {
		client->state = SERVER_SLOT_ACTIVE;
		printf("Server: Received client acknowledgement, setting client connected: %d\n", client_id);
	}
	client->ready_timeout_countdown = READY_TIMEOUT_LENGTH;
}

// Stores newly recieved input in the relevant client's input buffer. These
// buffered inputs are pulled from once every frame in the server active update
// function.
void server_handle_client_input_message(Server* server, i32 client_id, ClientInputMessage* client_input)
{
	ServerSlot* client = &server->slots[client_id];
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
			.client_id = client_id,
			.client_input = event_input
		});
	}
}

void server_handle_client_input(Server* server, i32 client_id, ClientInput* client_input) 
{
	ServerSlot* client = &server->slots[client_id];
	ClientInput* buffer_input = &client->inputs[client_input->frame % INPUT_BUFFER_SIZE];
	*buffer_input = *client_input;
}

void server_update_idle(Server* server, f32 dt)
{
	server->frame = 0;
	for(i32 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];
		if(client->state != SERVER_SLOT_ACTIVE) {
			continue;
		}

		if(client->type == SERVER_PLAYER_REMOTE) {
			client->ready_timeout_countdown -= dt;
			if(client->ready_timeout_countdown <= 0.0f) {
				ServerDisconnectMessage disconnect_message;
				disconnect_message.type = SERVER_MESSAGE_DISCONNECT;
				Network::send_packet(server->socket, client->connection_id, &disconnect_message, sizeof(disconnect_message));

				Network::free_connection(server->socket, client->connection_id);
				client->state = SERVER_SLOT_OPEN;
				printf("Server: Freed connection %u due to ready timeout.\n", i);
			}
		}
	}
}

void server_update_active(Server* server, f32 delta_time)
{
	for(u8 i = 0; i < 2; i++) {
		ServerSlot* client = &server->slots[i];

		i32 effective_input_frame = server->frame;

		if(client->type == SERVER_PLAYER_REMOTE) {
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
				Network::send_packet(server->socket, client->connection_id, (void*)&start_message, sizeof(start_message));
				printf("Server: Sent start game packet to client %u.\n", i);
				continue;
			// Client speed up if the server is too far ahead, client slow down if the
			// server is too far behind.
			// TODO: We want the thresholds to be based on current average ping to this client.
			} else if(latest_frame_received - server->frame < 3) {
				ServerSpeedUpMessage speed_message;
				speed_message.type = SERVER_MESSAGE_SPEED_UP;
				//speed_message.frame = server->frame;
				Network::send_packet(server->socket, client->connection_id, &speed_message, sizeof(speed_message));
			} else if(latest_frame_received - server->frame > 6) {
				ServerSlowDownMessage slow_message;
				slow_message.type = SERVER_MESSAGE_SLOW_DOWN;
				//slow_message.frame = server->frame;
				Network::send_packet(server->socket, client->connection_id, &slow_message, sizeof(slow_message));
			}

			// Optimally, we want to use the client input that coincides with the frame
			// the server is about to simulate, but if we don't have that frame, we will
			// use the last received input in the buffer.
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
			// 
			// TODO: We want to decouple this from the queue size, really. 
			if(server->frame - effective_input_frame > INPUT_BUFFER_SIZE) {
				ServerDisconnectMessage disconnect_message;
				disconnect_message.type = SERVER_MESSAGE_DISCONNECT;
				Network::send_packet(server->socket, client->connection_id, &disconnect_message, sizeof(disconnect_message));

				Network::free_connection(server->socket, client->connection_id);
				server->slots[i].state = SERVER_SLOT_OPEN;
				printf("Server: Freed connection %u due to input buffer overflow.\n", i);

				ServerSlot* other_client = &server->slots[0];
				if(i == 0) {
					other_client = &server->slots[1];
				}

				if(other_client->type == SERVER_PLAYER_REMOTE) {
					ServerEndGameMessage end_message;
					end_message.type = SERVER_MESSAGE_END_GAME;
					Network::send_packet(server->socket, other_client->connection_id, &end_message, sizeof(end_message));
					printf("Sent end game packet to connection %u due to other player timeout.\n", other_client->connection_id);
				}

				server_reset_game(server);
				return;
			}
		} else { 
			// Client is local or a bot. If the following assertion fires, it means
			// inputs events were not pushed for this frame.
			assert(client->inputs[effective_input_frame % INPUT_BUFFER_SIZE].frame == server->frame);
		}

		server->world.player_inputs[i] = client->inputs[effective_input_frame % INPUT_BUFFER_SIZE].input;
	}

	// Simulate using client inputs.
	world_simulate(&server->world, delta_time);
	// Reset the game if the ball has exited the play area.
	if(server->world.ball_position[0] < -1.5f
	|| server->world.ball_position[0] > 1.5f) {
		// TODO: Should be world_init.
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
		ServerSlot* client = &server->slots[i];
		if(client->type == SERVER_PLAYER_REMOTE) {
			Network::send_packet(server->socket, client->connection_id, &update_message, sizeof(update_message));
		}
	}

	server->frame++;
}

void server_process_packets(Server* server)
{
	// TODO: Allocate arena from existing arena.
	Arena packet_arena;
	arena_init(&packet_arena, 16000);
	Network::Packet* packet = Network::receive_packets(server->socket, &packet_arena);

	bool new_connections[2] = { false, false };

	while(packet != nullptr) {
		u8 type = *(u8*)packet->data;
		void* data = packet->data;

		i32 client_id = -1;
		if(packet->connection_id < 2) {
			client_id = server->connection_to_client_ids[packet->connection_id];
		}

		switch(type) {
			case CLIENT_MESSAGE_REQUEST_CONNECTION:
				assert(server != nullptr);

				if(!new_connections[packet->connection_id]) {
					new_connections[packet->connection_id] = true;
					server_push_event(server, (ServerEvent){ .type = SERVER_EVENT_CONNECTION_REQUEST, .connection_id = packet->connection_id });
				}
				break;
			case CLIENT_MESSAGE_READY_TO_START:
				assert(server != nullptr);
				assert(client_id != -1);
				assert(client_id < 2);

				server_push_event(server, (ServerEvent){ .type = SERVER_EVENT_CLIENT_READY_TO_START, .client_id = client_id });
				break;
			case CLIENT_MESSAGE_INPUT:
				assert(server != nullptr);
				assert(client_id != -1);
				assert(client_id < 2);

				server_handle_client_input_message(server, client_id, (ClientInputMessage*)packet->data);
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

void server_process_events(Server* server)
{
	for(u32 i = 0; i < server->events_len; i++) {
		ServerEvent* event = &server->events[i];
		switch(event->type) {
			case SERVER_EVENT_CONNECTION_REQUEST:
				server_handle_connection_request(server, event->connection_id);
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
	// Simulate bots
	for(u8 i = 0; i < 2; i++) {
		if(server->slots[i].type == SERVER_PLAYER_BOT) {
			World* world = &server->world;

			ClientInput event_input = {};
			event_input.frame = server->frame;

			if((i == 0 && world->ball_velocity[0] < 0.0f && world->ball_position[0] < 0.60f)
 			|| (i == 1 && world->ball_velocity[0] > 0.0f && world->ball_position[0] > -0.60f)) {
				f32 delta_pos = world->paddle_positions[i] - world->ball_position[1];
				if(delta_pos < -0.05f) {
					event_input.input.move_up = 1.0f;
				} else if(delta_pos > 0.05f) {
					event_input.input.move_down = 1.0f;
				}
			}

			server_push_event(server, (ServerEvent){ 
				.type = SERVER_EVENT_CLIENT_INPUT, 
				.client_id = i,
				.client_input = event_input
			});
		}
	}

	if(server->socket != nullptr) {
		server_process_packets(server);
	}

	server_process_events(server);

#if NETWORK_SIM_MODE
	Network::update_sim_mode(server->socket, delta_time);
#endif

	if(server_is_active(server)) {
		server_update_active(server, delta_time);
	} else {
		server_update_idle(server, delta_time);
	}
}

bool server_close_requested(Server* server)
{
	return false;
}
