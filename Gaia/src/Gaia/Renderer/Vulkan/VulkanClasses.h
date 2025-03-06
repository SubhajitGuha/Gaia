#pragma once
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include <vma/vk_mem_alloc.h>
#include "Gaia/Renderer/GaiaRenderer.h"

#define MAX_MIP_LEVELS 16
#define ONE_SEC_TO_NANOSEC 1000000000

//I am trying to put every vulkan class Implementation in this file
namespace Gaia {
	
	enum { MAX_COLOR_ATTACHMENTS = 8};

	struct VulkanImage final {
		inline bool isSampledImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_SAMPLED_BIT) > 0; }
		inline bool isStorageImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) > 0; }
		inline bool isColorAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0; }
		inline bool isDepthAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0; }
		inline bool isAttachment() const { return (vkUsageFlags_ & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0; }

		VkImageView createimageView(VkDevice device,
									VkImageViewType type,
									VkFormat format,
									VkImageAspectFlags aspectMask,
									uint32_t baseLevel,
									uint32_t numLevels = VK_REMAINING_MIP_LEVELS,
									uint32_t baseLayer = 0,
									uint32_t numLayers = 1,
									const VkComponentMapping mapping = { 
										.r = VK_COMPONENT_SWIZZLE_IDENTITY,
										.g = VK_COMPONENT_SWIZZLE_IDENTITY,
										.b = VK_COMPONENT_SWIZZLE_IDENTITY,
										.a = VK_COMPONENT_SWIZZLE_IDENTITY },
									const VkSamplerYcbcrConversionInfo* ycbcr = nullptr,
									const char* debugName = nullptr) const;

		void generateMipmap(VkCommandBuffer commadBuffer) const;
		void transitionLayout(VkCommandBuffer commandBuffer,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			const VkImageSubresourceRange& subresourceRange) const;

		VkImageAspectFlags getImageAspectFlags() const;

		// framebuffers can render only into one level/layer
		//VkImageView getOrCreateVkImageViewForFramebuffer(VulkanContext& ctx, uint8_t level, uint16_t layer);

		static bool isDepthFormat(VkFormat format);
		static bool isStencilFormat(VkFormat format);
	public:
		VkImage vkImage_ = VK_NULL_HANDLE;
		VkImageUsageFlags vkUsageFlags_ = 0;
		VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
		VkFormatProperties vkFormatProperties_ = {};
		VkExtent3D vkExtent_ = { 0,0,0 };
		VkImageType vkType_ = VK_IMAGE_TYPE_MAX_ENUM;
		VkFormat vkImageFormat_ = VK_FORMAT_UNDEFINED;
		VkSampleCountFlagBits vkSamples_ = VK_SAMPLE_COUNT_1_BIT;
		void* mappedPtr_ = nullptr;
		bool isSwapchainImage_ = false;
		bool isOwningVkImage_ = true;
		uint32_t numLevels_ = 1u;
		uint32_t numLayers_ = 1u;
		bool isDepthFormat_ = false;
		bool isStencilFormat_ = false;

		//current image layout
		mutable VkImageLayout vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
		//image views
		VkImageView imageView_ = VK_NULL_HANDLE;// default view with all mip-levels
		VkImageView imageViewStorage_ = VK_NULL_HANDLE;// default view with identity swizzle (all mip-levels)
		VkImageView imageViewForFramebuffer_[MAX_MIP_LEVELS][6] = {}; // max 6 faces for cubemap rendering
	};

	struct VulkanBuffer final
	{
		void bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data);
		void getBufferData(const VulkanContext& ctx, size_t offset, size_t size, void* data);
	public:
		VkBuffer vkBuffer_ = VK_NULL_HANDLE;
		VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
		VkBufferUsageFlags vkUsageFlags_ = 0;
		bool isCoherentMemory_ = false;
	};

	struct ShaderModuleState
	{
		VkShaderModule sm = VK_NULL_HANDLE;
		uint32_t pushConstantSize = 0;
	};

	struct RenderPipelineState final
	{
		RenderPipelineDesc desc_;

		uint32_t numBindings_ = 0;
		uint32_t numAttributes_ = 0;
		VkVertexInputAttributeDescription vkAttributes_[VertexInput::MAX_NUM_VERTEX_ATTRIBUTES] = {};
		VkVertexInputBindingDescription vkBindings_[VertexInput::MAX_NUM_VERTEX_BINDINGS] = {};

		VkPipeline pipeline_ = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
		VkShaderStageFlags shaderStageFlags_ = 0;
	};

	class VulkanPipelineBuilder
	{
		VulkanPipelineBuilder();
		~VulkanPipelineBuilder() = default;
	public:
		VulkanPipelineBuilder& dynamicState(VkDynamicState state);
		VulkanPipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);
		VulkanPipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples, float minSampleShading);
		VulkanPipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);
		VulkanPipelineBuilder& cullMode(VkCullModeFlagBits mode);
		VulkanPipelineBuilder& frontFace(VkFrontFace mode);
		VulkanPipelineBuilder& polygonMode(VkPolygonMode mode);
		VulkanPipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
		VulkanPipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states,
			const VkFormat* formats,
			uint32_t numColorAttachments);
		VulkanPipelineBuilder& depthAttachmentFormat(VkFormat format);
		VulkanPipelineBuilder& stencilAttachmentFormat(VkFormat format);
		VulkanPipelineBuilder& patchControlPoints(uint32_t numPoints);

		VkResult build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout, VkPipeline* outputPipeline, const char* debugName = nullptr);

	private:
		enum{MAX_DYNAMIC_STATES = 128};
		uint32_t numDynamicStates_ = 0;
		VkDynamicState dynamicStates_[MAX_DYNAMIC_STATES] = {};

		uint32_t numShaderStages_ = 0;
		VkPipelineShaderStageCreateInfo shaderStages_[ShaderStage::Stage_Frag + 1] = {};

		VkPipelineVertexInputStateCreateInfo vertexInputState_;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
		VkPipelineRasterizationStateCreateInfo rasterizationState_;
		VkPipelineMultisampleStateCreateInfo multisampleState_;
		VkPipelineDepthStencilStateCreateInfo depthStencilState_;
		VkPipelineTessellationStateCreateInfo tessellationState_;

		uint32_t numColorAttachments_ = 0;
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState_[MAX_COLOR_ATTACHMENTS] = {};
		VkFormat colorAttachmentFormat_[MAX_COLOR_ATTACHMENTS] = {};

		VkFormat depthAttachmentFormat_ = VK_FORMAT_UNDEFINED;
		VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

		static uint32_t numPipelineCreated_;
	};

	struct DeviceQueues final
	{
		const static uint32_t INVALID = 0xFFFFFFFF;

		uint32_t graphicsQueueFamilyIndex = INVALID;
		uint32_t computeQueueFamilyIndex = INVALID;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue computeQueue = VK_NULL_HANDLE;
	};

	class VulkanSwapchain final
	{
		enum{MAX_SWAPCHAIN_IMAGES = 16};
	public:
		VulkanSwapchain(VulkanContext& ctx, uint32_t width, uint32_t height);
		~VulkanSwapchain();

		void present(VkSemaphore waitSemaphore);
		VkImage getCurrentVkImage() const;
		VkImageView getCurrentVkImageView() const;
		TextureHandle getCurrentTexture();
		VkSurfaceFormatKHR getSurfaceFormat() const;
		uint32_t getNumSwapchainImages();
	public:
		static const VkFormat SWAP_CHAIN_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
		VulkanContext& ctx_;
		VkDevice vkDevice_ = VK_NULL_HANDLE;
		VkQueue vkGraphicsQueue_ = VK_NULL_HANDLE;
		VkSwapchainKHR vkSwapchain_ = VK_NULL_HANDLE;
		VkSurfaceFormatKHR surfaceFormat_ = { .format = VK_FORMAT_UNDEFINED };
		TextureHandle swapchainTextures_[MAX_SWAPCHAIN_IMAGES] = {};
		VkSemaphore acquireSemaphore_ = VK_NULL_HANDLE;

		uint32_t width_ = 0;
		uint32_t height_ = 0;
		uint32_t numSwapchainImages_ = 0;
		uint32_t currentImageIndex_ = 0; // [0...numSwapchainImages_)
		uint64_t currentFrameIndex_ = 0; // [0...+inf)
	};

	class VulkanContext final : public IContext
	{
		friend class Gaia::VulkanSwapchain;
	public:
		VulkanContext(void* window);
		~VulkanContext();

		Holder<BufferHandle> createBuffer(BufferDesc& desc, const char* debugName = "") override;
		Holder<TextureHandle> createTexture(TextureDesc& desc, const char* debugName = "") override;
		Holder<TextureHandle> createTextureView(TextureViewDesc& desc, const char* debugName = "") override;
		Holder<RenderPipelineHandle> createRenderPipeline(RenderPipelineDesc& desc) override;
		Holder<ShaderModuleHandle> ctreateShaderModule(ShaderModuleDesc& desc) override;

		void destroy(BufferHandle handle) override;
		void destroy(TextureHandle handle) override;
		void destroy(RenderPipelineHandle handle) override;
		void destroy(ShaderModuleHandle handle) override;

		//swapchain functions
		TextureHandle getCurrentSwapChainTexture() override;
		Format getSwapChainTextureFormat() const override;
		ColorSpace getSwapChainColorSpace() const override;
		uint32_t getNumSwapchainImages() const override;
		void recreateSwapchain(int newWidth, int newHeight) override;

		uint32_t getFrameBufferMSAABitMask() const override;

		VkInstance getInstance()
		{
			return vkInstance_;
		}

		VkDevice getDevice()
		{
			return vkDevice_;
		}

		VkPhysicalDevice getPhysicalDevice()
		{
			return vkPhysicsalDevice_;
		}

		VmaAllocator getVmaAllocator()
		{
			return vmaAllocator_;
		}

		VkPipeline getPipeline(RenderPipelineHandle handle);
	private:
		void createInstance();
		void createSurface();


	private:
		VkInstance vkInstance_ = VK_NULL_HANDLE;
		VkSurfaceKHR vkSurface_ = VK_NULL_HANDLE;
		VkPhysicalDevice vkPhysicsalDevice_ = VK_NULL_HANDLE;
		VkDevice vkDevice_ = VK_NULL_HANDLE;
		VmaAllocator vmaAllocator_ = VK_NULL_HANDLE;
		bool enableValidationLayers_ = true;

		VkPhysicalDeviceVulkan13Features vkFeatures13_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		VkPhysicalDeviceVulkan12Features vkFeatures12_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
												  .pNext = &vkFeatures13_ };
		VkPhysicalDeviceVulkan11Features vkFeatures11_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
														  .pNext = &vkFeatures12_ };
		VkPhysicalDeviceFeatures2 vkFeatures10_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &vkFeatures11_ };

	public:
		DeviceQueues deviceQueues_;
		std::unique_ptr<VulkanSwapchain> swapchain_;
		VkSemaphore renderingSemaphore_ = VK_NULL_HANDLE;

		Pool<Texture, VulkanImage> texturesPool_;
		Pool<Buffer, VulkanBuffer> bufferPool_;
		Pool<RenderPipeline, RenderPipelineState> renderPipelinePool_;
		Pool<ShaderModule, ShaderModuleState> shaderModulePool_;
	};


}

