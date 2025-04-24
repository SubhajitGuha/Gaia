#version 450
#extension GL_EXT_nonuniform_qualifier : enable

flat layout (location = 0) in uint materialId;
layout(location = 1) in vec2 texCoord;

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

layout(set = 1, binding = 0) readonly buffer materialLayout
{
	Material materials[];
} materialBuffer;

layout(set = 1, binding = 1) uniform sampler2D textures[];

void main()
{
	Material mat = materialBuffer.materials[materialId];

	vec4 albedo = mat.baseColorTexture!=-1? texture(textures[mat.baseColorTexture], texCoord): vec4(mat.baseColorFactor);
	if (albedo.a < 0.5)
	{
		discard;
	}
}