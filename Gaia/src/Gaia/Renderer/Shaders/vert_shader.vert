
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 model;
    mat4 projection;
    mat4 view;
} u_camera;

layout(location = 0) out vec3 color;

void main()
{
	//output the position of each vertex
	gl_Position = u_camera.projection * u_camera.view * u_camera.model * position;
	color = normalize(normal) * 0.5 + 0.5;
	
}