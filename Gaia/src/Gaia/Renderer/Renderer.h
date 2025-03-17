#pragma once
#include "Gaia/Renderer/GaiaRenderer.h"
#include "glm/glm.hpp"

namespace Gaia
{
	class Scene;
	struct MVPMatrices
	{
		glm::mat4 model = glm::mat4(1.0);
		glm::mat4 view = glm::mat4(1.0);
		glm::mat4 projection = glm::mat4(1.0);
	};

	class Renderer
	{
	public:
		static std::shared_ptr<Renderer> create(void* window);
		
		/// creates vertex and index buffers needed for rasterization
		void createStaticBuffers(Scene& scene); 
		void update(Scene& scene);// Todo Take a scene
		void windowResize(uint32_t width, uint32_t height);
		void render(Scene& scene);
		explicit Renderer(void* window);
		~Renderer();
	private:
		std::unique_ptr<IContext> renderContext_;

		Holder<RenderPipelineHandle> renderPipeline;
		Holder<ShaderModuleHandle> vertexShaderModule;
		Holder<ShaderModuleHandle> fragmentShaderModule;
		Holder<DescriptorSetLayoutHandle> mvpMatrixDescriptorSetLayout;
		Holder<BufferHandle> mvpBuffer;
		Holder<BufferHandle> mvpBufferStaging;
		std::vector<Holder<BufferHandle>> vertexBuffer;
		std::vector<Holder<BufferHandle>> indexBuffer;
		Holder<TextureHandle> depthAttachment;

		MVPMatrices mvpData = {};
	};
}