#include "game.h"

#include "server.cpp"

struct Player {
	float y;
	ButtonHandle input_up;
	ButtonHandle input_down;
};

struct Game {
	Player players[2];
	ButtonHandle input_quit;
	bool close_requested;
};

Game* game_init(Platform* platform, Arena* arena)
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->players[0].y = 0.0f;
	game->players[1].y = 0.0f;

	game->players[0].input_up = platform_register_button(platform, PLATFORM_KEY_W);
	game->players[0].input_down = platform_register_button(platform, PLATFORM_KEY_S);

	game->players[1].input_up = platform_register_button(platform, PLATFORM_KEY_UP);
	game->players[1].input_down = platform_register_button(platform, PLATFORM_KEY_DOWN);

	game->input_quit = platform_register_button(platform, PLATFORM_KEY_ESCAPE);
	game->close_requested = false;
	
	return game;
}

void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena)
{
	float speed = 1.0f * platform->delta_time;

	render_list->boxes_len = 2;
	for(u32 i = 0; i < 2; i++) {
		Player* player = &game->players[i];

		if(platform_button_down(platform, player->input_up))
			player->y += speed;
		if(platform_button_down(platform, player->input_down))
			player->y -= speed;

		Rect* box = &render_list->boxes[i];
		box->x = -0.75f + i * 1.5f;
		box->y = player->y;
		box->w = 0.025f;
		box->h = 0.1f;
	}

	if(platform_button_down(platform, game->input_quit))
		game->close_requested = true;
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}

