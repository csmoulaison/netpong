#version 430 core
in vec3 quad_color;
out vec4 frag_color;


void main()
{
    frag_color = vec4(quad_color, 1.0f);
} 
