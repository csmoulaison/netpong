#include "renderer.h"

#include "GL/gl3w.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include "stb/stb_image.h"

#define TEXT_MAX_CHARS 4096

typedef struct
{
	i32 index;
	float x;
	float y;
	float size;
	float color;
} TextChar;

typedef struct
{
	float translation[16];
	float scale[16];
} BoxUbo;

typedef struct
{
	// this is a mat2, but must be separated like this for padding requirements.
	alignas(16) float transform_a[2];
	alignas(16) float transform_b[2];
} TextUbo;

typedef struct {
	u32 box_program;
	u32 text_program;

	u32 box_ubo;
	u32 text_ubo;

	u32 quad_vao;
	u32 text_buffer_ssbo;
	u32 font_texture;
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

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena)
{
	Renderer* renderer = (Renderer*)arena_alloc(arena, sizeof(Renderer));
	renderer->backend = arena_alloc(arena, sizeof(GlBackend));
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(gl3wInit() != 0)
	{
		panic();
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->box_program = gl_create_program("shaders/box.vert", "shaders/box.frag");
	gl->text_program = gl_create_program("shaders/text.vert", "shaders/text.frag");

	// Vertex arrays/buffers
	float quad_vertices[] = {
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	/* Font atlas texture
	glGenTextures(1, &gl->font_texture);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	i32 w, h, channels;
	unsigned char* texture_data = stbi_load("fonts/plex_mono.bmp", &w, &h, &channels, 3);
	if(texture_data == nullptr) {
		panic();
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(texture_data);

	// Text SSBO
	glGenBuffers(1, &gl->text_buffer_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TextChar[TEXT_MAX_CHARS]), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl->text_buffer_ssbo);
	*/

	// UBOs
	gl->box_ubo = gl_create_ubo(sizeof(BoxUbo), nullptr);
	//gl->text_ubo = gl_create_ubo(sizeof(TextUbo), nullptr);

	glViewport(0, 0, platform->window_width, platform->window_height);

	return renderer;
}

void renderer_update(Renderer* renderer, RenderList* render_list, Platform* platform, Arena* arena)
{
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(platform->viewport_update_requested)
	{
		glViewport(0, 0, platform->window_width, platform->window_height);
		platform->viewport_update_requested = false;
	}
	
	// Gl render
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw boxes
	glUseProgram(gl->box_program);
	u32 box_ubo_block_index = glGetUniformBlockIndex(gl->box_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->box_ubo);
	glUniformBlockBinding(gl->box_program, box_ubo_block_index, 0);

	glBindVertexArray(gl->quad_vao);

	for(u32 i = 0; i < render_list->boxes_len; i++)
	{
		// Update ubo
		Rect box = render_list->boxes[i];

		BoxUbo box_ubo = {
			.translation = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				box.x, box.y, 0.0f, 1.0f
			},
			.scale = {
				((float)platform->window_height / platform->window_width) * box.w, 0.0f, 0.0f, 0.0f,
				0.0f, box.h, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			}
		};

		glBindBuffer(GL_UNIFORM_BUFFER, gl->box_ubo);
		void* p_box_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		memcpy(p_box_ubo, &box_ubo, sizeof(BoxUbo));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	/* Update text ubo
	TextUbo text_ubo;	
	float text_scale_x = 27.0f / render_list->window_width;
	float text_scale_y = 46.0f / render_list->window_height;

	//v2_init(text_ubo.transform_a, text_scale_x,  0);
	//v2_init(text_ubo.transform_b, 0, -text_scale_y);

	glBindBuffer(GL_UNIFORM_BUFFER, gl->text_ubo);
	void* p_text_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_text_ubo, &text_ubo, sizeof(text_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Update text ssbo buffer
	// TODO - 1. Improve text rendering API, moving some of it to game code.
	//        2. Reason about where different calculations should be made between
	//           GPU and host. Probably a lot more here, obviously.
	//        3. Keep in mind any additional features such as text color and things.
	TextChar text_buffer[TEXT_MAX_CHARS] = {0};
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(text_buffer), text_buffer);

	// Draw text
	glUseProgram(gl->text_program);

	u32 text_ubo_block_index = glGetUniformBlockIndex(gl->text_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->text_ubo);
	glUniformBlockBinding(gl->text_program, text_ubo_block_index, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glBindVertexArray(gl->quad_vao); // NOW - redundant, considering we have no other VAOs to bind, yes?
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 0); // NOW - draw correct amount of text quads
	*/
}
