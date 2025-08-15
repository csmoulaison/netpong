#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "base.h"
#include "platform.h"

typedef struct {
	void* backend;
} Renderer;

typedef struct {
} RendererInitSettings;

typedef struct {
	u32 window_width;
	u32 window_height;
} RenderList;

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderList* render_list, Platform* platform, Arena* arena);

#endif
