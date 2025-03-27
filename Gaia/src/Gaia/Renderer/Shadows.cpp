#include "pch.h"
#include "Shadows.h"
#include "Gaia/Scene/Scene.h"
#include "glm/gtx/transform.hpp"

//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace Gaia {
	float Shadows::lamda = 0.95;
	Shadows::Shadows(ShadowDescriptor desc, Renderer* renderer, Scene& scene)
		:shadowDesc_(desc), renderer_(renderer)
	{
		for (int i = 0; i < MAX_SHADOW_CASCADES; i++)
		{
			lightData_[i].numCascades = shadowDesc_.numCascades;
		}
		IContext* context = renderer_->getContext();
		//create shadow textures
		int index = 0;
		for (int i = shadowDesc_.numCascades - 1; i >=0; i--)
		{
			uint32_t width = shadowDesc_.maxShadowMapRes;// -i * shadowDesc_.maxShadowMapRes / shadowDesc_.numCascades;
			width = std::clamp(width, shadowDesc_.minShadowMapRes, shadowDesc_.maxShadowMapRes);
			uint32_t height = width;
			TextureDesc desc{
				.type = TextureType_2D,
				.format = Format_Z_F32,
				.dimensions = {width,height,1},
				.usage = TextureUsageBits_Attachment|TextureUsageBits_Sampled,
			};
			shadowmapResolutions_.push_back(width);
			shadowCascadeTextures_.push_back(context->createTexture(desc));
			index++;
		}
		createShadowMatrices(scene);

		BufferDesc lightDataBuffeDesc{
			.usage_type = BufferUsageBits_Uniform,
			.storage_type = StorageType_Device,
			.size = sizeof(LightData)
		};

		lightDataBuffer_ = context->createBuffer(lightDataBuffeDesc);

		lightDataBuffeDesc.storage_type = StorageType_HostVisible;
		lightDataBufferStaging_ = context->createBuffer(lightDataBuffeDesc);

		std::vector<DescriptorSetLayoutDesc> dslDesc = {
			DescriptorSetLayoutDesc{
				.binding = 0,
				.descriptorCount = 1,
				.descriptorType = DescriptorType_UniformBuffer,
				.shaderStage = Stage_Vert,
				.buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(lightDataBuffer_),
			},
		};
		shadowDescSetLayout_ = context->createDescriptorSetLayout(dslDesc);

		ShaderModuleDesc vertexShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/vert_shadow.spv", Stage_Vert);
		vertexShader_ = context->createShaderModule(vertexShaderDesc);

		ShaderModuleDesc frageShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/frag_shadow.spv", Stage_Frag);
		fragmentShader_ = context->createShaderModule(frageShaderDesc);

		RenderPipelineDesc rps{
			.vertexInput = renderer_->vertexInput,
			.smVertex = vertexShader_,
			.smFragment = fragmentShader_,
			.depthFormat = Format_Z_F32,
			.cullMode = CullMode_None,
			.windingMode = WindingMode_CCW,
			.polygonMode = PolygonMode_Fill,
		};
		rps.descriptorSetLayout[0] = renderer->mvpMatrixDescriptorSetLayout;
		rps.descriptorSetLayout[1] = renderer->meshDescriptorSet;
		rps.descriptorSetLayout[2] = shadowDescSetLayout_;
		
		shadowRenderPipeline_ = context->createRenderPipeline(rps);

		//change the layout of the depth textures
		ICommandBuffer& cmdBuffer = context->acquireCommandBuffer();
		for (int i = 0; i < shadowCascadeTextures_.size(); i++)
		{
			cmdBuffer.cmdTransitionImageLayout(shadowCascadeTextures_[i], ImageLayout_DEPTH_READ_ONLY_OPTIMAL);
		}
		context->submit(cmdBuffer);
	}

	Shadows::~Shadows()
	{
	}

	std::unique_ptr<Shadows> Shadows::create(ShadowDescriptor desc, Renderer* renderer, Scene& scene)
	{
		return std::make_unique<Shadows>(desc, renderer, scene);
	}

	void Shadows::update(Scene& scene)
	{
		//update the shadow matrices
		createShadowMatrices(scene);
	
	}

	void Shadows::render(Scene& scene)
	{
		IContext* context = renderer_->getContext();

		for (int k = 0; k < shadowDesc_.numCascades; k++)
		{
			ICommandBuffer& cmdBuffer = context->acquireCommandBuffer();
			ClearValue clearVal = {
			 .colorValue = {0.3,0.3,0.3,1.0},
			 .depthClearValue = 1.0,
			};
			cmdBuffer.cmdBindGraphicsPipeline(shadowRenderPipeline_);
			cmdBuffer.copyBuffer(lightDataBufferStaging_, &lightData_[k], sizeof(LightData));
			cmdBuffer.cmdCopyBufferToBuffer(lightDataBufferStaging_, lightDataBuffer_);

			cmdBuffer.cmdTransitionImageLayout(shadowCascadeTextures_[k], ImageLayout_DEPTH_ATTACHMENT_OPTIMAL);
			cmdBuffer.cmdBeginRendering(TextureHandle{}, shadowCascadeTextures_[k], &clearVal);
			auto size = context->getWindowSize();

			cmdBuffer.cmdSetViewport(Viewport{
				.width = (float)shadowmapResolutions_[k],
				.height = (float)shadowmapResolutions_[k],
				.minDepth = 0.0,
				.maxDepth = 1.0 });
			cmdBuffer.cmdSetScissor(Scissor{
				.width = shadowmapResolutions_[k],
				.height = shadowmapResolutions_[k],
				});
			cmdBuffer.cmdBindGraphicsDescriptorSets(0, shadowRenderPipeline_, { renderer_->mvpMatrixDescriptorSetLayout, renderer_->meshDescriptorSet, shadowDescSetLayout_ });
			//draw the batched mesh
			{
				cmdBuffer.cmdBindVertexBuffer(0, renderer_->vertexBuffer, 0);
				cmdBuffer.cmdBindIndexBuffer(renderer_->indexBuffer, IndexFormat_U32, 0);

				cmdBuffer.cmdDrawIndexed(renderer_->numIndicesPerMesh, 1, 0, 0, 0);
			}
			cmdBuffer.cmdEndRendering();
			cmdBuffer.cmdTransitionImageLayout(shadowCascadeTextures_[k], ImageLayout_DEPTH_READ_ONLY_OPTIMAL);
			context->submit(cmdBuffer);

		}
	}

	void Shadows::createShadowMatrices(Scene& scene)
	{
		EditorCamera& camera = scene.getMainCamera();
		//calculate the far ranges of cascades

		float cascadeSplits[MAX_SHADOW_CASCADES];
		float nearClip = camera.GetPerspectiveNear();
		float farClip = camera.GetPerspectiveFar();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < shadowDesc_.numCascades; i++) {
			float p = (i + 1) / static_cast<float>(shadowDesc_.numCascades);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = lamda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}


		//convert the frustum to world-space
		float lastSplitDist = 0.0;
		for (int i = 0; i < shadowDesc_.numCascades; i++)
		{
			//clip-space frustum
			glm::vec3 frustum[] = {
				glm::vec3(-1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f,  1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};
			glm::mat4 frustumProjMat = camera.GetProjectionMatrix();//glm::perspective(glm::radians(camera.GetVerticalFOV()), camera.GetAspectRatio(), _near, lightData_[i].farRanges);
			float splitDist = cascadeSplits[i];
			glm::mat4 viewMat = camera.GetViewMatrix();

			glm::mat4 pv_inverse = glm::inverse(frustumProjMat * viewMat);
			for (int k = 0; k < 8; k++)
			{
				glm::vec4 wsPos = pv_inverse * glm::vec4(frustum[k], 1.0);
				wsPos = wsPos/wsPos.w;
				frustum[k] = wsPos;
			}

			for (uint32_t j = 0; j < 4; j++) {
				glm::vec3 dist = frustum[j + 4] - frustum[j];
				frustum[j + 4] = frustum[j] + (dist * splitDist);
				frustum[j] = frustum[j] + (dist * lastSplitDist);
			}

			glm::vec3 centroid = glm::vec3(0.0);
			for (uint32_t j = 0; j < 8; j++) {
				centroid += frustum[j];
			}
			centroid /= 8.0;

			float radius = 0.0f;
			for (uint32_t j = 0; j < 8; j++) {
				float distance = glm::length(frustum[j] - centroid);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = glm::normalize(scene.lightParameter.direction);
			glm::mat4 lightViewMat =  glm::lookAt(centroid - lightDir * -minExtents.z, centroid, { 0.0,1.0,0.0 });
			glm::mat4 orthoProjMat = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
			
			lightData_[i].farRanges = (camera.GetPerspectiveNear() + splitDist * clipRange) * -1.0;
			lightData_[i].lightView = lightViewMat;
			lightData_[i].lightProjection = orthoProjMat;

			lastSplitDist = cascadeSplits[i];
		}
	}

}
