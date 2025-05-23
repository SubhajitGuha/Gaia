#include "pch.h"
#include "Gaia/Application.h"
#include"ImGuiLayer.h"
#define VK_NO_PROTOTYPES
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"
#include "imgui_internal.h"
#include "Gaia/Renderer/Vulkan/VulkanClasses.h"
#include "Gaia/Log.h"
#include "Gaia/Core.h"

//#include "ImGuizmo.h"
#include "GLFW/glfw3.h"

#define BIND_FUNC(x) std::bind(&ImGuiLayer::x,this,std::placeholders::_1)

namespace Gaia {
	ImFont* ImGuiLayer::Font;
	VkDescriptorPool ImGuiPool;

	std::shared_ptr<ImGuiLayer> ImGuiLayer::create(IContext* context)
	{
		return std::make_shared<ImGuiLayer>(context);
	}

	ImGuiLayer::ImGuiLayer(IContext* context)
		:Layer("ImGuiLayer")
	{
		Window& window = Application::Get().GetWindow();
		context_ = context;
		TextureDesc desc{
			.format = Format_RGBA_SRGB8,
			.dimensions = {window.GetWidth(),window.GetHeight(),1},
			.usage = TextureUsageBits_Attachment,
		};
		renderTarget_ = context->createTexture(desc);
		Font = nullptr;
	}
	ImGuiLayer::~ImGuiLayer()
	{

	}
	void ImGuiLayer::OnAttach()
	{
		
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		/*io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Font/OpenSans-Bold.ttf", 18.0f);
		Font = io.Fonts->AddFontFromFileTTF("Assets/Font/OpenSans-ExtraBold.ttf", 20.0f);*/


		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		
		SetDarkThemeColors();

		GLFWwindow* window =(GLFWwindow*) Application::Get().GetWindow().GetNativeWindow();
		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);

		VulkanContext* vulkan_ctx = (VulkanContext*)context_;

		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = (uint32_t)std::size(pool_sizes);
		poolInfo.pPoolSizes = pool_sizes;

		auto res = vkCreateDescriptorPool(vulkan_ctx->getDevice(), &poolInfo, nullptr, &ImGuiPool);
		if (res != VK_SUCCESS)
		{
			GAIA_CORE_ERROR("Failed to create Descriptor pool ERROR CODE: {}", res);
		}

		// this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo info = {};
		info.Instance = vulkan_ctx->getInstance();
		info.PhysicalDevice = vulkan_ctx->getPhysicalDevice();
		info.Device = vulkan_ctx->getDevice();
		info.Queue = vulkan_ctx->deviceQueues_.graphicsQueue;
		info.QueueFamily = vulkan_ctx->deviceQueues_.graphicsQueueFamilyIndex;
		info.DescriptorPool = ImGuiPool;
		info.MinImageCount = 3;
		info.ImageCount = 3;
		info.UseDynamicRendering = true;

		VkFormat format = context_ ? getVkFormatFromFormat(context_->getSwapChainTextureFormat()) : VK_FORMAT_R8G8B8A8_SRGB;
		info.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		VkInstance instance = vulkan_ctx->getInstance();
		ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
			return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
			}, &instance);
		ImGui_ImplVulkan_Init(&info);
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	void ImGuiLayer::OnDetach()
	{
		VulkanContext* vulkan_ctx = (VulkanContext*)context_;
		//wait for gpu to compleate all its tasks
		GAIA_ASSERT(vkDeviceWaitIdle(vulkan_ctx->getDevice())==VK_SUCCESS , "");
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(vulkan_ctx->getDevice(), ImGuiPool, nullptr);
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnImGuiRender()
	{
		/*bool demo_window = false;
		ImGui::ShowDemoWindow(&demo_window);*/
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		e.m_Handeled |= e.IsInCategory(EventCategory::EventCategoryMouse) & io.WantCaptureMouse;
		e.m_Handeled |= e.IsInCategory(EventCategory::EventCategoryKeyboard) & io.WantCaptureKeyboard;		
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
		//ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight());

		ImGui::Render();
		TextureHandle swapchainTex = context_->getCurrentSwapChainTexture();
		ICommandBuffer& cmdBuffer = context_->acquireCommandBuffer();
		cmdBuffer.cmdTransitionImageLayout(renderTarget_, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
		cmdBuffer.cmdBeginRendering(renderTarget_, TextureHandle{}, nullptr);
		VkCommandBuffer buf = static_cast<VulkanCommandBuffer*>(&cmdBuffer)->getVkCommandBuffer();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buf);
		cmdBuffer.cmdEndRendering();
		cmdBuffer.cmdTransitionImageLayout(renderTarget_, ImageLayout_TRANSFER_SRC_OPTIMAL);
		cmdBuffer.cmdTransitionImageLayout(swapchainTex, ImageLayout_TRANSFER_DST_OPTIMAL);
		cmdBuffer.cmdBlitImage(renderTarget_, swapchainTex);
		context_->submit(cmdBuffer, swapchainTex);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			//GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			//glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = { 0.1,0.105,0.1,1.0f };

		colors[ImGuiCol_Header] = { 0.2,0.205,0.21,1.0f };
		colors[ImGuiCol_HeaderHovered] = { 0.3,0.305,0.3,1.0f };
		colors[ImGuiCol_HeaderActive] = { 0.1,0.105,0.1,1.0f };

		colors[ImGuiCol_Button] = { 0.2,0.205,0.21,1.0f };
		colors[ImGuiCol_ButtonHovered] = { 0.4,0.405,0.4,1.0f };
		colors[ImGuiCol_ButtonActive] = { 0.1,0.105,0.1,1.0f };

		colors[ImGuiCol_FrameBg] = { 0.2,0.205,0.21,1.0f };
		colors[ImGuiCol_FrameBgHovered] = { 0.3,0.305,0.3,1.0f };
		colors[ImGuiCol_FrameBgActive] = { 0.1,0.105,0.1,1.0f };

		colors[ImGuiCol_Tab] = { 0.25,0.2505,0.251,1.0f };
		colors[ImGuiCol_TabHovered] = { 0.78,0.7805,0.78,1.0f };
		colors[ImGuiCol_TabActive] = { 0.681,0.6805,0.681,1.0f };
		colors[ImGuiCol_TabUnfocused] = { 0.35,0.3505,0.351,1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = { 0.5,0.505,0.51,1.0f };

		colors[ImGuiCol_TitleBg] = { 0.15,0.1505,0.15,1.0f };
		colors[ImGuiCol_TitleBgActive] = { 0.15,0.1505,0.15,1.0f };
		colors[ImGuiCol_TitleBgCollapsed]= { 0.95,0.1505,0.951,1.0f };
	}


}