#include "base.h"
#include "stdlib.h"

// TODO - implement this stuff

Arena arena_create(u64 size)
{
	Arena arena;

	arena.region = malloc(size);
	arena.index = 0;
	arena.size = 0;

	return arena;
}

void* arena_alloc(Arena* arena, u64 size)
{
	return NULL;
}

void arena_destroy(Arena* arena)
{
	free(arena->region);
}
