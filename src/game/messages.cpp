SerializeResult serialize_empty_typed_message(SerializeMode mode, void* type, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, (char*)type, arena);
	serialize_u32(&stream, (u32*)type);
	return serialize_result(&stream);
}
 
// Client messages
enum MessageType {
	CLIENT_MESSAGE_REQUEST_CONNECTION,
	CLIENT_MESSAGE_READY_TO_START,
	CLIENT_MESSAGE_INPUT,
	SERVER_MESSAGE_ACCEPT_CONNECTION,
	SERVER_MESSAGE_END_GAME,
	SERVER_MESSAGE_DISCONNECT,
	SERVER_MESSAGE_WORLD_UPDATE,
	SERVER_MESSAGE_SPEED_UP,
	SERVER_MESSAGE_SLOW_DOWN
};


// ClientRequestConnectionMessage 
struct ClientRequestConnectionMessage {
	u32 type;
};


// ClientReadyToStartMessage 
struct ClientReadyToStartMessage {
	u32 type;
};


// ClientInputMessage 
struct ClientInputMessage {
	u32 type;
	i32 latest_frame;
	i32 oldest_frame;
	bool input_moves_up[INPUT_WINDOW_FRAMES];
	bool input_moves_down[INPUT_WINDOW_FRAMES];
};

SerializeResult serialize_client_input(SerializeMode mode, ClientInputMessage* message, char* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data, arena);
	serialize_u32(&stream, &message->type);

	serialize_i32(&stream, &message->latest_frame);
	serialize_i32(&stream, &message->oldest_frame);
	for(i32 i = 0; i < INPUT_WINDOW_FRAMES; i++) {
		// NOW: Make this serialize a single bit. Not working at the moment.
		serialize_i32(&stream, (i32*)&message->input_moves_up[i]);
		serialize_i32(&stream, (i32*)&message->input_moves_down[i]);
	}
	return serialize_result(&stream);
}


// ServerAcceptConnectionMessage 
struct ServerAcceptConnectionMessage {
	u32 type;
	u8 client_id;
};

SerializeResult serialize_server_accept_connection(SerializeMode mode, ServerAcceptConnectionMessage* message, char* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data, arena);
	//serialize_bits(&stream, (char*)message, sizeof(ServerWorldUpdateMessage) * 8);

	serialize_u32(&stream, &message->type);
	serialize_u8(&stream, &message->client_id);
	return serialize_result(&stream);
}


// ServerEndGameMessage 
struct ServerEndGameMessage {
	u32 type;
};


// ServerDisconnectMessage 
struct ServerDisconnectMessage {
	u32 type;
};


// ServerWorldUpdateMessage 
struct ServerWorldUpdateMessage {
	u32 type;
	i32 frame;
	World world;
};

// NOW: This isn't working at the moment. For some reason, implementing it led
// to the joining client stuck on "Waiting to start..." (no start game message?)
// 
// We want to get this to work, and then finish implementing both receive and
// send message packing.
// 
// After, we want to change all these references to the arenas to be clearly
// referencing a sub-arena which is only for packet sending. We need to finally
// get sub-allocation set up for arenas so we can have some static memory we
// clear after every frame. Really, just a transient sub-arena would work just
// fine and allow us to use it for all sort of other stuff too at the same time
// without being in a situation where we have a billion arenas being passed
// around.
SerializeResult serialize_server_world_update(SerializeMode mode, ServerWorldUpdateMessage* message, char* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data, arena);

	serialize_u32(&stream, &message->type);
	serialize_i32(&stream, &message->frame);

	// Eventually, the following becomes a serialize_world function, or something.
	serialize_f32(&stream, &message->world.countdown_to_start);

	serialize_f32(&stream, &message->world.ball_position[0]);
	serialize_f32(&stream, &message->world.ball_position[1]);
	serialize_f32(&stream, &message->world.ball_velocity[0]);
	serialize_f32(&stream, &message->world.ball_velocity[1]);

	serialize_f32(&stream, &message->world.paddle_positions[0]);
	serialize_f32(&stream, &message->world.paddle_positions[1]);
	serialize_f32(&stream, &message->world.paddle_velocities[0]);
	serialize_f32(&stream, &message->world.paddle_velocities[1]);

	serialize_f32(&stream, &message->world.player_inputs[0].move_up);
	serialize_f32(&stream, &message->world.player_inputs[0].move_down);
	serialize_f32(&stream, &message->world.player_inputs[1].move_up);
	serialize_f32(&stream, &message->world.player_inputs[1].move_down);

	return serialize_result(&stream);
}


// TODO: Add frame to these so the client can tell whether they have already
// responded to a more recent speed update message.
// ServerSpeedUpMessage 
struct ServerSpeedUpMessage {
	u32 type;
};


// ServerSlowDownMessage 
struct ServerSlowDownMessage {
	u32 type;
};
