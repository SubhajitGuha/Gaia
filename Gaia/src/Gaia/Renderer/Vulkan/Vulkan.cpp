#include "pch.h"
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "Vulkan.h"
#include "gaia/Window.h"
#include "GLFW/glfw3.h"
#include "Gaia/Log.h"
#include "VkInitializers.h"
#include "VkUtils.h"
#include "backends/imgui_impl_vulkan.h"
#include "Gaia/LoadMesh.h"


namespace Gaia
{
	ref<Vulkan> Vulkan::m_vulkan_context;
	void Vulkan::Init(ref<Window> window)
	{
		//create a vulkan context
		m_vulkan_context = std::make_shared<Vulkan>();

		vkb::InstanceBuilder builder;
		auto inst_ret = builder.set_app_name("Gaia").
			enable_validation_layers(m_vulkan_context->b_enableValidationLayers).
			require_api_version(1, 3, 0).
			use_default_debug_messenger().
			build();

		if (!inst_ret)
		{
			std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
		}

		auto vkb_inst = inst_ret.value();
		m_vulkan_context->m_instance = vkb_inst.instance;
		m_vulkan_context->m_debug_messenger = vkb_inst.debug_messenger;

		//create the surface
		GLFWwindow* glfw_window = (GLFWwindow*)window->GetNativeWindow();
		auto res = glfwCreateWindowSurface(m_vulkan_context->m_instance, glfw_window, nullptr, &m_vulkan_context->m_surface);
		if (res != VK_SUCCESS)
		{
			GAIA_CORE_ERROR("failed to create window surface with error code: {}", res);
			exit(1);
		}

		//select physical device
		VkPhysicalDeviceVulkan13Features features;
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features.synchronization2 = true;
		features.dynamicRendering = true;

		VkPhysicalDeviceVulkan12Features features12;
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		std::vector<const char*> extensions{
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
		};

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto vkb_pd_res = selector.set_minimum_version(1, 3).
			//set_required_features_13(features).
			//set_required_features_12(features12).
			add_required_extensions(extensions).
			prefer_gpu_device_type(vkb::PreferredDeviceType::discrete).
			set_surface(m_vulkan_context->m_surface).
			select();

		if (!vkb_pd_res)
		{
			GAIA_CORE_ERROR("failed to get physical device error code: {}", vkb_pd_res.error().message());
			exit(1);
		}
		

		vkb::PhysicalDevice vkb_pd = vkb_pd_res.value();

		GAIA_CORE_TRACE("Physical device name: {}", vkb_pd.name);
		vkb::DeviceBuilder device_builder{ vkb_pd };
		auto dev_ret = device_builder.build();
		if (!dev_ret)
		{
			GAIA_CORE_ERROR("Failed to create vulkan device");
		}

		auto vkbDevice = dev_ret.value();
		m_vulkan_context->m_device = vkbDevice.device;
		m_vulkan_context->m_physical_device = vkbDevice.physical_device;
		m_vulkan_context->m_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_vulkan_context->m_queue_family_idx = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		//vma initilization
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.physicalDevice = m_vulkan_context->m_physical_device;
		allocatorCreateInfo.device = m_vulkan_context->m_device;
		allocatorCreateInfo.instance = m_vulkan_context->m_instance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		vmaCreateAllocator(&allocatorCreateInfo, &m_vulkan_context->m_allocator);
		
		m_vulkan_context->window_height = window->GetHeight();
		m_vulkan_context->window_width = window->GetWidth();

		m_vulkan_context->create_swapchain(window->GetWidth(), window->GetHeight());

		m_vulkan_context->m_renderer = std::make_shared<VkRenderer>(m_vulkan_context->window_width, m_vulkan_context->window_height);
	}
	void Vulkan::Destroy()
	{
		//wait for gpu to finish doing its things
		vkDeviceWaitIdle(m_vulkan_context->m_device);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			//destroy the command pool
			vkDestroyCommandPool(m_vulkan_context->m_device, m_vulkan_context->m_frames[i].m_commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(m_vulkan_context->m_device, m_vulkan_context->m_frames[i].m_renderFence, nullptr);
			vkDestroySemaphore(m_vulkan_context->m_device, m_vulkan_context->m_frames[i].m_renderSemaphore, nullptr);
			vkDestroySemaphore(m_vulkan_context->m_device, m_vulkan_context->m_frames[i].m_swapchainSemaphore, nullptr);
		}
		m_vulkan_context->destroy_swapchain();
		vkDestroySurfaceKHR(m_vulkan_context->m_instance, m_vulkan_context->m_surface, nullptr);
		vmaDestroyAllocator(m_vulkan_context->m_allocator);
		vkDestroyDevice(m_vulkan_context->m_device, nullptr);
		vkb::destroy_debug_utils_messenger(m_vulkan_context->m_instance, m_vulkan_context->m_debug_messenger, nullptr);
		vkDestroyInstance(m_vulkan_context->m_instance, nullptr);
	}
	void Vulkan::InitFrameCommandBuffers()
	{
		VkCommandPoolCreateInfo poolInfo = vkinit::command_pool_create_info(m_vulkan_context->m_queue_family_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		for(int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto res = vkCreateCommandPool(m_vulkan_context->m_device,&poolInfo, nullptr, &m_vulkan_context->m_frames[i].m_commandPool);
			GAIA_ASSERT(res == VK_SUCCESS, "failed to create command pool error code: {}", res);
			VkCommandBufferAllocateInfo allocInfo= vkinit::command_buffer_allocation_info(m_vulkan_context->m_frames[i].m_commandPool, 1);
			GAIA_ASSERT(vkAllocateCommandBuffers(m_vulkan_context->m_device, &allocInfo, &m_vulkan_context->m_frames[i].m_mainCommandBuffer) == VK_SUCCESS, "failed to Allocte command buffer");
		}

		//create syncronization structures
		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			GAIA_ASSERT(vkCreateFence(m_vulkan_context->m_device, &fenceCreateInfo, nullptr, &m_vulkan_context->m_frames[i].m_renderFence) == VK_SUCCESS, "Failed to create fence");

			GAIA_ASSERT(vkCreateSemaphore(m_vulkan_context->m_device, &semaphoreCreateInfo, nullptr, &m_vulkan_context->m_frames[i].m_swapchainSemaphore) == VK_SUCCESS, "Failed to create semaphore");
			GAIA_ASSERT(vkCreateSemaphore(m_vulkan_context->m_device, &semaphoreCreateInfo, nullptr, &m_vulkan_context->m_frames[i].m_renderSemaphore) == VK_SUCCESS, "Failed to create semaphore");
		}
	}
	void Vulkan::Render()
	{
		
		//wait until the gpu has finished rendering the last frame. Timeout of 1 second
		auto res = vkWaitForFences(m_device, 1, &GetCurrentFrame().m_renderFence, true, 1000000000);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to wait for fence, error code {}", res);
		uint32_t swapchainImageIndex;

		res = vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, GetCurrentFrame().m_swapchainSemaphore, nullptr, &swapchainImageIndex);
		//GAIA_ASSERT(res == VK_SUCCESS, "failed to aquire next Image for swapchain, error code {}", res);

