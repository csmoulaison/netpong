#include "base.h"
#include "platform.h"
#include "renderer.h"
#include "game.h"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);

	Platform* platform = platform_init_pre_graphics(nullptr, &program_arena);
	Renderer* renderer = renderer_init(nullptr, platform, &program_arena); // Must have platform to call into platform function and get needed data.
	platform_init_post_graphics(platform);
	Game* game = game_init(platform, &program_arena);
	RenderList* render_list = (RenderList*)arena_alloc(&program_arena, sizeof(RenderList));

	while(game_close_requested(game) != true) {
		platform_update(platform, &program_arena);
		game_update(game, platform, render_list, &program_arena);
		renderer_update(renderer, render_list, platform, &program_arena);
		platform_swap_buffers(platform);
	}
}
