#ifndef platform_h_INCLUDED
#define platform_h_INCLUDED

#include "base.h"

typedef struct Platform {
	void* backend;
} Platform;

typedef struct {
} PlatformInitSettings;

Platform* platform_init(PlatformInitSettings* settings, Arena* arena);
void platform_update(Platform* platform, Arena* arena);

#endif
