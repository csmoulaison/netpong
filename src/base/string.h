#ifndef string_h_INCLUDED
#define string_h_INCLUDED

#include "base/arena.h"

struct String {
	char* value;
	i32 len;
};

String string_create(Arena* arena, char* str, i32 len);

#ifdef CSM_BASE_IMPLEMENTATION

String string_create(Arena* arena, char* str, i32 len)
{
	String string;
	string.value = (char*)arena_alloc(arena, len);
	string.len = len;
	return string;
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // string_h_INCLUDED
