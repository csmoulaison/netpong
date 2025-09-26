#define WORLD_STATE_BUFFER_SIZE 64

#define FRAME_LENGTH_MOD 0.02f

// NOW: < THIS: We are now doing for clients what we did for the server,
// implementing all the planned logic and hoping all goes well.
// 
// As of right now the packets all have at least stub handling functions setup.
// We need to make sure the implementation of those is good, and we haven't even
// touched any of the update loop stuff.
//
// Time for a fine toothed comb the same way we did the server stuff.

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
	u8 id;
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

Client* client_init(Platform* platform, Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));

	client->socket = platform_init_client_socket(arena);
	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
	client->id = 0;
	client->close_requested = false;

	client->frame = 0;
	client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);

	client->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	client->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	client->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	for(i32 i = 0; i < WORLD_STATE_BUFFER_SIZE; i++) {
		client->states[i].frame = -1;

		// TODO: Theoretically, we don't need to do this, eh? I just figured we might
		// as well and figure out optimizations later.
		client->states[i].world = {};
		world_init(&client->states[i].world);
	}

	return client;
}

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

	u8 other_id = 0;
	if(client->id == 0) {
		other_id = 1;
	}

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
	i32 prev_frame_index = (client->frame - 1) % WORLD_STATE_BUFFER_SIZE;
	i32 frame_index = client->frame % WORLD_STATE_BUFFER_SIZE;
	if(client->frame > 0) {
		memcpy(&client->states[frame_index].world, &client->states[prev_frame_index].world, sizeof(World));
	}

	client->states[frame_index].frame = client->frame;
	World* world = &client->states[frame_index].world;
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

	// TODO: Send sliding window of inputs so that the server can check for holes
	// in what it has received.
	ClientInputPacket input_packet = {};
	input_packet.header.type = CLIENT_PACKET_INPUT;
	input_packet.header.client_id = client->id;
	input_packet.frame = client->frame;
	input_packet.input_move_up = (world->player_inputs[client->id].move_up > 0.0f);
	input_packet.input_move_down = (world->player_inputs[client->id].move_down > 0.0f);
	platform_send_packet(client->socket, 0, &input_packet, sizeof(ClientInputPacket));

	client->frame++;
}

void client_handle_world_update(Client* client, ServerWorldUpdatePacket* server_update, Platform* platform)
{
	i32 update_frame = server_update->frame;
	i32 update_frame_index = update_frame % WORLD_STATE_BUFFER_SIZE;

	//assert(client->states[update_frame_index].frame == update_frame);
	if(client->states[update_frame_index].frame != update_frame) {
		printf("\033[31mClient fell behind server! client %u, server %u\033[0m\n", client->states[update_frame_index].frame, update_frame);
		client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);

		while(client->frame < update_frame + 1) {
			client_simulate_and_advance_frame(client, platform);
		}
	}

	World* client_state = &client->states[update_frame_index].world;
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
		i32 prev_frame_index = (i - 1) % WORLD_STATE_BUFFER_SIZE;
		i32 frame_index = i % WORLD_STATE_BUFFER_SIZE;

		PlayerInput cached_player_input = client->states[frame_index].world.player_inputs[client->id];
		memcpy(&client->states[frame_index].world, &client->states[prev_frame_index].world, sizeof(World));

		World* world = &client->states[frame_index].world;
		world->player_inputs[client->id] = cached_player_input;

		client_simulate_frame(world, client);
	}
}

void client_handle_accept_connection(Client* client, ServerAcceptConnectionPacket* accept_packet)
{
	if(client->connection_state == CLIENT_STATE_REQUESTING_CONNECTION) {
		client->id = accept_packet->client_id;
		client->connection_state = CLIENT_STATE_WAITING_TO_START;
		printf("Recieved connection acceptance from server.\n");
	}

	ClientJoinAcknowledgePacket acknowledge_packet;
	acknowledge_packet.header.type = CLIENT_PACKET_JOIN_ACKNOWLEDGE;
	acknowledge_packet.header.client_id = client->id;
	platform_send_packet(client->socket, 0, &acknowledge_packet, sizeof(acknowledge_packet));
}

void client_handle_start_game(Client* client, Platform* platform)
{
	client->connection_state = CLIENT_STATE_ACTIVE;

	// NOW: Set state to the initial state. We are no longer receiving this
	// authoritatively from the server.
	for(i8 i = 0; i < 6; i++) {
		client_simulate_and_advance_frame(client, platform);
	}
}

void client_handle_end_game(Client* client)
{
	client->connection_state = CLIENT_STATE_WAITING_TO_START;
}

void client_handle_disconnect(Client* client)
{
	client->connection_state = CLIENT_STATE_REQUESTING_CONNECTION;
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
	Arena packet_arena = arena_create(4096);
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
				client_handle_slow_down(client); break;
			case SERVER_PACKET_SLOW_DOWN:
				client_handle_slow_down(client); break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

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

	// TODO: Probably make a utility function.
	i32 frame_index = (client->frame - 1) % WORLD_STATE_BUFFER_SIZE;
	World* world = &client->states[frame_index].world;

	client_visual_lerp(&client->visual_ball_position[0], world->ball_position[0], client->frame_length * 4.0f);
	client_visual_lerp(&client->visual_ball_position[1], world->ball_position[1], client->frame_length * 4.0f);
	u8 other_id = 0;
	if(client->id == 0) {
		other_id = 1;
	}
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
			// TODO: This is just for the blinky thing. Dumb reason to have it here but
			// I like the blinky thing.
			client->frame++; 
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
