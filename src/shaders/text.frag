#version 430 core
in vec2 uv;
out vec4 frag_color;

uniform sampler2D tex;
uniform vec4 text_color;

void main()
{
	frag_color = text_color * vec4(1.0, 1.0, 1.0, texture(tex, uv).r);
}
