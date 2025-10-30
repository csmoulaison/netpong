#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "time/time.cpp"
#include "network/network.cpp"
#include "window/window.cpp"
#include "renderer/renderer.cpp"

#include "game/config.cpp"
#include "game/world.cpp"
#include "game/messages.cpp"
#include "game/server.cpp"
#include "game/client.cpp"
#include "game/game.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena;
	arena_init(&program_arena, GIGABYTE);

	Windowing::Context* window = Windowing::init_pre_graphics(&program_arena);
	Render::Context* renderer = Render::init(window, &program_arena); 
	Windowing::init_post_graphics(window);

	Game* game;
	char* ip_string = nullptr;
	if(argc > 1) {
		ip_string = argv[1];
	}
	game = game_init(window, &program_arena, ip_string);

	Render::State* previous_render_state = (Render::State*)arena_alloc(&program_arena, sizeof(Render::State));
	Render::State* current_render_state = (Render::State*)arena_alloc(&program_arena, sizeof(Render::State));
	arena_init(&current_render_state->texts_arena, 16000); // TODO: Suballocation
	arena_init(&previous_render_state->texts_arena, 16000); // TODO: Suballocation
	*previous_render_state = {};
	*current_render_state = {};

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

			Windowing::update(window, &program_arena);

			Render::advance_state(previous_render_state, current_render_state);
			game_update(game, window, current_render_state, &program_arena);

			time_accumulator -= frame_length;
			time += frame_length;
		}

		Render::State interpolated_render_state;
		if(first_frame) {
			first_frame = false;
			interpolated_render_state = *current_render_state;
		} else {
			double time_alpha = time_accumulator / frame_length;
			interpolated_render_state = Render::interpolate_states(previous_render_state, current_render_state, time_alpha);
		}

		// Render based on render states now.
		Render::update(renderer, &interpolated_render_state, window, &program_arena);
		Windowing::swap_buffers(window);
	}
}
