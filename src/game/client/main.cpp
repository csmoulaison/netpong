#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "platform/platform_media.h"
#include "platform/platform_network.h"
#include "renderer/renderer.h"

#include "game/common/match_state.cpp"
#include "game/common/packets.cpp"
#include "game/server/server.cpp"
#include "game/client/client.cpp"
#include "game/client/game.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);

	Platform* platform = platform_init_pre_graphics(nullptr, &program_arena);
	Renderer* renderer = renderer_init(nullptr, platform, &program_arena); 

	platform_init_post_graphics(platform);
	Game* game = game_init(platform, &program_arena);
	RenderList* render_list = (RenderList*)arena_alloc(&program_arena, sizeof(RenderList));

	while(game_close_requested(game) != true) {
		platform_update(platform, &program_arena);
		game_update(game, platform, render_list, &program_arena);
		renderer_update(renderer, render_list, platform, &program_arena);
		platform_swap_buffers(platform);

		sleep(1);
	}
}
