#include <X11/extensions/Xfixes.h>

#include "window/window.h"

struct Xlib {
	Display* display;
	Window window;
	u32 mouse_moved_yet;
	u32 mouse_just_warped;
	i32 stored_cursor_x;
	i32 stored_cursor_y;
};

#include "window/xlib/glx.cpp"

Windowing::Context* platform_init_pre_graphics(Arena* arena) 
{
	Windowing::Context* context = (Windowing::Context*)arena_alloc(arena, sizeof(Windowing::Context));
	Xlib* xlib = (Xlib*)arena_alloc(arena, sizeof(Xlib));

	xlib->display = XOpenDisplay(0);
	if(xlib->display == nullptr) {
		panic();
	}

	context->backend = xlib;

	GlxInitInfo glx_info = glx_init_pre_window(xlib);

	Window root_window = RootWindow(xlib->display, glx_info.visual_info->screen);
	XSetWindowAttributes set_window_attributes = {};
	set_window_attributes.colormap = XCreateColormap(xlib->display, root_window, glx_info.visual_info->visual, AllocNone);
	set_window_attributes.background_pixmap = None;
	set_window_attributes.border_pixel = 0;
	set_window_attributes.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

	context->window_width = 100;
	context->window_height = 100;
	xlib->window = XCreateWindow(xlib->display, root_window, 0, 0, context->window_width, context->window_height, 0, glx_info.visual_info->depth, InputOutput, glx_info.visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &set_window_attributes);
	if(xlib->window == 0) {
		panic();
	}

	XFree(glx_info.visual_info);
	XStoreName(xlib->display, xlib->window, "2d_proto");
	XMapWindow(xlib->display, xlib->window);

	glx_init_post_window(xlib, glx_info.framebuffer_config);

	context->viewport_update_requested = true;
	context->backend = xlib;

	context->input_buttons_len = 1;
	for(u32 i = 0; i < INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN; i++) {
		context->input_keycode_to_button_lookup[i] = INPUT_KEYCODE_UNREGISTERED;
	}

	return context;
}

void platform_init_post_graphics(Windowing::Context* context)
{
	Xlib* xlib = (Xlib*)context->backend;

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	context->window_width = window_attributes.width;
	context->window_height = window_attributes.height;
}

Windowing::Keycode xlib_platform_from_x11_key(u32 keycode)
{
	switch(keycode) {
		case XK_w:
			return Windowing::Keycode::W;
		case XK_a:
			return Windowing::Keycode::A;
		case XK_s:
			return Windowing::Keycode::S;
		case XK_d:
			return Windowing::Keycode::D;
		case XK_q:
			return Windowing::Keycode::Q;
		case XK_e:
			return Windowing::Keycode::E;
		case XK_space:
			return Windowing::Keycode::Space;
		case XK_Up:
			return Windowing::Keycode::Up;
		case XK_Left:
			return Windowing::Keycode::Left;
		case XK_Down:
			return Windowing::Keycode::Down;
		case XK_Right:
			return Windowing::Keycode::Right;
		case XK_Escape:
			return Windowing::Keycode::Escape;
		default: return Windowing::Keycode::NoKey;
	}
}

void platform_update(Windowing::Context* context, Arena* arena) 
{
	Xlib* xlib = (Xlib*)context->backend;

	for(u32 i = 0; i < context->input_buttons_len; i++) {
		context->input_button_states[i] = context->input_button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}

	while(XPending(xlib->display)) {
		XEvent event;
		u32 keycode;
		Windowing::ButtonHandle btn;
		u8 set_flags;
		XEvent next_event;

		XNextEvent(xlib->display,  &event);
		switch(event.type) {
			case Expose: 
				break;
			case ConfigureNotify:
				XWindowAttributes win_attribs;
				XGetWindowAttributes(xlib->display, xlib->window, &win_attribs);
				context->window_width = win_attribs.width;
				context->window_height = win_attribs.height;
				context->viewport_update_requested = true;
				break;
			case MotionNotify: 
				break;
			case ButtonPress:
				break;
			case ButtonRelease:
				break;
			case KeyPress:
				keycode = (u32)xlib_platform_from_x11_key(XLookupKeysym(&(event.xkey), 0));
				if(keycode >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = context->input_keycode_to_button_lookup[keycode];
				if(btn == INPUT_KEYCODE_UNREGISTERED || (context->input_button_states[btn] & INPUT_DOWN_BIT)) {
					break;
				}

				context->input_button_states[btn] = context->input_button_states[btn] | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
				break;
			case KeyRelease:
	            if (XPending(xlib->display)) {
	                XPeekEvent(xlib->display, &next_event);
	                if (next_event.type == KeyPress && next_event.xkey.time == event.xkey.time 
	                && next_event.xkey.keycode == event.xkey.keycode) {
		                break;
	                }
	            }

				keycode = (u32)xlib_platform_from_x11_key(XLookupKeysym(&(event.xkey), 0));
				if(keycode >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = context->input_keycode_to_button_lookup[keycode];
				if(btn == INPUT_KEYCODE_UNREGISTERED) {
					break;
				}

				if(context->input_button_states[btn] & INPUT_DOWN_BIT) {
					context->input_button_states[btn] = INPUT_RELEASED_BIT;
				}
				break;
			default: break;
		}
	}
}

void platform_swap_buffers(Windowing::Context* context)
{
	Xlib* xlib = (Xlib*)context->backend;
	glXSwapBuffers(xlib->display, xlib->window);
}

u32 platform_register_key(Windowing::Context* context, Windowing::Keycode keycode)
{
	context->input_keycode_to_button_lookup[(u32)keycode] = context->input_buttons_len;
	context->input_buttons_len++;
	return context->input_buttons_len - 1;
}

bool platform_button_down(Windowing::Context* context, Windowing::ButtonHandle button_id) 
{
	return context->input_button_states[button_id] & INPUT_DOWN_BIT;
}

bool platform_button_pressed(Windowing::Context* context, Windowing::ButtonHandle button_id) 
{
	return context->input_button_states[button_id] & INPUT_PRESSED_BIT;
}

bool platform_button_released(Windowing::Context* context, Windowing::ButtonHandle button_id) 
{
	return context->input_button_states[button_id] & INPUT_RELEASED_BIT;
}
