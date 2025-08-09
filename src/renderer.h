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
} RenderList;

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderList* render_list, Arena* arena);

#endif
