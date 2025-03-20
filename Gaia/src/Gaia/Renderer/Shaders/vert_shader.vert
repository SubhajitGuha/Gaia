
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint mesh_index;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 model;
    mat4 view;
    mat4 projection;
} cameraBuffer;

flat layout(location = 0) out uint meshIndex;
layout(location = 1) out vec2 texCoord;

void main()
{
	//output the position of each vertex
	gl_Position = cameraBuffer.projection * cameraBuffer.view * cameraBuffer.model * position;
	meshIndex = mesh_index;
	texCoord = tex_coord;
}