#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define RENDER_LIST_BOXES_MAX 10

#include "base/base.h"
#include "platform/platform_media.h"

struct Renderer {
	void* backend;
};

struct RendererInitSettings{
};

struct RenderList {
	Rect boxes[RENDER_LIST_BOXES_MAX];
	u8 boxes_len;
};

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderList* render_list, Platform* platform, Arena* arena);

#endif
