#pragma once
#include "GaiaRenderer.h"
#include "Vulkan/VulkanClasses.h"

namespace Gaia
{
	class Renderer
	{
	public:
		static std::shared_ptr<Renderer> create(void* window);
		void update();// Todo Take a scene
		void windowResize(uint32_t width, uint32_t height);
		void render();
		explicit Renderer(void* window);
		~Renderer();
	private:
		std::unique_ptr<IContext> renderContext_;

		VulkanImmediateCommands::CommandBufferWrapper wraper_ = {};
		Holder<RenderPipelineHandle> renderPipeline;
		Holder<ShaderModuleHandle> vertexShaderModule;
		Holder<ShaderModuleHandle> fragmentShaderModule;
		Holder<DescriptorSetLayoutHandle> descriptorSetlayout;
	};
}