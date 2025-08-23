#include "base/base.h"
#include "renderer/renderer.h"
#include "platform/platform_media.h"
#include "platform/platform_network.h"

#include "game/common/match_state.cpp"

struct PlayerInput {
	ButtonHandle up;
	ButtonHandle down;
};

struct Game {
	MatchState match;
	PlayerInput players[2];
	ButtonHandle input_quit;
	bool close_requested;
};

Game* game_init(Platform* platform, Arena* arena)
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->match.player_positions[0] = 0.0f;
	game->match.player_positions[1] = 0.0f;

	game->players[0].up = platform_register_key(platform, PLATFORM_KEY_W);
	game->players[0].down = platform_register_key(platform, PLATFORM_KEY_S);
	
	game->players[1].up = platform_register_key(platform, PLATFORM_KEY_UP);
	game->players[1].down = platform_register_key(platform, PLATFORM_KEY_DOWN);

	game->input_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	game->close_requested = false;

	return game;
}

void game_update(Game* game, Platform* platform, RenderList* render_list, Arena* arena)
{
	float speed = 1.0f * platform->delta_time;

	render_list->boxes_len = 2;
	for(u32 i = 0; i < 2; i++) {
		PlayerInput* input = &game->players[i];
		float* position = &game->match.player_positions[i];

		if(platform_button_down(platform, input->up))
			*position += speed;
		if(platform_button_down(platform, input->down))
			*position -= speed;

		Rect* box = &render_list->boxes[i];
		box->x = -0.75f + i * 1.5f;
		box->y = *position;
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

