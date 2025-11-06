#version 430 core
layout (location = 0) in vec2 vert;
out vec2 uv;
out vec4 text_color;

struct Char {
	vec4 src;
	vec4 dst;
	vec4 color;
};

layout(std430, binding = 0) buffer txt
{
	Char string[];
} text;

void main()
{
	vec2 normal_vert = vert / vec2(2.0f) + vec2(0.5f);
	Char ch = text.string[gl_InstanceID];

	// Vert position
	vec2 pos2d = ch.dst.xy + ch.dst.zw * normal_vert;
	gl_Position = vec4(pos2d, 0.0f, 1.0f);

	// UV coordinates
	vec2 flipped_vert = vec2(normal_vert.x, 1.0f - normal_vert.y);
	uv = ch.src.xy + ch.src.zw * flipped_vert;

	// Text color
	text_color = ch.color;
}
