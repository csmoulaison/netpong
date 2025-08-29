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

	RenderState* previous_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));
	RenderState* current_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));

	double time = 0.0f;
	double delta_time = 0.005f;

	double current_time = platform_time_in_seconds();
	double time_accumulator = 0.0f;

	bool first_frame = true;

	while(game_close_requested(game) != true) {
		double new_time = platform_time_in_seconds();
		double frame_time = new_time - current_time;
		if(frame_time > 0.25f) {
			frame_time = 0.25f;
		}
		current_time = new_time;
		time_accumulator += frame_time;
		printf("RENDER FRAME - Accumulator: %f\n", time_accumulator);

		while(time_accumulator >= delta_time) {
			platform_update(platform, &program_arena);

			memcpy(previous_render_state, current_render_state, sizeof(RenderState));
			game_update(game, platform, current_render_state, &program_arena);

			if(first_frame) {
				memcpy(previous_render_state, current_render_state, sizeof(RenderState));
				first_frame = false;
			}

			time_accumulator -= delta_time;
			time += delta_time;
			printf("delta: %f\naccum: %f\n", delta_time, time_accumulator);
		}

		double time_alpha = time_accumulator / delta_time;
		RenderState interpolated_render_state = renderer_interpolate_states(previous_render_state, current_render_state, time_alpha);

		// Render based on render states now.
		renderer_update(renderer, &interpolated_render_state, platform, &program_arena);
		platform_swap_buffers(platform);
	}
}
