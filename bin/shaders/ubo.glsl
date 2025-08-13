struct Sphere
{
	vec3 center;
	float radius;
};

layout(std140, binding = 1) uniform in_ubo
{
	Sphere sphere;
	vec3 camera_position;
	mat4 projection;
	mat4 view;
	float time;
} ubo;
