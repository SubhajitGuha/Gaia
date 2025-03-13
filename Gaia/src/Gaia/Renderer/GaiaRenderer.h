#pragma once
#include <utility>
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "memory"
#include "Gaia/Renderer/Pool.h"

#define HALF_SIZE 2

namespace Gaia
{

	enum { MAX_COLOR_ATTACHMENTS = 8 };
	enum { MAX_VERTEX_BUFFERS = 16u };

	template<typename ObjectType>
	class Handle final
	{
	public:
		Handle() = default;

		bool isEmpty()
		{
			return gen_ == 0u;
		}

		bool isValid()
		{
			return gen_ != 0;
		}

		uint32_t index() { return index_; }
		uint32_t gen() { return gen_; }


		bool operator==(const Handle<ObjectType>& other)
		{
			return gen_ == other.gen_ && index_ == other.index_;
		}
		explicit operator bool() const
		{
			return gen_ != 0;
		}

	private:
		Handle(uint32_t index, uint32_t gen) : index_(index) , gen_(gen){}
		template<typename ObjectType, typename ImplObjectType>
		friend class Pool;

		uint32_t index_ = 0u;
		uint32_t gen_ = 0u;
	};

	using BufferHandle = Handle<struct Buffer>;
	using TextureHandle = Handle<struct Texture>;
	using RenderPipelineHandle = Handle<struct RenderPipeline>;
	using SamplerHande = Handle<struct Sampler>;
	using ShaderModuleHandle = Handle<struct ShaderModule>;
	using DescriptorSetLayoutHandle = Handle<struct DescriptorSetLayout>;

	//forward declare IContext
	class IContext;
	// forward declarations to access incomplete type IContext
	//void destroy(IContext* ctx, ComputePipelineHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::RenderPipelineHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::ShaderModuleHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::BufferHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::TextureHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::DescriptorSetLayoutHandle handle);

	//void destroy(IContext* ctx, RayTracingPipelineHandle handle);
	//void destroy(IContext* ctx, SamplerHandle handle);
	//void destroy(IContext* ctx, QueryPoolHandle handle);
	//void destroy(IContext* ctx, AccelStructHandle handle);

	template<typename HandleType>
	class Holder final
	{
	public:
		Holder() = default;
		Holder(IContext* context, HandleType handle) : handle_(handle), context_(context) {}
		Holder(Holder&& other) : handle_(other.handle_), context_(other.context_)
		{
			other.context_ = nullptr;
			other.handle_ = HandleType{};
		}
		~Holder()
		{
			Gaia::destroy(context_, handle_);
		}

		Holder& operator=(const Holder&) = delete;
		Holder& operator=(Holder&& other)
		{
			context_ = other.context_;
			handle_ = other.handle_;

			other.handle_ = HandleType{};
			other.context_ = nullptr;
			return *this;
		}
		inline operator HandleType() const
		{
			return handle_;
		}
	private:
		HandleType handle_ = {};
		IContext* context_ = nullptr;
	};

	enum BufferusageBits : uint8_t
	{
		BufferUsageBits_None = 0,
		BufferUsageBits_Index = 1 << 0,
		BufferUsageBits_Vertex = 1 << 1,
		BufferUsageBits_Uniform = 1 << 2,
		BufferUsageBits_Storage = 1 << 3,
		BufferUsageBits_Indirect = 1 << 4,

		// ray tracing
		BufferUsageBits_ShaderBindingTable = 1 << 5,
		BufferUsageBits_AccelStructBuildInputReadOnly = 1 << 6,
		BufferUsageBits_AccelStructStorage = 1 << 7
	};

	enum TextureType : uint8_t {
		TextureType_2D,
		TextureType_3D,
		TextureType_Cube,
	};

	struct Dimensions
	{
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
	};

	enum TextureUsageBits : uint32_t
	{
		TextureUsageBits_Sampled = 1 << 0,
		TextureUsageBits_Storage = 1 << 1,
		TextureUsageBits_Attachment = 1 << 2,
	};

	enum StorageType {
		StorageType_Device,
		StorageType_HostVisible,
		StorageType_Memoryless,
	};

	enum Format : uint32_t
	{
		Format_Invalid,

