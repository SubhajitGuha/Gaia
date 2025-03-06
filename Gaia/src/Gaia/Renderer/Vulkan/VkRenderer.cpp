#include "pch.h"
#include "VkRenderer.h"
#include "VkInitializers.h"
#include "Vulkan.h"
#include "Gaia/LoadMesh.h"
#include "Gaia/Renderer/Cameras/EditorCamera.h"


Gaia::VkRenderer::VkRenderer(float width, float height)
{
	mesh = std::make_shared<LoadMesh>("C:/Users/Subhajit/Downloads/cube.gltf");
	camera = std::make_shared<EditorCamera>(width,height);
	camera->SetCameraPosition({ 0.0,0.0,0.0 });
	graphics_pipeline = std::make_shared<VkPipelineBuilder>();

	graphics_pipeline->enable_depthtest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
	graphics_pipeline->set_color_attachment_format(VK_FORMAT_R8G8B8A8_SRGB);
	graphics_pipeline->set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	graphics_pipeline->set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	graphics_pipeline->set_multisampling_none();
	graphics_pipeline->set_polygon_mode(VK_POLYGON_MODE_FILL);

	vkutil::load_shader_module("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/frag_shader.spv", Vulkan::GetVulkanContext()->m_device, &fragment_shader);
	vkutil::load_shader_module("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/vert_shader.spv", Vulkan::GetVulkanContext()->m_device, &vertex_shader);

	graphics_pipeline->set_shaders(vertex_shader,fragment_shader);
	graphics_pipeline->enable_blending_alphablend();

	//create camera buffer
	camera_buffer.CreateBuffer(Vulkan::GetVulkanContext()->m_device, Vulkan::GetVulkanContext()->m_allocator, sizeof(CameraBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto camera_gpu_buffer = CameraBuffer({ mesh->GlobalTransform, camera->GetProjectionMatrix(), camera->GetViewMatrix() });
	camera_buffer.CopyBufferCPUToGPU(Vulkan::GetVulkanContext()->m_allocator, &camera_gpu_buffer.model_matrix, sizeof(CameraBuffer));

	//camera descriptor sets
	desc_layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	camera_desc_layout = desc_layout_builder.build(Vulkan::GetVulkanContext()->m_device, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);

	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f} };
	desc_allocator.init_pool(Vulkan::GetVulkanContext()->m_device, 10, sizes);
	camera_desc_set = desc_allocator.allocate(Vulkan::GetVulkanContext()->m_device, camera_desc_layout);

	desc_writer.write_buffer(0, camera_buffer.m_buffer, sizeof(CameraBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	desc_writer.update_set(Vulkan::GetVulkanContext()->m_device, camera_desc_set);

	graphics_pipeline->set_vertex_input_binding(mesh->m_vertexBindingDesc , mesh->m_vertexAttribDesc);
	VkPipelineLayoutCreateInfo info = vkinit::pipeline_layout_create_info();
	info.setLayoutCount = 1;
	info.pSetLayouts = &camera_desc_layout;

	vkCreatePipelineLayout(Vulkan::GetVulkanContext()->m_device, &info, nullptr, &graphics_pipeline->m_pipelineLayout);

	pipeline = graphics_pipeline->CreateGraphicsPipeline(Vulkan::GetVulkanContext()->m_device);
}

void Gaia::VkRenderer::OnEvent(Event& e)
{
	camera->OnEvent(e);
}

void Gaia::VkRenderer::OnUpdate(float ts)
{
	camera->OnUpdate(ts);

	//updte the gpu buffer
	auto camera_gpu_buffer = CameraBuffer({ mesh->GlobalTransform, camera->GetProjectionMatrix(), camera->GetViewMatrix() });
	camera_buffer.CopyBufferCPUToGPU(Vulkan::GetVulkanContext()->m_allocator, &camera_gpu_buffer.model_matrix, sizeof(CameraBuffer));
}

void Gaia::VkRenderer::Destroy()
{
}
