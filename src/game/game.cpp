#include "game.h"

struct Game {
	ButtonHandle button_up;
	ButtonHandle button_down;

	float paddle_y;
};

Game* game_init(Platform* platform, Arena* arena)
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->button_up = platform_register_button(platform, PLATFORM_KEY_W);
	game->button_down = platform_register_button(platform, PLATFORM_KEY_S);

	game->paddle_y = 0.0f;
	
	return game;
}

void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena)
{
	float speed = 1.0f * platform->delta_time;
	if(platform_button_down(platform, game->button_up))
		game->paddle_y += speed;
	if(platform_button_down(platform, game->button_down))
		game->paddle_y -= speed;

	render_list->cubes_len = 2;
	render_list->cubes[0][0] = -0.75f;
	render_list->cubes[0][1] = game->paddle_y;
	render_list->cubes[0][2] = 0.025f;
	render_list->cubes[0][3] = 0.1f;

	render_list->cubes[1][0] = 0.75f;
	render_list->cubes[1][1] = game->paddle_y;
	render_list->cubes[1][2] = 0.025f;
	render_list->cubes[1][3] = 0.1f;
}

bool game_close_requested(Game* game)
{
	return false;
}

