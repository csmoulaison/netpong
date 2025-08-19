#include "string.h"

String string_create(Arena* arena, char* str, i32 len)
{
	String string;
	string.value = (char*)arena_alloc(arena, len);
	string.len = len;
	return string;
}
