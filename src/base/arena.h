#ifndef arena_h_INCLUDED
#define arena_h_INCLUDED

#define DEBUG_LOG_ALLOCATIONS false

struct Arena {
	u64 index;
	u64 size;
	char* data;
	bool initialized;
};

void arena_init(Arena* arena, u64 size);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, u64 size);
void* arena_head(Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

void arena_init(Arena* arena, u64 size)
{
	arena->data = (char*)malloc(size);
	arena->index = 0;
	arena->size = 0;
	arena->initialized = true;
}

void arena_destroy(Arena* arena)
{
	if(arena->initialized) {
		free(arena->data);
		arena->initialized = false;
	}
}

// Frees the current memory in dst if it exists, then copies src to dst.
void arena_copy(Arena* src, Arena* dst)
{
	if(dst->data != nullptr) {
		arena_destroy(dst);
	}
	*dst = *src;
}

void* arena_head(Arena* arena)
{
	return (void*)&arena->data[arena->index];
}

void* arena_alloc(Arena* arena, u64 size)
{
	strict_assert(arena->data != nullptr);

#if DEBUG_LOG_ALLOCATIONS
	printf("Arena allocation from %u-%u (%u bytes)\n", arena->index, arena->index + size, size);
#endif

	arena->index += size;
	return &arena->data[arena->index - size];
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // arena_h_INCLUDED
