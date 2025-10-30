#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "base/base.h"
#include "window/window.h"

#define MAX_RENDER_RECTS 16
#define MAX_RENDER_TEXTS 32
#define TEXTS_ARENA_SIZE 16000

namespace Render {
	struct Context {
		void* backend;
	};

	struct Text {
		char* string;
		u32 len;

		float color[4];
		float position[2];
		float scale;
	};

	struct State {
		Rect rects[MAX_RENDER_RECTS];
		u8 rects_len;

		Arena texts_arena;
		Text texts[MAX_RENDER_TEXTS];
		u8 texts_len;
	};
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena);
void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena);

#endif