		res = vkResetFences(m_device, 1, &GetCurrentFrame().m_renderFence);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to reset fence, error code {}", res);

		res = vkResetCommandBuffer(GetCurrentFrame().m_mainCommandBuffer, 0);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to reset command buffer, error code {}", res);

		VkCommandBuffer cmd = GetCurrentFrame().m_mainCommandBuffer;
		VkCommandBufferBeginInfo cmdBufferBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer(cmd, &cmdBufferBeginInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->pipeline);
		vkutil::transition_image(cmd, m_swapchain_images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkClearValue clearVal{};
		clearVal.color = VkClearColorValue{ 1.0,1.0,1.0,1.0 };
		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(m_swapchain_image_views[swapchainImageIndex], &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo{};
		renderInfo = vkinit::rendering_info(m_swapchain_extenet,&colorAttachment, nullptr);

		VkViewport viewport{};
		viewport.width = window_width;
		viewport.height = window_height;
		viewport.minDepth = 0.0;
		viewport.maxDepth = 1.0;

		VkRect2D scissor{};
		scissor.extent.width = window_width;
		scissor.extent.height = window_height;
		
		vkCmdBeginRendering(cmd, &renderInfo);
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->graphics_pipeline->m_pipelineLayout, 0, 1, &m_renderer->camera_desc_set, 0, nullptr);
		for (int i = 0; i < m_renderer->mesh->m_subMeshes.size(); i++)
		{
			VkDeviceSize size = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_renderer->mesh->m_subMeshes[i].vk_mesh_vertex_buffer.m_buffer, &size);
			vkCmdBindIndexBuffer(cmd, m_renderer->mesh->m_subMeshes[i].vk_mesh_index_buffer.m_buffer, size, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd, m_renderer->mesh->m_subMeshes[i].indices.size(), 1, 0, 0,0);
		}
		vkCmdEndRendering(cmd);
		render_imgui(cmd, m_swapchain_image_views[swapchainImageIndex]);
		vkutil::transition_image(cmd, m_swapchain_images[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		vkEndCommandBuffer(cmd);

		//submit command buffer to queue
		VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);
		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().m_swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().m_renderSemaphore);

		VkSubmitInfo2 submitInfo = vkinit::submit_info(&cmdSubmitInfo, &signalInfo, &waitInfo);
		res = vkQueueSubmit2(m_queue, 1, &submitInfo, GetCurrentFrame().m_renderFence);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to submit queue, error code {}", res);

		//present to screen
		VkPresentInfoKHR presentInfo{};
		presentInfo.pNext = nullptr;
		presentInfo.pImageIndices = &swapchainImageIndex;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pWaitSemaphores = &GetCurrentFrame().m_renderSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		vkQueuePresentKHR(m_queue, &presentInfo);

		frameNumber++;
	}
	void Vulkan::create_swapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ m_physical_device, m_device, m_surface };
		m_swapchain_image_format = VK_FORMAT_R8G8B8A8_SRGB;

		VkSurfaceFormatKHR surfaceFormat;
		surfaceFormat.format = m_swapchain_image_format;
		surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(surfaceFormat)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_swapchain_extenet = vkbSwapchain.extent;

		m_swapchain = vkbSwapchain.swapchain;
		m_swapchain_images = vkbSwapchain.get_images().value();
		m_swapchain_image_views = vkbSwapchain.get_image_views().value();
	}
	void Vulkan::destroy_swapchain()
	{
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

		//destroy swapchain image views
		for (int i = 0; i < m_swapchain_image_views.size(); i++)
		{
			vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
		}
	}
	void Vulkan::render_imgui(VkCommandBuffer cmd, VkImageView swapchainImageView)
	{
		VkClearValue clearVal{};
		clearVal.color = VkClearColorValue{ 0.0,1.0,1.0,1.0 };
		//clearVal.depthStencil

		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(swapchainImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderinginfo = vkinit::rendering_info(m_swapchain_extenet, &colorAttachment, nullptr);
		vkCmdBeginRendering(cmd, &renderinginfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		vkCmdEndRendering(cmd);
	}
}