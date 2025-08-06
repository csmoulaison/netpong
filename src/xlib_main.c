#include "base.h"
#include "platform.h"
#include "graphics.h"
#include "game.h"

i32 main(i32 argc, char **argv)
{
	Arena program_arena = arena_create(Gigabyte);

	Platform* platform = platform_init(&program_arena, Null);
	Graphics* graphics = graphics_init(&program_arena, Null); // The GPU layer can also call platform code, it's all good. That's how we get our needed platform data.
	Game* game = game_init(&program_arena, Null);

	while(xlib->quit != true)
	{
		platform_update(platform, arena);
		game_update(game, platform, arena);
		gpu_render(gpu, arena)
	}
}
