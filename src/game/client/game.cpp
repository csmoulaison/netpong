struct Game {
	bool close_requested;

	/*
	ButtonHandle button_move_up;
	ButtonHandle button_move_down;
	*/
	ButtonHandle button_quit;

	Client* client;
};

Game* game_init(Platform* platform, Arena* arena, char* ip_string) 
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->close_requested = false;

	/*
	game->button_move_up = platform_register_key(platform, PLATFORM_KEY_W);
	game->button_move_down = platform_register_key(platform, PLATFORM_KEY_S);
	*/
	game->button_quit = platform_register_key(platform, PLATFORM_KEY_ESCAPE);

	game->client = client_init(platform, arena, ip_string);

	return game;
}

void game_update(Game* game, Platform* platform, RenderState* render_state)
{
	client_update(game->client, platform, render_state);
	game->close_requested = platform_button_down(platform, game->button_quit);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
