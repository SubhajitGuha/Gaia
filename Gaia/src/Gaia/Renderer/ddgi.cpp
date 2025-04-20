#include "pch.h"
#include "ddgi.h"
#include "Gaia/Application.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Gaia/Scene/Scene.h"


//DDGI gpu uniform buffer
struct DDGIUniform
{
	glm::vec3 gridStartPos;
	float maxDistance;

	glm::vec3 gridStep;
	float depthSharpness;

	glm::ivec3 probeCounts;
	float hysteresis;

	float normalBias;
	float energyPreservation;
	int irradianceProbeSideLength;
	int irradianceTextureWidth;

	int irradianceTextureHeight;
	int depthProbeSideLength;
	int depthTextureWidth;
	int depthTextureHeight;
};

namespace Gaia
{
	DDGI::DDGI(Renderer* renderer, Scene& scene)
	{
		bounds.min = scene.getSceneBounds().min;
		bounds.max = scene.getSceneBounds().max;

		this->renderer = renderer;
		auto dimension = renderer->getContext()->getWindowSize();
		width = dimension.first;
		height = dimension.second;

		glm::vec3 extent = (bounds.max - bounds.min);
		// Compute the number of probes along each axis.
		// Add 2 more probes to fully cover scene.
		probeData.probeCounts = glm::ivec3(extent / probeData.probeDistance) + glm::ivec3(2);
		probeData.gridStartPosition = bounds.min;
		probeData.maxDistance = probeData.probeDistance * 1.5f;

		randomGenerator = std::mt19937(randomDevice());
		randomDistribution_no = std::uniform_real_distribution<float>(-1.0, 1.0);
		randomDistribution_zo = std::uniform_real_distribution<float>(0.0, 1.0);

		createTextures();
		createBuffers();
		createPipelines();
	}
	DDGI::~DDGI()
	{
	}
	std::unique_ptr<DDGI> DDGI::create(Renderer* renderer,Scene& scene)
	{
		std::unique_ptr<DDGI> ptr = std::make_unique<DDGI>(renderer, scene);
		return ptr;
	}
	void DDGI::render(Scene& scene)
	{
		rayTrace();
		updateProbes();
		updateBorder();
	}
	void DDGI::update(Scene& scene)
	{
	}
	void DDGI::createTextures()
	{
		uint32_t totalProbes = probeData.probeCounts.x * probeData.probeCounts.y * probeData.probeCounts.z;
		IContext* context = renderer->getContext();

		SamplerStateDesc ssdesc{
			.minFilter = SamplerFilter_Linear,
			.magFilter = SamplerFilter_Linear,
			.wrapU = SamplerWrap_ClampBorder,
			.wrapV = SamplerWrap_ClampBorder,
			.wrapW = SamplerWrap_ClampBorder,
			.mipLodMin = 0,
			.mipLodMax = 0,
			.maxAnisotropic = 1,
		};
		bilinearSampler = context->createSampler(ssdesc);

		TextureDesc rtTexDesc{
			.type = TextureType_2D,
			.format = Format_RGBA_F16,
			.dimensions = {.width = probeData.raysPerProbe, .height = totalProbes, .depth = 1},
			.usage = TextureUsageBits_Storage | TextureUsageBits_Sampled,
			.storage = StorageType_Device,
		};
		rayTraceRadiance = context->createTexture(rtTexDesc);
		rayTraceDirectionDepth = context->createTexture(rtTexDesc);

		//store probes as octahedrons
		 irradianceWidth = (probeData.irradianceOctSize + 2) * probeData.probeCounts.x * probeData.probeCounts.y + 2;
		 irradianceHeight = (probeData.irradianceOctSize + 2) * probeData.probeCounts.z + 2;

		 depthWidth = (probeData.depthOctSize + 2) * probeData.probeCounts.x * probeData.probeCounts.y + 2;
		 depthHeight = (probeData.depthOctSize + 2) * probeData.probeCounts.z + 2;

		TextureDesc probeTexDesc{
			.type = TextureType_2D,
			.format = Format_RGBA_F16,
			.dimensions = {.width = static_cast<uint32_t>(irradianceWidth), .height = static_cast<uint32_t>(irradianceHeight), .depth = 1},
			.usage = TextureUsageBits_Storage | TextureUsageBits_Sampled,
			.storage = StorageType_Device,
		};
		probeIrradianceImage = context->createTexture(probeTexDesc);
		probeTexDesc.dimensions = { static_cast<uint32_t>(depthWidth), static_cast<uint32_t>(depthHeight), 1 };
		probeDepthImage = context->createTexture(probeTexDesc);

		
		//transition the image layout
		ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
		cmdBuf.cmdTransitionImageLayout(rayTraceRadiance, ImageLayout_GENERAL);
		cmdBuf.cmdTransitionImageLayout(rayTraceDirectionDepth, ImageLayout_GENERAL);
		cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_GENERAL);
		cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_GENERAL);
		context->submit(cmdBuf);
	}
	void DDGI::createBuffers()
	{
		IContext* context = renderer->getContext();
		DDGIUniform ubo;

		ubo.gridStartPos = probeData.gridStartPosition;
		ubo.maxDistance = probeData.maxDistance;
		ubo.gridStep = glm::vec3(probeData.probeDistance);
		ubo.depthSharpness = probeData.depthSharpness;
		ubo.probeCounts = probeData.probeCounts;
		ubo.hysteresis = probeData.hysteresis;
		ubo.normalBias = probeData.normalBias;
		ubo.energyPreservation = probeData.recursiveEnergyPreservation;
		ubo.irradianceProbeSideLength = probeData.irradianceOctSize;
		ubo.irradianceTextureWidth = irradianceWidth;
		ubo.irradianceTextureHeight = irradianceHeight;
		ubo.depthProbeSideLength = probeData.depthOctSize;
		ubo.depthTextureWidth = depthWidth;
		ubo.depthTextureHeight = depthHeight;

		BufferDesc uboBufdesc{
			.usage_type = BufferUsageBits_Uniform,
			.storage_type = StorageType_Device,
			.size = sizeof(DDGIUniform)
		};

		ddgiParametersBuffer = context->createBuffer(uboBufdesc);
		uboBufdesc.storage_type = StorageType_HostVisible;
		Holder<BufferHandle> stagingBuffer = context->createBuffer(uboBufdesc);

		//copy the buffers
		ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
		cmdBuf.copyBuffer(stagingBuffer, &ubo, sizeof(DDGIUniform));
		cmdBuf.cmdCopyBufferToBuffer(stagingBuffer, ddgiParametersBuffer);
		context->submit(cmdBuf);
	}
	void DDGI::createPipelines()
	{
		IContext* context = renderer->getContext();

		std::vector<DescriptorSetLayoutDesc> rtImagesDSLDesc
		{
			DescriptorSetLayoutDesc{
				.binding = 0,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_StorageImage,
				.shaderStage = Stage_RayGen | Stage_Com,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(rayTraceRadiance),
			},
			DescriptorSetLayoutDesc{
				.binding = 1,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_StorageImage,
				.shaderStage = Stage_RayGen | Stage_Com,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(rayTraceDirectionDepth),
			}
		};
		rayTraceImagesDSL = context->createDescriptorSetLayout(rtImagesDSLDesc);

		std::vector<DescriptorSetLayoutDesc> ddgiParamsDSLDesc
		{
			DescriptorSetLayoutDesc{
				.binding = 0,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_UniformBuffer,
				.shaderStage = Stage_RayGen | Stage_Miss | Stage_ClosestHit | Stage_Com | Stage_Frag,
				.buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(ddgiParametersBuffer),
			},
		};
		ddgiParametersDSL = context->createDescriptorSetLayout(ddgiParamsDSLDesc);

		//transition to a proper image layout
		{
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_GENERAL);
			cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_GENERAL);
			context->submit(cmdBuf);
		}
		std::vector<DescriptorSetLayoutDesc> probeDSLDesc
		{
			DescriptorSetLayoutDesc{
				.binding = 0,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_StorageImage,
				.shaderStage = Stage_RayGen | Stage_ClosestHit | Stage_Com,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(probeIrradianceImage),
			},
			DescriptorSetLayoutDesc{
				.binding = 1,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_StorageImage,
				.shaderStage = Stage_RayGen | Stage_ClosestHit | Stage_Com,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(probeDepthImage),
			},
		};
		probeDSL = context->createDescriptorSetLayout(probeDSLDesc);
		
		//transition to shader read only
		{
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
			cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
			context->submit(cmdBuf);
		}

		//create a descriptor set that contains the read only probe irradiance and depth textures.
		std::vector<DescriptorSetLayoutDesc> probeReadDSLDesc
		{
			DescriptorSetLayoutDesc{
				.binding = 0,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_CombinedImageSampler,
				.shaderStage = Stage_ClosestHit | Stage_Frag,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(probeIrradianceImage),
				.sampler = bilinearSampler,
			},
			DescriptorSetLayoutDesc{
				.binding = 1,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_CombinedImageSampler,
				.shaderStage = Stage_ClosestHit | Stage_Frag,
				.texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(probeDepthImage),
				.sampler = bilinearSampler,
			},
		};
		probeReadDSL = context->createDescriptorSetLayout(probeReadDSLDesc);


		ShaderModuleDesc rayGenSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_rayGen.rgen.spv", Stage_RayGen);
		rayGenSM.pushConstantSize = sizeof(PushConstant);
		ShaderModuleDesc missSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_rayMiss.rmiss.spv", Stage_Miss);
		ShaderModuleDesc missShadowSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_rayMissShadow.rmiss.spv", Stage_Miss);
		ShaderModuleDesc closestHitSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_closestHit.rchit.spv", Stage_ClosestHit);
		closestHitSM.pushConstantSize = sizeof(PushConstant);

		rayGenShader = context->createShaderModule(rayGenSM);
		missShader = context->createShaderModule(missSM);
		missShaderShadow = context->createShaderModule(missShadowSM);
		closestHitShader = context->createShaderModule(closestHitSM);

		RayTracingPipelineDesc rtPipelineDesc
		{
			.smRayGen = rayGenShader,
			.smClosestHit = closestHitShader,
		};
		rtPipelineDesc.smMiss[0] = missShader;
		rtPipelineDesc.smMiss[1] = missShaderShadow;

		rtPipelineDesc.descriptorSetLayout[0] = renderer->accelStructureDescSetLayout;
		rtPipelineDesc.descriptorSetLayout[1] = renderer->geometryBufferAddressDescSetLayout;
		rtPipelineDesc.descriptorSetLayout[2] = renderer->mvpMatrixDescriptorSetLayout;
		rtPipelineDesc.descriptorSetLayout[3] = renderer->meshDescriptorSet;
		rtPipelineDesc.descriptorSetLayout[4] = rayTraceImagesDSL;
		rtPipelineDesc.descriptorSetLayout[5] = ddgiParametersDSL;
		rtPipelineDesc.descriptorSetLayout[6] = probeReadDSL;

		probeRtPipeline = context->createRayTracingPipeline(rtPipelineDesc);

		ShaderModuleDesc irrProbeUpdateSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_irradiance_probe_update.comp.spv", Stage_Com);
		irrProbeUpdateSM.pushConstantSize = sizeof(PushConstant);
		
		ShaderModuleDesc depthProbeUpdateSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_depth_probe_update.comp.spv", Stage_Com);
		depthProbeUpdateSM.pushConstantSize = sizeof(PushConstant);

		probeIrradianceUpdateShader = context->createShaderModule(irrProbeUpdateSM);
		probeDepthUpdateShader = context->createShaderModule(depthProbeUpdateSM);

		ComputePipelineDesc probeUpdatePipelineDesc{
			.smComp = probeIrradianceUpdateShader
		};
		probeUpdatePipelineDesc.descriptorSetLayout[0] = rayTraceImagesDSL;
		probeUpdatePipelineDesc.descriptorSetLayout[1] = probeDSL;
		probeUpdatePipelineDesc.descriptorSetLayout[2] = ddgiParametersDSL;

		probeIrradianceUpdatePipeline = context->createComputePipeline(probeUpdatePipelineDesc);

		probeUpdatePipelineDesc.smComp = probeDepthUpdateShader;

		probeDepthUpdatePipeline = context->createComputePipeline(probeUpdatePipelineDesc);


		ShaderModuleDesc irrProbBorderUpdateSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_irradiance_border_update.comp.spv", Stage_Com);
		ShaderModuleDesc depthProbeBorderUpdateSM = ShaderModuleDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/gi_depth_border_update.comp.spv", Stage_Com);
		
		probeIrradianceBorderUpdateShader = context->createShaderModule(irrProbBorderUpdateSM);
		probeDepthBorderUpdateShader = context->createShaderModule(depthProbeBorderUpdateSM);

		ComputePipelineDesc probeBorderUpdatePipelineDesc{
			.smComp = probeIrradianceBorderUpdateShader
		};
		probeBorderUpdatePipelineDesc.descriptorSetLayout[0] = probeDSL;

		probeIrradianceBorderUpdatePipeline = context->createComputePipeline(probeBorderUpdatePipelineDesc);
		probeBorderUpdatePipelineDesc.smComp = probeDepthBorderUpdateShader;
		probeDepthBorderUpdatePipeline = context->createComputePipeline(probeBorderUpdatePipelineDesc);
	}
	void DDGI::rayTrace()
	{
		uint32_t totalProbes = probeData.probeCounts.x * probeData.probeCounts.y * probeData.probeCounts.z;

		PushConstant pc;
		pc.randomOrientation = glm::mat4_cast(glm::angleAxis(randomDistribution_zo(randomGenerator) * 2.0f * glm::pi<float>(), glm::vec3(randomDistribution_no(randomGenerator), randomDistribution_no(randomGenerator), randomDistribution_no(randomGenerator))));
		pc.isFirstFrame = Renderer::isFirstFrame ? 1 : 0;

		IContext* context = renderer->getContext();
		ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
		cmdBuf.cmdTransitionImageLayout(rayTraceRadiance, ImageLayout_GENERAL);
		cmdBuf.cmdTransitionImageLayout(rayTraceDirectionDepth, ImageLayout_GENERAL);

		cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
		cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
		
		cmdBuf.cmdPushConstants(probeRtPipeline, Stage_ClosestHit | Stage_RayGen, &pc, sizeof(PushConstant));
		cmdBuf.cmdBindRayTracingDescriptorSets(0, probeRtPipeline, {
			renderer->accelStructureDescSetLayout ,
			renderer->geometryBufferAddressDescSetLayout,
			renderer->mvpMatrixDescriptorSetLayout ,
			renderer->meshDescriptorSet ,
			rayTraceImagesDSL,
			ddgiParametersDSL,
			probeReadDSL,
			});
		cmdBuf.cmdBindRayTracingPipeline(probeRtPipeline);
		cmdBuf.cmdTraceRays(probeData.raysPerProbe, totalProbes, 1, probeRtPipeline);
		context->submit(cmdBuf);
	}
	void DDGI::updateProbes()
	{
		PushConstant pc;
		pc.isFirstFrame = Renderer::isFirstFrame ? 1 : 0;

		//update depth
		{
			IContext* context = renderer->getContext();
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdBindComputeDescriptorSets(0, probeDepthUpdatePipeline, { rayTraceImagesDSL , probeDSL, ddgiParametersDSL});
			cmdBuf.cmdBindComputePipeline(probeDepthUpdatePipeline);
			cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_GENERAL);
			cmdBuf.cmdPushConstants(probeDepthUpdatePipeline, Stage_Com, &pc, sizeof(PushConstant));
			const uint32_t dispatch_x = static_cast<uint32_t>(probeData.probeCounts.x * probeData.probeCounts.y);
			const uint32_t dispatch_y = static_cast<uint32_t>(probeData.probeCounts.z);
			cmdBuf.cmdDispatch(dispatch_x, dispatch_y, 1);
			context->submit(cmdBuf);
		}

		//update irradiance
		{
			IContext* context = renderer->getContext();
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdBindComputeDescriptorSets(0, probeIrradianceUpdatePipeline, { rayTraceImagesDSL , probeDSL, ddgiParametersDSL });
			cmdBuf.cmdBindComputePipeline(probeIrradianceUpdatePipeline);
			cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_GENERAL);
			cmdBuf.cmdPushConstants(probeDepthUpdatePipeline, Stage_Com, &pc, sizeof(PushConstant));
			const uint32_t dispatch_x = static_cast<uint32_t>(probeData.probeCounts.x * probeData.probeCounts.y);
			const uint32_t dispatch_y = static_cast<uint32_t>(probeData.probeCounts.z);
			cmdBuf.cmdDispatch(dispatch_x, dispatch_y, 1);
			context->submit(cmdBuf);
		}
	}
	void DDGI::updateBorder()
	{
		//update depth
		{
			IContext* context = renderer->getContext();
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdBindComputeDescriptorSets(0, probeDepthBorderUpdatePipeline, { probeDSL});
			cmdBuf.cmdBindComputePipeline(probeDepthBorderUpdatePipeline);
			cmdBuf.cmdTransitionImageLayout(probeDepthImage, ImageLayout_GENERAL);
			const uint32_t dispatch_x = static_cast<uint32_t>(probeData.probeCounts.x * probeData.probeCounts.y);
			const uint32_t dispatch_y = static_cast<uint32_t>(probeData.probeCounts.z);
			cmdBuf.cmdDispatch(dispatch_x, dispatch_y, 1);
			context->submit(cmdBuf);
		}

		//update irradiance
		{
			IContext* context = renderer->getContext();
			ICommandBuffer& cmdBuf = context->acquireCommandBuffer();
			cmdBuf.cmdBindComputeDescriptorSets(0, probeIrradianceBorderUpdatePipeline, { probeDSL});
			cmdBuf.cmdBindComputePipeline(probeIrradianceBorderUpdatePipeline);
			cmdBuf.cmdTransitionImageLayout(probeIrradianceImage, ImageLayout_GENERAL);
			const uint32_t dispatch_x = static_cast<uint32_t>(probeData.probeCounts.x * probeData.probeCounts.y);
			const uint32_t dispatch_y = static_cast<uint32_t>(probeData.probeCounts.z);
			cmdBuf.cmdDispatch(dispatch_x, dispatch_y, 1);
			context->submit(cmdBuf);
		}
		
	}
}
