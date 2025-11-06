#include "renderer/renderer.h"

#define RENDERER_NO_INTERPOLATION false

namespace Render {
	Context* init(Windowing::Context* window, Arena* arena) 
	{
		// API specific initialization
		Context* context = platform_render_init(window, arena);
		context->first_frame = true;

		const char* font_filenames[NUM_FONTS] = FONT_FILENAMES;
		for(u8 i = 0; i < NUM_FONTS; i++) {
			// Font loading
			FILE* font_file = fopen(font_filenames[i], "r");
			if(!font_file) { panic(); }

			Font* font = &context->fonts[i];
			u32 num_chars;
			fread(&font->texture_width, sizeof(u32), 1, font_file);
			fread(&num_chars, sizeof(u32), 1, font_file);
			for(u32 i = 0; i < num_chars; i++) {
				FontGlyph* glyph = &font->glyphs[i];
				fread(&glyph->x, sizeof(u32), 1, font_file);
				fread(&glyph->y, sizeof(u32), 1, font_file);
				fread(&glyph->w, sizeof(u32), 1, font_file);
				fread(&glyph->h, sizeof(u32), 1, font_file);
				fread(&glyph->bearing[0], sizeof(i32), 1, font_file);
				fread(&glyph->bearing[1], sizeof(i32), 1, font_file);
				fread(&glyph->advance, sizeof(u32), 1, font_file);
			}

			u32 texture_area = font->texture_width * font->texture_width;
			u8 font_pixels[texture_area];
			fread(font_pixels, sizeof(u8), texture_area, font_file);
			fclose(font_file);

			font->texture_id = platform_create_texture_mono(context, font_pixels, font->texture_width, font->texture_width);
			font->size = font->glyphs['O'].h;
		}

		return context;
	}

	void update(Context* renderer, Windowing::Context* window, double t, Arena* arena)
	{
		Render::State* current = &renderer->current_state;
		Render::State* previous = &renderer->previous_state;
		Render::State interpolated = *current;

		if(renderer->first_frame) {
			renderer->first_frame = false;
			goto skip_interpolation;
		}

#if RENDERER_NO_INTERPOLATION
		goto skip_interpolation;
#endif

		interpolated.rects_len = current->rects_len;

		for(u32 i = 0; i < previous->rects_len; i++) {
			interpolated.rects[i].x = lerp(previous->rects[i].x, current->rects[i].x, t);
			interpolated.rects[i].y = lerp(previous->rects[i].y, current->rects[i].y, t);
			interpolated.rects[i].w = lerp(previous->rects[i].w, current->rects[i].w, t);
			interpolated.rects[i].h = lerp(previous->rects[i].h, current->rects[i].h, t);
		}

skip_interpolation:
		platform_render_update(renderer, &interpolated, window, arena);
	}

	void advance_state(Context* renderer)
	{
		renderer->previous_state = renderer->current_state;
		renderer->current_state = {};
	}

	void character(Context* context, char c, float x, float y, float r, float g, float b, float a, FontFace face)
	{
		State* state = &context->current_state;
		CharacterList* list = &state->character_lists[face];
		Character* character = &list->characters[list->characters_len];

		Font* font = &context->fonts[face];
		FontGlyph* glyph = &font->glyphs[c];

		list->characters_len++;

		u32 tex_w = font->texture_width;
		character->src[0] = ((float)glyph->x) / tex_w;
		character->src[1] = ((float)glyph->y) / tex_w;
		character->src[2] = ((float)glyph->w) / tex_w;
		character->src[3] = ((float)glyph->h) / tex_w;

		character->dst[0] = x;
		character->dst[1] = y;
		character->dst[2] = glyph->w;
		character->dst[3] = glyph->h;

		character->color[0] = r;
		character->color[1] = g;
		character->color[2] = b;
		character->color[3] = a;
	}

	// Placements must be preallocated float * string length.
	void text_line_placements(
		Context* context,
		const char* string,
		float* x_placements,
		float* y_placements,
		float x,
		float y,
		float anchor_x,
		float anchor_y,
		FontFace face)
	{
		Font* font = &context->fonts[face];
		
		float line_width = 0;
		float cur_x = x;

		i32 len = strlen(string);
		for(i32 i = 0; i < len; i++) {
			char c = string[i];
			FontGlyph* glyph = &font->glyphs[c];

			x_placements[i] = cur_x + glyph->bearing[0];
			y_placements[i] = y - (glyph->h - glyph->bearing[1]);
	        cur_x += (glyph->advance >> 6);
		}

		float off_x = (cur_x - x) * anchor_x;
		float off_y = font->size * anchor_y;
		for(i32 i = 0; i < len; i++) {
			x_placements[i] -= off_x;
			y_placements[i] -= off_y;

			x_placements[i] = floor(x_placements[i]);
			y_placements[i] = floor(y_placements[i]);
		}
	}

	void text_line(
		Context* context, 
		const char* string, 
		float x, float y, 
		float anchor_x, float anchor_y, 
		float r, float g, float b, float a, 
		FontFace face)
	{
		State* state = &context->current_state;

		i32 len = strlen(string);
		float x_placements[len];
		float y_placements[len];
		text_line_placements(context, string, x_placements, y_placements, x, y, anchor_x, anchor_y, face);
		
		for(i32 i = 0; i < len; i++) {
			char c = string[i];
			FontGlyph* glyph = &context->fonts[face].glyphs[c];
			Render::character(context, c, x_placements[i], y_placements[i], r, g, b, a, face);
		}
	}
}
