// TODO: Compression! (bitpacking, delta compression, etc)
// This can be done one at a time, really. Just pack one and unpack it on the
// other end. Once we've done a few we can think about how to systemetize it
// further if that seems necessary.

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

struct ClientRequestConnectionMessage {
	MessageType type;
};

struct ClientReadyToStartMessage {
	MessageType type;
};

struct ClientInputMessage {
	MessageType type;
	i32 latest_frame;
	i32 oldest_frame;
	bool input_moves_up[INPUT_WINDOW_FRAMES];
	bool input_moves_down[INPUT_WINDOW_FRAMES];
};

struct ServerAcceptConnectionMessage {
	MessageType type;
	u8 client_id;
};

struct ServerStartGameMessage {
	MessageType type;
};

struct ServerEndGameMessage {
	MessageType type;
};

struct ServerDisconnectMessage {
	MessageType type;
};

struct ServerWorldUpdateMessage {
	MessageType type;
	World world;
	i32 frame;
};

// TODO: Add frame to these so the client can tell whether they have already
// responded to a more recent speed update message.
struct ServerSpeedUpMessage {
	MessageType type;
};

struct ServerSlowDownMessage {
	MessageType type;
};
