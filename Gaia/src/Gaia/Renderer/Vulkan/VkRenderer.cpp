#include "pch.h"
#include "VkRenderer.h"
#include "VkInitializers.h"
#include "Vulkan.h"

Gaia::VkRenderer::VkRenderer()
{
	graphics_pipeline = std::make_shared<VkPipelineBuilder>();

	graphics_pipeline->disable_depthtest();
	graphics_pipeline->set_color_attachment_format(VK_FORMAT_R8G8B8A8_SRGB);
	graphics_pipeline->set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	graphics_pipeline->set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	graphics_pipeline->set_multisampling_none();
	graphics_pipeline->set_polygon_mode(VK_POLYGON_MODE_FILL);

	vkutil::load_shader_module("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/frag_shader.spv", Vulkan::GetVulkanContext()->m_device, &fragment_shader);
	vkutil::load_shader_module("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/vert_shader.spv", Vulkan::GetVulkanContext()->m_device, &vertex_shader);

	graphics_pipeline->set_shaders(vertex_shader,fragment_shader);
	graphics_pipeline->enable_blending_alphablend();

	VkPipelineLayoutCreateInfo info = vkinit::pipeline_layout_create_info();
	
	vkCreatePipelineLayout(Vulkan::GetVulkanContext()->m_device, &info, nullptr, &graphics_pipeline->m_pipelineLayout);

	pipeline = graphics_pipeline->CreateGraphicsPipeline(Vulkan::GetVulkanContext()->m_device);
}

void Gaia::VkRenderer::Destroy()
{
}
