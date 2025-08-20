#ifndef platform_h_INCLUDED
#define platform_h_INCLUDED

#include "base.h"

#define MAX_PLATFORM_BUTTONS 8

#define PLATFORM_KEY_W 0
#define PLATFORM_KEY_A 1
#define PLATFORM_KEY_S 2
#define PLATFORM_KEY_D 3
#define PLATFORM_KEY_Q 4
#define PLATFORM_KEY_E 5
#define PLATFORM_KEY_SPACE 6

struct Platform {
	void* backend;

	bool viewport_update_requested;
	u32 window_width;
	u32 window_height;
};

struct PlatformInitSettings{
};

Platform* platform_init_pre_graphics(PlatformInitSettings* settings, Arena* arena);
void platform_init_post_graphics(Platform* platform);
void platform_update(Platform* platform, Arena* arena);
void platform_swap_buffers(Platform* platform);

// Resets the pressed/released state for all active buttons.
void platform_reset_input(Platform* platform);

// Returns an identifier that can be used to check the state of a particular
// keycode at a later time.
u32 platform_register_button(Platform* platform, u32 key_code);

// Returns whether a button is down/pressed/released given the identifier
// returned by platform_register_button.
bool platform_button_down(Platform* platform, u32 key_id);
bool platform_button_pressed(Platform* platform, u32 key_id);
bool platform_button_released(Platform* platform, u32 key_id);

#endif
