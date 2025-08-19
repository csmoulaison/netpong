#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "base.h"
#include "platform.h"

struct Renderer {
	void* backend;
};

struct RendererInitSettings{
};

struct RenderList {
	u32 window_width;
	u32 window_height;
};

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderList* render_list, Platform* platform, Arena* arena);

#endif
