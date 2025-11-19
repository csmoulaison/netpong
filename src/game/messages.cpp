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
		serialize_bool(&stream, &message->input_moves_up[i]);
		serialize_bool(&stream, &message->input_moves_down[i]);
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
