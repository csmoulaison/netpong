#define SERVER_TYPE_LOCAL 0
#define SERVER_TYPE_REMOTE 1

#define SERVER_STATE_IDLE 0
#define SERVER_STATE_LOBBY 1
#define SERVER_STATE_GAME 1

struct ServerLocal {

};

struct ServerRemote {
	PlatformSocket socket;
};

struct Server {
	u8 type;
	u8 state;

	MatchState match;
	
	bool close_requested;

	union {
		ServerLocal local;
		ServerRemote remote;
	};
};

Server* server_init(u8 type, Arena* arena)
{
	Server* server = (Server*)arena_alloc(arena, sizeof(Server));
	server->type = type;
	server->state = SERVER_STATE_IDLE;

	switch(type)
	{
		case SERVER_TYPE_LOCAL:
			break;
		case SERVER_TYPE_REMOTE:
			server->remote.socket = platform_init_server_socket();
			break;
		default: break;
	}

	return server;
}

bool server_close_requested(Server* server)
{
	return false;
}

void server_poll_events(Server* server)
{
}
