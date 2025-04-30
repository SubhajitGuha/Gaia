//#pragma once
#ifndef _DDGI_H_
#define _DDGI_H_

#include "GaiaRenderer.h"
#include "Renderer.h"

namespace Gaia
{
	class Scene;
	class DDGI
	{
		const glm::vec3 MAX_PROBES_PER_AXIS = glm::vec3(30.0f,30.0f,20.0f);
	public:
		DDGI(Renderer* renderer, Scene& scene);
		~DDGI();

		static std::unique_ptr<DDGI> create(Renderer* renderer, Scene& scene);
		void render(Scene& scene);
		void update(Scene& scene);
		//get the DDGI probe irradiance and depth images DSL
		inline DescriptorSetLayoutHandle getProbeDSL() { return probeReadDSL; }
		inline DescriptorSetLayoutHandle getDDGIParametersDSL() { return ddgiParametersDSL; }
		inline TextureHandle getProbeIrradianceImage() { return probeIrradianceImage; };
		inline TextureHandle getProbeDepthImage() { return probeDepthImage; };

	private:
		struct ProbeBounds
		{
			glm::vec3 min;
			glm::vec3 max;
		};
		struct PushConstant
		{
			glm::mat4 randomOrientation = glm::mat4(1.0);
			int isFirstFrame;
		};
		struct Probe
		{
			glm::vec3 probeDistance = glm::vec3(1.0f);
			float recursiveEnergyPreservation = 0.95f;
			uint32_t irradianceOctSize = 8;
			uint32_t depthOctSize = 16;
			float hysteresis = 0.99;
			float depthSharpness = 25.0f;
			float normalBias = 0.25f;
			float maxDistance = 4.0f;
			uint32_t raysPerProbe = 256;
			float irradianceDistanceBias = 0.0f;
			float irradianceVarianceBias = 0.02f;
			float irradianceChebyshevBias = 0.07f;
			glm::vec3 gridStartPosition;
			glm::ivec3 probeCounts;
		};

		ProbeBounds bounds;
		Renderer* renderer = nullptr;
		float giIntensity = 1.0;
		uint32_t width;
		uint32_t height;
		Probe probeData;
		int irradianceWidth = 1;
		int irradianceHeight = 1;

		int depthWidth = 1;
		int depthHeight = 1;
		std::random_device randomDevice;
		std::mt19937 randomGenerator;
		std::uniform_real_distribution<float> randomDistribution_zo;
		std::uniform_real_distribution<float> randomDistribution_no;


		//textures
		Holder<TextureHandle> rayTraceRadiance; //stores ray-traced color value
		Holder<TextureHandle> rayTraceDirectionDepth; //stores ray direction and hit distance

		Holder<TextureHandle> probeIrradianceImage;
		Holder<TextureHandle> probeDepthImage;

		//Holder<TextureHandle> giImage;

		//sampler
		Holder<SamplerHandle> bilinearSampler;

		//Uniform DDGI buffer
		Holder<BufferHandle> ddgiParametersBuffer;

		//shaders
		Holder<ShaderModuleHandle> rayGenShader;
		Holder<ShaderModuleHandle> missShader;
		Holder<ShaderModuleHandle> missShaderShadow;
		Holder<ShaderModuleHandle> closestHitShader;
		Holder<ShaderModuleHandle> probeIrradianceUpdateShader;
		Holder<ShaderModuleHandle> probeDepthUpdateShader;
		Holder<ShaderModuleHandle> probeIrradianceBorderUpdateShader;
		Holder<ShaderModuleHandle> probeDepthBorderUpdateShader;

		//pipelines
		Holder<RayTracingPipelineHandle> probeRtPipeline;
		Holder<ComputePipelineHandle> probeIrradianceUpdatePipeline;
		Holder<ComputePipelineHandle> probeDepthUpdatePipeline;

		Holder<ComputePipelineHandle> probeIrradianceBorderUpdatePipeline;
		Holder<ComputePipelineHandle> probeDepthBorderUpdatePipeline;

		//descriptor sets
		Holder<DescriptorSetLayoutHandle> rayTraceImagesDSL;
		Holder<DescriptorSetLayoutHandle> probeDSL; //contains storage textures
		Holder<DescriptorSetLayoutHandle> probeReadDSL; //contains read-only sampled images
		Holder<DescriptorSetLayoutHandle> ddgiParametersDSL;


	private:
		void createTextures();
		void createBuffers();
		void createPipelines();

		void updateTextures();
		void updateBuffers();

		void rayTrace();
		void updateProbes();
		void updateBorder();
	};

}
#endif // !_DDGI_H_L
