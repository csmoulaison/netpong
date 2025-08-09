#include "base.h"
#include "platform.h"
#include "renderer.h"
#include "game.h"

i32 main(i32 argc, char **argv)
{
	Arena program_arena = arena_create(GIGABYTE);

	Platform* platform = platform_init(NULL, &program_arena);
	Renderer* renderer = renderer_init(NULL, platform, &program_arena); // Must have platform to call into platform function and get needed data.
	Game* game = game_init(&program_arena);
	RenderList* render_list = arena_alloc(&program_arena, sizeof(RenderList));

	while(game_close_requested(game) != true) {
		platform_update(platform, &program_arena);
		game_update(game, platform, render_list, &program_arena);
		renderer_update(renderer, render_list, &program_arena);
	}
}
