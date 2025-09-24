// TODO: Eventually packets become the following:
// 
// struct Packet {
//     u8 type;
//     void* data;
// }
//
// Each packet type, from both client and server, will have a unique 8 bit id.
// The data will be serialized using read/write serialization methods and stored
// in the void* data member.

// Client packets
enum ClientPacketType {
	CLIENT_PACKET_REQUEST_CONNECTION,
	CLIENT_PACKET_JOIN_ACKNOWLEDGE,
	CLIENT_PACKET_INPUT
};

struct ClientPacketHeader {
	ClientPacketType type;
	u8 client_id;
};

struct ClientRequestConnectionPacket {
	ClientPacketHeader header;
};

struct ClientJoinAcknowledgePacket {
	ClientPacketHeader header;
};

struct ClientInputPacket {
	ClientPacketHeader header;
	i32 frame;
	bool input_move_up;
	bool input_move_down;
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

struct ServerJoinAcknowledgePacket {
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
