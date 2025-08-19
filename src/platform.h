#ifndef platform_h_INCLUDED
#define platform_h_INCLUDED

#include "base.h"

struct Platform {
	bool viewport_update_requested;
	u32 window_width;
	u32 window_height;
	void* backend;
};

struct PlatformInitSettings{
};

Platform* platform_init(PlatformInitSettings* settings, Arena* arena);
void platform_init_post_graphics(Platform* platform);
void platform_update(Platform* platform, Arena* arena);
void platform_swap_buffers(Platform* platform);

#endif
