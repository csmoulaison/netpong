#version 430 core
layout (location = 0) in vec2 in_position;

layout(std140, binding = 0) uniform in_ubo
{
	mat2 projection;
} ubo;

out vec3 f_color;

void main()
{
	//gl_Position = vec4(ubo.projection * in_position, 0.0f, 1.0f);
	gl_Position = vec4(in_position * 0.5f, 0.0f, 1.0f);
	f_color = vec3(1.0f, 1.0f, 1.0f);
}
