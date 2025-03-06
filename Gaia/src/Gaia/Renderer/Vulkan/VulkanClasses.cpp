#include "pch.h"
#include "VulkanClasses.h"
#include "GLFW/glfw3.h"
#include "VkUtils.h"

namespace Gaia {
	VkImageView VulkanImage::createimageView(VkDevice device, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseLevel, uint32_t numLevels, uint32_t baseLayer, uint32_t numLayers, const VkComponentMapping mapping, const VkSamplerYcbcrConversionInfo* ycbcr, const char* debugName) const
	{
		return VkImageView();
	}
	void VulkanImage::generateMipmap(VkCommandBuffer commadBuffer) const
	{
	}
	void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageSubresourceRange& subresourceRange) const
	{
	}
	VkImageAspectFlags VulkanImage::getImageAspectFlags() const
	{
		return VkImageAspectFlags();
	}
	bool VulkanImage::isDepthFormat(VkFormat format)
	{
		return false;
	}
	bool VulkanImage::isStencilFormat(VkFormat format)
	{
		return false;
	}
	VulkanImage CreateTexture(const TextureDesc& desc, const char* debugName)
	{
		return VulkanImage();
	}
	VulkanImage CreateTextureView(VulkanImage texture, const TextureViewDesc& desc, const char* debugName)
	{
		return VulkanImage();
	}
	VulkanContext::VulkanContext(void* window)
	{
		vkb::InstanceBuilder builder;
		vkb::Result instance_res = builder.set_app_name("Gaia").
			enable_validation_layers(enableValidationLayers_).
			require_api_version(1, 3, 0).
			use_default_debug_messenger().
			build();

		if (!instance_res)
			GAIA_CORE_ERROR("Failed to create Vulkan instance. Error: {}", instance_res.error().message());

		vkb::Instance vkb_instance = instance_res.value();
		vkInstance_ = vkb_instance.instance;

		//Create Surface
		GLFWwindow* glfwWindow = (GLFWwindow*)window;
		auto res = glfwCreateWindowSurface(vkInstance_, glfwWindow, nullptr, &vkSurface_);
		
		GAIA_ASSERT(res == VK_SUCCESS, "failed to create window surface with error code: {}", res)
		

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

		vkb::PhysicalDeviceSelector selector{ vkb_instance };
		vkb::Result vkb_pd_res = selector.set_minimum_version(1, 3).
			//set_required_features_13(features).
			//set_required_features_12(features12).
			add_required_extensions(extensions).
			prefer_gpu_device_type(vkb::PreferredDeviceType::discrete).
			set_surface(vkSurface_).
			select();

		GAIA_ASSERT(vkb_pd_res,"failed to get physical device error code: {}", vkb_pd_res.error().message());
		
		vkb::PhysicalDevice vkb_pd = vkb_pd_res.value();

		GAIA_CORE_TRACE("Physical device name: {}", vkb_pd.name);
		vkb::DeviceBuilder device_builder{ vkb_pd };
		auto dev_ret = device_builder.build();
		if (!dev_ret)
		{
			GAIA_CORE_ERROR("Failed to create vulkan device");
		}

		auto vkbDevice = dev_ret.value();
		vkDevice_ = vkbDevice.device;
		vkPhysicsalDevice_ = vkbDevice.physical_device;
		VkQueue vkGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		uint32_t graphicsQueueFamilyIdx = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		VkQueue vkComputeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
		uint32_t computeQueueFamilyIdx = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

		deviceQueues_ = DeviceQueues{ 
			.graphicsQueueFamilyIndex = graphicsQueueFamilyIdx,
			.computeQueueFamilyIndex = computeQueueFamilyIdx, 
			.graphicsQueue = vkGraphicsQueue, 
			.computeQueue = vkComputeQueue };

		//vma initilization
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.physicalDevice = vkPhysicsalDevice_;
		allocatorCreateInfo.device = vkDevice_;
		allocatorCreateInfo.instance = vkInstance_;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator_);

		//to do swapchain
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		swapchain_ = std::make_unique<VulkanSwapchain>(*this, width, height);
	}
	VulkanContext::~VulkanContext()
	{
	}
	Holder<BufferHandle> VulkanContext::createBuffer(BufferDesc& desc, const char* debugName)
	{
		return Holder<BufferHandle>();
	}
	Holder<TextureHandle> VulkanContext::createTexture(TextureDesc& desc, const char* debugName)
	{
		return Holder<TextureHandle>();
	}
	Holder<TextureHandle> VulkanContext::createTextureView(TextureViewDesc& desc, const char* debugName)
	{
		return Holder<TextureHandle>();
	}
	Holder<RenderPipelineHandle> VulkanContext::createRenderPipeline(RenderPipelineDesc& desc)
	{
		return Holder<RenderPipelineHandle>();
	}
	Holder<ShaderModuleHandle> VulkanContext::ctreateShaderModule(ShaderModuleDesc& desc)
	{
		return Holder<ShaderModuleHandle>();
	}
	void VulkanContext::destroy(BufferHandle handle)
	{
	}
	void VulkanContext::destroy(TextureHandle handle)
	{
	}
	void VulkanContext::destroy(RenderPipelineHandle handle)
	{
	}
	void VulkanContext::destroy(ShaderModuleHandle handle)
	{
	}
	TextureHandle VulkanContext::getCurrentSwapChainTexture()
	{
		return TextureHandle();
	}
	Format VulkanContext::getSwapChainTextureFormat() const
	{
		return Format();
	}
	ColorSpace VulkanContext::getSwapChainColorSpace() const
	{
		return ColorSpace();
	}
	uint32_t VulkanContext::getNumSwapchainImages() const
	{
		return 0;
	}
	void VulkanContext::recreateSwapchain(int newWidth, int newHeight)
	{
	}
	uint32_t VulkanContext::getFrameBufferMSAABitMask() const
	{
		return 0;
	}
	VkPipeline VulkanContext::getPipeline(RenderPipelineHandle handle)
	{
		return VkPipeline();
	}
	void VulkanContext::createInstance()
	{
	}
	void VulkanContext::createSurface()
	{
	}
	void VulkanBuffer::bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data)
	{
	}
	void VulkanBuffer::getBufferData(const VulkanContext& ctx, size_t offset, size_t size, void* data)
	{
	}
	VulkanPipelineBuilder::VulkanPipelineBuilder()
		:
		vertexInputState_(VkPipelineVertexInputStateCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr,
		}), 

		inputAssembly_(VkPipelineInputAssemblyStateCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			}), 

		rasterizationState_(VkPipelineRasterizationStateCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			}), 

		multisampleState_(VkPipelineMultisampleStateCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			}), 

		depthStencilState_(VkPipelineDepthStencilStateCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = VK_FALSE,
				.depthWriteEnable = VK_FALSE,
				.depthCompareOp = VK_COMPARE_OP_NEVER,
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = VK_FALSE,
				.front = {},
				.back = {},
				.minDepthBounds = 0.0,
				.maxDepthBounds = 1.0,
			}),

		tessellationState_(VkPipelineTessellationStateCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			})
	{
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicState(VkDynamicState state)
	{
		GAIA_ASSERT(numDynamicStates_ < MAX_DYNAMIC_STATES, "Cannot Have more than {} dynamic states", MAX_DYNAMIC_STATES);
		dynamicStates_[numDynamicStates_++] = state;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology)
	{
		inputAssembly_.topology = topology;
		inputAssembly_.primitiveRestartEnable = VK_FALSE;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples, float minSampleShading)
	{
		multisampleState_.rasterizationSamples = samples;
		multisampleState_.minSampleShading = minSampleShading;
		
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage)
	{
		shaderStages_[numShaderStages_++] = stage;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::cullMode(VkCullModeFlagBits mode)
	{
		rasterizationState_.cullMode = mode;

		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::frontFace(VkFrontFace mode)
	{
		rasterizationState_.frontFace = mode;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::polygonMode(VkPolygonMode mode)
	{
		rasterizationState_.polygonMode = mode;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
	{
		vertexInputState_ = state;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats, uint32_t numColorAttachments)
	{
		GAIA_ASSERT(numColorAttachments < MAX_COLOR_ATTACHMENTS, "number of color attachments exceeds the max color attachments of {}", MAX_COLOR_ATTACHMENTS);
		for (int i = 0; i < numColorAttachments; i++)
		{
			colorBlendAttachmentState_[i] = states[i];
			colorAttachmentFormat_[i] = formats[i];
			numColorAttachments_ = numColorAttachments;
		}
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
	{
		depthAttachmentFormat_ = format;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
	{
		stencilAttachmentFormat_ = format;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::patchControlPoints(uint32_t numPoints)
	{
		tessellationState_.patchControlPoints = numPoints;
	}
	VkResult VulkanPipelineBuilder::build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout, VkPipeline* outputPipeline, const char* debugName)
	{
		//viewport and scissor can be null as it is added as dynamic state.
		VkPipelineViewportStateCreateInfo vs_ci
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr,
		};

		VkPipelineDynamicStateCreateInfo ds_ci
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = numDynamicStates_,
			.pDynamicStates = dynamicStates_,
		};

		VkPipelineColorBlendStateCreateInfo cbs_ci
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = numColorAttachments_,
			.pAttachments = colorBlendAttachmentState_,
		};

		VkGraphicsPipelineCreateInfo gp_ci
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = numShaderStages_,
			.pStages = shaderStages_,
			.pVertexInputState = &vertexInputState_,
			.pInputAssemblyState = &inputAssembly_,
			.pTessellationState = &tessellationState_,
			.pViewportState = &vs_ci,
			.pRasterizationState = &rasterizationState_,
			.pMultisampleState = &multisampleState_,
			.pDepthStencilState = &depthStencilState_,
			.pColorBlendState = &cbs_ci,
			.pDynamicState = &ds_ci,
			.layout = pipelineLayout,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0,

		};
		VkResult res = vkCreateGraphicsPipelines(device, pipelineCache, 1, &gp_ci, nullptr, outputPipeline);

		numPipelineCreated_++;
		return res;
	}
	VulkanSwapchain::VulkanSwapchain(VulkanContext& ctx, uint32_t width, uint32_t height)
		: ctx_(ctx), width_(width), height_(height), vkDevice_(ctx.vkDevice_), vkGraphicsQueue_(ctx.deviceQueues_.graphicsQueue)
	{
		vkb::SwapchainBuilder swapchainBuilder(ctx.getPhysicalDevice(), ctx.getDevice(), ctx.vkSurface_);

		VkSurfaceFormatKHR surfaceFormat{ .format = SWAP_CHAIN_FORMAT, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		
		surfaceFormat_ = surfaceFormat;
		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(surfaceFormat)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		vkSwapchain_ = vkbSwapchain.swapchain;
		numSwapchainImages_ = vkbSwapchain.image_count;
		vkb::Result images_res = vkbSwapchain.get_images();

		GAIA_ASSERT(images_res, "Swapchain is empty error: {}", images_res.error().message());

		std::vector<VkImage> vkSwapchainImages = images_res.value();
		for (int i = 0; i < numSwapchainImages_; i++)
		{
			VulkanImage image = {
				.vkImage_ = vkSwapchainImages[i],
				.vkUsageFlags_ = vkbSwapchain.image_usage_flags,
				.vkExtent_ = VkExtent3D{.width = width, .height = height,.depth = 1},
				.vkType_ = VK_IMAGE_TYPE_2D,
				.vkImageFormat_ = vkbSwapchain.image_format,
				.isSwapchainImage_ = true,
				.isOwningVkImage_ = false,
				.isDepthFormat_ = false,
				.isStencilFormat_ = false,
			};

			image.imageView_ = image.createimageView(vkDevice_, VK_IMAGE_VIEW_TYPE_2D, vkbSwapchain.image_format,
				VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

			TextureHandle swapchainTexHandle = ctx_.texturesPool_.Create(std::move(image));
			swapchainTextures_[i] = swapchainTexHandle;

		}
			acquireSemaphore_ = vkutil::createSemaphore(vkDevice_);
	}
	VulkanSwapchain::~VulkanSwapchain()
	{
		for (auto texHandle : swapchainTextures_)
		{
			ctx_.destroy(texHandle); //need to implemet
		}
		vkDestroySwapchainKHR(vkDevice_, vkSwapchain_, nullptr);

		vkDestroySemaphore(vkDevice_, acquireSemaphore_, nullptr);
	}
	void VulkanSwapchain::present(VkSemaphore waitSemaphore)
	{
		VkPresentInfoKHR pi{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &waitSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &vkSwapchain_,
			.pImageIndices = &currentImageIndex_,
		};
		VkResult res = vkQueuePresentKHR(vkGraphicsQueue_, &pi);
		if (res != VK_SUCCESS && res != VK_ERROR_OUT_OF_DATE_KHR && res != VK_SUBOPTIMAL_KHR)
		{
			GAIA_CORE_ERROR("Cannot Present error code: {}", res);
		}
		currentFrameIndex_++;
	}

	VkImage VulkanSwapchain::getCurrentVkImage() const
	{
		VulkanImage* image = ctx_.texturesPool_.get(swapchainTextures_[currentImageIndex_]);
		return image->vkImage_;
	}
	VkImageView VulkanSwapchain::getCurrentVkImageView() const
	{
		VulkanImage* image = ctx_.texturesPool_.get(swapchainTextures_[currentImageIndex_]);
		return image->imageView_;
	}
	TextureHandle VulkanSwapchain::getCurrentTexture()
	{
		VkResult res = vkAcquireNextImageKHR(vkDevice_, vkSwapchain_, ONE_SEC_TO_NANOSEC, acquireSemaphore_, VK_NULL_HANDLE, &currentImageIndex_);
		GAIA_ASSERT(res == VK_SUCCESS, "Failed to acquire next Image from swapchain: Error code {}", res);
		TextureHandle swapchainTexture = swapchainTextures_[currentImageIndex_];
		return swapchainTexture;
	}
	VkSurfaceFormatKHR VulkanSwapchain::getSurfaceFormat() const
	{
		return surfaceFormat_;
	}
	uint32_t VulkanSwapchain::getNumSwapchainImages()
	{
		return numSwapchainImages_;
	}
}