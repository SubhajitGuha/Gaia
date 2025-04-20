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

#include "../random.glsl"

struct Hit{
	vec3 color;
	vec3 incommingLight;
	uint payloadSeed;
	float rayDistance;
};
layout (location = 0) rayPayloadInEXT Hit hitValue;

vec4 GroundColour = vec4(0.664f, 0.887f, 1.000f, 1.000f);
vec4 SkyColourHorizon = vec4(0.1,0.6,1.0,1.0);
vec4 SkyColourZenith = vec4(0.1,0.4,0.89,1.0);
float SunFocus=500.0;
float SunIntensity=10.0;

//very handy function to get a sky in shader "taken from Sebastian Lague"
vec3 GetEnvironmentLight(vec3 dir)
{
	float skyGradientT = pow(smoothstep(0.0, 0.8, dir.y), 0.35);
	float groundToSkyT = smoothstep(-0.01, 0.0, dir.y);
	vec3 skyGradient = mix(SkyColourHorizon.rgb, SkyColourZenith.rgb, skyGradientT);
	float sun = pow(max(0, dot(dir, normalize(lightParameters.direction))), SunFocus) * SunIntensity;
	// Combine ground, sky, and sun
	vec3 composite = mix(GroundColour.rgb, skyGradient, groundToSkyT) + sun * (groundToSkyT>=1.0?1.0:0.0);
	return composite;
}
void main()
{
	hitValue.incommingLight = normalize(GetEnvironmentLight(gl_WorldRayDirectionEXT));// * hitValue.color;
}