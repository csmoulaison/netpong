// TODO: Compression! (bitpacking, delta compression, etc)
// This can be done one at a time, really. Just pack one and unpack it on the
// other end. Once we've done a few we can think about how to systemetize it
// further if that seems necessary.

// Client messages
enum ClientMessageType {
	CLIENT_MESSAGE_REQUEST_CONNECTION,
	CLIENT_MESSAGE_READY_TO_START,
	CLIENT_MESSAGE_INPUT
};

struct ClientRequestConnectionMessage {
	ClientMessageType type;
};

struct ClientReadyToStartMessage {
	ClientMessageType type;
};

struct ClientInputMessage {
	ClientMessageType type;
	i32 latest_frame;
	i32 oldest_frame;
	bool input_moves_up[INPUT_WINDOW_FRAMES];
	bool input_moves_down[INPUT_WINDOW_FRAMES];
};

// Server messages
enum ServerMessageType {
	SERVER_MESSAGE_ACCEPT_CONNECTION,
	SERVER_MESSAGE_START_GAME,
	SERVER_MESSAGE_END_GAME,
	SERVER_MESSAGE_DISCONNECT,
	SERVER_MESSAGE_WORLD_UPDATE,
	SERVER_MESSAGE_SPEED_UP,
	SERVER_MESSAGE_SLOW_DOWN
};

struct ServerAcceptConnectionMessage {
	ServerMessageType type;
	u8 client_id;
};

struct ServerStartGameMessage {
	ServerMessageType type;
};

struct ServerEndGameMessage {
	ServerMessageType type;
};

struct ServerDisconnectMessage {
	ServerMessageType type;
};

struct ServerWorldUpdateMessage {
	ServerMessageType type;
	World world;
	i32 frame;
};

// TODO: Add frame to these so the client can tell whether they have already
// responded to a more recent speed update message.
struct ServerSpeedUpMessage {
	ServerMessageType type;
};

struct ServerSlowDownMessage {
	ServerMessageType type;
};
