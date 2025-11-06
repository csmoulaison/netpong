#version 430 core
in vec2 uv;
in vec4 text_color;
out vec4 frag_color;

uniform sampler2D tex;

void main()
{
	frag_color = text_color * vec4(1.0, 1.0, 1.0, texture(tex, vec2(uv.x, uv.y)).r);
}
