#version 450
#extension GL_EXT_nonuniform_qualifier : enable

//shader input
flat layout (location = 0) in uint materialId;
layout (location = 1) in vec2 texCoord;


struct Material
{
	vec4 baseColorFactor;
	int metallicFactor;
	int roughnessFactor;
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


//output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
	Material mat = materialBuffer.materials[materialId];

	vec4 albedo = mat.baseColorTexture!=-1? texture(textures[mat.baseColorTexture], texCoord): vec4(1.0,0.0,1.0,1.0);
	outFragColor = albedo;//vec4(1.0f);
}