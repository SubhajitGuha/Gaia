
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint material_id;
layout(location = 5) in uint mesh_index;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 model;
    mat4 view;
    mat4 projection;
} cameraBuffer;

layout(set = 0, binding = 1) buffer readonly transformLayout
{
    mat4 model[];
} transforms;

flat layout(location = 0) out uint materialId;
layout(location = 1) out vec2 texCoord;

void main()
{
	//output the position of each vertex
	gl_Position = cameraBuffer.projection * cameraBuffer.view * transforms.model[mesh_index] * position;
	materialId = material_id;
	texCoord = tex_coord;
}