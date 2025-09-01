#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "platform/platform_media.h"
#include "platform/platform_network.h"
#include "platform/platform_time.h"
#include "renderer/renderer.h"

// TODO - Rethink client/server/common split. It seems increasingly non-meaningful.
#include "game/common/config.cpp"
#include "game/common/world.cpp"
#include "game/common/packets.cpp"
#include "game/server/server.cpp"
#include "game/client/client.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);

	Platform* platform = platform_init_pre_graphics(nullptr, &program_arena);
	Renderer* renderer = renderer_init(nullptr, platform, &program_arena); 

	platform_init_post_graphics(platform);
	Client* client = client_init(platform, &program_arena);

	RenderState* previous_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));
	RenderState* current_render_state = (RenderState*)arena_alloc(&program_arena, sizeof(RenderState));

	double time = 0.0f;

	double current_time = platform_time_in_seconds();
	double time_accumulator = 0.0f;

	bool first_frame = true;

	while(client_close_requested(client) != true) {
		double new_time = platform_time_in_seconds();
		double frame_time = new_time - current_time;
		if(frame_time > 0.25f) {
			frame_time = 0.25f;
		}
		current_time = new_time;
		time_accumulator += frame_time;

		while(time_accumulator >= client->frame_length) {
			platform_update(platform, &program_arena);

			memcpy(previous_render_state, current_render_state, sizeof(RenderState));
			client_update(client, platform, current_render_state);

			if(first_frame) {
				memcpy(previous_render_state, current_render_state, sizeof(RenderState));
				first_frame = false;
			}

			time_accumulator -= client->frame_length;
			time += client->frame_length;
		}

		double time_alpha = time_accumulator / client->frame_length;
		RenderState interpolated_render_state = renderer_interpolate_states(previous_render_state, current_render_state, time_alpha);

		// Render based on render states now.
		renderer_update(renderer, &interpolated_render_state, platform, &program_arena);
		platform_swap_buffers(platform);
	}
}
