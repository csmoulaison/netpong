#version 430 core
layout (location = 0) in vec3 in_position;

layout(std430, binding = 0) buffer in_color_buffer
{
	float colors[];
} color_buffer;

layout(std430, binding = 1) buffer in_instance_to_voxel_buffer
{
	int map[];
} instance_to_voxel_buffer;

layout(std140, binding = 0) uniform in_ubo
{
	mat4 projection;
	int grid_length;
} ubo;

out float f_color;

// TODO - Frustum culling for when we are inside the cube, if we want that in the first place.
void main()
{
	int voxel_id = instance_to_voxel_buffer.map[gl_InstanceID];

	vec3 offset = vec3(mod(voxel_id, ubo.grid_length), (voxel_id / ubo.grid_length) % ubo.grid_length, (voxel_id / ubo.grid_length) / ubo.grid_length);
	offset += vec3(-(ubo.grid_length / 2.0f) + 0.5f);
	offset /= 16.0f;

	gl_Position = ubo.projection * vec4((in_position / 48.0f) + offset, 1.0f);
	f_color = 0.05f + color_buffer.colors[voxel_id];
}