		Format_R_UN8, //Red channel only. 8 bit integer per channel. [0, 255] converted to/from float [0, 1] in shader.
		Format_R_UN16,
		Format_R_UI8, //Red channel only. 8 bit integer per channel. Unsigned in shader.
		Format_R_UI16,
		Format_R_UI32,
		Format_R_SI8,
		Format_R_SN8,
		Format_R_SI16,
		Format_R_SN16,
		Format_R_SI32,
		Format_R_F16,
		Format_R_F32,

		Format_RG_UN8, //Red Green channel. 8 bit integer per channel. [0, 255] converted to/from float [0, 1] in shader.
		Format_RG_UN16,
		Format_RG_UI8, //Red Green channel only. 8 bit integer per channel. Unsigned in shader.
		Format_RG_UI16,
		Format_RG_UI32,
		Format_RG_SI8,
		Format_RG_SN8,
		Format_RG_SI16,
		Format_RG_SN16,
		Format_RG_SI32,
		Format_RG_F16,
		Format_RG_F32,

		Format_RGB_F16,
		Format_RGB_F32,
		Format_RGB_UN8,
		Format_RGB_UI8,
		Format_RGB_UN16,
		Format_RGB_UI16,
		Format_RGB_UI32,
		Format_RGB_SI8,
		Format_RGB_SN8,
		Format_RGB_SI16,
		Format_RGB_SN16,
		Format_RGB_SI32,

		Format_RGBA_UN8, //RGBA channel. 8 bit integer per channel. [0, 255] converted to/from float [0, 1] in shader.
		Format_RGBA_SRGB8,
		Format_RGBA_UI8, //RGBA channel. 8 bit integer per channel. Unsigned in shader.
		Format_RGBA_UI32,
		Format_RGBA_SI8,
		Format_RGBA_SN8,
		Format_RGBA_SI32,
		Format_RGBA_F16,
		Format_RGBA_F32,

		Format_BGRA_UN8,
		Format_BGRA_SRGB8,

		Format_Z_UN16,
		Format_Z_UN24,
		Format_Z_F32,
		Format_Z_UN24_S_UI8,
		Format_Z_F32_S_UI8,

		Format_ETC2_RGB8,
		Format_ETC2_SRGB8,
		Format_BC7_SRGB,
	};

	enum ColorSpace
	{
		ColorSpace_SRGB_LINEAR,
		ColorSpace_SRGB_NONLINEAR,
	};

	struct BufferDesc final {
		uint8_t usage_type = BufferUsageBits_None;
		uint8_t storage_type = StorageType_HostVisible;
		size_t size = 0;
		const void* data = nullptr;
		const char* debugName = nullptr;
	};

	struct TextureViewDesc {
		TextureType type = TextureType_2D;
		uint32_t layer = 0;
		uint32_t numLayers = 1;
		uint32_t mipLevel = 0;
		uint32_t numMipLevels = 1;
		//ComponentMapping swizzle = {};
	};

	struct TextureDesc {
		TextureType type = TextureType_2D;
		Format format = Format_Invalid;

		Dimensions dimensions = { 1, 1, 1 };
		uint32_t numLayers = 1;
		uint32_t numSamples = 1;
		uint8_t usage = TextureUsageBits_Sampled;
		uint32_t numMipLevels = 1;
		StorageType storage = StorageType_Device;
		//ComponentMapping swizzle = {};
		const void* data = nullptr;
		uint32_t dataNumMipLevels = 1; // how many mip-levels we want to upload
		bool generateMipmaps = false; // generate mip-levels immediately, valid only with non-null data
		const char* debugName = "";
	};

	inline uint32_t getVertxFormatSize(Format format)
	{
		switch (format)
		{
		case Format_R_F32:
			return sizeof(float) * 1;
		case Format_R_F16:
			return sizeof(short) * 1;
		case Format_RG_F32:
			return sizeof(float) * 2;
		case Format_RG_F16:
			return sizeof(short) * 2;
		case Format_RGB_F32:
			return sizeof(float) * 3;
		case Format_RGB_F16:
			return sizeof(short) * 3;
		case Format_RGBA_F32:
			return sizeof(float) * 4;
		case Format_RGBA_F16:
			return sizeof(short) * 4;
		}
	}

	struct VertexInput final
	{
		enum { MAX_NUM_VERTEX_ATTRIBUTES = 16u };
		enum { MAX_NUM_VERTEX_BINDINGS = 16u };

