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

typedef u32 ButtonHandle;

struct Platform {
	void* backend;

	bool viewport_update_requested;
	u32 window_width;
	u32 window_height;

	float delta_time;
};

struct PlatformInitSettings{
};

// Performs all initialization tasks that can be done before initialization of
// the graphics API backend, returning the allocated Platform layer.
Platform* platform_init_pre_graphics(PlatformInitSettings* settings, Arena* arena);

// Performs all initialization tasks that cannot be done before initialization
// of the graphics API backend.
void platform_init_post_graphics(Platform* platform);

// Updates the platform layer, responding to OS events and such.
void platform_update(Platform* platform, Arena* arena);

// Presents the contents of the backbuffer. Called after graphics API backend
// has performed an update.
void platform_swap_buffers(Platform* platform);

// Returns an identifier that can be used to check the state of a particular
// keycode (assigned to a button) at a later time.
ButtonHandle platform_register_button(Platform* platform, u32 keycode);

// Returns whether a button is down/pressed/released given the identifier
// returned by platform_register_button.
bool platform_button_down(Platform* platform, u32 button_id);
bool platform_button_pressed(Platform* platform, u32 button_id);
bool platform_button_released(Platform* platform, u32 button_id);

#endif
