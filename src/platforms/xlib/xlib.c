#include "platform.h"

#include <X11/extensions/Xfixes.h>

typedef struct {
	Display* display;
	Window window;
	uint32_t window_width;
	uint32_t window_height;
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
	platform->backend = arena_alloc(arena, sizeof(Xlib));

	Xlib* xlib = (Xlib*)&platform->backend;

	xlib->display = XOpenDisplay(0);
	if(xlib->display == NULL) 
	{
		panic();
	}

	GlxInitInfo glx_info = glx_init(xlib);

	Window root_window = RootWindow(xlib->display, glx_info.visual_info->screen);
	XSetWindowAttributes set_window_attributes =
	{
		.colormap = XCreateColormap(xlib->display, root_window, glx_info.visual_info->visual, AllocNone),
		.background_pixmap = None,
		.border_pixel = 0,
		.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
	};

	xlib->window_width = 100;
	xlib->window_height = 100;
	xlib->window = XCreateWindow(xlib->display, root_window, 0, 0, xlib->window_width, xlib->window_height, 0, glx_info.visual_info->depth, InputOutput, glx_info.visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &set_window_attributes);
	if(xlib->window == 0) 
	{
		panic();
	}

	XFree(glx_info.visual_info);
	XStoreName(xlib->display, xlib->window, "fourdee");
	XMapWindow(xlib->display, xlib->window);

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	xlib->window_width = window_attributes.width;
	xlib->window_height = window_attributes.height;
	glViewport(0, 0, xlib->window_width, xlib->window_height);

	return platform;
}

void platform_update(Platform* platform, Arena* arena) 
{
	Xlib* xlib = (Xlib*)&platform->backend;
	while(XPending(xlib->display))
	{
		XEvent event;
		XNextEvent(xlib->display,  &event);
		switch(event.type)
		{
			case Expose: 
				break;
			default: 
				break;
		}
	}
}

