#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "platform/platform_media.h"
#include "platform/platform_network.h"
#include "platform/platform_time.h"
#include "renderer/renderer.h"

bool test_add_remove_connections()
{
	Arena arena = arena_create(GIGABYTE);

	// Server connections
	PlatformSocket* socket = platform_init_server_socket(&arena);

	u64 dummy_address = 0;
	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	platform_free_connection(socket, 1);

	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	platform_free_connection(socket, 0);

	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	arena_destroy(&arena);

	return true;
}

i32 main(i32 argc, char** argv)
{
	assert(test_add_remove_connections());

	printf("Test passed!\n");
}
