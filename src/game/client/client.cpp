#define WORLD_STATE_BUFFER_SIZE 64

#define FRAME_LENGTH_MOD 0.02f

enum ClientConnectionState {
	CLIENT_STATE_REQUESTING_CONNECTION,
	CLIENT_STATE_WAITING_TO_START,
	CLIENT_STATE_ACTIVE
};

struct ClientWorldState {
	i32 frame;
	World world;
};

struct Client {
	// TODO: Probably abstract the platform socket away into a proxy socket for
	// local play.
	PlatformSocket* socket;

	ClientConnectionState connection_state;
	i8 id;
	bool close_requested;

	// The buffer holds present and past states of the game simulation.
	ClientWorldState states[WORLD_STATE_BUFFER_SIZE];
	i32 frame;

	double frame_length;

	ButtonHandle button_move_up;
	ButtonHandle button_move_down;
	ButtonHandle button_quit;

	float visual_ball_position[2];
	float visual_paddle_positions[2];
};

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

		// TODO: Theoretically, we don't need to do this, eh? I figure we might as
		// well and figure out if we need to later.
		client->states[i].world = {};
		world_init(&client->states[i].world);
	}
}

Client* client_init(Platform* platform, Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));

	client->socket = platform_init_client_socket(arena);
	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
	client->id = -1;
	client->close_requested = false;

	client->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	client->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	client->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

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

	if(platform_button_down(platform, client->button_move_up)) {
		world->player_inputs[client->id].move_up = 1.0f;
	} else {
		world->player_inputs[client->id].move_up = 0.0f;
	}
	if(platform_button_down(platform, client->button_move_down)) {
		world->player_inputs[client->id].move_down = 1.0f;
	} else {
		world->player_inputs[client->id].move_down = 0.0f;
	}
	client_simulate_frame(world, client);

	ClientInputPacket input_packet = {};
	input_packet.header.type = CLIENT_PACKET_INPUT;
	input_packet.header.client_id = client->id;
	input_packet.latest_frame = client->frame;

	input_packet.oldest_frame = client->frame - INPUT_WINDOW_FRAMES + 1;
	if(input_packet.oldest_frame < 0) { 
		input_packet.oldest_frame = 0;
	}

	i32 frame_delta = input_packet.latest_frame - input_packet.oldest_frame;
	for(i32 i = 0; i <= frame_delta; i++) {
		i32 input_frame = client->frame - frame_delta + i;
		if(input_frame == client->frame) {
			printf("SENDING CURRENT CLIENT FRAME (%u)\n", client->frame);
		}
		
		World* input_world = &client_state_from_frame(client, input_frame)->world;
		assert(client_state_from_frame(client, input_frame)->frame == input_frame);
		
		input_packet.input_moves_up[i] = (input_world->player_inputs[client->id].move_up > 0.0f);
		input_packet.input_moves_down[i] = (input_world->player_inputs[client->id].move_down > 0.0f);
	}

	platform_send_packet(client->socket, 0, &input_packet, sizeof(ClientInputPacket));

	client->frame++;
}

// Packet handling functions
void client_handle_world_update(Client* client, ServerWorldUpdatePacket* server_update, Platform* platform)
{
	i32 update_frame = server_update->frame;
	ClientWorldState* update_state = client_state_from_frame(client, update_frame);

	if(update_state->frame != update_frame) {
		client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
		while(client->frame < update_frame + 1) {
			client_simulate_and_advance_frame(client, platform);
		}

		printf("\033[31mClient fell behind server! client %u, server %u\033[0m\n", update_state->frame, update_frame);
	}

	World* client_state = &update_state->world;
	World* server_state = &server_update->world;

	// If the states are equal, client side prediction was successful and we do not
	// need to resimulate.
	if(server_state->paddle_positions[0] == client_state->paddle_positions[0]
	&& server_state->paddle_positions[1] == client_state->paddle_positions[1]
	&& server_state->paddle_velocities[0] == client_state->paddle_velocities[0]
	&& server_state->paddle_velocities[1] == client_state->paddle_velocities[1]) {
		return;
	}

	assert(client->frame - update_frame >= 0);
	memcpy(client_state, server_state, sizeof(World));

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
void client_handle_accept_connection(Client* client, ServerAcceptConnectionPacket* accept_packet)
{
	if(client->connection_state == CLIENT_STATE_REQUESTING_CONNECTION) {
		client->id = accept_packet->client_id;
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

void client_process_packets(Client* client, Platform* platform)
{
	// TODO: this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(16000);
	PlatformPayload payload = platform_receive_packets(client->socket,&packet_arena);
	PlatformPacket* packet = payload.head;

	while(packet != nullptr) {
		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;
		switch(header->type) {
			case SERVER_PACKET_WORLD_UPDATE:
				client_handle_world_update(client, (ServerWorldUpdatePacket*)packet->data, platform); break;
			case SERVER_PACKET_ACCEPT_CONNECTION:
				client_handle_accept_connection(client, (ServerAcceptConnectionPacket*)packet->data); break;
			case SERVER_PACKET_START_GAME:
				client_handle_start_game(client, platform); break;
			case SERVER_PACKET_END_GAME:
				client_handle_end_game(client); break;
			case SERVER_PACKET_DISCONNECT:
				client_handle_disconnect(client); break;
			case SERVER_PACKET_SPEED_UP:
				client_handle_speed_up(client); break;
			case SERVER_PACKET_SLOW_DOWN:
				client_handle_slow_down(client); break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

// Client update functions
void client_render_box(RenderState* render_state, Rect box, Platform* platform)
{
	float x_scale = (float)platform->window_height / platform->window_width;

	Rect* rect = &render_state->boxes[render_state->boxes_len];
	render_state->boxes_len += 1;

	rect->x = box.x * x_scale;
	rect->y = box.y;
	rect->w = box.w;
	rect->h = box.h;
}

void client_update_requesting_connection(Client* client, Platform* platform, RenderState* render_state)
{
	ClientRequestConnectionPacket request_packet;
	request_packet.header.type = CLIENT_PACKET_REQUEST_CONNECTION;
	platform_send_packet(client->socket, 0, (void*)&request_packet, sizeof(request_packet));

	// Render "connecting" indicator.
	for(u8 i = 0; i < 3; i++) {
		float xoff = -1000.0f;
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
	ClientReadyToStartPacket ready_packet;
	ready_packet.header.type = CLIENT_PACKET_READY_TO_START;
	ready_packet.header.client_id = client->id;
	platform_send_packet(client->socket, 0, &ready_packet, sizeof(ready_packet));

	// Render "connecting" indicator.
	for(u8 i = 0; i < 3; i++) {
		float xoff = 0.0f;
		xoff += ((client->frame / 30) % 3) * 0.025;

		Rect box;
		box.x = -0.75f + xoff;
		box.y = 0.75f;
		box.w = 0.025f;
		box.h = 0.025f;
		client_render_box(render_state, box, platform);
	}
}

void client_visual_lerp(float* visual, float real, float dt)
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

// We will probably want to package input from platform and input as needed here
// separately, but that's feeling a little icky.
void client_update(Client* client, Platform* platform, RenderState* render_state) 
{
	client_process_packets(client, platform);

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

	client->close_requested = platform_button_down(platform, client->button_quit);
}

bool client_close_requested(Client* client)
{
	return client->close_requested;
}
