#pragma once
#include "glm/glm.hpp"

namespace Gaia
{

struct Material
{
	glm::vec4 baseColorFactor = glm::vec4(1.0);
	int metallicFactor = 0.0;
	int roughnessFactor = 1.0;
	float normalStrength = 1.0;

	int baseColorTexture = -1;
	int metallicRoughnessTexture = -1;
	int normalTexture = -1;

	int _pad1 = 0;
	int _pad2 = 0;

};

struct Texture {
	int width = 1;
	int height = 1;
	int num_channels = 4;
	std::vector<uint8_t> textureData = {};
};
}

