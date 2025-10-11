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

#define PACKET_CLIENT_REQUEST_CONNECTION 0
#define PACKET_CLIENT_READY_TO_START 1
#define PACKET_CLIENT_INPUT 2

#define PACKET_SERVER_ACCEPT_CONNECTION 3
#define PACKET_SERVER_START_GAME 4
#define PACKET_SERVER_END_GAME 5
#define PACKET_SERVER_DISCONNECT 6
#define PACKET_SERVER_WORLD_UPDATE 7
#define PACKET_SERVER_SPEED_UP 8
#define PACKET_SERVER_SLOW_DOWN 9

// Client packets
enum ClientPacketType {
	CLIENT_PACKET_REQUEST_CONNECTION,
	CLIENT_PACKET_READY_TO_START,
	CLIENT_PACKET_INPUT
};

struct ClientPacketHeader {
	ClientPacketType type;
	i32 client_id;
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
