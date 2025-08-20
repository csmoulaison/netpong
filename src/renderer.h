#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define RENDER_LIST_CUBES_MAX 10

#include "base.h"
#include "platform.h"

struct Renderer {
	void* backend;
};

struct RendererInitSettings{
};

struct RenderList {
	float cubes[4][RENDER_LIST_CUBES_MAX];
	u8 cubes_len;
};

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderList* render_list, Platform* platform, Arena* arena);

#endif
