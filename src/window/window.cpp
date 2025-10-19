#include "window/window.h"

namespace Windowing {
	Context* init_pre_graphics(Arena* arena) {
		return platform_init_pre_graphics(arena);
	}

	void init_post_graphics(Context* context) {
		platform_init_post_graphics(context);
	}

	void update(Context* context, Arena* arena) {
		platform_update(context, arena);
	}

	void swap_buffers(Context* context) {
		platform_swap_buffers(context);
	}

	ButtonHandle register_key(Context* context, Keycode keycode) 
	{
		return platform_register_key(context, keycode);
	}

	bool button_down(Context* context, ButtonHandle button_id) 
	{
		return platform_button_down(context, button_id);
	}

	bool button_pressed(Context* context, ButtonHandle button_id) 
	{
		return platform_button_pressed(context, button_id);
	}

	bool button_released(Context* context, ButtonHandle button_id) 
	{
		return platform_button_released(context, button_id);
	}
}
