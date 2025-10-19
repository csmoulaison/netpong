#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "time/time.cpp"
#include "network/network.cpp"
#include "renderer/renderer.h"

#include "game/common/config.cpp"
#include "game/common/world.cpp"
#include "game/common/messages.cpp"
#include "game/server/server.cpp"
#include "game/client/client.cpp"
#include "game/client/game.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);

	PlatformWindow* platform = platform_init_pre_graphics(nullptr, &program_arena);
	Renderer* renderer = renderer_init(nullptr, platform, &program_arena); 

	platform_init_post_graphics(platform);

	Game* game;
	char* ip_string = nullptr;
	if(argc > 1) {
		ip_string = argv[1];
	}
	game = game_init(platform, &program_arena, ip_string);

	RenderState* previous_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));
	RenderState* current_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));

	double time = 0.0f;
	double current_time = Time::seconds();
	double time_accumulator = 0.0f;
	bool first_frame = true;
	double frame_length = BASE_FRAME_LENGTH;

	while(game_close_requested(game) != true) {
		double new_time = Time::seconds();
		double frame_time = new_time - current_time;
		if(frame_time > 0.25f) {
			frame_time = 0.25f;
		}
		current_time = new_time;
		time_accumulator += frame_time;

		while(time_accumulator >= frame_length) {
			if(game->client != nullptr) {
				frame_length = game->client->frame_length;
			} else {
				frame_length = BASE_FRAME_LENGTH;
			}

			platform_update(platform, &program_arena);

			memcpy(previous_render_state, current_render_state, sizeof(RenderState));
			*current_render_state = {};
			game_update(game, platform, current_render_state);

			if(first_frame) {
				memcpy(previous_render_state, current_render_state, sizeof(RenderState));
				first_frame = false;
			}

			time_accumulator -= frame_length;
			time += frame_length;
		}

		double time_alpha = time_accumulator / frame_length;
		RenderState interpolated_render_state = renderer_interpolate_states(previous_render_state, current_render_state, time_alpha);

		// Render based on render states now.
		renderer_update(renderer, &interpolated_render_state, platform, &program_arena);
		platform_swap_buffers(platform);
	}
}
