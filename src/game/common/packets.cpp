// TODO: Compression! (bitpacking, delta compression, etc)
// This can be done one at a time, really. Just pack one and unpack it on the
// other end. Once we've done a few we can think about how to systemetize it
// further if that seems necessary.

// Client packets
enum PacketType {
	CLIENT_PACKET_REQUEST_CONNECTION,
	CLIENT_PACKET_READY_TO_START,
	CLIENT_PACKET_INPUT,
	SERVER_PACKET_ACCEPT_CONNECTION,
	SERVER_PACKET_START_GAME,
	SERVER_PACKET_END_GAME,
	SERVER_PACKET_DISCONNECT,
	SERVER_PACKET_WORLD_UPDATE,
	SERVER_PACKET_SPEED_UP,
	SERVER_PACKET_SLOW_DOWN
};

struct ClientRequestConnectionPacket {
	PacketType type;
};

struct ClientReadyToStartPacket {
	PacketType type;
};

struct ClientInputPacket {
	PacketType type;
	i32 latest_frame;
	i32 oldest_frame;
	bool input_moves_up[INPUT_WINDOW_FRAMES];
	bool input_moves_down[INPUT_WINDOW_FRAMES];
};

// Server packets
enum PacketType {
};

struct ServerAcceptConnectionPacket {
	PacketType type;
	u8 client_id;
};

struct ServerStartGamePacket {
	PacketType type;
};

struct ServerEndGamePacket {
	PacketType type;
};

struct ServerDisconnectPacket {
	PacketType type;
};

struct ServerWorldUpdatePacket {
	PacketType type;
	World world;
	i32 frame;
};

// TODO: Add frame to these so the client can tell whether they have already
// responded to a more recent speed update packet.
struct ServerSpeedUpPacket {
	PacketType type;
};

struct ServerSlowDownPacket {
	PacketType type;
};
