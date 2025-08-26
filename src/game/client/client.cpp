#define CLIENT_STATE_IDLE 0
#define CLIENT_STATE_LOBBY 1
#define CLIENT_STATE_GAME 2

Client* client_init(Arena* arena)
{
	Client* client = (Client*)arena_alloc(arena, sizeof(Client));
	client->state = CLIENT_STATE_IDLE;
	return client;
}
