
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in uint material_id;
layout(location = 5) in uint mesh_index;

#define MAX_CASCADES 8
flat layout (location = 0) out uint materialId;
layout(location = 1) out vec2 texCoord;

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

layout(set = 2, binding = 0) uniform LightData
{
	mat4 lightView;
	mat4 lightProjection;
	float farRanges;
	int numCascades;
} lightData;

void main()
{
	texCoord = tex_coord;
	materialId = material_id;
	//output the position of each vertex
	gl_Position = lightData.lightProjection * lightData.lightView * transforms.model[mesh_index] * position;
}