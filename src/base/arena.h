#ifndef arena_h_INCLUDED
#define arena_h_INCLUDED

#define DEBUG_LOG_ALLOCATIONS true

struct Arena {
	u64 index;
	u64 size;
	char* region;
};

Arena arena_create(u64 size);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, u64 size);

#ifdef CSM_BASE_IMPLEMENTATION

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
#if DEBUG_LOG_ALLOCATIONS
	printf("Arena allocation from %u-%u (%u bytes)\n", arena->index, arena->index + size, size);
#endif

	arena->index += size;
	return &arena->region[arena->index - size];
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // arena_h_INCLUDED
