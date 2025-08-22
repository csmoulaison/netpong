#ifndef base_h_INCLUDED
#define base_h_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GIGABYTE 1000000000

#define panic() printf("Panic at %s:%u\n", __FILE__, __LINE__); exit(1)

#define DEBUG
#define DEBUG_STRICT

#define ASSERT(assertion) if(!assertion) { printf("Assertion failed at %s:%u\n", __FILE, __LINE__); exit(1); }
#ifdef DEBUG
	#define DEBUG_ASSERT(assertion) ASSERT(assertion)
#else
	#define DEBUG_ASSERT
#endif
#ifdef STRICT
	#define STRICT_ASSERT(assertion) ASSERT(assertion)
#else
	#define STRICT_ASSERT
#endif

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

struct Arena {
	u64 index;
	u64 size;
	char* region;
};

Arena arena_create(u64 size);
void* arena_alloc(Arena* arena, u64);
void arena_destroy(Arena* arena);

struct String {
	char* value;
	i32 len;
};

String string_create(Arena* arena, char* str, i32 len);

struct Rect {
	float x, y, w, h;
};

#endif
