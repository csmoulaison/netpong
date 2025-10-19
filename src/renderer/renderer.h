#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define MAX_RENDER_RECTS 10

#include "base/base.h"
#include "window/window.h"

namespace Render {
	struct Context {
		void* backend;
	};

	struct State {
		Rect rects[MAX_RENDER_RECTS];
		u8 rects_len;
	};
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena);
void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena);

#endif
