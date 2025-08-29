// Client packets
enum ClientPacketType {
	CLIENT_PACKET_JOIN,
	CLIENT_PACKET_INPUT
};

struct ClientPacketHeader {
	ClientPacketType type;
	u8 client_id;
	i32 frame_number;
};

struct ClientJoinPacket {
	ClientPacketHeader header;
};

struct ClientInputPacket {
	ClientPacketHeader header;
	bool input_up;
	bool input_down;
};

// Server packets
enum ServerPacketType {
	SERVER_PACKET_JOIN_ACKNOWLEDGE,
	SERVER_PACKET_STATE_UPDATE
};

struct ServerPacketHeader {
	ServerPacketType type;
	u32 frame_number;
};

struct ServerJoinAcknowledgePacket {
	ServerPacketHeader header;
	u8 client_id;
	u32 frame_number;
};

struct ServerStateUpdatePacket {
	ServerPacketHeader header;
	float paddle_positions[2];
	float paddle_velocities[2];
};
