#version 430 core
layout (location = 0) in vec2 in_position;

struct Char
{
	int index;
	float x;
	float y;
	float size;
	float color;
};

// Includes all text written to the screen this frame.
layout(std430, binding = 2) buffer in_text
{
	Char chars[];
} text;

// The screen and font resolution, and the screen aspect ratio has been accounted
// for on the CPU side and encoded in the transform matrix.
layout(std140, binding = 0) uniform in_ubo
{
	mat2 transform;
} ubo;

out float f_color;
out vec2 f_uv;

void main()
{
	Char ch = text.chars[gl_InstanceID];
	
	vec2 offset = vec2(-1.0f, 1.0f);
	vec2 text_pos = vec2(
		ch.x,
		ch.y
	) * ch.size * 2.0f;
	gl_Position = vec4((ubo.transform * ((in_position * ch.size) + text_pos)) + offset, -1.0f, 1.0f);
	f_color = mix(ch.color, 1.0f, 0.0f);

	int i = ch.index;
	float x = mod(i, 16.0f);
	float y = -5.0f + i / 16;
	vec2 char_offset = vec2(x, y) * 2.0f;

	vec2 char_unit = vec2((27.0f / 432.0f) / 2.0f, (46.0f / 276.0f) / 2.0f);
	f_uv = (in_position + vec2(1.0f, -1.0f) + char_offset) * char_unit;
}
