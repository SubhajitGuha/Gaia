
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint material_id;
layout(location = 5) in uint mesh_index;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
} cameraBuffer;

layout(set = 0, binding = 1) buffer readonly transformLayout
{
    mat4 model[];
} transforms;

flat layout(location = 0) out uint materialId;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec4 vertexPosition;
layout(location = 3) out vec3 vertexNormal;
layout(location = 4) out vec3 vertexTangent;

void main()
{
	//output the position of each vertex
	materialId = material_id;
	texCoord = tex_coord;
	vertexNormal = normalize(transforms.model[mesh_index] * vec4(normal,0.0)).xyz; //in ws
	vertexTangent = normalize(transforms.model[mesh_index] * vec4(tangent,0.0)).xyz; //in ws
	vertexPosition = transforms.model[mesh_index] * position; //in ws
	gl_Position = cameraBuffer.projection * cameraBuffer.view * vertexPosition;
}