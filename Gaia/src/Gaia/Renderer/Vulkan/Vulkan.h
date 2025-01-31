#pragma once
#include "VkBootstrap.h";
#include "VkBootstrapDispatch.h"
#include "Gaia/Core.h"
#include "VkRenderer.h"

namespace Gaia {
	class Window;
	struct FrameData {
		VkSemaphore m_swapchainSemaphore, m_renderSemaphore;
		VkFence m_renderFence;

		VkCommandPool m_commandPool;
		VkCommandBuffer m_mainCommandBuffer;
	};
	constexpr uint32_t FRAME_OVERLAP = 2;

	class Vulkan
	{
	public:
		//frame data
		int frameNumber = 0;
		FrameData m_frames[FRAME_OVERLAP];

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device;
		VkDevice m_device;
		VkQueue m_queue;
		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchain_image_format;
		uint32_t m_queue_family_idx;

		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		VkExtent2D m_swapchain_extenet;
		VkExtent2D m_drawExtent;

	public:
		static void Init(ref<Window> window);
		static void Destroy();
		static inline ref<Vulkan> GetVulkanContext() { return m_vulkan_context; }
		static void InitFrameCommandBuffers();
		FrameData& GetCurrentFrame() { return m_frames[frameNumber % FRAME_OVERLAP]; }
		//This function renders every thing from geometry to ui(in order)
		void Render();

	private:
		bool b_enableValidationLayers = true;
		static ref<Vulkan> m_vulkan_context;
		ref<VkRenderer> m_renderer;
		float window_width, window_height;
	private:
		void create_swapchain(uint32_t width, uint32_t height);
		void destroy_swapchain();
		void render_imgui(VkCommandBuffer cmd, VkImageView swapchainImageView);
	};
}