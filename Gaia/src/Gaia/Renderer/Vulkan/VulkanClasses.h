#pragma once
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include <vma/vk_mem_alloc.h>
#include "Gaia/Renderer/GaiaRenderer.h"
#include <future>

#define MAX_MIP_LEVELS 16
#define ONE_SEC_TO_NANOSEC 1000000000

//I am trying to put every vulkan class Implementation in this file
namespace Gaia {

	class VulkanContext;

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
										.r = VK_COMPONENT_SWIZZLE_R,
										.g = VK_COMPONENT_SWIZZLE_G,
										.b = VK_COMPONENT_SWIZZLE_B,
										.a = VK_COMPONENT_SWIZZLE_A },
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
		inline uint8_t* getMappedPtr() { return static_cast<uint8_t*>(mappedPtr_); }
		inline bool isMapped() { return mappedPtr_ != nullptr; }

		void bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data);
		void getBufferData(const VulkanContext& ctx, size_t offset, size_t size, void* data);
		void flushMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size);
		void invalidateMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size);
	public:
		VkBuffer vkBuffer_ = VK_NULL_HANDLE;
		VmaAllocation vmaAllocation_ = VK_NULL_HANDLE;
		VkBufferUsageFlags vkUsageFlags_ = 0;
		VkDeviceSize bufferSize_ = 0;
		void* mappedPtr_ = nullptr;
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

	class VulkanPipelineBuilder final
	{
	public:

		VulkanPipelineBuilder();
		~VulkanPipelineBuilder() = default;

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

		static uint32_t numPipelineCreated_;
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
		VkPipelineRenderingCreateInfo renderingInfo_;

		uint32_t numColorAttachments_ = 0;
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState_[MAX_COLOR_ATTACHMENTS] = {};
		VkFormat colorAttachmentFormat_[MAX_COLOR_ATTACHMENTS] = {};

		VkFormat depthAttachmentFormat_ = VK_FORMAT_UNDEFINED;
		VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

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

	struct DeferredTask final {
		DeferredTask(std::packaged_task<void()>&& task) : task_(std::move(task))
		{}
		std::packaged_task<void()> task_;
	};

	struct SubmitHandle final
	{
		uint32_t bufferIndex_ = 0;
		uint32_t submitId_ = 0;
		SubmitHandle() = default;
		explicit SubmitHandle(uint32_t handle) : bufferIndex_(uint32_t(handle & 0xffffffff)), submitId_(uint32_t(handle >> 32)) {}
		bool empty() const {
			return submitId_ == 0;
		}
		uint64_t handle() const {
			return ((uint64_t)submitId_ << 32) + bufferIndex_;
		}
	};
	//simple command buffer setup inspired from lightweightVK
	/*
		We allocate 64 command buffers at once and when we nned a command buffe
		we supply from the pool of empty command buffers. If no command buffer is
		empty wait for atleat one cbb to be empty.
	*/
	class VulkanImmediateCommands final
	{
	public:
		static const uint32_t maxCommandBuffers = 64;
		VulkanImmediateCommands(VkDevice device, uint32_t queueFamilyIndex, const char* debugName = nullptr);
		~VulkanImmediateCommands();
		VulkanImmediateCommands(const VulkanImmediateCommands&) = delete;
		VulkanImmediateCommands& operator=(const VulkanImmediateCommands&) = delete;

		struct CommandBufferWrapper final
		{
			VkCommandBuffer cmdBuffer_ = VK_NULL_HANDLE;
			VkCommandBuffer cmdBufferAllocated_ = VK_NULL_HANDLE;
			SubmitHandle handle_ = {};
			VkFence fence_ = VK_NULL_HANDLE;
			VkSemaphore semaphore_ = VK_NULL_HANDLE;
			bool isEncoding = false;
		};

		// returns the current command buffer (creates one if it does not exist)
		const CommandBufferWrapper& acquire();
		SubmitHandle submit(const CommandBufferWrapper& wrapper);
		void waitSemaphore(VkSemaphore semaphore);
		void signalSemaphore(VkSemaphore semaphore, uint32_t signalValue);
		VkSemaphore acquireLastSubmitSemaphore();
		VkFence getVkFence(SubmitHandle handle) const;
		SubmitHandle getLastSubmitHandle() const;
		SubmitHandle getNextSubmitHandle() const;
		bool isReady(SubmitHandle handle, bool fastCheckNoVulkan = false) const;
		void wait(SubmitHandle handle);
		void waitAll();

	private:
		void purge();

	private:
		VkDevice device_ = VK_NULL_HANDLE;
		VkQueue queue_ = VK_NULL_HANDLE;
		VkCommandPool commandPool_ = VK_NULL_HANDLE;
		uint32_t queueFamilyIndex_ = 0;
		const char* debugName_ = "";
		CommandBufferWrapper buffers_[maxCommandBuffers] = {};
		SubmitHandle lastSubmitHandle_ = SubmitHandle{};
		SubmitHandle nextSubmitHandle_ = SubmitHandle{};
		VkSemaphoreSubmitInfo lastSubmitSemaphore_ = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO ,
			.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		};
		VkSemaphoreSubmitInfo waitSemaphore_ = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO ,
			.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		}; //extra wait semaphore
		VkSemaphoreSubmitInfo signalSemaphore_ = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO ,
			.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		};//extra signal semaphore
		uint32_t numAvailableCommandBuffers_ = maxCommandBuffers;
		uint32_t submitCounter_ = 1;
	};

	struct VulkanDescriptorSetLayout final
	{
		VkDescriptorSetLayout descSetLayout;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	struct VulkanDescriptorSet final
	{
	public:
		
		void write(DescriptorSetLayoutDesc& desc, VulkanContext& ctx_);
		void updateSet();
		void allocatePool();
		VkDescriptorPool getPool() { return pool_; }
		VkDescriptorSet getSet() { return set_; }

		VkDevice device_ = VK_NULL_HANDLE;
		VkDescriptorPool pool_ = VK_NULL_HANDLE;
		VkDescriptorSet set_ = VK_NULL_HANDLE;
		uint32_t maxSets_ = 5;
		std::vector<VkWriteDescriptorSet> writes_ = {};
		const char* debugName_ = "";
	public:
		VulkanDescriptorSetLayout layout_;

	};

	class VulkanCommandBuffer final : public ICommandBuffer
	{
	public:
		VulkanCommandBuffer() = default;
		explicit VulkanCommandBuffer(VulkanContext* ctx);
		~VulkanCommandBuffer() override;

		void copyBuffer(BufferHandle bufferHandle, void* data, size_t sizeInBytes, uint32_t offset = 0) override;

		void cmdCopyBufferToBuffer(BufferHandle srcBufferHandle, BufferHandle dstBufferHandle, uint32_t offset = 0) override;
		void cmdTransitionImageLayout(TextureHandle image, ImageLayout newLayout) override;
		void cmdBindGraphicsPipeline(RenderPipelineHandle renderPipeline) override;
		void cmdBeginRendering(TextureHandle colorAttachmentHandle, TextureHandle depthTextureHandle, ClearValue* clearValue) override;
		void cmdEndRendering() override;

		void cmdCopyBufferToImage(BufferHandle buffer, TextureHandle texture) override;

		void cmdSetViewport(Viewport viewport) override;
		void cmdSetScissor(Scissor scissor) override;

		void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset) override;
		void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat indexFormat, uint64_t indexBufferOffset) override;
		void cmdPushConstants(const void* data, size_t size, size_t offset) override;

		void cmdBindGraphicsDescriptorSets(uint32_t firstSet, RenderPipelineHandle pipeline, const std::vector<DescriptorSetLayoutHandle>& descriptorSetLayouts);

		void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) override;

		operator VkCommandBuffer () const {
			if (commandBufferWraper_ == nullptr)
			{
				return VK_NULL_HANDLE;
			}
			else
			{
				commandBufferWraper_->cmdBuffer_;
			}
		}
	private:
		friend class VulkanContext;
		const VulkanImmediateCommands::CommandBufferWrapper* commandBufferWraper_;
		VulkanContext* ctx_ = nullptr;
	};

	class VulkanContext final : public IContext
	{
		friend class Gaia::VulkanSwapchain;
	public:
		explicit VulkanContext(void* window);
		~VulkanContext();

		ICommandBuffer& acquireCommandBuffer() override;
		void submit(ICommandBuffer& cmd, TextureHandle presentTexture) override;
		void submit(ICommandBuffer& cmd) override;

		std::pair<uint32_t, uint32_t> getWindowSize() override;

		Holder<BufferHandle> createBuffer(BufferDesc& desc, const char* debugName = "") override;
		Holder<TextureHandle> createTexture(TextureDesc& desc, const char* debugName = "") override;
		Holder<TextureHandle> createTextureView(TextureViewDesc& desc, const char* debugName = "") override;
		Holder<RenderPipelineHandle> createRenderPipeline(RenderPipelineDesc& desc) override;
		Holder<ShaderModuleHandle> createShaderModule(ShaderModuleDesc& desc) override;
		Holder<DescriptorSetLayoutHandle> createDescriptorSetLayout(std::vector<DescriptorSetLayoutDesc>& desc) override;

		void destroy(BufferHandle handle) override;
		void destroy(TextureHandle handle) override;
		void destroy(RenderPipelineHandle handle) override;
		void destroy(ShaderModuleHandle handle) override;
		void destroy(DescriptorSetLayoutHandle handle) override;

		//swapchain functions
		TextureHandle getCurrentSwapChainTexture() override;
		Format getSwapChainTextureFormat() const override;
		ColorSpace getSwapChainColorSpace() const override;
		uint32_t getNumSwapchainImages() const override;
		void recreateSwapchain(int newWidth, int newHeight) override;

		VulkanDescriptorSet* getDescriptorSet(DescriptorSetLayoutHandle handle);
		uint32_t getFrameBufferMSAABitMask() const override;

		VkInstance getInstance()
		{
			return vkInstance_;
		}

		VkDevice getDevice() const
		{
			return vkDevice_;
		}

		VkPhysicalDevice getPhysicalDevice() const
		{
			return vkPhysicsalDevice_;
		}

		VmaAllocator getVmaAllocator() const
		{
			return vmaAllocator_;
		}

		VkPipeline getPipeline(RenderPipelineHandle handle);
	private:
		void createInstance();
		void createSurface();


	private:
		int window_width = 0, window_height = 0;
		VkInstance vkInstance_ = VK_NULL_HANDLE;
		VkSurfaceKHR vkSurface_ = VK_NULL_HANDLE;
		VkPhysicalDevice vkPhysicsalDevice_ = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT vkDebugUtilsMessenger_ = VK_NULL_HANDLE;
		VkDevice vkDevice_ = VK_NULL_HANDLE;
		VmaAllocator vmaAllocator_ = VK_NULL_HANDLE;
		bool enableValidationLayers_ = true;
		std::unique_ptr<VulkanCommandBuffer> vulkanCommandBuffer_;

		VkPhysicalDeviceVulkan13Features vkFeatures13_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		VkPhysicalDeviceVulkan12Features vkFeatures12_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
												  .pNext = &vkFeatures13_ };
		VkPhysicalDeviceVulkan11Features vkFeatures11_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
														  .pNext = &vkFeatures12_ };
		VkPhysicalDeviceFeatures2 vkFeatures10_ = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &vkFeatures11_ };

		std::deque<DeferredTask> deferredTask_;
		void waitForDeferredTasks();
		void deferredTask(std::packaged_task<void()>&& task);
	public:
		DeviceQueues deviceQueues_;
		std::unique_ptr<VulkanSwapchain> swapchain_;
		std::unique_ptr<VulkanImmediateCommands> immediateCommands_;
		VkSemaphore renderingSemaphore_ = VK_NULL_HANDLE;

		Pool<Texture, VulkanImage> texturesPool_;
		Pool<Buffer, VulkanBuffer> bufferPool_;
		Pool<RenderPipeline, RenderPipelineState> renderPipelinePool_;
		Pool<ShaderModule, ShaderModuleState> shaderModulePool_;
		Pool<DescriptorSetLayout, VulkanDescriptorSet> descriptorSetPool_;
	};

	inline VkBlendOp getVkBlendOpFromBlendOp(BlendOp operation);
	inline VkBlendFactor getVkBlendFactorFromBlendFactor(BlendFactor factor);
	inline VkFormat getVkFormatFromFormat(Format format);
	inline VkCullModeFlagBits getVkCullModeFromCullMode(CullMode mode);
	inline VkFrontFace getVkFrontFaceFromWindingMode(WindingMode mode);
	inline VkPrimitiveTopology getVkPrimitiveTopologyFromTopology(Topology topology);
	inline VkSampleCountFlagBits getVkSampleCountFromSampleCount(uint32_t numSamples);
	inline VkPolygonMode getVkPolygonModeFromPolygonMode(PolygonMode mode);
	inline VkShaderModule loadShaderModule(const char* filePath,
		VkDevice device);
	inline VkDescriptorType getVkDescTypeFromDescType(DescriptorType type);
	inline VkImageLayout getVkImageLayoutFromImageLayout(ImageLayout layout);
	inline VkShaderStageFlags getVkShaderStageFromShaderStage(uint32_t stage)
	{
		uint32_t flag = 0u;
		
		if(stage & Stage_Vert)
			flag |= VK_SHADER_STAGE_VERTEX_BIT;
		if(stage & Stage_Tesc)
			flag |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if(stage& Stage_Tese)
			flag |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		if (stage& Stage_Geo)
			flag |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if (stage& Stage_Frag)
			flag |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (stage& Stage_Com)
			flag |= VK_SHADER_STAGE_COMPUTE_BIT;
		if (stage& Stage_Mesh)
			flag |= VK_SHADER_STAGE_MESH_BIT_EXT;
		if (stage& Stage_AnyHit)
			flag |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (stage& Stage_ClosestHit)
			flag |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (stage& Stage_Intersection)
			flag |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (stage& Stage_Miss)
			flag |= VK_SHADER_STAGE_MISS_BIT_KHR;
		if (stage& Stage_Callable)
			flag |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		
		return flag;
	}
}

