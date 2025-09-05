// Client packets
enum ClientPacketType {
	CLIENT_PACKET_JOIN,
	CLIENT_PACKET_INPUT
};

struct ClientPacketHeader {
	ClientPacketType type;
	u8 client_id;
	i32 frame;
};

struct ClientJoinPacket {
	ClientPacketHeader header;
};

struct ClientInputPacket {
	ClientPacketHeader header;
	bool input_move_up;
	bool input_move_down;
};

// Server packets
enum ServerPacketType {
	SERVER_PACKET_JOIN_ACKNOWLEDGE,
	SERVER_PACKET_STATE_UPDATE,
	SERVER_PACKET_SPEED_UP,
	SERVER_PACKET_SLOW_DOWN
};

struct ServerPacketHeader {
	ServerPacketType type;
	i32 frame;
};

struct ServerJoinAcknowledgePacket {
	ServerPacketHeader header;
	u8 client_id;
	i32 frame;
};

struct ServerStateUpdatePacket {
	ServerPacketHeader header;
	World world_state;
};

struct ServerSpeedUpPacket {
	ServerPacketHeader header;
};

struct ServerSlowDownPacket {
	ServerPacketHeader header;
};
