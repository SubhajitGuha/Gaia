#include "pch.h"
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "VulkanClasses.h"
#include "GLFW/glfw3.h"
#include "VkUtils.h"

namespace Gaia {
	uint32_t VulkanPipelineBuilder::numPipelineCreated_ = 0;
	VkImageView VulkanImage::createimageView(VkDevice device, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseLevel, uint32_t numLevels, uint32_t baseLayer, uint32_t numLayers, const VkComponentMapping mapping, const VkSamplerYcbcrConversionInfo* ycbcr, const char* debugName) const
	{
		VkImageViewCreateInfo iv_ci
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = vkImage_,
			.viewType = type,
			.format = format,
			.components = mapping,
			.subresourceRange = VkImageSubresourceRange
			{
				.aspectMask = aspectMask,
				.baseMipLevel = baseLevel,
				.levelCount = numLevels,
				.baseArrayLayer = baseLayer,
				.layerCount = numLayers,
			},
		};
		VkImageView imageView;
		vkCreateImageView(device, &iv_ci, nullptr, &imageView);
		return imageView;
	}
	void VulkanImage::generateMipmap(VkCommandBuffer commadBuffer) const
	{
	}
	void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageSubresourceRange& subresourceRange) const
	{
		VkImageMemoryBarrier2 imageBarrier
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = srcStageMask,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			.dstStageMask = dstStageMask,
			.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

			.oldLayout = vkImageLayout_,
			.newLayout = newImageLayout,

			.image = vkImage_,
			.subresourceRange = subresourceRange,
		};
		VkDependencyInfo depInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageBarrier,
		};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo );
		vkImageLayout_ = newImageLayout;
	}
	VkImageAspectFlags VulkanImage::getImageAspectFlags() const
	{
		if (isDepthFormat(vkImageFormat_))
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (isStencilFormat(vkImageFormat_))
		{
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}

	bool VulkanImage::isDepthFormat(VkFormat format)
	{
		if (format == VK_FORMAT_D24_UNORM_S8_UINT || 
			format == VK_FORMAT_D16_UNORM_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT || 
			format == VK_FORMAT_D16_UNORM 
			)
		{
			return true;
		}
		return false;
	}
	bool VulkanImage::isStencilFormat(VkFormat format)
	{
		if (format == VK_FORMAT_D24_UNORM_S8_UINT ||
			format == VK_FORMAT_D16_UNORM_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			format == VK_FORMAT_S8_UINT)
		{
			return true;
		}
		return false;
	}

	VulkanContext::VulkanContext(void* window)
	{
		vkb::InstanceBuilder builder;
		vkb::Result instance_res = builder.set_app_name("Gaia").
			enable_validation_layers(enableValidationLayers_).
			require_api_version(1, 3, 0).
			enable_validation_layers(enableValidationLayers_).
			use_default_debug_messenger().
			build();

		if (!instance_res)
			GAIA_CORE_ERROR("Failed to create Vulkan instance. Error: {}", instance_res.error().message());

		vkb::Instance vkb_instance = instance_res.value();
		vkInstance_ = vkb_instance.instance;
		vkDebugUtilsMessenger_ = vkb_instance.debug_messenger;

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
		immediateCommands_ = std::make_unique<VulkanImmediateCommands>(vkDevice_, deviceQueues_.graphicsQueueFamilyIndex);
	}
	VulkanContext::~VulkanContext()
	{
		GAIA_ASSERT(vkDeviceWaitIdle(vkDevice_) == VK_SUCCESS, "");
		swapchain_.reset(nullptr);

		waitForDeferredTasks();

		vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
		immediateCommands_.release();
		// Clean up VMA
		
		vmaDestroyAllocator(vmaAllocator_);

		// Device has to be destroyed prior to Instance
		vkDestroyDevice(vkDevice_, nullptr);

		/*if (vkDebugUtilsMessenger_) {
			vkDestroyDebugUtilsMessengerEXT(vkInstance_, vkDebugUtilsMessenger_, nullptr);
		}*/

		vkDestroyInstance(vkInstance_, nullptr);

	}
	Holder<BufferHandle> VulkanContext::createBuffer(BufferDesc& desc, const char* debugName)
	{
		VkBufferUsageFlags usageFlags = desc.storage_type == StorageType_Device? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;

		if (desc.storage_type == StorageType_HostVisible)
		{
			usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		if (desc.usage_type & BufferUsageBits_Index)
		{
			usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (desc.usage_type & BufferUsageBits_Vertex)
		{
			usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (desc.usage_type & BufferUsageBits_Indirect)
		{
			usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (desc.usage_type & BufferUsageBits_Storage)
		{
			usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (desc.usage_type & BufferUsageBits_Uniform)
		{
			usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (desc.usage_type & BufferUsageBits_AccelStructBuildInputReadOnly)
		{
			usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		}
		if (desc.usage_type & BufferUsageBits_AccelStructStorage)
		{
			usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
		}
		if (desc.usage_type & BufferUsageBits_ShaderBindingTable)
		{
			usageFlags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
		}
		
		VulkanBuffer buffer
		{
			.vkUsageFlags_ = usageFlags,
			.bufferSize_ = desc.size,
			.isCoherentMemory_ = false,
		};

		VkBufferCreateInfo buffer_ci
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = desc.size,
			.usage = usageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VmaAllocationCreateInfo vma_ci
		{
			.usage = VMA_MEMORY_USAGE_AUTO,
		};

		if (desc.storage_type == StorageType_HostVisible)
		{
			vma_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			vma_ci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			vma_ci.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		}
		vmaCreateBuffer(getVmaAllocator(), &buffer_ci, &vma_ci, &buffer.vkBuffer_, &buffer.vmaAllocation_, VK_NULL_HANDLE);

		//get the mapped address location if the storage type is HostVisible
		if (desc.storage_type == StorageType_HostVisible)
		{
			vmaMapMemory(getVmaAllocator(), buffer.vmaAllocation_, &buffer.mappedPtr_);
		}

		BufferHandle bufferHandle = bufferPool_.Create(std::move(buffer));
		return {this,bufferHandle};
	}
	Holder<TextureHandle> VulkanContext::createTexture(TextureDesc& desc, const char* debugName)
	{
		VkFormat imageFormat = getVkFormatFromFormat(desc.format);
		VkImageUsageFlags usageFlags = desc.storage == StorageType_Device ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;

		if (desc.usage & TextureUsageBits_Storage)
		{
			usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (desc.usage & TextureUsageBits_Attachment)
		{
			if (VulkanImage::isDepthFormat(imageFormat))
				usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if (desc.storage == StorageType_Memoryless)
			{
				usageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			}
		}
		if (desc.usage & TextureUsageBits_Sampled)
		{
			usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}


		VkImageCreateFlags vkCreateFlags = 0;
		VkImageViewType vkImageViewType;
		VkImageType vkImageType;
		VkSampleCountFlagBits vkSamples = VK_SAMPLE_COUNT_1_BIT;
		uint32_t numLayers = desc.numLayers;
		switch (desc.type) {
		case TextureType_2D:
			vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			vkImageType = VK_IMAGE_TYPE_2D;
			vkSamples = getVkSampleCountFromSampleCount(desc.numSamples);
			break;
		case TextureType_3D:
			vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
			vkImageType = VK_IMAGE_TYPE_3D;
			break;
		case TextureType_Cube:
			vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
			vkImageType = VK_IMAGE_TYPE_2D;
			vkCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			numLayers *= 6;
			break;
		default:
			GAIA_ASSERT(false, "Code should NOT be reached");
			return {};
		}
		VkImageCreateInfo image_ci
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = getVkFormatFromFormat(desc.format),
			.extent = VkExtent3D{
				.width = desc.dimensions.width,
				.height = desc.dimensions.height,
				.depth = desc.dimensions.depth,
			},
			.mipLevels = desc.numMipLevels,
			.arrayLayers = desc.numLayers,
			.samples = getVkSampleCountFromSampleCount(desc.numSamples),
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VulkanImage image
		{
			.vkUsageFlags_ = usageFlags,
			.vkExtent_ = {desc.dimensions.width, desc.dimensions.height, desc.dimensions.depth},
			.vkType_ = vkImageType,
			.vkImageFormat_ = getVkFormatFromFormat(desc.format),
			.vkSamples_ = vkSamples,
			.isSwapchainImage_ = false,
			.numLevels_ = desc.numMipLevels,
			.numLayers_ = numLayers,
			.isDepthFormat_ = VulkanImage::isDepthFormat(imageFormat),
			.isStencilFormat_ = VulkanImage::isStencilFormat(imageFormat),
		};

		VmaAllocationCreateInfo vma_ci
		{
			.usage = desc.storage == StorageType_HostVisible ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_AUTO,
		};
		vmaCreateImage(getVmaAllocator(), &image_ci, &vma_ci, &image.vkImage_, &image.vmaAllocation_, VK_NULL_HANDLE);
		
		//get the mapped address location if the storage type is HostVisible
		if (desc.storage == StorageType_HostVisible)
		{
			vmaMapMemory(getVmaAllocator(), image.vmaAllocation_, &image.mappedPtr_);
		}

		image.createimageView(vkDevice_, vkImageViewType, imageFormat, image.getImageAspectFlags(), 0, desc.numMipLevels, 0, numLayers);

		TextureHandle texHandle = texturesPool_.Create(std::move(image));

		return {this, texHandle};
	}
	Holder<TextureHandle> VulkanContext::createTextureView(TextureViewDesc& desc, const char* debugName)
	{
		return Holder<TextureHandle>();
	}
	Holder<RenderPipelineHandle> VulkanContext::createRenderPipeline(RenderPipelineDesc& desc)
	{
		if (!desc.smVertex.isValid())
		{
			GAIA_CORE_WARN("Vertex Shader is empty");
		}

		if (!desc.smFragment.isValid())
		{
			GAIA_CORE_WARN("Fragment Shader is empty");
		}

		VertexInput vertexInput = desc.vertexInput;

		RenderPipelineState renderPipelineState{
			.desc_ = desc,
			.numBindings_ = vertexInput.getNumInputBindings(),
			.numAttributes_ = vertexInput.getNumAttributes(),
		};

		for (int i = 0; i < vertexInput.getNumAttributes(); i++)
		{
			const VertexInput::VertexAttribute attribute = vertexInput.attributes[i];

			renderPipelineState.vkAttributes_[i] = VkVertexInputAttributeDescription{
				.location = attribute.location,
				.binding = attribute.binding,
				.format = getVkFormatFromFormat(attribute.format),
				.offset = attribute.offset,
			};
			renderPipelineState.vkBindings_[attribute.binding] = VkVertexInputBindingDescription
			{
				.binding = attribute.binding,
				.stride = vertexInput.inputBindings[attribute.binding].stride,
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			};
		}

		RenderPipelineHandle renderPipelineHandle = renderPipelinePool_.Create(std::move(renderPipelineState));
		getPipeline(renderPipelineHandle);
		return { this, renderPipelineHandle };
	}
	Holder<ShaderModuleHandle> VulkanContext::createShaderModule(ShaderModuleDesc& desc)
	{
		//Todo glslang to load shaders
		ShaderModuleState shaderModuleState
		{
			.pushConstantSize = 0, //TODO change this based on number of push-constants
		};
		shaderModuleState.sm = loadShaderModule(desc.shaderPath_SPIRV, vkDevice_);

		ShaderModuleHandle shaderModuleHandle = shaderModulePool_.Create(std::move(shaderModuleState));
		return { this, shaderModuleHandle};
	}
	Holder<DescriptorSetLayoutHandle> VulkanContext::createDescriptorSetLayout(std::vector<DescriptorSetLayoutDesc>& desc)
	{
		VulkanDescriptorSetLayout layout{};

		for (auto& descSetLayout : desc)
		{
			VkDescriptorSetLayoutBinding newbind{};
			newbind.binding = descSetLayout.binding;
			newbind.descriptorCount = descSetLayout.descriptorCount;
			newbind.descriptorType = getVkDescTypeFromDescType(descSetLayout.descriptorType);
			newbind.stageFlags = getVkShaderStageFromShaderStage(descSetLayout.shaderStage);

			layout.bindings.push_back(newbind);
		}
		VkDescriptorSetLayoutCreateInfo dsl_ci
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = (uint32_t)layout.bindings.size(),
			.pBindings = layout.bindings.data(),
		};

		vkCreateDescriptorSetLayout(vkDevice_, &dsl_ci, nullptr, &layout.descSetLayout);

		VulkanDescriptorSet set{
			.device_ = vkDevice_,
			.layout_ = layout,
		};
		for (auto& descSetLayout : desc)
		{
			set.write(descSetLayout, *this);
		}

		set.allocatePool();
		set.updateSet();

		DescriptorSetLayoutHandle handle = descriptorSetPool_.Create(std::move(set));
		return {this, handle};
	}
	void VulkanContext::destroy(BufferHandle handle)
	{
		VulkanBuffer* buffer = bufferPool_.get(handle);
		deferredTask(std::packaged_task<void()>([vkBuffer = buffer->vkBuffer_, vma = getVmaAllocator(), vmaAllocation = buffer->vmaAllocation_]() {
			vmaDestroyBuffer(vma, vkBuffer, vmaAllocation);
			}));
		bufferPool_.Destroy(handle);
	}
	void VulkanContext::destroy(TextureHandle handle)
	{
		VulkanImage* image = texturesPool_.get(handle);

		if (!image)
		{
			return;
		}
		if (image->imageView_ != VK_NULL_HANDLE)
		{
		deferredTask(std::packaged_task<void()>([device = vkDevice_, imageView = image->imageView_]() {
			vkDestroyImageView(device, imageView, nullptr);
			}));
		}
		if (image->imageViewStorage_ != VK_NULL_HANDLE)
		{
			deferredTask(std::packaged_task<void()>([device = vkDevice_, imageView = image->imageViewStorage_]() {
				vkDestroyImageView(device, imageView, nullptr);
				}));
		}

		deferredTask(std::packaged_task<void()>([allocator = getVmaAllocator(), image = image->vkImage_, allocation = image->vmaAllocation_]() {
			vmaDestroyImage(allocator, image, allocation);
			}));
		texturesPool_.Destroy(handle);
	}
	void VulkanContext::destroy(RenderPipelineHandle handle)
	{
		RenderPipelineState* rps = renderPipelinePool_.get(handle);
		//destroy pipeline layout 
		deferredTask(std::packaged_task<void()>([device = vkDevice_, pl = rps->pipelineLayout_]() {
			vkDestroyPipelineLayout(device, pl, nullptr);
			}));

		deferredTask(std::packaged_task<void()>([device = vkDevice_, pipeline = rps->pipeline_]() {
			vkDestroyPipeline(device, pipeline, nullptr);
			}));
		renderPipelinePool_.Destroy(handle);
	}
	void VulkanContext::destroy(ShaderModuleHandle handle)
	{
		ShaderModuleState* sms = shaderModulePool_.get(handle);

		deferredTask(std::packaged_task<void()>([device = vkDevice_, shaderModule = sms->sm]() {
			vkDestroyShaderModule(device, shaderModule, nullptr);
			}));
		shaderModulePool_.Destroy(handle);
	}

	void VulkanContext::destroy(DescriptorSetLayoutHandle handle)
	{
		VulkanDescriptorSet* ds = descriptorSetPool_.get(handle);

		deferredTask(std::packaged_task<void()>([device = vkDevice_, layout = ds->layout_.descSetLayout, pool = ds->getPool()]() {
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
			vkDestroyDescriptorPool(device, pool, nullptr);
			}));
		descriptorSetPool_.Destroy(handle);
	}

	TextureHandle VulkanContext::getCurrentSwapChainTexture()
	{
		return swapchain_->getCurrentTexture();
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
		return swapchain_->numSwapchainImages_;
	}
	void VulkanContext::recreateSwapchain(int newWidth, int newHeight)
	{
		swapchain_ = std::make_unique<VulkanSwapchain>(*this, newWidth, newHeight);
	}
	VulkanDescriptorSet* VulkanContext::getDescriptorSet(DescriptorSetLayoutHandle handle)
	{
		VulkanDescriptorSet* set =  descriptorSetPool_.get(handle);
		return set;
	}
	uint32_t VulkanContext::getFrameBufferMSAABitMask() const
	{
		return 0;
	}

	void VulkanContext::submit(const VulkanImmediateCommands::CommandBufferWrapper& wraper, TextureHandle swapchainImageHandle)
	{
		VulkanImage* swapchainImage = texturesPool_.get(swapchainImageHandle);

		immediateCommands_->waitSemaphore(swapchain_->acquireSemaphore_);
		VkImageSubresourceRange subRange{
			.aspectMask = swapchainImage->getImageAspectFlags(),
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		swapchainImage->transitionLayout(wraper.cmdBuffer_, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subRange);
		immediateCommands_->submit(wraper);

		swapchain_->present(immediateCommands_->acquireLastSubmitSemaphore());
		if (wraper.fence_ != VK_NULL_HANDLE)
			vkWaitForFences(vkDevice_, 1, &wraper.fence_, VK_TRUE, ONE_SEC_TO_NANOSEC);
	}
	
	VkPipeline VulkanContext::getPipeline(RenderPipelineHandle handle)
	{
		RenderPipelineState* rps = renderPipelinePool_.get(handle);
		if (!rps)
		{
			return VK_NULL_HANDLE;
		}
		if (rps->pipeline_ != VK_NULL_HANDLE)
		{
			return rps->pipeline_;
		}

		std::unique_ptr<Gaia::VulkanPipelineBuilder> pipelineBuilder = std::make_unique<Gaia::VulkanPipelineBuilder>();

		VkPipelineVertexInputStateCreateInfo vis_ci
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = rps->numBindings_,
			.pVertexBindingDescriptions = rps->vkBindings_,
			.vertexAttributeDescriptionCount = rps->numAttributes_,
			.pVertexAttributeDescriptions = rps->vkAttributes_,
		};

		ShaderModuleState* smsVertex = shaderModulePool_.get(rps->desc_.smVertex);
		ShaderModuleState* smsFrag = shaderModulePool_.get(rps->desc_.smFragment);
		ShaderModuleState* smsTesc = shaderModulePool_.get(rps->desc_.smTesc);
		ShaderModuleState* smsTese = shaderModulePool_.get(rps->desc_.smTese);
		ShaderModuleState* smsGeo = shaderModulePool_.get(rps->desc_.smGeo);

		VkPipelineColorBlendAttachmentState vkColorAttachments[MAX_COLOR_ATTACHMENTS] = {};
		VkFormat colorFormats[MAX_COLOR_ATTACHMENTS] = {};
		const uint32_t numColAttachments = rps->desc_.getNumColorAttachments();

		for (int i = 0; i < numColAttachments; i++)
		{
			ColorAttachment& colorAttachment = rps->desc_.colorAttachments[i];
			if (colorAttachment.blendEnabled)
			{
				vkColorAttachments[i] = VkPipelineColorBlendAttachmentState
				{
					.blendEnable = true,
					.srcColorBlendFactor = getVkBlendFactorFromBlendFactor(colorAttachment.srcRGBBlendFactor),
					.dstColorBlendFactor = getVkBlendFactorFromBlendFactor(colorAttachment.dstRGBBlendFactor),
					.colorBlendOp = getVkBlendOpFromBlendOp(colorAttachment.rgbBlendOp),
					.srcAlphaBlendFactor = getVkBlendFactorFromBlendFactor(colorAttachment.srcAlphaBlendFactor),
					.dstAlphaBlendFactor = getVkBlendFactorFromBlendFactor(colorAttachment.dstAlphaBlendFactor),
					.alphaBlendOp = getVkBlendOpFromBlendOp(colorAttachment.alphaBlendOp),
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				};
			}
			else {
				vkColorAttachments[i] = VkPipelineColorBlendAttachmentState
				{
					.blendEnable = false,
					.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				};
			}
			colorFormats[i] = getVkFormatFromFormat(colorAttachment.format);
		}
		//createing a simple pipeline layout without any push constant or Descriptor sets
		VkPipelineLayoutCreateInfo pl_ci
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};
		vkCreatePipelineLayout(vkDevice_, &pl_ci, nullptr, &rps->pipelineLayout_);

		//fill according to Render pipeline descriptor
		pipelineBuilder->dynamicState(VK_DYNAMIC_STATE_VIEWPORT)
			.dynamicState(VK_DYNAMIC_STATE_SCISSOR)
			.cullMode(getVkCullModeFromCullMode(rps->desc_.cullMode))
			.frontFace(getVkFrontFaceFromWindingMode(rps->desc_.windingMode))
			.polygonMode(getVkPolygonModeFromPolygonMode(rps->desc_.polygonMode))
			.primitiveTopology(getVkPrimitiveTopologyFromTopology(rps->desc_.topology))
			.rasterizationSamples(getVkSampleCountFromSampleCount(rps->desc_.samplesCount), rps->desc_.minSampleShading)
			.vertexInputState(vis_ci)
			.depthAttachmentFormat(getVkFormatFromFormat(rps->desc_.depthFormat))
			
			.colorAttachments(vkColorAttachments, colorFormats, numColAttachments);
		//add the shader stages
		if (smsVertex)
		{
			pipelineBuilder->shaderStage(vkutil::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, smsVertex->sm, rps->desc_.entryPointVert, nullptr));
		}
		if (smsFrag)
		{
			pipelineBuilder->shaderStage(vkutil::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, smsFrag->sm, rps->desc_.entryPointFrag, nullptr));
		}
		if (smsTesc)
		{
			pipelineBuilder->shaderStage(vkutil::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, smsTesc->sm, rps->desc_.entryPointTesc, nullptr));
		}
		if (smsTese)
		{
			pipelineBuilder->shaderStage(vkutil::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, smsTese->sm, rps->desc_.entryPointTese, nullptr));
		}
		if (smsGeo)
		{
			pipelineBuilder->shaderStage(vkutil::getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, smsGeo->sm, rps->desc_.entryPointGeom, nullptr));
		}

		//build the pipeline
		GAIA_ASSERT(pipelineBuilder->build(vkDevice_, VK_NULL_HANDLE, rps->pipelineLayout_, &rps->pipeline_) == VK_SUCCESS, "error");

		return rps->pipeline_;
	}
	void VulkanContext::createInstance()
	{
	}
	void VulkanContext::createSurface()
	{
	}

	void VulkanContext::waitForDeferredTasks()
	{
		for (auto& task : deferredTask_)
		{
			GAIA_ASSERT(vkDeviceWaitIdle(vkDevice_) == VK_SUCCESS, ""); //wait for device to finish its tasks
			task.task_();
		}
	}

	void VulkanContext::deferredTask(std::packaged_task<void()>&& task)
	{
		deferredTask_.emplace_back(DeferredTask(std::move(task)));
	}
	void VulkanBuffer::bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data)
	{
		GAIA_ASSERT(isMapped(), "Only Host-visible buffers can be uploaded in this way");
		
		GAIA_ASSERT(offset + size <= bufferSize_, "");
		if (data)
		{
			memcpy((uint8_t*)mappedPtr_ + offset, data, size);
		}
		else {
			memcpy((uint8_t*)mappedPtr_ + offset, 0, size);
		}

		if (!isCoherentMemory_)
		{
			flushMappedMemory(ctx, offset, size);
		}
	}
	void VulkanBuffer::getBufferData(const VulkanContext& ctx, size_t offset, size_t size, void* data)
	{
		GAIA_ASSERT(isMapped(), "Only Host-visible buffers can be downloaded in this way");

		GAIA_ASSERT(offset + size <= bufferSize_, "");
		
		if (!isCoherentMemory_)
		{
			invalidateMappedMemory(ctx, offset, size);
		}
		const uint8_t* src = static_cast<uint8_t*>(mappedPtr_) + offset;
		memcpy(data, src, size);
	}
	void VulkanBuffer::flushMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size)
	{
		GAIA_ASSERT(isMapped(), "Mapped ptr is nullptr");

		vmaFlushAllocation(ctx.getVmaAllocator(), vmaAllocation_, offset, size);
	}
	void VulkanBuffer::invalidateMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size)
	{
		GAIA_ASSERT(isMapped(), "Mapped ptr is nullptr");
		vmaInvalidateAllocation(ctx.getVmaAllocator(), vmaAllocation_, offset, size);
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
				.lineWidth = 1.0,
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
			}),
		renderingInfo_(VkPipelineRenderingCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
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
		renderingInfo_.colorAttachmentCount = numColorAttachments;
		renderingInfo_.pColorAttachmentFormats = formats;

		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
	{
		depthAttachmentFormat_ = format;
		renderingInfo_.depthAttachmentFormat = depthAttachmentFormat_;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
	{
		stencilAttachmentFormat_ = format;
		renderingInfo_.stencilAttachmentFormat = stencilAttachmentFormat_;
		return *this;
	}
	VulkanPipelineBuilder& VulkanPipelineBuilder::patchControlPoints(uint32_t numPoints)
	{
		tessellationState_.patchControlPoints = numPoints;
		return *this;
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
			.pNext = &renderingInfo_,
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

			image.isDepthFormat_ = VulkanImage::isDepthFormat(vkbSwapchain.image_format);
			image.isStencilFormat_ = VulkanImage::isStencilFormat(vkbSwapchain.image_format);

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
	VkBlendOp getVkBlendOpFromBlendOp(BlendOp operation)
	{
		switch (operation)
		{
		case BlendOp_Add:
			return VK_BLEND_OP_ADD;
		case BlendOp_Substract:
			return VK_BLEND_OP_SUBTRACT;
		case BlendOp_Max:
			return VK_BLEND_OP_MAX;
		case BlendOp_Min:
			return VK_BLEND_OP_MIN;
		case BlendOp_ReverseSubstract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		}
	}
	VkBlendFactor getVkBlendFactorFromBlendFactor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor_Zero:
			return VK_BLEND_FACTOR_ZERO;
		case BlendFactor_One:
			return VK_BLEND_FACTOR_ONE;
		case BlendFactor_SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor_OneMinusSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor_DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor_OneMinusDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor_SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor_OneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor_DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor_OneMinusDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor_BlendColor:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFactor_OneMinusBlendColor:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case BlendFactor_BlendAlpha:
			return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case BlendFactor_OneMinusBlendAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case BlendFactor_SrcAlphaSaturated:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case BlendFactor_Src1Color:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case BlendFactor_OneMinusSrc1Color:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case BlendFactor_Src1Alpha:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case BlendFactor_OneMinusSrc1Alpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		}
	}
	VkFormat getVkFormatFromFormat(Format format)
	{
		switch (format)
		{
		case Format_Invalid:
			return VK_FORMAT_UNDEFINED;

			// R formats
		case Format_R_UN8:
			return VK_FORMAT_R8_UNORM;
		case Format_R_UN16:
			return VK_FORMAT_R16_UNORM;
		case Format_R_UI8:
			return VK_FORMAT_R8_UINT;
		case Format_R_UI16:
			return VK_FORMAT_R16_UINT;
		case Format_R_UI32:
			return VK_FORMAT_R32_UINT;
		case Format_R_SI8:
			return VK_FORMAT_R8_SINT;
		case Format_R_SN8:
			return VK_FORMAT_R8_SNORM;
		case Format_R_SI16:
			return VK_FORMAT_R16_SINT;
		case Format_R_SN16:
			return VK_FORMAT_R16_SNORM;
		case Format_R_SI32:
			return VK_FORMAT_R32_SINT;
		case Format_R_F16:
			return VK_FORMAT_R16_SFLOAT;
		case Format_R_F32:
			return VK_FORMAT_R32_SFLOAT;

			// RG formats
		case Format_RG_UN8:
			return VK_FORMAT_R8G8_UNORM;
		case Format_RG_UN16:
			return VK_FORMAT_R16G16_UNORM;
		case Format_RG_UI8:
			return VK_FORMAT_R8G8_UINT;
		case Format_RG_UI16:
			return VK_FORMAT_R16G16_UINT;
		case Format_RG_UI32:
			return VK_FORMAT_R32G32_UINT;
		case Format_RG_SI8:
			return VK_FORMAT_R8G8_SINT;
		case Format_RG_SN8:
			return VK_FORMAT_R8G8_SNORM;
		case Format_RG_SI16:
			return VK_FORMAT_R16G16_SINT;
		case Format_RG_SN16:
			return VK_FORMAT_R16G16_SNORM;
		case Format_RG_SI32:
			return VK_FORMAT_R32G32_SINT;
		case Format_RG_F16:
			return VK_FORMAT_R16G16_SFLOAT;
		case Format_RG_F32:
			return VK_FORMAT_R32G32_SFLOAT;

			// RGB formats
		case Format_RGB_F16:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case Format_RGB_F32:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case Format_RGB_UN8:
			return VK_FORMAT_R8G8B8_UNORM;
		case Format_RGB_UI8:
			return VK_FORMAT_R8G8B8_UINT;
		case Format_RGB_UN16:
			return VK_FORMAT_R16G16B16_UNORM;
		case Format_RGB_UI16:
			return VK_FORMAT_R16G16B16_UINT;
		case Format_RGB_UI32:
			return VK_FORMAT_R32G32B32_UINT;
		case Format_RGB_SI8:
			return VK_FORMAT_R8G8B8_SINT;
		case Format_RGB_SN8:
			return VK_FORMAT_R8G8B8_SNORM;
		case Format_RGB_SI16:
			return VK_FORMAT_R16G16B16_SINT;
		case Format_RGB_SN16:
			return VK_FORMAT_R16G16B16_SNORM;
		case Format_RGB_SI32:
			return VK_FORMAT_R32G32B32_SINT;

			// RGBA formats
		case Format_RGBA_UN8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Format_RGBA_SRGB8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case Format_RGBA_UI8:
			return VK_FORMAT_R8G8B8A8_UINT;
		case Format_RGBA_UI32:
			return VK_FORMAT_R32G32B32A32_UINT;
		case Format_RGBA_SI8:
			return VK_FORMAT_R8G8B8A8_SINT;
		case Format_RGBA_SN8:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case Format_RGBA_SI32:
			return VK_FORMAT_R32G32B32A32_SINT;
		case Format_RGBA_F16:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format_RGBA_F32:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

			// BGRA formats
		case Format_BGRA_UN8:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Format_BGRA_SRGB8:
			return VK_FORMAT_B8G8R8A8_SRGB;

			// Compressed formats
		case Format_ETC2_RGB8:
			return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		case Format_ETC2_SRGB8:
			return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case Format_BC7_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;

			//depth formats
		case Format_Z_UN16:
			return VK_FORMAT_D16_UNORM;
		case Format_Z_UN24:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format_Z_F32:
			return VK_FORMAT_D32_SFLOAT;
		case Format_Z_UN24_S_UI8:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format_Z_F32_S_UI8:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}
	VkCullModeFlagBits getVkCullModeFromCullMode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode_Back:
			return VK_CULL_MODE_BACK_BIT;
		case CullMode_Front:
			return VK_CULL_MODE_FRONT_BIT;
		case CullMode_None:
			return VK_CULL_MODE_NONE;
		}
	}
	VkFrontFace getVkFrontFaceFromWindingMode(WindingMode mode)
	{
		switch (mode)
		{
		case WindingMode_CCW:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		case WindingMode_CW:
			return VK_FRONT_FACE_CLOCKWISE;
		}
	}
	VkPrimitiveTopology getVkPrimitiveTopologyFromTopology(Topology topology)
	{
		switch (topology)
		{
		case Topology_Line:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case Topology_LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case Topology_Patch:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		case Topology_Point:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case Topology_Triangle:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case Topology_TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
	}
	VkSampleCountFlagBits getVkSampleCountFromSampleCount(uint32_t numSamples)
	{
		switch (numSamples)
		{
		case 1:
			return VK_SAMPLE_COUNT_1_BIT;
		case 2:
			return VK_SAMPLE_COUNT_2_BIT;
		case 4:
			return VK_SAMPLE_COUNT_4_BIT;
		case 8:
			return VK_SAMPLE_COUNT_8_BIT;
		case 16:
			return VK_SAMPLE_COUNT_16_BIT;
		case 32:
			return VK_SAMPLE_COUNT_32_BIT;
		default:
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}
	VkPolygonMode getVkPolygonModeFromPolygonMode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode_Fill:
			return VK_POLYGON_MODE_FILL;
		case PolygonMode_Line:
			return VK_POLYGON_MODE_LINE;
		}
	}


	VkShaderModule loadShaderModule(const char* filePath, VkDevice device)
	{
		// open the file. With cursor at the end
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			return VK_NULL_HANDLE;
		}

		// find what the size of the file is by looking up the location of the cursor
		// because the cursor is at the end, it gives the size directly in bytes
		size_t fileSize = (size_t)file.tellg();

		// spirv expects the buffer to be on uint32, so make sure to reserve a int
		// vector big enough for the entire file
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		// put file cursor at beginning
		file.seekg(0);

		// load the entire file into the buffer
		file.read((char*)buffer.data(), fileSize);

		// now that the file is loaded into the buffer, we can close it
		file.close();

		// create a new shader module, using the buffer we loaded
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		// codeSize has to be in bytes, so multply the ints in the buffer by size of
		// int to know the real size of the buffer
		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		// check that the creation goes well.
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			return VK_NULL_HANDLE;
		}
		return shaderModule;
	}
	VkDescriptorType getVkDescTypeFromDescType(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType_Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case DescriptorType_CombinedImageSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case DescriptorType_SampledImage:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case DescriptorType_StorageImage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case DescriptorType_UniformTexelBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case DescriptorType_StorageTexelBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case DescriptorType_UniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case DescriptorType_StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case DescriptorType_UniformBufferDynamic:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case DescriptorType_StorageBufferDynamic:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case DescriptorType_InputAttachment:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		case DescriptorType_InlineUniformBlock:
			return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
		}
	}
	VulkanImmediateCommands::VulkanImmediateCommands(VkDevice device, uint32_t queueFamilyIndex, const char* debugName)
		: device_(device), queueFamilyIndex_(queueFamilyIndex), debugName_(debugName)
	{
		vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue_);
		
		//create a command buffer pool
		VkCommandPoolCreateInfo cp_ci{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndex_,
		};
		vkCreateCommandPool(device, &cp_ci, nullptr, &commandPool_);

		//allocate command buffers
		VkCommandBufferAllocateInfo ai =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = commandPool_,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		for (int i = 0; i < maxCommandBuffers; i++)
		{
			CommandBufferWrapper& wrapper = buffers_[i];
			wrapper.semaphore_ = vkutil::createSemaphore(device);
			wrapper.fence_ = vkutil::createFence(device);

			GAIA_ASSERT(vkAllocateCommandBuffers(device, &ai, &wrapper.cmdBufferAllocated_) == VK_SUCCESS, "");
			wrapper.handle_.bufferIndex_ = i;
			wrapper.isEncoding = false;
		}
	}
	VulkanImmediateCommands::~VulkanImmediateCommands()
	{
		waitAll();
		for (auto& buf : buffers_)
		{
			vkDestroyFence(device_, buf.fence_, nullptr);
			vkDestroySemaphore(device_, buf.semaphore_, nullptr);
		}
		vkDestroyCommandPool(device_, commandPool_, nullptr);
	}
	const VulkanImmediateCommands::CommandBufferWrapper& VulkanImmediateCommands::acquire()
	{
		if (!numAvailableCommandBuffers_)
		{
			purge();
		}

		//wait until a command buffer is available
		while (numAvailableCommandBuffers_ == 0)
		{
			GAIA_CORE_INFO("Waiting for command buffers.....");
			purge();
		}

		VulkanImmediateCommands::CommandBufferWrapper* current = nullptr;

		for (auto& buf : buffers_)
		{
			if (buf.cmdBuffer_ == VK_NULL_HANDLE)
			{
				current = &buf;
				break;
			}
		}

		assert(current);

		GAIA_ASSERT(current->cmdBufferAllocated_ != VK_NULL_HANDLE, "");
		current->handle_.submitId_ = submitCounter_;
		numAvailableCommandBuffers_--;

		current->cmdBuffer_ = current->cmdBufferAllocated_;
		current->isEncoding = true;

		VkCommandBufferBeginInfo bi{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		vkBeginCommandBuffer(current->cmdBuffer_, &bi);

		nextSubmitHandle_ = current->handle_;
		return *current;
	}
	SubmitHandle VulkanImmediateCommands::submit(const VulkanImmediateCommands::CommandBufferWrapper& wrapper)
	{
		//if command buffer is not submitted (ie isEncoding = false) then raise error
		GAIA_ASSERT(wrapper.isEncoding, "begin encoding the command buffer before submitting");
		GAIA_ASSERT(vkEndCommandBuffer(wrapper.cmdBuffer_) == VK_SUCCESS, "");

		VkSemaphoreSubmitInfo waitSemaphores[] = { {},{} };
		uint32_t numWaitSemphores = 0;
		if (waitSemaphore_.semaphore)
		{
			waitSemaphores[numWaitSemphores++] = waitSemaphore_;
		}
		if (lastSubmitSemaphore_.semaphore)
		{
			waitSemaphores[numWaitSemphores++] = lastSubmitSemaphore_;
		}

		VkSemaphoreSubmitInfo signalSemaphore[] = {
			VkSemaphoreSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = wrapper.semaphore_,
				.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			}
		,
			{}
		};

		uint32_t numSignalSemaphore = 1;

		if (signalSemaphore_.semaphore)
		{
			signalSemaphore[numSignalSemaphore++] = signalSemaphore_;
		}

		VkCommandBufferSubmitInfo cb_si{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBuffer = wrapper.cmdBuffer_,
		};

		VkSubmitInfo2 si = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = numWaitSemphores,
			.pWaitSemaphoreInfos = waitSemaphores,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cb_si,
			.signalSemaphoreInfoCount = numSignalSemaphore,
			.pSignalSemaphoreInfos = signalSemaphore,
		};

		GAIA_ASSERT(vkQueueSubmit2(queue_, 1u, &si, wrapper.fence_) == VK_SUCCESS, "");
		lastSubmitSemaphore_.semaphore = wrapper.semaphore_;
		lastSubmitHandle_ = wrapper.handle_;
		
		waitSemaphore_.semaphore = VK_NULL_HANDLE;
		signalSemaphore_.semaphore = VK_NULL_HANDLE;

		//reset
		const_cast<VulkanImmediateCommands::CommandBufferWrapper&>(wrapper).isEncoding = false;
		submitCounter_++;

		if (!submitCounter_)
		{
			submitCounter_++;
		}
		return lastSubmitHandle_;
	}
	void VulkanImmediateCommands::waitSemaphore(VkSemaphore semaphore)
	{
		GAIA_ASSERT(waitSemaphore_.semaphore == VK_NULL_HANDLE, "");
		waitSemaphore_.semaphore = semaphore;
	}
	void VulkanImmediateCommands::signalSemaphore(VkSemaphore semaphore, uint32_t signalValue)
	{
		GAIA_ASSERT(signalSemaphore_.semaphore == VK_NULL_HANDLE, "");
		signalSemaphore_.semaphore = semaphore;
		signalSemaphore_.value = signalValue;
	}
	VkSemaphore VulkanImmediateCommands::acquireLastSubmitSemaphore()
	{
		return std::exchange(lastSubmitSemaphore_.semaphore, VK_NULL_HANDLE);
	}
	VkFence VulkanImmediateCommands::getVkFence(SubmitHandle handle) const
	{
		if (handle.empty())
		{
			return VK_NULL_HANDLE;
		}
		return buffers_[handle.bufferIndex_].fence_;
	}
	SubmitHandle VulkanImmediateCommands::getLastSubmitHandle() const
	{
		return lastSubmitHandle_;
	}
	SubmitHandle VulkanImmediateCommands::getNextSubmitHandle() const
	{
		return nextSubmitHandle_;
	}
	bool VulkanImmediateCommands::isReady(SubmitHandle handle, bool fastCheckNoVulkan) const
	{
		if (handle.empty())
		{
			return true;
		}

		const VulkanImmediateCommands::CommandBufferWrapper& buf = buffers_[handle.bufferIndex_];

		if (buf.cmdBuffer_ == VK_NULL_HANDLE)
		{
			return true;
		}

		if (buf.handle_.submitId_ != handle.submitId_)
		{
			return true;
		}

		if (fastCheckNoVulkan)
		{
			return false;
		}

		return vkWaitForFences(device_, 1, &buf.fence_, VK_TRUE, 0);
	}
	void VulkanImmediateCommands::wait(SubmitHandle handle)
	{
		if (handle.empty())
		{
			vkDeviceWaitIdle(device_);
			return;
		}

		if (isReady(handle))
		{
			return;
		}
		vkWaitForFences(device_, 1, &buffers_[handle.bufferIndex_].fence_, VK_TRUE, UINT64_MAX);
		purge();
	}
	void VulkanImmediateCommands::waitAll()
	{
		VkFence fences[maxCommandBuffers];

		int i = 0;
		for (auto& buf : buffers_)
		{
			if (buf.cmdBuffer_ != VK_NULL_HANDLE && !buf.isEncoding)
			{
				fences[i++] = buf.fence_;
			}
		}

		VkResult res = vkWaitForFences(device_, i, fences, VK_TRUE, 100000);
		GAIA_ASSERT(res == VK_SUCCESS, "error {}", res);
		purge();
	}
	void VulkanImmediateCommands::purge()
	{
		for (int i = 0; i < maxCommandBuffers; i++)
		{
			VulkanImmediateCommands::CommandBufferWrapper& buf = buffers_[(i + lastSubmitHandle_.bufferIndex_ + 1) % maxCommandBuffers];

			//if the bommand buffer is in encoding state or if it is empty skip it
			if (buf.cmdBuffer_ == VK_NULL_HANDLE || buf.isEncoding)
			{
				continue;
			}

			VkResult res = vkWaitForFences(device_, 1, &buf.fence_, VK_TRUE, 0);
			if (res == VK_SUCCESS)
			{
				GAIA_ASSERT(vkResetCommandBuffer(buf.cmdBuffer_, 0) == VK_SUCCESS,"");
				GAIA_ASSERT(vkResetFences(device_, 1, &buf.fence_) == VK_SUCCESS, "");
				buf.cmdBuffer_ = VK_NULL_HANDLE;
				numAvailableCommandBuffers_++;
			}
			else {
				if (res != VK_TIMEOUT)
				{
					GAIA_ASSERT(res == VK_SUCCESS, "");
				}
			}
		}
	}
	/*VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device,VulkanContext& ctx, VulkanDescriptorSetLayout layout, const char* debugName)
		: device_(device), ctx_(ctx), debugName_(debugName), layout_(layout)
	{

	}
	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
	}*/
	void VulkanDescriptorSet::write(DescriptorSetLayoutDesc& desc, VulkanContext& ctx_)
	{
		VkDescriptorImageInfo imageInfo{};
		if (!desc.texture.isEmpty())
		{
			VulkanImage* image = ctx_.texturesPool_.get(desc.texture);
			imageInfo.imageLayout = image->vkImageLayout_;
			imageInfo.imageView = image->imageView_;
			//imageInfo.sampler = {};
		}

		VkDescriptorBufferInfo bufInfo{};
		if (!desc.buffer.isEmpty())
		{
			VulkanBuffer* buffer = ctx_.bufferPool_.get(desc.buffer);
			bufInfo.buffer = buffer->vkBuffer_;
			bufInfo.offset = 0;
			bufInfo.range = buffer->bufferSize_;
		}

		VkWriteDescriptorSet write{};

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = desc.descriptorCount;
		write.write = set_;
		write.pImageInfo = !desc.texture.isEmpty() ? &imageInfo : nullptr;
		write.dstBinding = desc.binding;
		write.descriptorType = getVkDescTypeFromDescType(desc.descriptorType);
		write.pBufferInfo = !desc.buffer.isEmpty() ? &bufInfo : nullptr;

		writes_.push_back(write);
	}
	void VulkanDescriptorSet::updateSet()
	{
		vkUpdateDescriptorSets(device_, writes_.size(), writes_.data(), 0, nullptr);
	}
	void VulkanDescriptorSet::allocatePool()
	{
		//create pool
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (auto& write : writes_)
		{
			VkDescriptorPoolSize poolSize{};
			poolSize.descriptorCount = uint32_t(write.descriptorCount * maxSets_);
			poolSize.type = write.descriptorType;
			poolSizes.push_back(poolSize);
		}

		VkDescriptorPoolCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr, 
			.flags = 0,
			.maxSets = maxSets_,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		};
		GAIA_ASSERT(vkCreateDescriptorPool(device_, &ci, nullptr, &pool_) == VK_SUCCESS,"");

		//allocate the descriptor set

		VkDescriptorSetAllocateInfo ds_ai
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = pool_,
			.descriptorSetCount = 1,
			.pSetLayouts = &layout_.descSetLayout,
		};
		GAIA_ASSERT(vkAllocateDescriptorSets(device_, &ds_ai, &set_)==VK_SUCCESS,"");
	}
}