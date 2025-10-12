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

enum ClientEventType {
	CLIENT_EVENT_START_GAME,
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
	PlatformSocket* socket;

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

	f32 visual_ball_position[2];
	f32 visual_paddle_positions[2];
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

Client* client_init(Platform* platform, Arena* arena, char* ip_string)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));

	client->events_len = 0;
	client->move_up = false;
	client->move_down = false;

	client->socket = platform_init_client_socket(arena, ip_string);
	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
	client->id = -1;

	client_reset_game(client);

	return client;
}

// Frame simulation functions
void client_simulate_frame(World* world, Client* client)
{
	// TODO: This is kind of a big huge thing I missed. The frame_length is being
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
	// TODO: Test input attenuation against intended case.
	world->player_inputs[other_id].move_up -= INPUT_ATTENUATION_SPEED * client->frame_length;
	if(world->player_inputs[other_id].move_up < 0.0f) {
		world->player_inputs[other_id].move_up == 0.0f;
	}
	world->player_inputs[other_id].move_down -= INPUT_ATTENUATION_SPEED * client->frame_length;
	if(world->player_inputs[other_id].move_down < 0.0f) {
		world->player_inputs[other_id].move_down == 0.0f;
	}
}

void client_simulate_and_advance_frame(Client* client, Platform* platform)
{
	ClientWorldState* previous_state = client_state_from_frame(client, client->frame - 1);
	ClientWorldState* current_state = client_state_from_frame(client, client->frame);

	if(client->frame > 0) {
		memcpy(&current_state->world, &previous_state->world, sizeof(World));
	}

	current_state->frame = client->frame;
	World* world = &current_state->world;

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

	platform_send_packet(client->socket, 0, &input_message, sizeof(input_message));

	client->frame++;
}

// Message handling functions
void client_handle_world_update(Client* client, ClientWorldState* server_state, Platform* platform)
{
	i32 update_frame = server_state->frame;
	ClientWorldState* update_state = client_state_from_frame(client, update_frame);

	if(update_state->frame != update_frame) {
		client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
		while(client->frame < update_frame + 1) {
			client_simulate_and_advance_frame(client, platform);
		}

		printf("\033[31mClient fell behind server! client %u, server %u\033[0m\n", update_state->frame, update_frame);
	}

	World* client_world = &update_state->world;
	World* server_world = &server_state->world;

	// If the states are equal, client side prediction was successful and we do not
	// need to resimulate.
	if(server_world->paddle_positions[0] == client_world->paddle_positions[0]
	&& server_world->paddle_positions[1] == client_world->paddle_positions[1]
	&& server_world->paddle_velocities[0] == client_world->paddle_velocities[0]
	&& server_world->paddle_velocities[1] == client_world->paddle_velocities[1]) {
		return;
	}

	assert(client->frame - update_frame >= 0);
	memcpy(client_world, server_world, sizeof(World));

	for(i32 i = update_frame + 1; i <= client->frame; i++) {
		World* previous_world = &client_state_from_frame(client, i - 1)->world;
		World* current_world = &client_state_from_frame(client, i)->world;

		PlayerInput cached_player_input = current_world->player_inputs[client->id];
		memcpy(current_world, previous_world, sizeof(World));

		current_world->player_inputs[client->id] = cached_player_input;

		client_simulate_frame(current_world, client);
	}
}

// Our request to connect has been accepted by the server, so we store our
// newly received client id and switch to the WAITING_TO_START state.
// 
// In this state, we will continually send back a counter acknowledgement so the
// server knows we are ready. This happens in the client update loop.
void client_handle_accept_connection(Client* client, i32 client_id)
{
	if(client->connection_state == CLIENT_STATE_REQUESTING_CONNECTION) {
		client->id = client_id;
		client->connection_state = CLIENT_STATE_WAITING_TO_START;

		printf("Recieved connection acceptance from server.\n");
	}
}

// The server wants us to start the game, so we simulate some advance frames and
// enter the ACTIVE state.
void client_handle_start_game(Client* client, Platform* platform)
{
	client->connection_state = CLIENT_STATE_ACTIVE;
	for(i8 i = 0; i < 6; i++) {
		client_simulate_and_advance_frame(client, platform);
	}

	printf("Received start game notice from server.\n");
}

