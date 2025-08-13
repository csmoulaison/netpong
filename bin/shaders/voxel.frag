#version 430 core
out vec4 FragColor;

in float f_color;

void main()
{
    FragColor = vec4(
	    mix(0.5f, 1.0f, f_color), 
	    0.2f + mix(0.5f, 0.0f, f_color),
	    0.2f + mix(0.5f, 0.0f, f_color),
	    f_color * 0.5f);
} 
