#ifndef window_h_INCLUDED
#define window_h_INCLUDED

#include "base/base.h"

#define MAX_PLATFORM_BUTTONS 8

#define INPUT_DOWN_BIT     0b00000001
#define INPUT_PRESSED_BIT  0b00000010
#define INPUT_RELEASED_BIT 0b00000100

#define INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN 256
#define INPUT_KEYCODE_UNREGISTERED -1

namespace Windowing {
	enum class Keycode {
		NoKey,
		W,
		A,
		S,
		D,
		Q,
		E,
		Space,
		Up,
		Left,
		Down,
		Right,
		Escape
	};

	typedef u32 ButtonHandle;

	struct Context {
		void* backend;

		bool viewport_update_requested;
		u32 window_width;
		u32 window_height;

		u8 input_buttons_len;
		u8 input_button_states[MAX_PLATFORM_BUTTONS];
		i16 input_keycode_to_button_lookup[INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN];
	};
}

// Forward declarations: Must be implemented by anyone including input.h

// Performs all initialization tasks that can be done before initialization of
// the graphics API backend, returning the allocated Platform layer.
Windowing::Context* platform_init_pre_graphics(Arena* arena);

// Performs all initialization tasks that cannot be done before initialization
// of the graphics API backend.
void platform_init_post_graphics(Windowing::Context* context);

// Updates the platform layer, responding to OS events and such.
void platform_update(Windowing::Context* context, Arena* arena);

// Presents the contents of the backbuffer. Called after graphics API backend
// has performed an update.
void platform_swap_buffers(Windowing::Context* context);

// Returns an identifier that can be used to check the state of a particular
// keycode (assigned to a button) at a later time.
Windowing::ButtonHandle platform_register_key(Windowing::Context* context, Windowing::Keycode keycode);
// Returns whether a button is down/pressed/released given the identifier
// returned by platform_register_button.
bool platform_button_down(Windowing::Context* context, Windowing::ButtonHandle button_id);
bool platform_button_pressed(Windowing::Context* context, Windowing::ButtonHandle button_id);
bool platform_button_released(Windowing::Context* context, Windowing::ButtonHandle button_id);

#endif
