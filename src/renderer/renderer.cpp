#include "renderer/renderer.h"

#define RENDERER_NO_INTERPOLATION false

namespace Render {
	Context* init(Windowing::Context* window, Arena* arena) 
	{
		return platform_render_init(window, arena);
	}

	void update(Context* renderer, State* render_state, Windowing::Context* window, Arena* arena)
	{
		platform_render_update(renderer, render_state, window, arena);
	}

	State interpolate_states(State* previous, State* current, f32 t)
	{
#if RENDERER_NO_INTERPOLATION
		return *current;
#endif

		// TODO - handle differing numbers of cubes. we assert that this isn't the case for now.
		assert(previous->rects_len == current->rects_len);

		Render::State interpolated;
		interpolated.rects_len = current->rects_len;
		
		for(u32 i = 0; i < previous->rects_len; i++) {
			interpolated.rects[i].x = lerp(previous->rects[i].x, current->rects[i].x, t);
			interpolated.rects[i].y = lerp(previous->rects[i].y, current->rects[i].y, t);
			interpolated.rects[i].w = lerp(previous->rects[i].w, current->rects[i].w, t);
			interpolated.rects[i].h = lerp(previous->rects[i].h, current->rects[i].h, t);
		}

		return interpolated;
	}
}
