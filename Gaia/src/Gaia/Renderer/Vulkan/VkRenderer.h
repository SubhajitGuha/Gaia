#pragma once
#include "VkPipeline.h"
#include "Gaia/Core.h"
#include "Gaia/Log.h"

namespace Gaia {
	class VkRenderer
	{
	public:
		VkRenderer();

		//void Render()
		void Destroy();
		VkPipeline pipeline;
	private:
		ref<VkPipelineBuilder> graphics_pipeline;

		VkShaderModule vertex_shader;
		VkShaderModule fragment_shader;
	};
}

