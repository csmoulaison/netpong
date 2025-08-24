#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "base/base.h"
#include "platform/platform_network.h"
#include "renderer/renderer.h"

#include "game/common/match_state.cpp"
#include "game/server/server.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);
	Server* server = server_init(SERVER_TYPE_REMOTE, &program_arena);

	while(server_close_requested(server) != true) {
		server_poll_events(server);
	}
}
