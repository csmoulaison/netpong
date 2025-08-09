#include "platform.h"

#include <X11/extensions/Xfixes.h>

#include "glx.c"

typedef struct {

} Xlib;

Platform* platform_init(PlatformInitSettings* settings, Arena* arena) 
{
	Platform* platform = (Platform*)arena_alloc(arena, sizeof(Platform));
	platform->backend = arena_alloc(arena, sizeof(Xlib));

	Xlib* xlib = (Xlib*)&platform->backend;

	// NOW - okay, jeez, how do we deal with GLX and the like?
	// For now, let's just assume GLX and try to factor all the functionality
	// out into a function. From there, we will do the necessary abstractions
	// for a port to Vulkan, for instance.
	//
	// Remember long term that the intended constraint is for all build differences
	// to be defined by which .c files we compile/link, so there is no place in
	// the code which can know any two of the options at once between platform,
	// game, and renderer.
}

void platform_update(Platform* platform, Arena* arena) 
{

}

