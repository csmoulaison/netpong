#include "base.h"
#include "stdlib.h"

Arena arena_create(u64 size)
{
	Arena arena;
	arena.region = (char*)malloc(size);
	arena.index = 0;
	arena.size = 0;
	return arena;
}

void arena_destroy(Arena* arena)
{
	free(arena->region);
}

void* arena_alloc(Arena* arena, u64 size)
{
	printf("Arena allocation from %u-%u (%u bytes)\n", arena->index, arena->index + size, size);
	arena->index += size;
	return &arena->region[arena->index - size];
}
