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

struct RenderState {
	Rect boxes[RENDER_LIST_BOXES_MAX];
	u8 boxes_len;
};

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena);
void renderer_update(Renderer* renderer, RenderState* render_state, Platform* platform, Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

RenderState renderer_interpolate_states(RenderState* previous, RenderState* current, float t)
{
	// TODO - handle differing numbers of cubes. we assert that this isn't the case for now.
	assert(previous->boxes_len == current->boxes_len);

	RenderState interpolated;
	interpolated.boxes_len = current->boxes_len;
	
	for(u32 i = 0; i < previous->boxes_len; i++) {
		interpolated.boxes[i].x = lerp(previous->boxes[i].x, current->boxes[i].x, t);
		interpolated.boxes[i].y = lerp(previous->boxes[i].y, current->boxes[i].y, t);
		interpolated.boxes[i].w = lerp(previous->boxes[i].w, current->boxes[i].w, t);
		interpolated.boxes[i].h = lerp(previous->boxes[i].h, current->boxes[i].h, t);
	}

	return interpolated;
}

#endif

#endif
