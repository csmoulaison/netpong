#include "platform/platform_media.h"

#include <X11/extensions/Xfixes.h>

#define INPUT_DOWN_BIT     0b00000001
#define INPUT_PRESSED_BIT  0b00000010
#define INPUT_RELEASED_BIT 0b00000100

#define INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN 256
#define INPUT_KEYCODE_UNREGISTERED -1

struct Xlib {
	Display* display;
	Window window;
	u32 mouse_moved_yet;
	u32 mouse_just_warped;
	i32 stored_cursor_x;
	i32 stored_cursor_y;

	u8 input_buttons_len;
	u8 input_button_states[MAX_PLATFORM_BUTTONS];

	i16 input_keycode_to_button_lookup[INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN];
};

#include "glx.cpp"

PlatformWindow* platform_init_pre_graphics(PlatformInitSettings* settings, Arena* arena) 
{
	PlatformWindow* platform = (PlatformWindow*)arena_alloc(arena, sizeof(PlatformWindow));
	Xlib* xlib = (Xlib*)arena_alloc(arena, sizeof(Xlib));

	xlib->display = XOpenDisplay(0);
	if(xlib->display == nullptr) {
		panic();
	}

	platform->backend = xlib;

	GlxInitInfo glx_info = glx_init_pre_window(xlib);

	Window root_window = RootWindow(xlib->display, glx_info.visual_info->screen);
	XSetWindowAttributes set_window_attributes = {};
	set_window_attributes.colormap = XCreateColormap(xlib->display, root_window, glx_info.visual_info->visual, AllocNone);
	set_window_attributes.background_pixmap = None;
	set_window_attributes.border_pixel = 0;
	set_window_attributes.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

	platform->window_width = 100;
	platform->window_height = 100;
	xlib->window = XCreateWindow(xlib->display, root_window, 0, 0, platform->window_width, platform->window_height, 0, glx_info.visual_info->depth, InputOutput, glx_info.visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &set_window_attributes);
	if(xlib->window == 0) {
		panic();
	}

	XFree(glx_info.visual_info);
	XStoreName(xlib->display, xlib->window, "2d_proto");
	XMapWindow(xlib->display, xlib->window);

	glx_init_post_window(xlib, glx_info.framebuffer_config);

	platform->viewport_update_requested = true;
	platform->backend = xlib;

	xlib->input_buttons_len = 1;
	for(u32 i = 0; i < INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN; i++) {
		xlib->input_keycode_to_button_lookup[i] = INPUT_KEYCODE_UNREGISTERED;
	}

	return platform;
}

void platform_init_post_graphics(PlatformWindow* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	platform->window_width = window_attributes.width;
	platform->window_height = window_attributes.height;
}

u8 xlib_platform_from_x11_key(u32 keycode)
{
	switch(keycode) {
		case XK_w:
			return PLATFORM_KEY_W;
		case XK_a:
			return PLATFORM_KEY_A;
		case XK_s:
			return PLATFORM_KEY_S;
		case XK_d:
			return PLATFORM_KEY_D;
		case XK_q:
			return PLATFORM_KEY_Q;
		case XK_e:
			return PLATFORM_KEY_E;
		case XK_space:
			return PLATFORM_KEY_SPACE;
		case XK_Up:
			return PLATFORM_KEY_UP;
		case XK_Left:
			return PLATFORM_KEY_LEFT;
		case XK_Down:
			return PLATFORM_KEY_DOWN;
		case XK_Right:
			return PLATFORM_KEY_RIGHT;
		case XK_Escape:
			return PLATFORM_KEY_ESCAPE;
		default: return 0;
	}
}

void platform_update(PlatformWindow* platform, Arena* arena) 
{
	Xlib* xlib = (Xlib*)platform->backend;

	for(u32 i = 0; i < xlib->input_buttons_len; i++) {
		xlib->input_button_states[i] = xlib->input_button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}

	while(XPending(xlib->display)) {
		XEvent event;
		u8 keycode;
		ButtonHandle btn;
		u8 set_flags;
		XEvent next_event;

		XNextEvent(xlib->display,  &event);
		switch(event.type) {
			case Expose: 
				break;
			case ConfigureNotify:
				XWindowAttributes win_attribs;
				XGetWindowAttributes(xlib->display, xlib->window, &win_attribs);
				platform->window_width = win_attribs.width;
				platform->window_height = win_attribs.height;
				platform->viewport_update_requested = true;
				break;
			case MotionNotify: 
				break;
			case ButtonPress:
				break;
			case ButtonRelease:
				break;
			case KeyPress:
				keycode = xlib_platform_from_x11_key(XLookupKeysym(&(event.xkey), 0));
				if(keycode >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = xlib->input_keycode_to_button_lookup[keycode];
				if(btn == INPUT_KEYCODE_UNREGISTERED || (xlib->input_button_states[btn] & INPUT_DOWN_BIT)) {
					break;
				}

				xlib->input_button_states[btn] = xlib->input_button_states[btn] | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
				break;
			case KeyRelease:
	            if (XPending(xlib->display)) {
	                XPeekEvent(xlib->display, &next_event);
	                if (next_event.type == KeyPress && next_event.xkey.time == event.xkey.time 
	                && next_event.xkey.keycode == event.xkey.keycode) {
		                break;
	                }
	            }

				keycode = xlib_platform_from_x11_key(XLookupKeysym(&(event.xkey), 0));
				if(keycode >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = xlib->input_keycode_to_button_lookup[keycode];
				if(btn == INPUT_KEYCODE_UNREGISTERED) {
					break;
				}

				if(xlib->input_button_states[btn] & INPUT_DOWN_BIT) {
					xlib->input_button_states[btn] = INPUT_RELEASED_BIT;
				}
				break;
			default: break;
		}
	}
}

void platform_swap_buffers(PlatformWindow* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;
	glXSwapBuffers(xlib->display, xlib->window);
}

u32 platform_register_key(PlatformWindow* platform, u32 keycode)
{
	Xlib* xlib = (Xlib*)platform->backend;
	xlib->input_keycode_to_button_lookup[keycode] = xlib->input_buttons_len;
	xlib->input_buttons_len++;
	return xlib->input_buttons_len - 1;
}

u8 xlib_button_state(Xlib* xlib, u32 button_id)
{
	return xlib->input_button_states[button_id];
}

bool platform_button_down(PlatformWindow* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_DOWN_BIT;
}

bool platform_button_pressed(PlatformWindow* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_PRESSED_BIT;
}

bool platform_button_released(PlatformWindow* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_RELEASED_BIT;
}
