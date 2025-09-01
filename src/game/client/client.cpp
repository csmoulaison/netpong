#define WORLD_STATE_BUFFER_LEN 2048

#define FRAME_LENGTH_MOD 0.5f

enum ClientState {
	CLIENT_STATE_CONNECTING,
	CLIENT_STATE_CONNECTED
};

struct Client {
	// TODO - Probably abstract the platform socket away into a proxy socket for
	// local play.
	PlatformSocket* socket;

	u8 state;
	u8 id;
	bool close_requested;

	// The buffer holds present and past states of the game simulation.
	// The 0th element represents the oldest frame.
	World world_states[WORLD_STATE_BUFFER_LEN];
	u32 oldest_stored_frame;
	u32 frame;

	double frame_length;

	ButtonHandle button_move_up;
	ButtonHandle button_move_down;
	ButtonHandle button_quit;
};

Client* client_init(Platform* platform, Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));


	client->socket = platform_init_client_socket(arena);
	client->state = CLIENT_STATE_CONNECTING;
	client->id = 0;
	client->close_requested = false;

	client->oldest_stored_frame = 0;
	client->frame = 0;
	client->frame_length = BASE_FRAME_LENGTH;

	client->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	client->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	client->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	return client;
}

// NOW - Right now it looks like we are rolling back every update or something? It's inconsistent.
// Sometimes, we are just staying put, sometimes rolling back. I'm not sure we are ever jumping
// ahead. It's most obvious when the frame time is long and/or when the speed is high.
//
// When the debug display below is enabled, we see that the delta between the client present and
// server update is growing and shrinking, as we would expect with our scheme. Best guess is that
// this is correlated with our fluctuations in position, which means?
// 
// Best next step unless you can logic it out yourself is to write down the effects of the below
// procedure on pen and paper, verifying that the logic leads to the frames we expect being processed,
// then try to compare the actual world state values to what we would expect.
//
// If we are really in a pinch, we can also change the world model to make it easier to test this
// behavior. Think of ways of essentializing the problem we are trying to solve. Maybe just
// discretizing movement would do the trick.
void client_resolve_state_update(Client* client, ServerStateUpdatePacket* server_update)
{
	u32 DEBUG_oldest_stored = client->oldest_stored_frame;
	
	u32 server_frame = server_update->header.frame_number;

	i32 oldest_to_server_frame_delta = server_frame - client->oldest_stored_frame;
	client->oldest_stored_frame = server_frame;
	assert(oldest_to_server_frame_delta >= 0);

	for(u32 i = 0; i < oldest_to_server_frame_delta; i++) {
		client->world_states[i] = client->world_states[i + oldest_to_server_frame_delta];
	}

	memcpy(&client->world_states[0], &server_update->world_state, sizeof(World));
	i32 server_to_present_frame_delta = client->frame - server_frame;

	// NOW - This assertion failed at high speed. Think about why.
	assert(server_to_present_frame_delta >= 0);

	for(u32 i = 1; i < server_to_present_frame_delta; i++) {
		PlayerInput cached_player_input = client->world_states[i].player_inputs[client->id];

		memcpy(&client->world_states[i], &client->world_states[i - 1], sizeof(World));

		World* world = &client->world_states[i];
		world->player_inputs[client->id] = cached_player_input;
		world_simulate(world, client->frame_length);
	}

	if(false) {
		printf("\nRESOLVING STATE UPDATE\n======================\n");

		printf("client present         ", client->frame); 
		for(u32 i = 0; i < client->frame; i++) { printf("[ ]"); }; printf("[*]\n");

		printf("oldest to server       ", server_frame);
		for(u32 i = 0; i < DEBUG_oldest_stored; i++) { printf("[X]"); }; 
		for(u32 i = 0; i < oldest_to_server_frame_delta; i++) { printf("[>]"); }; 
		printf("\n");

		printf("server update          ", server_frame);
		for(u32 i = 0; i < server_frame; i++) { printf("[ ]"); }; printf("[*]\n");

		printf("server to present      ", server_frame);
		for(u32 i = 0; i < server_frame; i++) { printf("[ ]"); }; 
		for(u32 i = 0; i < server_to_present_frame_delta; i++) { printf("[>]"); }; 
		printf("\n");
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
				client->frame = acknowledge_packet->frame_number;
				client->state = CLIENT_STATE_CONNECTED;
				printf("Recieved join acknowledgment from server.\n");
				break;
			case SERVER_PACKET_STATE_UPDATE:
				update_packet = (ServerStateUpdatePacket*)packet->data;
				client_resolve_state_update(client, update_packet);
				break;
			case SERVER_PACKET_SPEED_UP:
				client->frame_length = BASE_FRAME_LENGTH - (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
				break;
			case SERVER_PACKET_SLOW_DOWN:
				client->frame_length = BASE_FRAME_LENGTH + (BASE_FRAME_LENGTH * FRAME_LENGTH_MOD);
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
	// TODO - Eventually, do a ring buffer. Right now, if no reset is performed, we
	// segfault after max buffered frames is reached. If not a ring buffer, I suppose
	// we could do a dynamic array, but that's really kind of ridiculous. Honestly,
	// maybe it's even better to just pause and wait for server response if we are
	// at the max?
	u32 frame_index = client->frame - client->oldest_stored_frame;
	assert(frame_index < WORLD_STATE_BUFFER_LEN);
	if(frame_index > 0) {
		memcpy(&client->world_states[frame_index], &client->world_states[frame_index - 1], sizeof(World));
	}

	World* world = &client->world_states[frame_index];
	world->player_inputs[client->id].move_up = platform_button_down(platform, client->button_move_up);
	world->player_inputs[client->id].move_down = platform_button_down(platform, client->button_move_down);
	world_simulate(world, client->frame_length);

	// We need to do the following now:
	//   1. Store the newly simulated frame in the world state buffer.
	//   2. Send a packet of this frame's input to the server.
	//   3. Buffer client inputs on the server side, pulling from them every server
	//      update. Once we do this we will want to see about properly syncing the
	//      client and server times and what might be complex about that.
	//      - This represents the only unknown at the moment, this problem of making
	//        sure the client and server frame counters are synced up. Perhaps this
	//        just isn't really a problem and computers are very time accurate.
	//   4. Every server update, after pulling inputs from the client side, send the
	//      authoritative state for that simulated frame back to the client.
	//   5. On the client side, respond to these server update packets by doing the
	//      following:
	//      - Clear the buffer up to this most recent server frame, shifting that
	//        one over to [0].
	//      - Starting from this server frame, resimulate until you reach the
	//        newest client frame, using the stored inputs as, well, input.
	//      - Rinse and repeat.
	// All this has been done, but with some incorrect stuff, as the notes explain.

	// NOW - we also aren't sending a sliding window of inputs. Rewatch overwatch
	// and then rocket league netcode part of talks.
	ClientInputPacket input_packet = {};
	input_packet.header.type = CLIENT_PACKET_INPUT;
	input_packet.header.client_id = client->id;
	input_packet.header.frame_number = client->frame;
	input_packet.input_move_up = world->player_inputs[client->id].move_up;
	input_packet.input_move_down = world->player_inputs[client->id].move_down;
	// NOW - down input is making it to this packet.
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
	
	switch(client->state) {
		case CLIENT_STATE_CONNECTING:
			client_update_connecting(client, platform, render_state);
			// TODO - This is just for the blinky thing. Dumb reason to have it here but
			// I like the blinky thing.
			client->frame++; 
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
