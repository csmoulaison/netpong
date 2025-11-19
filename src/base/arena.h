#ifndef arena_h_INCLUDED
#define arena_h_INCLUDED

#define DEBUG_LOG_ALLOCATIONS false
#define DEBUG_CAPACITY_WARNING true

struct Arena {
	u64 index;
	u64 capacity;
	char* data;
	bool initialized;
};

void arena_init(Arena* arena, u64 capacity);
void arena_clear(Arena* arena);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, u64 size);
void* arena_head(Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

void arena_init(Arena* arena, u64 capacity)
{
	arena->data = (char*)malloc(capacity);
	arena->index = 0;
	arena->capacity = capacity;
	arena->initialized = true;
}

void arena_clear(Arena* arena)
{
	arena->index = 0;
}

void arena_destroy(Arena* arena)
{
	assert(arena->initialized);

	free(arena->data);
	arena->data = nullptr;
	arena->index = 0;
	arena->capacity = 0;
	arena->initialized = false;
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
	assert(arena->index + size < arena->capacity);

#if DEBUG_LOG_ALLOCATIONS
	printf("Arena allocation from %u-%u (%u bytes)\n", arena->index, arena->index + size, size);
#endif

	arena->index += size;

#if DEBUG_CAPACITY_WARNING
	if(arena->index > arena->capacity / 2) {
		printf("\033[31mArena more than half full!\033[0m\n");
	}
#endif

	return &arena->data[arena->index - size];
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // arena_h_INCLUDED
