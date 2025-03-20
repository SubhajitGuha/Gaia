#version 450
#extension GL_EXT_nonuniform_qualifier : enable

//shader input
flat layout (location = 0) in uint meshIndex;
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
	Material mat = materialBuffer.materials[meshIndex];

	vec4 albedo = texture(textures[mat.baseColorTexture], texCoord);
	outFragColor = albedo;//vec4(1.0f);
}