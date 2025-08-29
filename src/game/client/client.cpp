#define WORLD_STATE_BUFFER_LEN 1024

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

	client->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	client->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	client->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	return client;
}

void client_resolve_state_update(Client* client)
{
}

void client_process_packets(Client* client)
{
	// TODO - this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(client->socket, &packet_arena);
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
				client_resolve_state_update(client);
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

void client_update_connected(Client* client, Platform* platform, RenderState* render_state, float delta_time)
{
	//printf("Updating client(id-%u:frame-%u)\n", client->id, client->frame);

	World* world = &client->world_states[client->frame - client->oldest_stored_frame];
	world->player_inputs[client->id].move_up = platform_button_down(platform, client->button_move_up);
	world->player_inputs[client->id].move_down = platform_button_down(platform, client->button_move_down);

	//printf("down? %i\n", platform_button_down(platform, client->button_move_up));

	world_simulate(world, delta_time);

	// NOW - We need to do the following now:
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

	ClientInputPacket input_packet;
	input_packet.header.type = CLIENT_PACKET_INPUT;
	input_packet.header.client_id = client->id;
	input_packet.header.frame_number = client->frame;
	input_packet.input_up = world->player_inputs[client->id].move_up;
	input_packet.input_down = world->player_inputs[client->id].move_down;

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
void client_update(Client* client, Platform* platform, RenderState* render_state, float delta_time) 
{
	client_process_packets(client);
	
	switch(client->state) {
		case CLIENT_STATE_CONNECTING:
			client_update_connecting(client, platform, render_state);
			break;
		case CLIENT_STATE_CONNECTED:
			client_update_connected(client, platform, render_state, delta_time);
			client->frame++;
			// NOW - We are doing this so we are always dealing with frame 0 for now.
			// Time to actually deal with this frame buffer business, though.
			client->oldest_stored_frame = client->frame;
			break;
		default: break;
	}

	client->close_requested = platform_button_down(platform, client->button_quit);
}

bool client_close_requested(Client* client)
{
	return client->close_requested;
}
