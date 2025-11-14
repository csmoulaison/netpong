#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "platform/platform_media.h"
#include "platform/platform_network.h"
#include "platform/platform_time.h"
#include "renderer/renderer.h"

bool test_add_remove_connections()
{
	// Server connections
	Arena net_arena;
	arena_init(&net_arena, GIGABYTE);

	// TODO: some asserts
	PlatformSocket* socket = platform_init_server_socket(&net_arena);

	u64 dummy_address = 0;
	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	platform_free_connection(socket, 1);

	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	platform_free_connection(socket, 0);

	platform_add_connection(socket, &dummy_address);
	platform_add_connection(socket, &dummy_address);

	arena_destroy(&net_arena);


	// Bitstream stuff.
	// TODO: asserts
	Arena bit_arena;
	arena_init(&bit_arena, 128);

	Bitstream s1 = bitstream_init(SerializeMode::Write, arena_head(&bit_arena));
	u32 v1 = 4;
	u32 v2 = 1;
	bitstream_write_bits(&s1, &bit_arena, (u8*)&v1, 16);
	bitstream_write_bits(&s1, &bit_arena, (u8*)&v2, 16);
	printf("data: %u\n", *(u32*)s1.data);

	Bitstream s2 = bitstream_init(SerializeMode::Write, arena_head(&bit_arena));
	u32 byte[8] = { 0, 1, 0, 0, 1, 1, 1, 0 };
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[0], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[1], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[2], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[3], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[4], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[5], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[6], 1);
	bitstream_write_bits(&s2, &bit_arena, (u8*)&byte[7], 1);
	printf("data: %b\n", *s2.data);

	Bitstream s3 = bitstream_init(SerializeMode::Write, arena_head(&bit_arena));
	const char* str = "Hello!";
	i32 len = strlen(str) + 1;
	bitstream_write_bits(&s3, &bit_arena, (u8*)str, len * 8);
	for(u8 i = 0; i < len - 1; i++) {
		printf("datac: %c\n", (char*)s3.data[i]);
	}
	printf("datra: %s\n", (char*)s3.data);

	arena_destroy(&bit_arena);


	// Better test. Get this all working eventually.
	Arena bit_arena;
	arena_init(&bit_arena, 128);
	TestMsg m1 = { .a = 133041, .b = 8, .c = true, .d = 600.1234 };
	Bitstream s1 = bitstream_init(SerializeMode::Write, arena_head(&bit_arena));

	bitstream_write_bits(&s1, &bit_arena, (u8*)&m1.a, 32);
	bitstream_write_bits(&s1, &bit_arena, (u8*)&m1.b, 8);
	bitstream_write_bits(&s1, &bit_arena, (u8*)&m1.c, 1);
	bitstream_write_bits(&s1, &bit_arena, (u8*)&m1.d, 32);

	TestMsg m2 = {};
	Bitstream s2 = bitstream_init(SerializeMode::Read, s1.data);
	bitstream_read_bits(&s2, (u8*)&m2.a, 32);
	bitstream_read_bits(&s2, (u8*)&m2.b, 8);
	bitstream_read_bits(&s2, (u8*)&m2.c, 1);
	bitstream_read_bits(&s2, (u8*)&m2.d, 32);

	printf("m1: %u %u %u %f\n", m1.a, m1.b, m1.c, m1.d);
	printf("m2: %u %u %u %f\n", m2.a, m2.b, m2.c, m2.d);

	arena_destroy(&bit_arena);


	return true;
}

i32 main(i32 argc, char** argv)
{
	assert(test_add_remove_connections());

	printf("Test passed!\n");
}
