#include "renderer.h"

#include "GL/gl3w.h"

#define STB_IMPLEMENTATION
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
	// NOW - v4 type
	float transform[4];
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

u32 gl_compile_shader(char* filename, GLenum type)
{
	// Read file
	FILE* file = fopen(filename, "r");
	if(file == NULL) panic();
	
	fseek(file, 0, SEEK_END);
	uint32_t fsize = ftell(file);
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
		glGetShaderInfoLog(shader, 512, NULL, info);
		printf(info);
		panic();
	}

	return shader;
}

u32 gl_create_program(char* vert_src, char* frag_src)
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
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ubo;
}

Renderer* renderer_init(RendererInitSettings* settings, Platform* platform, Arena* arena)
{
	Renderer* renderer = (Renderer*)arena_alloc(arena, sizeof(Renderer));
	renderer->backend = arena_alloc(arena, sizeof(GlBackend));
	GlBackend* gl = (GlBackend*)renderer->backend;
	
	if(gl3wInit() != 0) panic();

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

	// Font atlas texture
	glGenTextures(1, &gl->font_texture);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	i32 w, h, channels;
	unsigned char* texture_data = stbi_load("fonts/plex_mono.bmp", &w, &h, &channels, 3);
	if(texture_data == NULL) {
		panic();
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(texture_data);

	// Text SSBO
	glGenBuffers(1, &gl->text_buffer_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TextChar[TEXT_MAX_CHARS]), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl->text_buffer_ssbo);

	// UBOs
	gl->box_ubo = gl_create_ubo(sizeof(BoxUbo), NULL);
	gl->text_ubo = gl_create_ubo(sizeof(TextUbo), NULL);
}

void renderer_update(Renderer* renderer, RenderList* render_list, Arena* arena)
{
	/* NOW - from old project, integrate.
	// Gl render
	glClearColor(0.84, 0.84, 0.84, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Game mode specific settings
	Mode* mode = &game->modes[game->current_mode];
	uint32_t mode_program = gl->mode_programs[game->current_mode];

	uint32_t grid_length = mode->grid_length;
	uint32_t grid_area = grid_length * grid_length;
	uint32_t grid_volume = grid_length * grid_area;

	// Update buffer
	ModeUbo mode_ubo;
	mode_ubo.time = game->time_since_init;
	memcpy(mode_ubo.data, game->mode_data, sizeof(game->mode_data));
	
	glBindBuffer(GL_UNIFORM_BUFFER, gl->mode_data_ubo_buffer);
	void* p_mode_data_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_mode_data_ubo, &mode_ubo, sizeof(ModeUbo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Dispatch compute program
	glUseProgram(mode_program);
	uint32_t mode_data_ubo_block_index = glGetUniformBlockIndex(mode_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, gl->mode_data_ubo_buffer);
	glUniformBlockBinding(mode_program, mode_data_ubo_block_index, 0);

	glDispatchCompute(grid_length / 4, grid_length / 4, grid_length / 4);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Update voxel ubo
	VoxelUbo voxel_ubo;
	voxel_ubo.grid_length = grid_length;

	mat4 perspective;
	glm_perspective(glm_rad(75.0f), window_width / window_height, 0.05f, 100.0f, perspective);

	mat4 view;
	glm_mat4_identity(view);
	float cam_target[3] = {0, 0, 0};
	float up[3] = {0, 1, 0};
	glm_lookat((float*)&game->cam_position, (float*)&cam_target, (float*)&up, view);

	glm_mat4_mul(perspective, view, voxel_ubo.projection);

	glBindBuffer(GL_UNIFORM_BUFFER, gl->voxel_ubo_buffer);
	void* p_voxel_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_voxel_ubo, &voxel_ubo, sizeof(voxel_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Update instance to voxel map ssbo
	int32_t instance_to_voxel_map[grid_volume];
	float cam_pos[3];
	v3_copy(game->cam_position, cam_pos);

	sort_voxels(instance_to_voxel_map, grid_length, grid_area, grid_volume, cam_pos);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->instance_to_voxel_buffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(instance_to_voxel_map), instance_to_voxel_map);

	// Draw grid
	glUseProgram(gl->voxel_program);

	uint32_t voxel_ubo_block_index = glGetUniformBlockIndex(gl->voxel_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->voxel_ubo_buffer);
	glUniformBlockBinding(gl->voxel_program, voxel_ubo_block_index, 0);

	glBindVertexArray(gl->voxel_vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, grid_volume);

	// Update text ubo
	TextUbo text_ubo;	
	float text_scale_x = 27.0f / window_width;
	float text_scale_y = 46.0f / window_height;
	
	v2_init(text_ubo.transform_a, text_scale_x,  0);
	v2_init(text_ubo.transform_b, 0, -text_scale_y);

	glBindBuffer(GL_UNIFORM_BUFFER, gl->text_ubo_buffer);
	void* p_text_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_text_ubo, &text_ubo, sizeof(text_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Update text ssbo buffer
	// 
	// TODO - 1. Improve text rendering API, moving some of it to game code.
	//        2. Reason about where different calculations should be made between
	//           GPU and host. Probably a lot more here, obviously.
	//        3. Keep in mind any additional features such as text color and things.
	//           
	// Then, we should probably move on to formalizing the way we are keeping track
	// of level-specific things, and how they will end up in an asset.
	//
	// As a part of that, we will want to make the flow between levels and
	// implement the level selector.
	Mode* current_mode = &game->modes[game->current_mode];
	TextChar text_buffer[TEXT_MAX_CHARS];

	char desc_str[128];
	float text_pos[2];

	sprintf(desc_str, "%s", current_mode->compute_filename);
	uint32_t text_i = fill_text_buffer(desc_str, text_buffer, v2_init(text_pos, 2.35f, 28.25f), 1.0f, 1.0f);

	char* sub_desc_str = "This is just a small showcase of what our world, nay, our universe, is capable of.";
	text_i += fill_text_buffer(sub_desc_str, &text_buffer[text_i], v2_init(text_pos, 4, 59), 0.5f, 1.0f);

	char* space_prompt_str = "[Space]";
	text_i += fill_text_buffer(space_prompt_str, &text_buffer[text_i], v2_init(text_pos, 49.8f, 36.0f), 0.66f, 1.0f - sin(game->time_since_init * 1.0f));

#define DIMLEN 7
	for(uint8_t i = 0; i < current_mode->visible_dimensions; i++)
	{
		char s[128];
		sprintf(s, "[%i] %.1f", i, game->mode_data[i]);
		text_i += fill_text_buffer(s, &text_buffer[text_i], v2_init(text_pos, 4, 2.5 + i * 1.5), 0.66f, 1.0f);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(text_buffer), text_buffer);

	// Draw text
	glUseProgram(gl->text_program);

	uint32_t text_ubo_block_index = glGetUniformBlockIndex(gl->text_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->text_ubo_buffer);
	glUniformBlockBinding(gl->text_program, text_ubo_block_index, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glBindVertexArray(gl->text_vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_i);
	*/
}
