#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "GaiaRenderer.h"
#include "Renderer.h"
#include "glm/glm.hpp"

#define MAX_SHADOW_CASCADES 8

namespace Gaia {
	class Scene;
	struct ShadowDescriptor {
		uint32_t numCascades = 4;
		uint32_t maxShadowMapRes = 4096;
		uint32_t minShadowMapRes = 128;
	};
	struct LightData
	{
		glm::mat4 lightView = glm::mat4(1.0);
		glm::mat4 lightProjection = glm::mat4(1.0);
		float farRanges = 1.0;
		int numCascades = 1;
		int pad_1 = 0;
		int pad_2 = 0;
	};

class Shadows
{
public:
	explicit Shadows(ShadowDescriptor desc, Renderer* renderer, Scene& scene);
	~Shadows();
	Shadows(Shadows&& other)
	{
		//do stuff
	}
	Shadows operator=(Shadows& other) = delete;
	Shadows operator=(Shadows&& other)
	{
		shadowDesc_ = other.shadowDesc_;
		renderer_ = other.renderer_;

		other.renderer_ = nullptr;
		other.shadowDesc_ = {};
	}
	static std::unique_ptr<Shadows> create(ShadowDescriptor desc, Renderer* renderer, Scene& scene);
	void update(Scene& scene);
	void render(Scene& scene);

public:
	static float lamda;
	std::vector<Holder<TextureHandle>> shadowCascadeTextures_;
	Holder<BufferHandle> lightDataBuffer_;
	LightData lightData_[MAX_SHADOW_CASCADES] = {};
private:
	ShadowDescriptor shadowDesc_{};
	Renderer* renderer_ = nullptr;//to access the context and other stuff

	Holder<RenderPipelineHandle> shadowRenderPipeline_;
	Holder<ShaderModuleHandle> vertexShader_;
	Holder<ShaderModuleHandle> fragmentShader_;
	Holder<DescriptorSetLayoutHandle> shadowDescSetLayout_;
	Holder<BufferHandle> lightDataBufferStaging_;

	std::vector<uint32_t> shadowmapResolutions_;
private:
	void createShadowMatrices(Scene& scene);
};
}

