#include "renderer/renderer.h"

#include "GL/gl3w.h"

typedef struct
{
	f32 translation[16];
	f32 scale[16];
} QuadUbo;

typedef struct {
	u32 quad_program;
	u32 quad_ubo;
	u32 quad_vao;

	u32 text_program;
	u32 text_vao;
	u32 text_vbo;
	
	u32 text_buffer_ssbo;
} GlBackend;

u32 gl_compile_shader(const char* filename, GLenum type)
{
	// Read file
	FILE* file = fopen(filename, "r");
	if(file == nullptr) 
	{
		panic();
	}
	
	fseek(file, 0, SEEK_END);
	u32 fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char src[fsize];

	char c;
	u32 i = 0;
	while((c = fgetc(file)) != EOF) {
		src[i] = c;
		i++;
	}
	src[i] = '\0';
	fclose(file);

	// Compile shader
	u32 shader = glCreateShader(type);
	const char* src_ptr = src;
	glShaderSource(shader, 1, &src_ptr, 0);
	glCompileShader(shader);

	i32 success;
	char info[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(success == false) {
		glGetShaderInfoLog(shader, 512, nullptr, info);
		printf(info);
		panic();
	}

	return shader;
}

u32 gl_create_program(const char* vert_src, const char* frag_src)
{
	u32 vert_shader = gl_compile_shader(vert_src, GL_VERTEX_SHADER);
	u32 frag_shader = gl_compile_shader(frag_src, GL_FRAGMENT_SHADER);

	u32 program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return program;
}

u32 gl_create_ubo(u64 size, void* data)
{
	u32 ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ubo;
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena)
{
	Render::Context* renderer = (Render::Context*)arena_alloc(arena, sizeof(Render::Context));
	renderer->backend = arena_alloc(arena, sizeof(GlBackend));
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(gl3wInit() != 0)
	{
		panic();
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Quad rendering
	gl->quad_program = gl_create_program("shaders/quad.vert", "shaders/quad.frag");

	f32 quad_vertices[] = {
		 1.0f,  1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,

		 1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f,  1.0f
	};

	glGenVertexArrays(1, &gl->quad_vao);
	glBindVertexArray(gl->quad_vao);

	u32 quad_vbo;
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

	gl->quad_ubo = gl_create_ubo(sizeof(QuadUbo), nullptr);

	// Text rendering
	gl->text_program = gl_create_program("shaders/text.vert", "shaders/text.frag");

	glGenBuffers(1, &gl->text_buffer_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Render::Character) * MAX_RENDER_CHARS, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gl->text_buffer_ssbo);

	// Unbind stuff
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glViewport(0, 0, window->window_width, window->window_height);
	return renderer;
}

void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena)
{
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(window->viewport_update_requested)
	{
		glViewport(0, 0, window->window_width, window->window_height);
		window->viewport_update_requested = false;
	}
	
	// Gl render
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw rects
	glUseProgram(gl->quad_program);
	u32 quad_ubo_block_index = glGetUniformBlockIndex(gl->quad_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->quad_ubo);
	glUniformBlockBinding(gl->quad_program, quad_ubo_block_index, 0);

	glBindVertexArray(gl->quad_vao);

	for(u32 i = 0; i < render_state->rects_len; i++)
	{
		// Update ubo
		Rect quad = render_state->rects[i];

		QuadUbo quad_ubo = {
			.translation = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				quad.x, quad.y, 0.0f, 1.0f
			},
			.scale = {
				((f32)window->window_height / window->window_width) * quad.w, 0.0f, 0.0f, 0.0f,
				0.0f, quad.h, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			}
		};

		glBindBuffer(GL_UNIFORM_BUFFER, gl->quad_ubo);
		void* p_quad_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		memcpy(p_quad_ubo, &quad_ubo, sizeof(QuadUbo));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	// Text rendering
	glUseProgram(gl->text_program);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(gl->quad_vao);

	for(u8 i = 0; i < NUM_FONTS; i++) {
		Render::CharacterList* list = &render_state->character_lists[i];
		Render::Font* font = &renderer->fonts[i];

		for(u32 j = 0; j < list->characters_len; j++) {
			Render::Character* character = &list->characters[j];

			character->dst[0] /= window->window_width;
			character->dst[1] /= window->window_height;
			character->dst[0] *= 2.0f;
			character->dst[1] *= 2.0f;
			character->dst[0] -= 1.0f;
			character->dst[1] -= 1.0f;

			character->dst[2] /= window->window_width;
			character->dst[3] /= window->window_height;
			character->dst[2] *= 2.0f;
			character->dst[3] *= 2.0f;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Render::Character) * list->characters_len, list->characters);
		glBindTexture(GL_TEXTURE_2D, font->texture_id);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, list->characters_len);
	}

	// Unbind stuff
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

u32 platform_create_texture_mono(Render::Context* renderer, u8* pixels, u32 w, u32 h)
{
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(
		GL_TEXTURE_2D, 
		0, 
		GL_RED, 
		w,
		h,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return id;
}
