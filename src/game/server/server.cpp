#define INPUT_BUFFER_SIZE 64
#define INPUT_SLOWDOWN_THRESHOLD 3

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
	// So that we can tell if a certain frame has been received yet.
	i32 frame;
	PlayerInput input;
};

struct ConnectedClient {
	// NOW: Keep ID here, which will require decoupling two things that are
	// currently coupled with this ID.
	// 1. The list of connected clients on the platform side.
	// 2. The index into the connected_clients list.
	// Right now, this id will be used to allow pending_clients to be moved to
	// the right place on connected_clients.
	i32 id;
	ClientInput inputs[INPUT_BUFFER_SIZE];
};

struct Server {
	PlatformSocket* socket;

	World world;

	// Represents the current simulation frame, the number of frames elapsed since
	// the server session was initialized.
	//
	// INPUT_BUFFER_SIZE % Server.frame = The client input associated with the
	// current simulation frame.
	i32 frame;
	i32 latest_frame_received;

	ConnectedClient connected_clients[MAX_CLIENTS];
	u32 connected_clients_len;

	// NOW: << THIS: Currently, our goal is to split connected_clients into those
	// which have acknowledged the server join and those who haven't.
	// Why? In order to allow the client to get some frames simulated before it
	// receives any server update packets. The server will only send update packets
	// to those who have joined.
	// So: we want the flow to look like this from the server perspective:
	// 1. Receive join request, send back acknowledgement and add ID to pending_clients.
	// 2. While waiting for client acknowledgement, only send updates to those on
	//    the connected_clients list. Not sure if we care about buffering inputs from
	//    pending clients.
	// 3. Receive client join acknowledgement. Move that client to the
	//    connected_clients list, mem copying data if necessary.
	//
	// We just tried putting a bool on the connected_clients list in order to
	// differentiate these two, but I don't want to have to check that bool everywhere
	// connected_clients is used, hence the two lists. However, keep in mind that
	// all the logic you'll find concerning pending clients is still referencing
	// this old bool scheme, so we are in the process of transitioning that over.
	//
	// The next state for the codebase will be the pending_clients list holding
	// clients, and then being copied over to the connected_clients list on
	// acknowledge, using the stored ConnectedClient.id in order to know where to
	// put it.
	//
	// What comes after that will depend on what data we actually need for both
	// of these lists, whether they are equivalent. If they both need the same data
	// (i.e. ClientInput[] buffer), the natural thing to do would be to have an
	// array of ConnectedClients, with two separate lookup tables holding indices
	// into that ConnectedClients list. This is most likely what ends up happening.

	// NOW: Right now, this last has to be checked in order to give the player
	// a client id. We are clumsily handling this as this only actually matters in
	// the case of players disconnecting or more than two players, which we aren't
	// concerned with right now. Storing the client ID seperately in
	// connected_clients will go along with cleaning this up.
	// 
	// List of clients which haven't acknowledged the server join yet.
	ConnectedClient pending_clients[MAX_CLIENTS];
	u32 pending_clients_len;
};

Server* server_init(Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->socket = platform_init_server_socket(arena);
	server->frame = 0;
	server->latest_frame_received = 0;

	server->connected_clients_len = 0;
	for(u8 i = 0; i < MAX_CLIENTS; i++) {
		ConnectedClient* client;
		for(u32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			client->inputs[j].frame = -1;
			client->inputs[j].input = {};
		}
	}

	world_init(&server->world);

	return server;
}

