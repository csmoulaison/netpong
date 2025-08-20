#version 430 core
layout (location = 0) in vec2 in_position;

layout(std140, binding = 0) uniform in_ubo
{
	mat4 translate;
	mat4 scale;
} ubo;

out vec3 f_color;

void main()
{
	vec4 pos_v4 = vec4(in_position, 0.0f, 1.0f);
	gl_Position = vec4(ubo.translate * ubo.scale * pos_v4);
	//gl_Position = vec4(ubo.projection * vec3(in_position, 1.0f), 1.0f);
	//gl_Position = vec4(in_position * 0.5f, 0.0f, 1.0f);
	f_color = vec3(1.0f, 1.0f, 1.0f);
}