void client_handle_end_game(Client* client)
{
	client->connection_state = CLIENT_STATE_WAITING_TO_START;
	client_reset_game(client);

	printf("Received end game notice from server.\n");
}

void client_handle_disconnect(Client* client)
{
	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
	client->id = -1;
	client_reset_game(client);
	
	printf("Received disconnect notice from server.\n");
}

void client_handle_speed_up(Client* client)
{
	client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
}

void client_handle_slow_down(Client* client)
{
	client->frame_length = BASE_FRAME_LENGTH + (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
}

void client_pull_messages(Client* client)
{
	// TODO: Allocate arena from existing arena.
	Arena packet_arena = arena_create(16000);
	PlatformPacket* packet = platform_receive_packets(client->socket,&packet_arena);

	while(packet != nullptr) {
		// Variables used in the switch statement.
		ClientWorldState update_state;

		void* data = packet->data;
		u8 type = *(u8*)packet->data;
		switch(type) {
			case SERVER_MESSAGE_WORLD_UPDATE:
				update_state.world = ((ServerWorldUpdateMessage*)data)->world;
				update_state.frame = ((ServerWorldUpdateMessage*)data)->frame;
				client_push_event(client, (ClientEvent){ 
					.type = CLIENT_EVENT_WORLD_UPDATE, 
					.world_update = update_state 
				}); 
				break;
			case SERVER_MESSAGE_ACCEPT_CONNECTION:
				client_push_event(client, (ClientEvent){ 
					.type = CLIENT_EVENT_CONNECTION_ACCEPTED, 
					.assigned_id = ((ServerAcceptConnectionMessage*)data)->client_id 
				});
				break;
			case SERVER_MESSAGE_START_GAME:
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_START_GAME }); 
				break;
			case SERVER_MESSAGE_END_GAME:
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_END_GAME }); 
				break;
			case SERVER_MESSAGE_DISCONNECT:
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_DISCONNECT }); 
				break;
			case SERVER_MESSAGE_SPEED_UP:
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_SPEED_UP }); 
				break;
			case SERVER_MESSAGE_SLOW_DOWN:
				client_push_event(client, (ClientEvent){ .type = CLIENT_EVENT_SLOW_DOWN }); 
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

// Client update functions
void client_render_box(RenderState* render_state, Rect box, Platform* platform)
{
	f32 x_scale = (f32)platform->window_height / platform->window_width;

	Rect* rect = &render_state->boxes[render_state->boxes_len];
	render_state->boxes_len += 1;

	rect->x = box.x * x_scale;
	rect->y = box.y;
	rect->w = box.w;
	rect->h = box.h;
}

void client_update_requesting_connection(Client* client, Platform* platform, RenderState* render_state)
{
	ClientRequestConnectionMessage request_message;
	request_message.type = CLIENT_MESSAGE_REQUEST_CONNECTION;
	platform_send_packet(client->socket, 0, (void*)&request_message, sizeof(request_message));

	// Render "connecting" indicator.
	for(u8 i = 0; i < 3; i++) {
		f32 xoff = -1000.0f;
		if((client->frame / 60) % 3 == 0) {
			xoff = 0.0f;
		}
	
		Rect box;
		box.x = -0.75f + xoff;
		box.y = 0.75f;
		box.w = 0.025f;
		box.h = 0.025f;
		client_render_box(render_state, box, platform);
	}
}

void client_update_waiting_to_start(Client* client, Platform* platform, RenderState* render_state)
{
	ClientReadyToStartMessage ready_message;
	ready_message.type = CLIENT_MESSAGE_READY_TO_START;
	platform_send_packet(client->socket, 0, &ready_message, sizeof(ready_message));

	// Render "connecting" indicator.
	for(u8 i = 0; i < 3; i++) {
		f32 xoff = 0.0f;
		xoff += ((client->frame / 30) % 3) * 0.025;

		Rect box;
		box.x = -0.75f + xoff;
		box.y = 0.75f;
		box.w = 0.025f;
		box.h = 0.025f;
		client_render_box(render_state, box, platform);
	}
}

