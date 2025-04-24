#version 450
#extension GL_EXT_nonuniform_qualifier : enable

//shader input
flat layout (location = 0) in uint materialId;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 vertexPosition;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in vec3 vertexTangent;

#define MAX_CASCADES 8

struct Material
{
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalStrength;

	int baseColorTexture;
	int metallicRoughnessTexture;
	int normalTexture;
};

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
	vec3 viewDir;
	vec3 camPos;
} cameraBuffer;

layout(set = 0, binding = 2) uniform LightParameters
{
	vec3 color;
	float intensity;
	vec3 direction;
} lightParameters;

layout(set = 1, binding = 0) readonly buffer materialLayout
{
	Material materials[];
} materialBuffer;

layout(set = 1, binding = 1) uniform sampler2D textures[];


//output write
layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outMetallicRoughness;
layout (location = 2) out vec4 outNormal;

void main() 
{
	Material mat = materialBuffer.materials[materialId];
	vec4 albedo = mat.baseColorTexture!=-1? texture(textures[mat.baseColorTexture], texCoord): vec4(mat.baseColorFactor);
	if (albedo.a < 0.5)
	{
		discard;
	}
	vec4 matRoughness = mat.metallicRoughnessTexture!=-1?texture(textures[mat.metallicRoughnessTexture], texCoord):vec4(1.0);
	float roughness = matRoughness.g * mat.roughnessFactor;
	float metallic = matRoughness.b * mat.metallicFactor;

	outAlbedo = vec4(albedo.rgb,1.0);
	outMetallicRoughness = vec4(metallic, roughness, 0.0,1.0);
	outNormal = vec4(vertexNormal,1.0);
}