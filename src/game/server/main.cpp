#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include <unistd.h>

#include "base/base.h"
#include "time/time.cpp"
#include "network/network.cpp"
#include "renderer/renderer.h"

#include "game/common/config.cpp"
#include "game/common/world.cpp"
#include "game/common/messages.cpp"
#include "game/server/server.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);
	Server* server = server_init(&program_arena, true);

	double time = 0.0f;
	double delta_time = BASE_FRAME_LENGTH;

	double current_time = Time::seconds();
	double accumulator_time = 0.0f;

	while(server_close_requested(server) != true) {
		server_update(server, delta_time);

		while(accumulator_time < delta_time) {
			double new_time = Time::seconds();
			accumulator_time += new_time - current_time;
			current_time = new_time;
		}
		accumulator_time -= delta_time;
	}
}