void server_process_packets(Server* server)
{
	// TODO: this is dirty shit dude. arena_create should be able to allocate from
	// an existing arena
	Arena packet_arena = arena_create(4096);
	PlatformPayload payload = platform_receive_packets(server->socket, &packet_arena);
	PlatformPacket* packet = payload.head;

	// TODO: Make lookup table from connection_id to connected_client index, or
	// come up with some other solution for clients being disconnected.
	
	while(packet != nullptr) {
		i8 connection_id = packet->connection_id;
		ClientPacketHeader* header = (ClientPacketHeader*)packet->data;

		ServerJoinAcknowledgePacket server_acknowledge_packet;
		ClientInputPacket* input_packet;
		ConnectedClient* client;
		ClientInput* buffer_input;

		switch(header->type) {
			case CLIENT_PACKET_JOIN:
				assert(connection_id <= server->connected_clients_len);
				printf("Acknowledging client join.\n");

				server_acknowledge_packet.header.type = SERVER_PACKET_JOIN_ACKNOWLEDGE;
				server_acknowledge_packet.client_id = connection_id;
				server_acknowledge_packet.frame = server->frame;
				server_acknowledge_packet.world_state = server->world;
				platform_send_packet(server->socket, connection_id, (void*)&server_acknowledge_packet, sizeof(ServerJoinAcknowledgePacket));

				if(connection_id == server->connected_clients_len) {
					server->connected_clients[connection_id].client_acknowledged = false;
					server->connected_clients_len++;
				}
				break;
			case CLIENT_PACKET_JOIN_ACKNOWLEDGE:
				server->connected_clients[connection_id].client_acknowledged = true;
				break;
			case CLIENT_PACKET_INPUT:
				input_packet = (ClientInputPacket*)packet->data;
				client = &server->connected_clients[header->client_id];

				if(server->latest_frame_received < input_packet->header.frame) {
					server->latest_frame_received = input_packet->header.frame;
				}

				buffer_input = &client->inputs[input_packet->header.frame % INPUT_BUFFER_SIZE];
				buffer_input->frame = input_packet->header.frame;
				buffer_input->input.move_up = input_packet->input_move_up;
				buffer_input->input.move_down = input_packet->input_move_down;
				break;
			default: break;
		}
		packet = packet->next;
	}
	arena_destroy(&packet_arena);

}

void server_update(Server* server, float delta_time)
{
	server_process_packets(server);

#if NETWORK_SIM_MODE
	platform_update_sim_mode(server->socket, delta_time);
#endif

	for(i8 i = 0; i < server->connected_clients_len; i++) {
		ConnectedClient* client = &server->connected_clients[i];
		i32 latest_frame = 0;

		for(i32 j = 0; j < INPUT_BUFFER_SIZE; j++) {
			if(latest_frame < client->inputs[j].frame) {
				latest_frame = client->inputs[j].frame;
			}
		}

		// TODO: We want these thresholds to be set by measuring the ping for each
		// individual client.
		if(latest_frame - server->frame < 3) {
			ServerSpeedUpPacket speed_packet;
			speed_packet.header.type = SERVER_PACKET_SPEED_UP;
			speed_packet.header.frame = server->frame;
			platform_send_packet(server->socket, i, &speed_packet, sizeof(ServerSpeedUpPacket));
		} else if(latest_frame - server->frame > 5) {
			ServerSlowDownPacket slow_packet;
			slow_packet.header.type = SERVER_PACKET_SLOW_DOWN;
			slow_packet.header.frame = server->frame;
			platform_send_packet(server->socket, i, &slow_packet, sizeof(ServerSlowDownPacket));
		}
	}

	if(server->connected_clients_len > 0) {
		for(u8 i = 0; i < server->connected_clients_len; i++) {
			ConnectedClient* client = &server->connected_clients[i];

			i32 last_input_frame = server->frame;
			while(client->inputs[last_input_frame % INPUT_BUFFER_SIZE].frame != last_input_frame) {
				last_input_frame--;
				if(last_input_frame <= 0) {
					printf("Server: No inputs yet from client %u.\n", i);
					last_input_frame = 0;
					break;
				}

				// TODO: This is a client timeout. We should just disconnect the client here.
				if(server->frame - last_input_frame > INPUT_BUFFER_SIZE) {
					panic();
				}
			}
			server->world.player_inputs[i] = client->inputs[last_input_frame % INPUT_BUFFER_SIZE].input;

			if(last_input_frame != server->frame) {
				printf("Server: Missed a packet from client %u.\n", i);
			}
		}

		world_simulate(&server->world, delta_time);

		ServerStateUpdatePacket update_packet = {};
		update_packet.header.type = SERVER_PACKET_STATE_UPDATE;
		update_packet.header.frame = server->frame;
		update_packet.world_state = server->world;

		for(u8 i = 0; i < server->connected_clients_len; i++) {
			platform_send_packet(server->socket, i, &update_packet, sizeof(ServerStateUpdatePacket));
		}

		server->frame++;
	} else {
		server->frame = 0;
	}
}


bool server_close_requested(Server* server)
{
	return false;
}
