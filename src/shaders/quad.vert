#version 430 core
layout (location = 0) in vec2 pos;

layout(std140, binding = 0) uniform in_ubo
{
	mat4 translate;
	mat4 scale;
} ubo;

out vec3 quad_color;

void main()
{
	gl_Position = vec4(ubo.translate * ubo.scale * vec4(pos, 0.0f, 1.0f));
	quad_color = vec3(1.0f, 1.0f, 1.0f);
}
