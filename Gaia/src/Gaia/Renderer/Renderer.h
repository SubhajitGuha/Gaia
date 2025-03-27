#pragma once
#include "Gaia/Renderer/GaiaRenderer.h"
#include "glm/glm.hpp"

namespace Gaia
{
	class Scene;
	class Shadows;
	struct MVPMatrices
	{
		glm::mat4 view = glm::mat4(1.0);
		glm::mat4 projection = glm::mat4(1.0);
		glm::vec3 viewDir = { 0.0,1.0,0.0 };
		float pad_1 = 0.0;
		glm::vec3 camPos = { 0.0,0.0,0.0 };
		float pad_2 = 0.0;
	};

	class Renderer
	{
		friend class Shadows;
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
		explicit Renderer(void* window, Scene& scene);
		~Renderer();
	private:

		std::unique_ptr<IContext> renderContext_;
		uint32_t numIndicesPerMesh;

		//render the entire scene into this texture
		Holder<TextureHandle> renderTarget_;

		Holder<RenderPipelineHandle> renderPipeline;
		Holder<ShaderModuleHandle> vertexShaderModule;
		Holder<ShaderModuleHandle> fragmentShaderModule;
		Holder<BufferHandle> mvpBuffer;
		Holder<BufferHandle> mvpBufferStaging;
		Holder<BufferHandle> transformsBuffer;
		Holder<BufferHandle> lightParameterBuffer;
		Holder<BufferHandle> lightParameterBufferStaging;
		Holder<BufferHandle> lightMatricesBuffer;
		Holder<BufferHandle> lightMatricesBufferStaging;

		Holder<BufferHandle> vertexBuffer;
		Holder<BufferHandle> indexBuffer;
		Holder<TextureHandle> depthAttachment;
		std::vector<Holder<TextureHandle>> glTfTextures;
		Holder<BufferHandle> materialsBuffer;
		Holder<SamplerHandle> imageSampler;
		Holder<SamplerHandle> shadowImageSampler;


		Holder<DescriptorSetLayoutHandle> mvpMatrixDescriptorSetLayout;
		Holder<DescriptorSetLayoutHandle> meshDescriptorSet;
		Holder<DescriptorSetLayoutHandle> shadowDescSetLayout;
		
		MVPMatrices mvpData = {};

		//other components
		std::unique_ptr<Shadows> shadows_;
	private:
		void createGpuMeshTexturesAndBuffers(Scene& scene);
	};
}