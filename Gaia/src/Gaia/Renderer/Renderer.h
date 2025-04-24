#pragma once
#include "Gaia/Renderer/GaiaRenderer.h"
#include "glm/glm.hpp"

namespace Gaia
{
	class Scene;
	class Shadows;
	class DDGI;
	struct MVPMatrices
	{
		glm::mat4 view = glm::mat4(1.0);
		glm::mat4 projection = glm::mat4(1.0);
		glm::vec3 viewDir = { 0.0,1.0,0.0 };
		float pad_1 = 0.0;
		glm::vec3 camPos = { 0.0,0.0,0.0 };
		uint32_t frameNumber = 0;
	};

	struct MeshGPUBufferAddress
	{
		uint64_t vertexBufferAddress;
		uint64_t indexBufferAddress;
	};

	class Renderer
	{
		friend class Shadows;
		friend class DDGI;
	public:
		static VertexInput vertexInput;

	public:
		static std::shared_ptr<Renderer> create(void* window, Scene& scene);
		
		/// creates vertex and index buffers needed for rasterization
		void createStaticBuffers(Scene& scene); 
		void update(Scene& scene);// Todo Take a scene
		void windowResize(uint32_t width, uint32_t height);
		void render(Scene& scene);
		TextureHandle getRenderTarget() { return renderTarget_; }
		inline IContext* getContext() { return renderContext_.get(); }
		void onFirstFrame(Scene& scene);
		Renderer(void* window, Scene& scene);
		~Renderer();
	public:
		static bool isFirstFrame;
	private:
		std::unique_ptr<IContext> renderContext_;
		uint32_t numIndicesPerMesh;
		float rayTraceOutputScale = 1.0;
		std::pair<uint32_t, uint32_t> rayTraceOutput;

		//render the entire scene into this texture
		Holder<TextureHandle> renderTarget_;
		Holder<TextureHandle> rtOutputTexture; //storage texture that stores the ray-tracing output

		//this renders into GBuffer textures
		Holder<RenderPipelineHandle> renderPipelineGBuffer;
		//deferred pipeline that shades the pixels
		Holder<RenderPipelineHandle> renderPipelineDeferred;
		//Gi pass render pipeline
		Holder<RenderPipelineHandle> renderPipelineGI;


		Holder<RayTracingPipelineHandle> rtPipeline;
		//Holder<RayTracingPipelineHandle> fullScreenPipeline;

		std::vector<Holder<AccelStructHandle>> BLAS;
		Holder<AccelStructHandle> TLAS;
		Holder<BufferHandle> instancesBuffer; //its the tlas instance buffer
		Holder<BufferHandle> geometryBufferAddress; //buffer of vertex and index buffer gpu addresses

		Holder<ShaderModuleHandle> vertexShaderModule;
		Holder<ShaderModuleHandle> fragmentShaderModule;
		Holder<ShaderModuleHandle> rayGenShaderModule;
		Holder<ShaderModuleHandle> closestHitShaderModule;
		Holder<ShaderModuleHandle> anyHitShaderModule;
		Holder<ShaderModuleHandle> missShaderModule;
		Holder<ShaderModuleHandle> shadowMissShaderModule;

		Holder<BufferHandle> mvpBuffer;
		Holder<BufferHandle> mvpBufferStaging;
		Holder<BufferHandle> transformsBuffer;
		Holder<BufferHandle> lightParameterBuffer;
		Holder<BufferHandle> lightParameterBufferStaging;
		Holder<BufferHandle> lightMatricesBuffer;
		Holder<BufferHandle> lightMatricesBufferStaging;

		Holder<BufferHandle> vertexBuffer;
		Holder<BufferHandle> indexBuffer;
		Holder<BufferHandle> indexBufferRT; //need a seperate index buffer with out the offset applied for ray tracing

		//gbuffer data.
		Holder<TextureHandle> depthAttachment;
		Holder<TextureHandle> gBufferAlbedo;
		Holder<TextureHandle> gBufferNormal;
		Holder<TextureHandle> gBufferMetallicRoughness;
		Holder<TextureHandle> giTexture;

		std::vector<Holder<TextureHandle>> glTfTextures;

		Holder<BufferHandle> materialsBuffer;

		Holder<SamplerHandle> imageSampler;
		Holder<SamplerHandle> shadowImageSampler;


		Holder<DescriptorSetLayoutHandle> mvpMatrixDescriptorSetLayout;
		Holder<DescriptorSetLayoutHandle> meshDescriptorSet;
		Holder<DescriptorSetLayoutHandle> shadowDescSetLayout;
		Holder<DescriptorSetLayoutHandle> accelStructureDescSetLayout;
		Holder<DescriptorSetLayoutHandle> geometryBufferAddressDescSetLayout;
		Holder<DescriptorSetLayoutHandle> rayTraceImageDescSetLayout;
		Holder<DescriptorSetLayoutHandle> gBufferDescSetLayout;
		Holder<DescriptorSetLayoutHandle> giOutputDescSetLayout;

		MVPMatrices mvpData = {};

		//other components
		std::unique_ptr<Shadows> shadows_;
		std::unique_ptr<DDGI> ddgi_;
	private:
		void createGpuMeshTexturesAndBuffers(Scene& scene);
	};
}