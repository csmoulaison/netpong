// NOTE: On Platform, Game, and GPU Interfacing:
// 
// The platform_specific_main.c orchestrates the modules which are
// combinatoric, being the root dimension, if you will.
//
// The two other dimensions are the game and the graphics API. The graphics API,
// however, respects the contract of the game, while the game has no knowledge
// of the API. The game updates its state, the platform sends this state to the
// active graphics API, and the graphics API renders accordingly.
//
// To restate: the platform is agnostic to both the API and the game, while the
// API is agnostic to the platform but specific to the game. The game is
// agnostic to both the API and the platform.
//
// Finally, commonalities between the APIs are handled by shared procedures
// called by each specific API.

#define GRID_MAX_VOLUME 4096

typedef struct
{
	mat4 projection;
	int32_t grid_length;
} QuadUbo;

typedef struct
{
	Sphere sphere;
	Vec3f position;
	float time;
} PathtraceUbo;

typedef struct
{
	Sphere sphere;
	Vec3f position;
	float time;
} HolographUbo;

typedef struct
{
	union
	{
		PathtraceUbo pathtrace;
		HolographUbo holograph;
	};
} SpecialUbo;

typedef struct
{
	int32_t map[GRID_MAX_VOLUME];
} InstanceToVoxelSsbo;

typedef struct
{
	uint32_t quad_program;
	uint32_t quad_vao;

	uint32_t pathtrace_program;
	uint32_t holograph_program;

	uint32_t quad_ubo_buffer;
	uint32_t special_ubo_buffer;
	uint32_t instance_to_voxel_buffer;
} GlContext;