void client_visual_lerp(f32* visual, f32 real, f32 dt)
{
	*visual = lerp(*visual, real, VISUAL_SMOOTHING_SPEED * dt);
	if(abs(*visual - real) < VISUAL_SMOOTHING_EPSILON) {
		*visual = real;
	}
}

void client_update_active(Client* client, Platform* platform, RenderState* render_state)
{
	client_simulate_and_advance_frame(client, platform);

	World* world = &client_state_from_frame(client, client->frame - 1)->world;

	client_visual_lerp(&client->visual_ball_position[0], world->ball_position[0], client->frame_length * 4.0f);
	client_visual_lerp(&client->visual_ball_position[1], world->ball_position[1], client->frame_length * 4.0f);

	i8 other_id = client_get_other_id(client);
	client_visual_lerp(&client->visual_paddle_positions[other_id], world->paddle_positions[other_id], client->frame_length);
	client->visual_paddle_positions[client->id] = world->paddle_positions[client->id];

	// Render
	for(u8 i = 0; i < 2; i++) {
		Rect paddle;
		paddle.x = -PADDLE_X + i * PADDLE_X * 2.0f;
		paddle.y = client->visual_paddle_positions[i];
		paddle.w = PADDLE_WIDTH;
		paddle.h = PADDLE_HEIGHT;
		client_render_box(render_state, paddle, platform);
	}
	Rect ball;
	ball.x = client->visual_ball_position[0];
	ball.y = client->visual_ball_position[1];
	ball.w = BALL_WIDTH;
	ball.h = BALL_WIDTH;
	client_render_box(render_state, ball, platform);
}

void client_process_events(Client* client, Platform* platform)
{
	client->move_up = false;
	client->move_down = false;
	for(u32 i = 0; i < client->events_len; i++) {
		ClientEvent* event = &client->events[i];
		switch(event->type) {
			case CLIENT_EVENT_INPUT_MOVE_UP:
				client->move_up = true; 
				break;
			case CLIENT_EVENT_INPUT_MOVE_DOWN:
				client->move_down = true; 
				break;
			case CLIENT_EVENT_WORLD_UPDATE:
				client_handle_world_update(client, &event->world_update, platform); 
				break;
			case CLIENT_EVENT_CONNECTION_ACCEPTED:
				client_handle_accept_connection(client, event->assigned_id); 
				printf("connection accepted\n");
				break;
			case CLIENT_EVENT_START_GAME:
				client_handle_start_game(client, platform); 
				break;
			case CLIENT_EVENT_END_GAME:
				client_handle_end_game(client); 
				break;
			case CLIENT_EVENT_DISCONNECT:
				client_handle_disconnect(client); 
				break;
			case CLIENT_EVENT_SPEED_UP:
				client_handle_speed_up(client); 
				break;
			case CLIENT_EVENT_SLOW_DOWN:
				client_handle_slow_down(client); 
				break;
			default: break;
		}
	}
	client->events_len = 0;
}

// NOW: Get rid of platform and render_state here. Rendering should be done by
// the game layer. It will pull from either the client or world state.
void client_update(Client* client, Platform* platform, RenderState* render_state) 
{
	client_pull_messages(client);
	client_process_events(client, platform);

#if NETWORK_SIM_MODE
	// TODO: It is certainly wrong to use client->frame_length for this, and we
	// should certainly have a notion of delta time which is decoupled from the
	// network tick.
	platform_update_sim_mode(client->socket, client->frame_length);
#endif
	
	switch(client->connection_state) {
		case CLIENT_STATE_REQUESTING_CONNECTION:
			client_update_requesting_connection(client, platform, render_state);
			break;
		case CLIENT_STATE_WAITING_TO_START:
			client_update_waiting_to_start(client, platform, render_state);
			break;
		case CLIENT_STATE_ACTIVE:
			client_update_active(client, platform, render_state);
			break;
		default: break;
	}
}

