#version 430 core
out vec4 FragColor;

in float f_color;
in vec2 f_uv;

uniform sampler2D in_sampler;

void main()
{	
	vec4 color = texture(in_sampler, f_uv);
	float alpha = length(color.xyz);

	// uncomment for visible background
	// alpha = length(color.xyz) / 2.0f + 0.5f;

	FragColor = vec4(vec3(0.25f), alpha * f_color);
}
