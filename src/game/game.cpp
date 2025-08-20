#include "game.h"

struct Game {

};

Game* game_init(Arena* arena)
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));
	return game;
}

void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena)
{
	render_list->cubes_len = 1;
	render_list->cubes[0][0] = -0.75f;
	render_list->cubes[0][1] = -0.1f;
	render_list->cubes[0][2] = 0.025f;
	render_list->cubes[0][3] = 0.1f;
}

bool game_close_requested(Game* game)
{
	return false;
}

