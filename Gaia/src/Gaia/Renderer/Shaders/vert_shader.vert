
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 model;
    mat4 view;
    mat4 projection;
} u_camera;

layout(location = 0) out vec3 color;

void main()
{
	//output the position of each vertex
	gl_Position = u_camera.projection * u_camera.view * u_camera.model * position;
	color = normalize(normal) * 0.5 + 0.5;
	
}

//#version 450
//layout(location = 0) out vec3 color;
//
//void main()
//{
//	//const array of positions for the triangle
//	const vec3 positions[3] = vec3[3](
//		vec3(1.f,1.f, 0.0f),
//		vec3(-1.f,1.f, 0.0f),
//		vec3(0.f,-1.f, 0.0f)
//	);
//
//	//const array of colors for the triangle
//	const vec3 colors[3] = vec3[3](
//		vec3(1.0f, 0.0f, 0.0f), //red
//		vec3(0.0f, 1.0f, 0.0f), //green
//		vec3(00.f, 0.0f, 1.0f)  //blue
//	);
//
//	//output the position of each vertex
//	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
//	color = colors[gl_VertexIndex];
//	
//}