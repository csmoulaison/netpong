#define WORLD_STATE_BUFFER_SIZE 64
#define MAX_CLIENT_EVENTS 256

#define FRAME_LENGTH_MOD 0.01f

enum ClientConnectionState {
	CLIENT_STATE_REQUESTING_CONNECTION,
	CLIENT_STATE_WAITING_TO_START,
	CLIENT_STATE_ACTIVE
};

struct ClientWorldState {
	World world;
	i32 frame;
};

// Processing network packets into events isn't strictly necessary on the client
// side, as the client is never receiving messages from a local source, but it
// will likely be useful for testing and the like.
enum ClientEventType {
	CLIENT_EVENT_END_GAME,
	CLIENT_EVENT_CONNECTION_ACCEPTED,
	CLIENT_EVENT_DISCONNECT,
	CLIENT_EVENT_INPUT_MOVE_UP,
	CLIENT_EVENT_INPUT_MOVE_DOWN,
	CLIENT_EVENT_WORLD_UPDATE,
	CLIENT_EVENT_SPEED_UP,
	CLIENT_EVENT_SLOW_DOWN
};

struct ClientEvent {
	ClientEventType type;
    union {
		i32 assigned_id;
		ClientWorldState world_update;
	};
};

struct Client {
	Network::Socket* socket;

	ClientEvent events[MAX_CLIENT_EVENTS];
	u32 events_len;

	bool move_up;
	bool move_down;

	ClientConnectionState connection_state;
	i8 id;

	// The buffer holds present and past states of the game simulation.
	ClientWorldState states[WORLD_STATE_BUFFER_SIZE];
	i32 frame;

	double frame_length;
};

void client_push_event(Client* client, ClientEvent event) {
	client->events[client->events_len] = event;
	client->events_len++;
}

// Utility functions
ClientWorldState* client_state_from_frame(Client* client, i32 frame)
{
	return &client->states[frame % WORLD_STATE_BUFFER_SIZE];
}

i8 client_get_other_id(Client* client)
{
	if(client->id == 0) {
		return 1;
	}
	return 0;
}

// Initialization functions
void client_reset_game(Client* client)
{
	client->frame = 0;
	client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);

	for(i32 i = 0; i < WORLD_STATE_BUFFER_SIZE; i++) {
		client->states[i].frame = -1;

		world_init(&client->states[i].world);
	}
}

Client* client_init(Arena* session_arena, char* ip_string)
{
	Client* client = (Client*)arena_alloc(session_arena, sizeof(Client));
	client->socket = Network::init_client_socket(session_arena, ip_string);

	client->events_len = 0;
	client->move_up = false;
	client->move_down = false;

	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
	client->id = -1;

	client_reset_game(client);

	return client;
}

// NOW: The names of client_simulate_frame and client_simulate_and_advance_frame
// are not particularly evocative of what they are used for in context, so we
// should come up with a good description of them based on their usage and see
// if there's a better name we can come up with.

void client_simulate_frame(World* world, Client* client)
{
	// $TODO: This is kind of a big huge thing I missed. The frame_length is being
	// used here, and it's now often going to be different than in the case of the
	// original simulation. We want to store the original frame length in each
	// stored state after simulation, and reuse it the same way we currently reuse
	// original inputs.
	// 
	// NOTE: The above ^ was written when this world_simulate call was inline to the
	// client_resolve_state_update rollback simulation loop. It really only applies
	// there, so when/if we resolve this, it will involve parameterizing
	// client_simulate_frame with the frame length.
	world_simulate(world, client->frame_length);

	i8 other_id = client_get_other_id(client);

	world->player_inputs[other_id].move_up -= INPUT_ATTENUATION_SPEED * client->frame_length;
	if(world->player_inputs[other_id].move_up < 0.0f) {
		world->player_inputs[other_id].move_up == 0.0f;
	}
	world->player_inputs[other_id].move_down -= INPUT_ATTENUATION_SPEED * client->frame_length;
	if(world->player_inputs[other_id].move_down < 0.0f) {
		world->player_inputs[other_id].move_down == 0.0f;
	}
}

