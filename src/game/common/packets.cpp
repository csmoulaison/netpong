// TODO: Compression! (bitpacking, delta compression, etc)
// This can be done one at a time, really. Just pack one and unpack it on the
// other end. Once we've done a few we can think about how to systemetize it
// further if that seems necessary.

// Client packets
enum ClientPacketType {
	CLIENT_PACKET_REQUEST_CONNECTION,
	CLIENT_PACKET_READY_TO_START,
	CLIENT_PACKET_INPUT
};

struct ClientPacketHeader {
	ClientPacketType type;
};

struct ClientRequestConnectionPacket {
	ClientPacketHeader header;
};

struct ClientReadyToStartPacket {
	ClientPacketHeader header;
};

struct ClientInputPacket {
	ClientPacketHeader header;
	i32 latest_frame;
	i32 oldest_frame;
	bool input_moves_up[INPUT_WINDOW_FRAMES];
	bool input_moves_down[INPUT_WINDOW_FRAMES];
};

// Server packets
enum ServerPacketType {
	SERVER_PACKET_ACCEPT_CONNECTION,
	SERVER_PACKET_START_GAME,
	SERVER_PACKET_END_GAME,
	SERVER_PACKET_DISCONNECT,
	SERVER_PACKET_WORLD_UPDATE,
	SERVER_PACKET_SPEED_UP,
	SERVER_PACKET_SLOW_DOWN
};

struct ServerPacketHeader {
	ServerPacketType type;
};

struct ServerAcceptConnectionPacket {
	ServerPacketHeader header;
	u8 client_id;
};

struct ServerStartGamePacket {
	ServerPacketHeader header;
};

struct ServerEndGamePacket {
	ServerPacketHeader header;
};

struct ServerDisconnectPacket {
	ServerPacketHeader header;
};

struct ServerWorldUpdatePacket {
	ServerPacketHeader header;
	World world;
	i32 frame;
};

// TODO: Add frame to these so the client can tell whether they have already
// responded to a more recent speed update packet.
struct ServerSpeedUpPacket {
	ServerPacketHeader header;
};

struct ServerSlowDownPacket {
	ServerPacketHeader header;
};
