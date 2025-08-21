#include "platform.h"

#include <X11/extensions/Xfixes.h>
#include <time.h>

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
	struct timespec time_previous;

	u8 input_buttons_len;
	u8 input_button_states[MAX_PLATFORM_BUTTONS];

	i16 input_keycode_to_button_lookup[INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN];
};

#include "glx.cpp"

Platform* platform_init_pre_graphics(PlatformInitSettings* settings, Arena* arena) 
{
	Platform* platform = (Platform*)arena_alloc(arena, sizeof(Platform));
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

	xlib->input_buttons_len = 0;
	for(u32 i = 0; i < INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN; i++) {
		xlib->input_keycode_to_button_lookup[i] = INPUT_KEYCODE_UNREGISTERED;
	}

	return platform;
}

void platform_init_post_graphics(Platform* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	platform->window_width = window_attributes.width;
	platform->window_height = window_attributes.height;

    if(clock_gettime(CLOCK_REALTIME, &xlib->time_previous))
    {
        panic();
    }
}

void platform_update(Platform* platform, Arena* arena) 
{
	Xlib* xlib = (Xlib*)platform->backend;

	for(u32 i = 0; i < xlib->input_buttons_len; i++) {
		xlib->input_button_states[i] = xlib->input_button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}

	while(XPending(xlib->display)) {
		XEvent event;
		u32 keysym;
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
						keysym = XLookupKeysym(&(event.xkey), 0);
				if(keysym >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = xlib->input_keycode_to_button_lookup[keysym];
				if(btn == INPUT_KEYCODE_UNREGISTERED) {
					break;
				}

				if(xlib->input_button_states[btn] & INPUT_DOWN_BIT) {
					break;
				}

				set_flags = INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
				xlib->input_button_states[btn] = xlib->input_button_states[btn] | set_flags;
						break;
			case KeyRelease:
	            if (XPending(xlib->display)) {
	                XPeekEvent(xlib->display, &next_event);
	                if (next_event.type == KeyPress && next_event.xkey.time == event.xkey.time 
	                && next_event.xkey.keycode == event.xkey.keycode) {
		                break;
	                }
	            }

				keysym = XLookupKeysym(&(event.xkey), 0);
				if(keysym >= INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN) {
					break;
				}

				btn = xlib->input_keycode_to_button_lookup[keysym];
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


    struct timespec time_cur;
    if(clock_gettime(CLOCK_REALTIME, &time_cur))
    {
		panic();
    }
	platform->delta_time = time_cur.tv_sec - xlib->time_previous.tv_sec + (float)time_cur.tv_nsec / 1000000000 - (float)xlib->time_previous.tv_nsec / 1000000000;
    xlib->time_previous = time_cur;
}

void platform_swap_buffers(Platform* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;
	glXSwapBuffers(xlib->display, xlib->window);
}

u32 platform_register_button(Platform* platform, u32 keycode)
{
	// NOW - Should convert keycode to Xlib native keycode here to avoid
	// indirection on all future event polling.
	Xlib* xlib = (Xlib*)platform->backend;

	u8 xlib_keycode = 0;
	switch(keycode)
	{
		case PLATFORM_KEY_W:
			xlib_keycode = XK_w;
			break;
		case PLATFORM_KEY_A:
			xlib_keycode = XK_a;
			break;
		case PLATFORM_KEY_S:
			xlib_keycode = XK_s;
			break;
		case PLATFORM_KEY_D:
			xlib_keycode = XK_d;
			break;
		case PLATFORM_KEY_Q:
			xlib_keycode = XK_q;
			break;
		case PLATFORM_KEY_E:
			xlib_keycode = XK_e;
			break;
		case PLATFORM_KEY_SPACE:
			xlib_keycode = XK_space;
			break;
		default: break;
	}
	
	xlib->input_keycode_to_button_lookup[xlib_keycode] = xlib->input_buttons_len;
	xlib->input_buttons_len++;
	return xlib->input_buttons_len - 1;
}

u8 xlib_button_state(Xlib* xlib, u32 button_id)
{
	return xlib->input_button_states[button_id];
}

bool platform_button_down(Platform* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_DOWN_BIT;
}

bool platform_button_pressed(Platform* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_PRESSED_BIT;
}

bool platform_button_released(Platform* platform, u32 button_id) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	return xlib->input_button_states[button_id] & INPUT_RELEASED_BIT;
}