void client_simulate_and_advance_frame(Client* client, Arena* frame_arena)
{
	// Copy previous frame if it exists, update input, and simulate.
	ClientWorldState* current_state = client_state_from_frame(client, client->frame);
	current_state->frame = client->frame;
	World* world = &current_state->world;

	if(client->frame > 0) {
		ClientWorldState* previous_state = client_state_from_frame(client, client->frame - 1);
		memcpy(world, &previous_state->world, sizeof(World));
	}

	if(client->move_up) {
		world->player_inputs[client->id].move_up = 1.0f;
	} else {
		world->player_inputs[client->id].move_up = 0.0f;
	}
	if(client->move_down) {
		world->player_inputs[client->id].move_down = 1.0f;
	} else {
		world->player_inputs[client->id].move_down = 0.0f;
	}

	client_simulate_frame(world, client);

	// Send rolling window of client inputs to the server.
	ClientInputMessage input_message = {};
	input_message.type = CLIENT_MESSAGE_INPUT;
	input_message.latest_frame = client->frame;

	input_message.oldest_frame = client->frame - INPUT_WINDOW_FRAMES + 1;
	if(input_message.oldest_frame < 0) { 
		input_message.oldest_frame = 0;
	}

	i32 frame_delta = input_message.latest_frame - input_message.oldest_frame;
	for(i32 i = 0; i <= frame_delta; i++) {
		i32 input_frame = client->frame - frame_delta + i;
	
		World* input_world = &client_state_from_frame(client, input_frame)->world;
		assert(client_state_from_frame(client, input_frame)->frame == input_frame);
		
		input_message.input_moves_up[i] = (input_world->player_inputs[client->id].move_up > 0.0f);
		input_message.input_moves_down[i] = (input_world->player_inputs[client->id].move_down > 0.0f);
	}

	SerializeResult serialized = serialize_client_input(SerializeMode::Write, &input_message, nullptr, frame_arena);
	Network::send_packet(client->socket, 0, serialized.data, serialized.size_bytes);

	client->frame++;
}

void client_handle_world_update(Client* client, ClientWorldState* server_state, Arena* frame_arena)
{
	if(client->connection_state != CLIENT_STATE_ACTIVE) {
		client->connection_state = CLIENT_STATE_ACTIVE;
		// NOW: Remove magic number 6.
		for(i8 i = 0; i < 6; i++) {
			client_simulate_and_advance_frame(client, frame_arena);
		}
		printf("Client: Received first world update from server. Starting simulation.\n");
	}

	i32 update_frame = server_state->frame;
	ClientWorldState* client_update_state = client_state_from_frame(client, update_frame);

	// If the update frame has not been previously simulated by the client, we
	// simulate up to that point. We want to avoid this happening in general, as
	// it means the client has fallen behind the server.
	if(client_update_state->frame != update_frame) {
		strict_assert(update_frame - client->frame < 100);

		client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
		while(client->frame < update_frame + 1) {
			client_simulate_and_advance_frame(client, frame_arena);
			printf("Client: Behind server (%i-%i), simulating catch up frame.\n", update_frame, client->frame);
		}
		printf("\033[31mClient fell behind server! client %u, server %u\033[0m\n", client_update_state->frame, update_frame);
	}

	World* client_world = &client_update_state->world;
	World* server_world = &server_state->world;

	// If the states are equal, client side prediction was successful and we do not
	// need to resimulate.
	if(server_world->paddle_positions[0] == client_world->paddle_positions[0]
	&& server_world->paddle_positions[1] == client_world->paddle_positions[1]
	&& server_world->paddle_velocities[0] == client_world->paddle_velocities[0]
	&& server_world->paddle_velocities[1] == client_world->paddle_velocities[1]) {
		return;
	}

	// Resimulate from the update frame received from the server up until our most
	// current client frame.
	assert(client->frame - update_frame >= 0);
	memcpy(client_world, server_world, sizeof(World));

	for(i32 i = update_frame; i < client->frame; i++) {
		World* previous_world = &client_state_from_frame(client, i)->world;
		World* current_world = &client_state_from_frame(client, i + 1)->world;

		PlayerInput cached_player_input = current_world->player_inputs[client->id];
		memcpy(current_world, previous_world, sizeof(World));
		current_world->player_inputs[client->id] = cached_player_input;
		client_simulate_frame(current_world, client);
	}
}

