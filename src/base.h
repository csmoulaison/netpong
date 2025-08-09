#ifndef base_h_INCLUDED
#define base_h_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define GIGABYTE 1000000000

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef struct {
	u64 index;
	u64 size;
	char* region;
} Arena;

Arena arena_create(u64 size);
void* arena_alloc(Arena* arena, u64);
void arena_destroy(Arena* arena);

#endif
