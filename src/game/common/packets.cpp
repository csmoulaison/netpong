enum ClientPacketType {
	CLIENT_PACKET_JOIN,
	CLIENT_PACKET_UPDATE
};

struct ClientPacketHeader {
	ClientPacketType type;
	i64 tick_counter;
};

struct ClientJoinPacket {
	ClientPacketHeader header;
};

struct ClientUpdatePacket {
	ClientPacketHeader header;
	bool input_up;
	bool input_down;
};

enum ServerPacketType {
	SERVER_PACKET_JOIN_ACKNOWLEDGE,
	SERVER_PACKET_UPDATE
};

struct ServerPacketHeader {
	ServerPacketType type;
	i64 tick_counter;
};

struct ServerJoinAcknowledgePacket {
	ServerPacketHeader header;
	u8 client_id;
};

struct ServerUpdatePacket {
	ServerPacketHeader header;
	float paddle_positions[2];
	float ball_position[2];
};
