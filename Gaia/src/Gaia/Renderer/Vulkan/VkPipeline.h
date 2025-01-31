#pragma once
#include "vulkan/vulkan.hpp"


namespace Gaia {
	class VkPipelineBuilder
	{
	public:

		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

		VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
		VkPipelineVertexInputStateCreateInfo m_vertexInput;
		VkPipelineRasterizationStateCreateInfo m_rasterizer;
		VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo m_multisampling;
		VkPipelineLayout m_pipelineLayout;
		VkPipelineDepthStencilStateCreateInfo m_depthStencil;
		VkPipelineRenderingCreateInfo m_renderInfo;
		VkFormat m_colorAttachmentformat;

		VkPipelineBuilder();
		~VkPipelineBuilder();
		
		VkPipeline CreateGraphicsPipeline(VkDevice device);

		void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader,
			const char* vertex_shader_entery = "main",
			const char* frag_shader_entery="main");
		void set_input_topology(VkPrimitiveTopology topology);
		void set_polygon_mode(VkPolygonMode mode);
		void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void set_multisampling_none();
		void disable_blending();
		void enable_blending_additive();
		void enable_blending_alphablend();
		void set_vertex_input_binding(std::vector<VkVertexInputBindingDescription>& vertBindingDesc, std::vector<VkVertexInputAttributeDescription>& vertexAttribDesc);

		void set_color_attachment_format(VkFormat format);
		void set_depth_format(VkFormat format);
		void disable_depthtest();
		void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
	};
	namespace vkutil {
		bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
	}
}