		struct VertexAttribute
		{
			uint32_t binding = 0;
			uint32_t location = 0;
			Format format = Format_Invalid;
			uint32_t offset = 0;
		} attributes[MAX_NUM_VERTEX_ATTRIBUTES];
		struct VertexInputBinding
		{
			uint32_t stride = 0;
		}inputBindings[MAX_NUM_VERTEX_BINDINGS];

		uint32_t getNumAttributes() const
		{
			uint32_t n = 0;
			while (n < MAX_NUM_VERTEX_ATTRIBUTES && attributes[n].format != Format_Invalid)
			{
				n++;
			}
			return n;
		}

		uint32_t getNumInputBindings() const
		{
			uint32_t n = 0;
			while (n < MAX_NUM_VERTEX_BINDINGS && inputBindings[n].stride)
			{
				n++;
			}
			return n;
		}

		uint32_t GetVertexSize() const
		{
			uint32_t vertexSize = 0;
			for (int i = 0; i < MAX_NUM_VERTEX_ATTRIBUTES; i++)
			{
				vertexSize += Gaia::getVertxFormatSize(attributes[i].format);
			}
			return vertexSize;
		}

		bool operator==(VertexInput& other)
		{
			return std::memcmp(this, &other, sizeof(VertexInput)) == 0;
		}
	};
	enum Topology : uint8_t
	{
		Topology_Point,
		Topology_Line,
		Topology_LineStrip,
		Topology_Triangle,
		Topology_TriangleStrip,
		Topology_Patch,
	};

	enum CullMode {
		CullMode_None,
		CullMode_Back,
		CullMode_Front,
	};
	enum WindingMode {
		WindingMode_CCW,
		WindingMode_CW,
	};
	enum PolygonMode {PolygonMode_Line, PolygonMode_Fill};

	enum BlendOp : uint8_t
	{
		BlendOp_Add = 0,
		BlendOp_Substract,
		BlendOp_ReverseSubstract,
		BlendOp_Min,
		BlendOp_Max,
	};
	enum BlendFactor : uint8_t {
		BlendFactor_Zero = 0,
		BlendFactor_One,
		BlendFactor_SrcColor,
		BlendFactor_OneMinusSrcColor,
		BlendFactor_SrcAlpha,
		BlendFactor_OneMinusSrcAlpha,
		BlendFactor_DstColor,
		BlendFactor_OneMinusDstColor,
		BlendFactor_DstAlpha,
		BlendFactor_OneMinusDstAlpha,
		BlendFactor_SrcAlphaSaturated,
		BlendFactor_BlendColor,
		BlendFactor_OneMinusBlendColor,
		BlendFactor_BlendAlpha,
		BlendFactor_OneMinusBlendAlpha,
		BlendFactor_Src1Color,
		BlendFactor_OneMinusSrc1Color,
		BlendFactor_Src1Alpha,
		BlendFactor_OneMinusSrc1Alpha
	};

	struct ColorAttachment final
	{
		Format format = Format_Invalid;
		bool blendEnabled = false;
		BlendOp rgbBlendOp = BlendOp_Add;
		BlendOp alphaBlendOp = BlendOp_Add;
		BlendFactor srcRGBBlendFactor = BlendFactor_One;
		BlendFactor srcAlphaBlendFactor = BlendFactor_One;
		BlendFactor dstRGBBlendFactor = BlendFactor_Zero;
		BlendFactor dstAlphaBlendFactor = BlendFactor_Zero;
	};

	struct RenderPipelineDesc final
	{
		Topology topology = Topology_Triangle;
		VertexInput vertexInput;

		ShaderModuleHandle smVertex;
		ShaderModuleHandle smFragment;
		ShaderModuleHandle smTesc;
		ShaderModuleHandle smTese;
		ShaderModuleHandle smGeo;
		ShaderModuleHandle smMesh;

		const char* entryPointVert = "main";
		const char* entryPointTesc = "main";
		const char* entryPointTese = "main";
		const char* entryPointGeom = "main";
		const char* entryPointMesh = "main";
		const char* entryPointFrag = "main";

		ColorAttachment colorAttachments[MAX_COLOR_ATTACHMENTS] = {};
		Format depthFormat = Format_Invalid;
		Format stencilFormat = Format_Invalid;

		CullMode cullMode = CullMode_None;
		WindingMode windingMode = WindingMode_CCW;
		PolygonMode polygonMode = PolygonMode_Fill;

