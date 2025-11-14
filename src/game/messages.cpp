// NOW: Write a detailed description of the inner machinations of this file.
// It's going to be one we forget. 

// Serialization functions, by convention:
// - ~~Take either an arena for write mode, or a data pointer for read mode. The
// one not used by the given mode should be left null.~~
// - Return a SerializeResult containing the size in bytes and the packed data.
 
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


// ServerAcceptConnectionMessage 
struct ServerAcceptConnectionMessage {
	u32 type;
	u8 client_id;
};

// NOW: The arena is ONLY used to allocate more space for the void pointer IN
// THE CASE OF WRITING, and shall go unused in the case of reading. The
// serialize_x functions take the arena in order to perform this function. This
// means in the case of reading, the arena can be passed as a nullptr, as it
// won't be referenced at all if we are in reading mode.
u64 serialize_server_accept_connection(SerializeMode mode, ServerAcceptConnectionMessage* data, Arena* arena)
{
	Bitstream stream = bitstream_init(mode, data);
	serialize_u32(&stream, &data->type, arena);
	serialize_u8(&stream, &data->client_id, arena);
	return stream.byte_offset + 1;
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