uint32_t gl_compile_shader(char* filename, GLenum type)
{
	// Read file
	FILE* file = fopen(filename, "r");
	if(file == NULL) 
	{
		panic();
	}
	fseek(file, 0, SEEK_END);
	uint32_t fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char src[fsize];

	char c;
	uint32_t i = 0;
	while((c = fgetc(file)) != EOF)
	{
		src[i] = c;
		i++;
	}
	src[i] = '\0';
	fclose(file);

	// Compile shader
	uint32_t shader = glCreateShader(type);
	const char* src_ptr = src;
	glShaderSource(shader, 1, &src_ptr, 0);
	glCompileShader(shader);

	int32_t success;
	char info[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(success == false)
	{
		glGetShaderInfoLog(shader, 512, NULL, info);
		printf(info);
		panic();
	}

	printf("compiled %s\n", filename);

	return shader;
}


void gl_init(GlContext* gl)
{
	if(gl3wInit() != 0) 
	{
		panic();
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Quad program
	// TODO - factor out program creation
	uint32_t vert_shader = gl_compile_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	uint32_t frag_shader = gl_compile_shader("shaders/quad.frag", GL_FRAGMENT_SHADER);

	gl->quad_program = glCreateProgram();
	glAttachShader(gl->quad_program, vert_shader);
	glAttachShader(gl->quad_program, frag_shader);
	glLinkProgram(gl->quad_program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	// Pathtrace program
	// TODO - further factor out compute program creation
	uint32_t pathtrace_shader = gl_compile_shader("shaders/pathtrace.comp", GL_COMPUTE_SHADER);
	gl->pathtrace_program = glCreateProgram();
	glAttachShader(gl->pathtrace_program, pathtrace_shader);
	glLinkProgram(gl->pathtrace_program);
	glDeleteShader(pathtrace_shader);

	// Pathtrace program
	uint32_t holograph_shader = gl_compile_shader("shaders/holograph.comp", GL_COMPUTE_SHADER);
	gl->holograph_program = glCreateProgram();
	glAttachShader(gl->holograph_program, holograph_shader);
	glLinkProgram(gl->holograph_program);
	glDeleteShader(holograph_shader);

	// Vertex array/buffer
	float vertices[] =
	{
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,

		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,

		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,	
	};

	glGenVertexArrays(1, &gl->quad_vao);
	glBindVertexArray(gl->quad_vao);

	uint32_t vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	float colors[GRID_MAX_VOLUME];
	for(uint32_t i = 0; i < GRID_MAX_VOLUME ; i++)
	{
		colors[i] = 0.5;
	}

	// SSBOs
	// TODO - factor out ssbo creation
	uint32_t color_ssbo;
	glGenBuffers(1, &color_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, color_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(colors), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, color_ssbo);

	glGenBuffers(1, &gl->instance_to_voxel_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->instance_to_voxel_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int32_t[GRID_MAX_VOLUME]), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gl->instance_to_voxel_buffer);

	// UBOs
	// TODO - factor out ubo creation
	glGenBuffers(1, &gl->special_ubo_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, gl->special_ubo_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SpecialUbo), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &gl->quad_ubo_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, gl->quad_ubo_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(QuadUbo), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void gl_loop(GlContext* gl, Game* game, float window_width, float window_height)
{
	// Gl render
	glClearColor(0.8, 0.8, 0.85, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Game mode specific settings
	SpecialUbo special_ubo;
	uint32_t compute_program;

	uint32_t grid_length;
	uint32_t grid_area;
	uint32_t grid_volume;
	
	switch(game->mode)
	{
		case MODE_PATHTRACE:
		{
			// TODO - Factor out getting these bits populated.
			compute_program = gl->pathtrace_program;
			grid_length = 16;

			PathtraceUbo* pathtrace = &special_ubo.pathtrace;
			pathtrace->time = game->time_since_init;
			pathtrace->sphere = game->pathtrace.sphere;
			pathtrace->position = game->pathtrace.position;

			break;
		}
		case MODE_HOLOGRAPH:
		{
			compute_program = gl->holograph_program;
			grid_length = 8;

			HolographUbo* holograph = &special_ubo.holograph;
			holograph->time = game->time_since_init;
			holograph->sphere = game->holograph.sphere;
			holograph->position = game->holograph.position;
			break;
		}
		default: break;
	}

	grid_area = grid_length * grid_length;
	grid_volume = grid_length * grid_area;

	// Update buffer
	glBindBuffer(GL_UNIFORM_BUFFER, gl->special_ubo_buffer);
	void* p_special_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_special_ubo, &special_ubo, sizeof(special_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Dispatch compute program
	glUseProgram(compute_program);
	uint32_t special_ubo_block_index = glGetUniformBlockIndex(compute_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, gl->special_ubo_buffer);
	glUniformBlockBinding(compute_program, special_ubo_block_index, 0);

	// TODO - this is only because we are still doing dumb 2d things
	uint32_t dispatch_z = grid_length;
	if(game->mode == MODE_PATHTRACE)
	{
		dispatch_z = 1;
	}

	glDispatchCompute(grid_length, grid_length, dispatch_z);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Update quad ubo
	QuadUbo quad_ubo;
	quad_ubo.grid_length = grid_length;

	mat4 perspective;
	glm_perspective(glm_rad(75.0f), window_width / window_height, 0.05f, 100.0f, perspective);

	mat4 view;
	glm_mat4_identity(view);
	Vec3f cam_target = {0, 0, 0};
	Vec3f up = {0, 1, 0};
	glm_lookat((float*)&game->position, (float*)&cam_target, (float*)&up, view);

	glm_mat4_mul(perspective, view, quad_ubo.projection);

	// Update instance to voxel map ssbo
	int32_t instance_to_voxel_map[grid_volume];
	Vec3f cam_pos = game->position;

	// All these variables suffexed "_term" will be 0 or 1, and are used to
	// selectively terms we don't want in the final calculation.
	int32_t x_positive_term = 0;
	int32_t x_negative_term = 0;
	int32_t x_unit_term = 0;
	int32_t x_row_term = 0;
	int32_t x_slice_term = 0;

	int32_t y_positive_term = 0;
	int32_t y_negative_term = 0;
	int32_t y_unit_term = 0;
	int32_t y_row_term = 0;
	int32_t y_slice_term = 0;

	int32_t z_positive_term = 0;
	int32_t z_negative_term = 0;
	int32_t z_unit_term = 0;
	int32_t z_row_term = 0;
	int32_t z_slice_term = 0;

	float x_abs = abs(cam_pos.x);
	float y_abs = abs(cam_pos.y);
	float z_abs = abs(cam_pos.z);

	if(z_abs > y_abs)
	{
		if(z_abs > x_abs)
		{
			z_slice_term = 1;
			if(x_abs > y_abs)
			{
				x_row_term = 1;
				y_unit_term = 1;
			}
			else
			{
				y_row_term = 1;
				x_unit_term = 1;
			}
		}
		else
		{
			x_slice_term = 1;
			z_row_term = 1;
			y_unit_term = 1;
		}
	}
	else // y_abs > z_abs
	{
		if(y_abs > x_abs)
		{
			y_slice_term = 1;
			if(z_abs > x_abs)
			{
				z_row_term = 1;
				x_unit_term = 1;
			}
			else
			{
				x_row_term = 1;
				z_unit_term = 1;
			}
		}
		else
		{
			x_slice_term = 1;
			y_row_term = 1;
			z_unit_term = 1;
		}
	}

	if(cam_pos.x >= 0)
	{
		x_positive_term = 1;
	}
	else
	{
		x_negative_term = 1;
	}

	if(cam_pos.y >= 0)
	{
		y_positive_term = 1;
	}
	else
	{
		y_negative_term = 1;
	}

	if(cam_pos.z >= 0)
	{
		z_positive_term = 1;
	}
	else
	{
		z_negative_term = 1;
	}

	for(int32_t i = 0; i < grid_volume; i++)
	{
		int32_t unit_index = i % grid_length;
		int32_t row_index = (i % grid_area) / grid_length;
		int32_t slice_index = i / grid_area;

		int32_t x =
			  (unit_index  * x_positive_term + (grid_length - 1 - unit_index)  * x_negative_term) * x_unit_term
			+ (row_index   * x_positive_term + (grid_length - 1 - row_index)   * x_negative_term) * x_row_term
			+ (slice_index * x_positive_term + (grid_length - 1 - slice_index) * x_negative_term) * x_slice_term;
		int32_t y =
			  (unit_index  * y_positive_term + (grid_length - 1 - unit_index)  * y_negative_term) * y_unit_term
			+ (row_index   * y_positive_term + (grid_length - 1 - row_index)   * y_negative_term) * y_row_term
			+ (slice_index * y_positive_term + (grid_length - 1 - slice_index) * y_negative_term) * y_slice_term;
		int32_t z =
			  (unit_index  * z_positive_term + (grid_length - 1 - unit_index)  * z_negative_term) * z_unit_term
			+ (row_index   * z_positive_term + (grid_length - 1 - row_index)   * z_negative_term) * z_row_term
			+ (slice_index * z_positive_term + (grid_length - 1 - slice_index) * z_negative_term) * z_slice_term;

		instance_to_voxel_map[i] = z * grid_area + y * grid_length + x;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->instance_to_voxel_buffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(instance_to_voxel_map), instance_to_voxel_map);

	glBindBuffer(GL_UNIFORM_BUFFER, gl->quad_ubo_buffer);
	void* p_quad_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_quad_ubo, &quad_ubo, sizeof(quad_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Draw grid
	glUseProgram(gl->quad_program);

	uint32_t quad_ubo_block_index = glGetUniformBlockIndex(gl->quad_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->quad_ubo_buffer);
	glUniformBlockBinding(gl->quad_program, quad_ubo_block_index, 0);

	glBindVertexArray(gl->quad_vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, grid_volume);
}
