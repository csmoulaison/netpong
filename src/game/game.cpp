// NOW: < LIST:
// - Cleanup. Comb through these file by file. Maybe take a quick second pass
//   after you're done.
// 
//   simulation/network stuff
//   [ ] client.cpp
//   [ ] server.cpp
//   [ ] world.cpp
//   [ ] messages.cpp
//   game stuff 
//   [X] game.cpp
//   [ ] session.cpp
//   [ ] menu.cpp
//   // main/config stuff
//   [ ] headless_main.cpp
//   [ ] main.cpp
//   [ ] config.cpp
//   core stuff 
//   [ ] networking
//   [ ] rendering
//   [ ] windowing
//   [ ] serialization
//
// - Todos. High priority todos have been marked with a '$' suffix.
//
//
// WHETHER we want the following before moving to the next project is debatable:
// 
// - Most majorly, a task system with a thread pool, dependency graph, all that.
//   I think it makes the most sense to do this in another project first before
//   including it with a networked project.
//   
// - An audio subsystem. This will certainly be different for pong than for
//   whatever stuff we get up to in the next project, so it's not particularly
//   important. But do we want fun beeps? Maybe we want fun beeps.
//   
// - Advanced compression stuff for packets beyond just bitpacking.

#include "game/config.cpp"
#include "game/world.cpp"
#include "game/messages.cpp"
#include "game/server.cpp"
#include "game/client.cpp"

#include "game/game_data.cpp"
#include "game/session.cpp"
#include "game/menu.cpp"

Game* game_init(Windowing::Context* window, char* ip_string, Arena* program_arena) 
{
	Game* game = (Game*)arena_alloc(program_arena, sizeof(Game));

	arena_init(&game->persistent_arena, MEGABYTE);
	arena_init(&game->session_arena, MEGABYTE);
	arena_init(&game->frame_arena, MEGABYTE);

	game->state = GameState::Menu;
	game->close_requested = false;
	game->frames_since_init = 0;

	game->move_up_buttons[0] = Windowing::register_key(window, Windowing::Keycode::W);
	game->move_down_buttons[0] = Windowing::register_key(window, Windowing::Keycode::S);
	game->move_up_buttons[1] = Windowing::register_key(window, Windowing::Keycode::Up);
	game->move_down_buttons[1] = Windowing::register_key(window, Windowing::Keycode::Down);
	game->quit_button = Windowing::register_key(window, Windowing::Keycode::Escape);
	game->select_button = Windowing::register_key(window, Windowing::Keycode::Space);

	if(ip_string != nullptr) {
		memcpy(game->ip_string, ip_string, 16);
	} else {
		game->ip_string[0] = '\0';
	}

	game->menu_selection = 0;
	for(i32 i = 0; i < MENU_OPTIONS_LEN; i++) {
		game->menu_activations[i] = 0.0f;
	}

	game->client = nullptr;
	game->server = nullptr;

	return game;
}

void game_update(Game* game, Windowing::Context* window, Render::Context* renderer)
{
	switch(game->state) {
		case GameState::Menu:
			menu_update(game, window, renderer);
			break;
		case GameState::Session:
			session_update(game, window, renderer);
			break;
		default: break;
	} 
	game->frames_since_init++;
	arena_clear(&game->frame_arena);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
