// NOW - we want to be able to selectively exclude windowing and such from the
// server build.
//
// What I'm currently thinking: One more translation unit for each the x11 platform.
// platform.h interface split into platform_media.h(?) and platform_sockets.h, with
// an associated translation unit each.

#include "base/base.h"
#include "platform/platform_network.h"
#include "renderer/renderer.h"

#include "game/server/server.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena = arena_create(GIGABYTE);
	Server* server = server_init(SERVER_TYPE_REMOTE, &program_arena);

	while(server_close_requested(server) != true) {
		server_poll_events(server);
	}
}
