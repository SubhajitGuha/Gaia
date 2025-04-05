#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 0) buffer readonly geometryBufferAddressLayout{
	uint64_t vertexBufferAddress;
	uint64_t indexBufferAddress;
}bufferAddress[];

layout(set = 2, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
	uint frameNumber;
} cameraBuffer;
layout(set = 2, binding = 1) buffer readonly transformLayout
{
    mat4 model[];
} transforms;
layout(set = 2, binding = 2) uniform LightParameters
{
	vec3 color;
	float intensity;
	vec3 direction;
} lightParameters;

layout(set = 3, binding = 0) readonly buffer materialLayout
{
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalStrength;

	int baseColorTexture;
	int metallicRoughnessTexture;
	int normalTexture;
} materials[];

layout(set = 3, binding = 1) uniform sampler2D textures[];

#include "random.glsl"

struct Ray{
	vec3 origin;
	vec3 dir;
};

layout (location = 1) rayPayloadInEXT bool isShadow;

void main()
{
	isShadow = false;
}