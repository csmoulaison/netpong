#define WORLD_STATE_BUFFER_SIZE 64

#define FRAME_LENGTH_MOD 0.025f

enum ClientConnectionState {
	CLIENT_STATE_CONNECTING,
	CLIENT_STATE_CONNECTED
};

struct ClientWorldState {
	i32 frame;
	World world;
};

struct Client {
	// TODO - Probably abstract the platform socket away into a proxy socket for
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
};

Client* client_init(Platform* platform, Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));

	client->socket = platform_init_client_socket(arena);
	client->connection_state = CLIENT_STATE_CONNECTING;
	client->id = 0;
	client->close_requested = false;

	client->frame = 0;
	client->frame_length = BASE_FRAME_LENGTH;

	client->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	client->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	client->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	for(i32 i = 0; i < WORLD_STATE_BUFFER_SIZE; i++) {
		client->states[i].frame = -1;

		// TODO - Theoretically, we don't need to do this, eh? I just figured we might
		// as well and figure out optimizations later.
		client->states[i].world = {};
		world_init(&client->states[i].world);
	}

	return client;
}

void client_resolve_state_update(Client* client, ServerStateUpdatePacket* server_update)
{
	i32 update_frame = server_update->header.frame;
	i32 update_frame_index = update_frame % WORLD_STATE_BUFFER_SIZE;
	assert(client->states[update_frame_index].frame == update_frame);

	World* client_state = &client->states[update_frame_index].world;
	World* server_state = &server_update->world_state;

	// If the states are equal, client side prediction was successful and we do not
	// need to resimulate.
	if(server_state->paddle_positions[0] == client_state->paddle_positions[0]
	&& server_state->paddle_positions[1] == client_state->paddle_positions[1]) {
		printf("Client: Matched frame %u, aborting simulation.\n", update_frame);
		return;
	}

	printf("Client: Unmatched frame %u, initiating simulation.\n", update_frame);
	assert(client->frame - update_frame >= 0);
	memcpy(client_state, server_state, sizeof(World));

	for(i32 i = update_frame + 1; i <= client->frame; i++) {
		i32 prev_frame_index = (i - 1) % WORLD_STATE_BUFFER_SIZE;
		i32 frame_index = i % WORLD_STATE_BUFFER_SIZE;

		PlayerInput cached_player_input = client->states[frame_index].world.player_inputs[client->id];
		memcpy(&client->states[frame_index].world, &client->states[prev_frame_index].world, sizeof(World));

		World* world = &client->states[frame_index].world;
		world->player_inputs[client->id] = cached_player_input;
		// NOW - This is kind of a big huge thing I missed. The frame_length is being
		// used here, and it's now often going to be different than in the case of the
		// original simulation. For now, I disabled the frame slowing/speeding.
		//
		// If we re-enable it, I suspect the thing to do will be to store the frame
		// length of the original client-side predicted simulation at the time of
		// simulation, and reuse it here the same way we did with cached player input.
		world_simulate(world, client->frame_length); 
	}
}

void client_process_packets(Client* client)
{
	// TODO - this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(client->socket,&packet_arena);
	PlatformPacket* packet = payload.head;

	while(packet != nullptr) {
		ServerPacketHeader* header = (ServerPacketHeader*)packet->data;
		ServerJoinAcknowledgePacket* acknowledge_packet;
		ServerStateUpdatePacket* update_packet;

		switch(header->type) {
			case SERVER_PACKET_JOIN_ACKNOWLEDGE:
				acknowledge_packet = (ServerJoinAcknowledgePacket*)packet->data;
				client->id = acknowledge_packet->client_id;
				client->frame = acknowledge_packet->frame;
				client->connection_state = CLIENT_STATE_CONNECTED;
				printf("Recieved join acknowledgment from server.\n");
				break;
			case SERVER_PACKET_STATE_UPDATE:
				update_packet = (ServerStateUpdatePacket*)packet->data;
				client_resolve_state_update(client, update_packet);
				break;
			case SERVER_PACKET_SPEED_UP:
				//client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
				break;
			case SERVER_PACKET_SLOW_DOWN:
				//client->frame_length = BASE_FRAME_LENGTH + (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);
}

void client_update_connecting(Client* client, Platform* platform, RenderState* render_state)
{
	// Keep sending join packet until we get an acknowledgement.
	ClientJoinPacket join_packet;
	join_packet.header.type = CLIENT_PACKET_JOIN;
	platform_send_packet(client->socket, 0, (void*)&join_packet, sizeof(ClientJoinPacket));

	// Render "connecting" indicator.
	render_state->boxes_len = 2;
	for(u8 i = 0; i < 2; i++) {
		float xoff = -1000.0f;
		if((client->frame / 60) % 3 == 0) {
			xoff = 0.0f;
		}
		
		Rect* box = &render_state->boxes[i];
		box->x = -0.75f + xoff;
		box->y = 0.75f;
		box->w = 0.025f;
		box->h = 0.025f;
	}
}

void client_update_connected(Client* client, Platform* platform, RenderState* render_state)
{
	i32 prev_frame_index = (client->frame - 1) % WORLD_STATE_BUFFER_SIZE;
	i32 frame_index = client->frame % WORLD_STATE_BUFFER_SIZE;
	if(client->frame > 0) {
		memcpy(&client->states[frame_index].world, &client->states[prev_frame_index].world, sizeof(World));
	}

	client->states[frame_index].frame = client->frame;
	World* world = &client->states[frame_index].world;
	world->player_inputs[client->id].move_up = platform_button_down(platform, client->button_move_up);
	world->player_inputs[client->id].move_down = platform_button_down(platform, client->button_move_down);
	world_simulate(world, client->frame_length);


	// NOW - we also aren't sending a sliding window of inputs. Rewatch overwatch
	// and then rocket league netcode part of talks.
	ClientInputPacket input_packet = {};
	input_packet.header.type = CLIENT_PACKET_INPUT;
	input_packet.header.client_id = client->id;
	input_packet.header.frame = client->frame;
	input_packet.input_move_up = world->player_inputs[client->id].move_up;
	input_packet.input_move_down = world->player_inputs[client->id].move_down;
	platform_send_packet(client->socket, 0, &input_packet, sizeof(ClientInputPacket));

	// Render
	render_state->boxes_len = 2;
	for(u8 i = 0; i < 2; i++) {
		Rect* box = &render_state->boxes[i];
		box->x = -0.75f + i * 1.5f;
		box->y = world->paddle_positions[i];
		box->w = 0.025f;
		box->h = 0.1f;
	}
}

// We will probably want to package input from platform and input as needed here
// separately, but that's feeling a little icky.
void client_update(Client* client, Platform* platform, RenderState* render_state) 
{
	client_process_packets(client);
	
	switch(client->connection_state) {
		case CLIENT_STATE_CONNECTING:
			// TODO - This is just for the blinky thing. Dumb reason to have it here but
			// I like the blinky thing.
			client->frame++; 
			client_update_connecting(client, platform, render_state);
			break;
		case CLIENT_STATE_CONNECTED:
			client_update_connected(client, platform, render_state);
			client->frame++;
			break;
		default: break;
	}

	client->close_requested = platform_button_down(platform, client->button_quit);
}

bool client_close_requested(Client* client)
{
	return client->close_requested;
}
