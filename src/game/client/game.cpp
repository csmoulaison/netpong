// NOW: < THIS: We are on the path to allowing locally hosted servers and fully
// local play. Here's the steps:
// - Remove rendering logic from the client. Maybe some other stuff too, we'll
// figure it out.
// - Allow for the game to allocate a local server instead of client. The client
// won't be used at all in the case of a local server.
// - To start with, try the fully local case, with the inputs being passed into
// the server directly as an event, the server authoritatively simulating the
// world state, and the game layer rendering the results in both the local and
// remote case.
// - Make whatever changes are needed to support the local server and 1 remote
// client case.
// - Make a dead simple bot which sends input messages to the server like a
// client.
// - Assess whether the way events are being handled is sensical or if anything
// needs to be more systemetized.
//
// Afterwards, the following are on the agenda:
// - Restructure the platform layer, turning it into a series of namespaced
// engine subsystems which forward declare whatever platform specific stuff they
// need. The engine part will be the same in every build, and this way we can
// go all in on a maximally thin platform layer. As we add more platforms, the
// goal of thinning out the platform layer while allowing for platform specific
// optimizations should make things good and clear.
//      (even those that other subsystems use if needed)
// - Move connection acceptance/request pipeline over to the platform side of
// things, including timeouts.
// - Add some text rendering functionality so we can have some menus and debug
// stuff up.
// - Bitpacking of packets: might open up some ideas about streamlining the
// packet stuff, who knows?
// - Cleanup. I think it makes sense to save major cleanups for after a lot of
// this potentially very disruptive work.
//
// Whether we want the following before moving to the mech stuff is debatable:
// - Most majorly, a task system with a thread pool, dependency graph, all that.
// I think it makes the most sense to do this in another project first before
// including it with a networked project.
// - An audio subsystem. This will certainly be different for pong than for
// whatever stuff we get up to in the mech project, so it's not particularly
// important. But do we want fun beeps? Maybe we want fun beeps.
// - Advanced compression stuff for packets beyond just bitpacking.

struct Game {
	bool close_requested;

	ButtonHandle button_move_up;
	ButtonHandle button_move_down;
	ButtonHandle button_quit;

	Client* client;
};

Game* game_init(Platform* platform, Arena* arena, char* ip_string) 
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->close_requested = false;

	game->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	game->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	game->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	game->client = client_init(platform, arena, ip_string);

	return game;
}

void game_update(Game* game, Platform* platform, RenderState* render_state)
{
	Client* client = game->client;
	if(platform_button_down(platform, game->button_move_up)) {
		client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_UP;
		client->events_len++;
	}
	if(platform_button_down(platform, game->button_move_down)) {
		client->events[client->events_len].type = CLIENT_EVENT_INPUT_MOVE_DOWN;
		client->events_len++;
	}
	
	client_update(client, platform, render_state);
	game->close_requested = platform_button_down(platform, game->button_quit);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
