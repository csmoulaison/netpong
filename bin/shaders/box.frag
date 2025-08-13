#version 430 core
out vec4 FragColor;

in float f_color;

void main()
{
	float comp_a = f_color * 0.8f;
	float comp_b = f_color * 0.7f;
	float comp_g = f_color * 0.4f;
    FragColor = vec4(0.2f + comp_b, 0.4f - comp_g, 0.8f - comp_a, f_color * 0.66 + 0.0f);
} 
