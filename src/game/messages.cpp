struct SerializeResult {
	u64 size_bytes;
	void* data;
};

SerializeResult serialize_empty_typed_message(SerializeMode mode, void* type, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, type);
	serialize_u32(&stream, (u32*)type, arena);
	return (SerializeResult) { .size_bytes = stream.byte_offset + 1, .data = stream.data };
}
 
// Client messages
enum MessageType {
	CLIENT_MESSAGE_REQUEST_CONNECTION,
	CLIENT_MESSAGE_READY_TO_START,
	CLIENT_MESSAGE_INPUT,
	SERVER_MESSAGE_ACCEPT_CONNECTION,
	SERVER_MESSAGE_START_GAME,
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

SerializeResult serialize_client_input_message(SerializeMode mode, ClientInputMessage* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data);
	serialize_u32(&stream, &data->type, arena);
	serialize_i32(&stream, &data->latest_frame, arena);
	serialize_i32(&stream, &data->oldest_frame, arena);
	for(i32 i = 0; i < INPUT_WINDOW_FRAMES; i++) {
		serialize_bool(&stream, &data->input_moves_up[i], arena);
		serialize_bool(&stream, &data->input_moves_down[i], arena);
	}

	return (SerializeResult) { .size_bytes = stream.byte_offset + 1, .data = stream.data };
}


// ServerAcceptConnectionMessage 
struct ServerAcceptConnectionMessage {
	u32 type;
	u8 client_id;
};

SerializeResult serialize_server_accept_connection(SerializeMode mode, ServerAcceptConnectionMessage* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data);
	serialize_u32(&stream, &data->type, arena);
	serialize_u8(&stream, &data->client_id, arena);
	return (SerializeResult) { .size_bytes = stream.byte_offset + 1, .data = stream.data };
}


// ServerStartGameMessage 
struct ServerStartGameMessage {
	u32 type;
};


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
	World world;
	i32 frame;
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
SerializeResult serialize_server_world_update(SerializeMode mode, ServerWorldUpdateMessage* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data);
	serialize_u32(&stream, &data->type, arena);

	// Eventually, this becomes a serialize_world function, or something.
	serialize_f32(&stream, &data->world.countdown_to_start, arena);

	serialize_f32(&stream, &data->world.ball_position[0], arena);
	serialize_f32(&stream, &data->world.ball_position[1], arena);
	serialize_f32(&stream, &data->world.ball_velocity[0], arena);
	serialize_f32(&stream, &data->world.ball_velocity[1], arena);

	serialize_f32(&stream, &data->world.paddle_positions[0], arena);
	serialize_f32(&stream, &data->world.paddle_positions[1], arena);
	serialize_f32(&stream, &data->world.paddle_velocities[0], arena);
	serialize_f32(&stream, &data->world.paddle_velocities[1], arena);

	serialize_f32(&stream, &data->world.player_inputs[0].move_up, arena);
	serialize_f32(&stream, &data->world.player_inputs[0].move_down, arena);
	serialize_f32(&stream, &data->world.player_inputs[1].move_up, arena);
	serialize_f32(&stream, &data->world.player_inputs[1].move_down, arena);

	serialize_i32(&stream, &data->frame, arena);
	return (SerializeResult) { .size_bytes = stream.byte_offset + 1, .data = stream.data };
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