void client_process_packets(Client* client, Arena* frame_arena)
{
	Network::Packet* packet = Network::receive_packets(client->socket, frame_arena);

	while(packet != nullptr) {
		u32 type = *(u32*)packet->data;
		void* data = packet->data;

		// Variables used in the switch statement.
		ClientWorldState update_state;
		ServerWorldUpdateMessage world_update_message;
		ServerAcceptConnectionMessage accept_connection_message;

		assert(client != nullptr);
		switch(type) {
			case SERVER_MESSAGE_ACCEPT_CONNECTION:
				serialize_server_accept_connection(SerializeMode::Read, &accept_connection_message, (char*)data, nullptr);
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_CONNECTION_ACCEPTED, .assigned_id = accept_connection_message.client_id });
				break;
			case SERVER_MESSAGE_END_GAME:
				client_push_event(client, (ClientEvent) { .type = CLIENT_EVENT_END_GAME }); 
				break;
			case SERVER_MESSAGE_DISCONNECT:
				client_push_event(client, (ClientEvent) { .type = CLIENT_EVENT_DISCONNECT }); 
				break;
			case SERVER_MESSAGE_WORLD_UPDATE:
				serialize_server_world_update(SerializeMode::Read, &world_update_message, (char*)data, nullptr);
				update_state.world = world_update_message.world;
				update_state.frame = world_update_message.frame;
				client_push_event(client, (ClientEvent) { .type = CLIENT_EVENT_WORLD_UPDATE, .world_update = update_state }); 
				break;
			case SERVER_MESSAGE_SPEED_UP:
				client_push_event(client, (ClientEvent) { .type = CLIENT_EVENT_SPEED_UP }); 
				break;
			case SERVER_MESSAGE_SLOW_DOWN:
				client_push_event(client, (ClientEvent) { .type = CLIENT_EVENT_SLOW_DOWN }); 
				break;
			default: break;
		}
		packet = packet->next;
	}
}

void client_process_events(Client* client, Arena* frame_arena)
{
	client->move_up = false;
	client->move_down = false;

	for(u32 i = 0; i < client->events_len; i++) {
		ClientEvent* event = &client->events[i];
		switch(event->type) {
			case CLIENT_EVENT_CONNECTION_ACCEPTED:
				if(client->connection_state == CLIENT_STATE_REQUESTING_CONNECTION) {
					client->connection_state = CLIENT_STATE_WAITING_TO_START;
					client->id = event->assigned_id;
					printf("Client: Recieved connection acceptance from server.\n");
				}
				break;
			case CLIENT_EVENT_END_GAME:
				if(client->connection_state == CLIENT_STATE_ACTIVE) {
					client->connection_state = CLIENT_STATE_WAITING_TO_START;
					client_reset_game(client);
					printf("Received end game notice from server.\n");
				}
				break;
			case CLIENT_EVENT_DISCONNECT:
				if(client->connection_state != CLIENT_STATE_REQUESTING_CONNECTION) {
					client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
					client->id = -1;
					client_reset_game(client);
					printf("Received disconnect notice from server.\n");
				}
				break;
			case CLIENT_EVENT_WORLD_UPDATE:
				client_handle_world_update(client, &event->world_update, frame_arena); 
				break;
			case CLIENT_EVENT_INPUT_MOVE_UP:
				client->move_up = true; 
				break;
			case CLIENT_EVENT_INPUT_MOVE_DOWN:
				client->move_down = true; 
				break;
			case CLIENT_EVENT_SPEED_UP:
				client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
				break;
			case CLIENT_EVENT_SLOW_DOWN:
				client->frame_length = BASE_FRAME_LENGTH + (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
				break;
			default: break;
		}
	}
	client->events_len = 0;
}

void client_update(Client* client, Arena* frame_arena) 
{
	client_process_packets(client, frame_arena);
	client_process_events(client, frame_arena);

#if NETWORK_SIM_MODE
	// $TODO: It is certainly wrong to use client->frame_length for this, and we
	// should certainly have a notion of delta time which is decoupled from the
	// network tick.
	Network::update_sim_mode(client->socket, client->frame_length);
#endif

	// Used in switch statement
	ClientRequestConnectionMessage request_message;
	ClientReadyToStartMessage ready_message;

	switch(client->connection_state) {
		case CLIENT_STATE_REQUESTING_CONNECTION:
			request_message.type = CLIENT_MESSAGE_REQUEST_CONNECTION;
			Network::send_packet(client->socket, 0, (void*)&request_message, sizeof(request_message));
			break;
		case CLIENT_STATE_WAITING_TO_START:
			ready_message.type = CLIENT_MESSAGE_READY_TO_START;
			Network::send_packet(client->socket, 0, &ready_message, sizeof(ready_message));
			break;
		case CLIENT_STATE_ACTIVE:
			client_simulate_and_advance_frame(client, frame_arena);
			break;
		default: break;
	}
}