		//multisampling
		uint32_t samplesCount = 1u;
		uint32_t patchControlPoints = 0;
		float minSampleShading = 0.0f;

		const char* debugName = "";

		uint32_t getNumColorAttachments() const
		{
			uint32_t n = 0;
			while(n < MAX_COLOR_ATTACHMENTS && colorAttachments[n].format != Format_Invalid)
			{
				n++;
			}
			return n;
		}
	};

	enum ShaderStage : uint8_t
	{
		Stage_None,
		Stage_Vert,
		Stage_Tesc,
		Stage_Tese,
		Stage_Geo,
		Stage_Frag,
		Stage_Com,
		Stage_Mesh,

		// ray tracing
		Stage_RayGen,
		Stage_AnyHit,
		Stage_ClosestHit,
		Stage_Miss,
		Stage_Intersection,
		Stage_Callable,
	};

	struct ShaderModuleDesc
	{
		ShaderStage shaderStage;
		const char* shaderPath_SPIRV = nullptr; //Need it for now TODO REMOVE it
		const char* data = nullptr;
		size_t dataSize = 0; // if "dataSize" is non-zero, interpret "data" as binary shader data
		const char* debugName = "";

		//ShaderModuleDesc(const char* source, ShaderStage stage, const char* debugName = ""): data(source), shaderStage(stage), debugName(debugName){}
		ShaderModuleDesc(const void* data, size_t dataLength, ShaderStage stage, const char* debugName = "") : data(static_cast<const char*>(data)), shaderStage(stage), debugName(debugName), dataSize(dataLength) {
		}
		ShaderModuleDesc(const char* path, ShaderStage stage, const char* debugName = nullptr)
			:shaderPath_SPIRV(path), shaderStage(stage), debugName(debugName)
		{ }
	};

	enum DescriptorType : uint8_t {
		DescriptorType_None,
		DescriptorType_Sampler,
		DescriptorType_CombinedImageSampler,
		DescriptorType_SampledImage,
		DescriptorType_StorageImage,
		DescriptorType_UniformTexelBuffer,
		DescriptorType_StorageTexelBuffer,
		DescriptorType_UniformBuffer,
		DescriptorType_StorageBuffer,
		DescriptorType_UniformBufferDynamic,
		DescriptorType_StorageBufferDynamic,
		DescriptorType_InputAttachment,
		DescriptorType_InlineUniformBlock,
	};

	struct DescriptorSetLayoutDesc final
	{
		uint32_t binding = 0;
		uint32_t descriptorCount = 1;
		DescriptorType descriptorType = DescriptorType_None;
		ShaderStage shaderStage = Stage_None;

		//Resources in Descriptor Set
		TextureHandle texture;
		BufferHandle buffer;
		//TODO include other resource types
	};

	class IContext
	{
	protected:
		IContext() = default;

	public:
		virtual ~IContext() = default;

		virtual Holder<BufferHandle> createBuffer(BufferDesc& desc, const char* debugName = "") = 0;
		virtual Holder<TextureHandle> createTexture(TextureDesc& desc, const char* debugName = "") = 0;
		virtual Holder<TextureHandle> createTextureView(TextureViewDesc& desc, const char* debugName = "") = 0;
		virtual Holder<RenderPipelineHandle> createRenderPipeline(RenderPipelineDesc& desc) = 0;
		virtual Holder<ShaderModuleHandle> createShaderModule(ShaderModuleDesc& desc) = 0;
		virtual Holder<DescriptorSetLayoutHandle> createDescriptorSetLayout(std::vector<DescriptorSetLayoutDesc>& desc) = 0;

		virtual void destroy(BufferHandle handle) = 0;
		virtual void destroy(TextureHandle handle) = 0;
		virtual void destroy(RenderPipelineHandle handle) = 0;
		virtual void destroy(ShaderModuleHandle handle) = 0;
		virtual void destroy(DescriptorSetLayoutHandle handle) = 0;

		//swapchain functions
		virtual TextureHandle getCurrentSwapChainTexture() = 0;
		virtual Format getSwapChainTextureFormat() const = 0;
		virtual ColorSpace getSwapChainColorSpace() const = 0;
		virtual uint32_t getNumSwapchainImages() const = 0;
		virtual void recreateSwapchain(int newWidth, int newHeight) = 0;

		virtual uint32_t getFrameBufferMSAABitMask() const = 0;
	};
}