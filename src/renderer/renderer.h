#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define RENDERER_NO_INTERPOLATION false
#define RENDER_LIST_RECTS_MAX 10

#include "base/base.h"
#include "window/window.h"

struct Renderer {
	void* backend;
};

struct RendererInitSettings{
};

struct RenderState {
	Rect rects[RENDER_LIST_RECTS_MAX];
	u8 rects_len;
};

Renderer* renderer_init(RendererInitSettings* settings, Windowing::Context* window, Arena* arena);
void renderer_update(Renderer* renderer, RenderState* render_state, Windowing::Context* window, Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

RenderState renderer_interpolate_states(RenderState* previous, RenderState* current, f32 t)
{
#if RENDERER_NO_INTERPOLATION
	return *current;
#endif

	// TODO - handle differing numbers of cubes. we assert that this isn't the case for now.
	assert(previous->rects_len == current->rects_len);

	RenderState interpolated;
	interpolated.rects_len = current->rects_len;
	
	for(u32 i = 0; i < previous->rects_len; i++) {
		interpolated.rects[i].x = lerp(previous->rects[i].x, current->rects[i].x, t);
		interpolated.rects[i].y = lerp(previous->rects[i].y, current->rects[i].y, t);
		interpolated.rects[i].w = lerp(previous->rects[i].w, current->rects[i].w, t);
		interpolated.rects[i].h = lerp(previous->rects[i].h, current->rects[i].h, t);
	}

	return interpolated;
}

#endif

#endif
