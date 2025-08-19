#include "platform.h"

#include <X11/extensions/Xfixes.h>

typedef struct {
	Display* display;
	Window window;
	uint32_t mouse_moved_yet;
	uint32_t mouse_just_warped;
	int32_t stored_cursor_x;
	int32_t stored_cursor_y;
	struct timespec time_previous;
} Xlib;

#include "glx.c"

Platform* platform_init(PlatformInitSettings* settings, Arena* arena) 
{
	Platform* platform = (Platform*)arena_alloc(arena, sizeof(Platform));
	Xlib* xlib = (Xlib*)arena_alloc(arena, sizeof(Xlib));


	xlib->display = XOpenDisplay(0);
	if(xlib->display == NULL) {
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

	return platform;
}

void platform_init_post_graphics(Platform* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	platform->window_width = window_attributes.width;
	platform->window_height = window_attributes.height;
}

void platform_update(Platform* platform, Arena* arena) 
{
	Xlib* xlib = (Xlib*)platform->backend;
	while(XPending(xlib->display)) {
		XEvent event;
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
				break;
			case KeyRelease:
				break;
			default: break;
		}
	}
}

void platform_swap_buffers(Platform* platform)
{
	Xlib* xlib = (Xlib*)platform->backend;
	glXSwapBuffers(xlib->display, xlib->window);
}
