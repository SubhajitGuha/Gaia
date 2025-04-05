#pragma once
#include <utility>
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "memory"
#include "Gaia/Renderer/Pool.h"
#include "glm/glm.hpp"

#define HALF_SIZE 2

namespace Gaia
{

	enum { MAX_COLOR_ATTACHMENTS = 8 };
	enum { MAX_VERTEX_BUFFERS = 16u };
	enum {MAX_DESCRIPTOR_SETS = 100u};
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
	using SamplerHandle = Handle<struct Sampler>;
	using ShaderModuleHandle = Handle<struct ShaderModule>;
	using DescriptorSetLayoutHandle = Handle<struct DescriptorSetLayout>;
	using AccelStructHandle = Handle<struct AcclerationStructure>;
	using RayTracingPipelineHandle = Handle<struct RayTracingPipeline>;

	//forward declare IContext
	class IContext;
	// forward declarations to access incomplete type IContext
	//void destroy(IContext* ctx, ComputePipelineHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::RenderPipelineHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::ShaderModuleHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::BufferHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::TextureHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::DescriptorSetLayoutHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::SamplerHandle handle);
	void destroy(Gaia::IContext* ctx, Gaia::AccelStructHandle);
	void destroy(Gaia::IContext* ctx, Gaia::RayTracingPipelineHandle handle);

	//void destroy(IContext* ctx, QueryPoolHandle handle);

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

	enum ImageLayout {
		ImageLayout_UNDEFINED = 0,
		ImageLayout_GENERAL = 1,
		ImageLayout_COLOR_ATTACHMENT_OPTIMAL = 2,
		ImageLayout_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
		ImageLayout_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
		ImageLayout_SHADER_READ_ONLY_OPTIMAL = 5,
		ImageLayout_TRANSFER_SRC_OPTIMAL = 6,
		ImageLayout_TRANSFER_DST_OPTIMAL = 7,
		ImageLayout_PREINITIALIZED = 8,
		ImageLayout_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
		ImageLayout_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
		ImageLayout_DEPTH_ATTACHMENT_OPTIMAL = 1000241000,
		ImageLayout_DEPTH_READ_ONLY_OPTIMAL = 1000241001,
		ImageLayout_STENCIL_ATTACHMENT_OPTIMAL = 1000241002,
		ImageLayout_STENCIL_READ_ONLY_OPTIMAL = 1000241003,
		ImageLayout_READ_ONLY_OPTIMAL = 1000314000,
		ImageLayout_ATTACHMENT_OPTIMAL = 1000314001,
		ImageLayout_PRESENT_SRC_KHR = 1000001002,
		ImageLayout_VIDEO_DECODE_DST_KHR = 1000024000,
		ImageLayout_VIDEO_DECODE_SRC_KHR = 1000024001,
		ImageLayout_VIDEO_DECODE_DPB_KHR = 1000024002,
		ImageLayout_SHARED_PRESENT_KHR = 1000111000,
		ImageLayout_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT = 1000218000,
		ImageLayout_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR = 1000164003,
		ImageLayout_RENDERING_LOCAL_READ_KHR = 1000232000,
		ImageLayout_VIDEO_ENCODE_DST_KHR = 1000299000,
		ImageLayout_VIDEO_ENCODE_SRC_KHR = 1000299001,
		ImageLayout_VIDEO_ENCODE_DPB_KHR = 1000299002,
		ImageLayout_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT = 1000339000,
		ImageLayout_MAX_ENUM = 0x7FFFFFFF
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

	enum SamplerFilter : uint8_t { SamplerFilter_Nearest = 0, SamplerFilter_Linear };
	enum SamplerMip : uint8_t {SamplerMip_Nearest, SamplerMip_Linear };
	enum SamplerWrap : uint8_t { SamplerWrap_Repeat = 0, SamplerWrap_Clamp, SamplerWrap_ClampBorder, SamplerWrap_MirrorRepeat };

	enum CompareOp : uint8_t {
		CompareOp_Never = 0,
		CompareOp_Less,
		CompareOp_Equal,
		CompareOp_LessEqual,
		CompareOp_Greater,
		CompareOp_NotEqual,
		CompareOp_GreaterEqual,
		CompareOp_AlwaysPass
	};

	struct SamplerStateDesc {
		SamplerFilter minFilter = SamplerFilter_Linear;
		SamplerFilter magFilter = SamplerFilter_Linear;
		SamplerMip mipMap = SamplerMip_Linear;
		SamplerWrap wrapU = SamplerWrap_Repeat;
		SamplerWrap wrapV = SamplerWrap_Repeat;
		SamplerWrap wrapW = SamplerWrap_Repeat;
		CompareOp depthCompareOp = CompareOp_LessEqual;
		uint8_t mipLodMin = 0;
		uint8_t mipLodMax = 15;
		uint8_t maxAnisotropic = 8;
		bool depthCompareEnabled = false;
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

	enum PipelineBindpoint : uint8_t
	{
		PipelineBindpoint_Graphics = 1,
		PipelineBindpoint_Compute = 2,
		PipelineBindpoint_RayTracing = 3,
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

		DescriptorSetLayoutHandle descriptorSetLayout[MAX_DESCRIPTOR_SETS];
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

		uint32_t getNumDescriptorSetLayouts()
		{
			uint32_t n = 0;
			while (n < MAX_DESCRIPTOR_SETS && descriptorSetLayout[n].isValid())
			{
				n++;
			}
			return n;
		}
	};

	struct RayTracingPipelineDesc final {
		static enum {MAX_MISS_SHADERS = 4};
		ShaderModuleHandle smRayGen;
		ShaderModuleHandle smAnyHit;
		ShaderModuleHandle smClosestHit;
		ShaderModuleHandle smMiss[MAX_MISS_SHADERS];
		ShaderModuleHandle smIntersection;
		ShaderModuleHandle smCallable;
		const char* enteryPoint = "main";
		DescriptorSetLayoutHandle descriptorSetLayout[MAX_DESCRIPTOR_SETS];
		const char* debugName = "";
		uint32_t getNumDescriptorSetLayouts()
		{
			uint32_t n = 0;
			while (n < MAX_DESCRIPTOR_SETS && descriptorSetLayout[n].isValid())
			{
				n++;
			}
			return n;
		}
		uint32_t getNumMissShaders()
		{
			uint32_t n = 0;
			while (n < MAX_MISS_SHADERS && smMiss[n].isValid())
			{
				n++;
			}
			return n;
		}
	};

	enum ShaderStage : uint32_t
	{
		Stage_None = 0,
		Stage_Vert = 1<<0,
		Stage_Tesc = 1<<2,
		Stage_Tese = 1<<3,
		Stage_Geo = 1<<4,
		Stage_Frag = 1<<5,
		Stage_Com = 1<<6,
		Stage_Mesh = 1<<7,

		// ray tracing
		Stage_RayGen = 1 << 8,
		Stage_Miss = 1 << 9,
		Stage_AnyHit = 1 << 10,
		Stage_ClosestHit = 1 << 11,
		Stage_Intersection = 1 << 12,
		Stage_Callable = 1 << 13,
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

	enum DescriptorType : uint32_t {
		DescriptorType_None = 0,
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
		DescriptorType_AccelerationStructure,
	};

	
	struct DescriptorSetLayoutDesc final
	{
		uint32_t binding = 0;
		uint32_t descriptorCount = 1;
		DescriptorType descriptorType = DescriptorType_None;
		uint32_t shaderStage = Stage_None;

		//Resources in Descriptor Set
		std::vector<TextureHandle> texture;
		std::vector<BufferHandle> buffer;
		SamplerHandle sampler;
		AccelStructHandle accelStructure; //for now supporting one TLAS acceleration structure
		//TODO include other resource types

		template<typename HandleType, typename HolderType>
		static std::vector<HandleType> getResource(HolderType& handle)
		{
			return std::vector<HandleType>{handle};
		}

		template<typename HandleType, typename HolderType>
		static std::vector<HandleType> getResource(std::vector<HolderType>& holder)
		{
			std::vector<HandleType> handles;
			for (HolderType& holder_ : holder)
			{
				handles.push_back(holder_);
			}
			return handles;
		}
	};

	enum IndexFormat : uint8_t
	{
		IndexFormat_U8,
		IndexFormat_U16,
		IndexFormat_U32,

	};

	enum AccelStructType : uint8_t {
		AccelStructType_Invalid = 0,
		AccelStructType_TLAS = 1,
		AccelStructType_BLAS = 2,
	};

	enum AccelStructGeomType : uint8_t {
		AccelStructGeomType_Triangles = 0,
		AccelStructGeomType_AABBs = 1,
		AccelStructGeomType_Instances = 2,
	};

	enum AccelStructBuildFlagBits : uint8_t {
		AccelStructBuildFlagBits_AllowUpdate = 1 << 0,
		AccelStructBuildFlagBits_AllowCompaction = 1 << 1,
		AccelStructBuildFlagBits_PreferFastTrace = 1 << 2,
		AccelStructBuildFlagBits_PreferFastBuild = 1 << 3,
		AccelStructBuildFlagBits_LowMemory = 1 << 4,
	};

	enum AccelStructGeometryFlagBits : uint8_t {
		AccelStructGeometryFlagBits_Opaque = 1 << 0,
		AccelStructGeometryFlagBits_NoDuplicateAnyHit = 1 << 1,
	};

	enum AccelStructInstanceFlagBits : uint8_t {
		AccelStructInstanceFlagBits_TriangleFacingCullDisable = 1 << 0,
		AccelStructInstanceFlagBits_TriangleFlipFacing = 1 << 1,
		AccelStructInstanceFlagBits_ForceOpaque = 1 << 2,
		AccelStructInstanceFlagBits_ForceNoOpaque = 1 << 3,
	};

	struct AccelStructBuildRange {
		uint32_t primitiveCount = 0;
		uint32_t primitiveOffset = 0;
		uint32_t firstVertex = 0;
		uint32_t transformOffset = 0;
	};

	struct mat3x4 {
		float matrix[3][4];
	};

	//this structure has same layout and bits offset as "VkAccelerationStructureInstanceKHR"
	struct AccelStructInstance {
		glm::mat3x4 transform;
		uint32_t instanceCustomIndex : 24 = 0;
		uint32_t mask : 8 = 0xff;
		uint32_t instanceShaderBindingTableRecordOffset : 24 = 0;
		uint32_t flags : 8 = AccelStructInstanceFlagBits_TriangleFacingCullDisable;
		uint64_t accelerationStructureReference = 0;
	};

	struct AccelStructDesc {
		AccelStructType type = AccelStructType_Invalid;
		AccelStructGeomType geometryType = AccelStructGeomType_Triangles;
		uint8_t geometryFlags = AccelStructGeometryFlagBits_Opaque;

		Format vertexFormat = Format_Invalid;
		uint64_t vertexBufferAddress; //stores the vertex buffer gpu address
		uint32_t vertexStride = 0;//zero means the size of `vertexFormat`
		uint32_t numVertices = 0;
		IndexFormat indexFormat = IndexFormat_U32;
		uint64_t indexBufferAddress = 0; //stores the index buffer gpu address
		uint64_t transformBufferAddress = 0; //stores the transforms buffer gpu address
		uint64_t instancesBufferAddress = 0; //stores the instances buffer gpu address
		AccelStructBuildRange buildrange = {};
		uint8_t buildFlags = AccelStructBuildFlagBits_PreferFastTrace;
		const char* debugName = "";
	};

	struct Viewport {
		float x = 0.0f;
		float y = 0.0f;
		float width = 1.0f;
		float height = 1.0f;
		float minDepth = 0.0f;
		float maxDepth = 1.0f;
	};

	struct Scissor
	{
		int x = 0;
		int y = 0;
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct ClearValue {
		float colorValue[4] = { 1.0,1.0 ,1.0 ,1.0 };
		float depthClearValue = 1.0;
		uint32_t stencilClearValue = 1u;
	};

	class ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer() = default;

		virtual void copyBuffer(BufferHandle bufferHandle, void* data, size_t sizeInBytes, uint32_t offset = 0) = 0;

		virtual void cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) = 0;
		virtual void cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth, RayTracingPipelineHandle handle) = 0;

		virtual void cmdTransitionImageLayout(TextureHandle image, ImageLayout newLayout) = 0;
		virtual void cmdBindGraphicsPipeline(RenderPipelineHandle renderPipeline) = 0;
		virtual void cmdBeginRendering(TextureHandle colorAttachmentHandle, TextureHandle depthTextureHandle, ClearValue* clearValue) = 0;
		virtual void cmdEndRendering() = 0;

		virtual void cmdCopyBufferToBuffer(BufferHandle srcBufferHandle, BufferHandle dstBufferHandle, uint32_t offset = 0) = 0;
		virtual void cmdCopyBufferToImage(BufferHandle buffer, TextureHandle texture) = 0;
		virtual void cmdCopyImageToImage(TextureHandle srcImageHandle, TextureHandle dstImageHandle) = 0;

		virtual void cmdSetViewport(Viewport viewport) = 0;
		virtual void cmdSetScissor(Scissor scissor) = 0;

		virtual void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset) = 0;
		virtual void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat indexFormat, uint64_t indexBufferOffset) = 0;
		virtual void cmdPushConstants(const void* data, size_t size, size_t offset) = 0;

		virtual void cmdBindGraphicsDescriptorSets(uint32_t firstSet, RenderPipelineHandle pipeline,const std::vector<DescriptorSetLayoutHandle>& descriptorSetLayouts) = 0;
		virtual void cmdBindRayTracingDescriptorSets(uint32_t firstSet, RayTracingPipelineHandle pipeline, const std::vector<DescriptorSetLayoutHandle>& descriptorSetLayouts) = 0;

		virtual void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,uint32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void cmdBlitImage(TextureHandle srcImageHandle, TextureHandle dstImageHandle) = 0;
	};

	class IContext
	{
	protected:
		IContext() = default;

	public:
		virtual ~IContext() = default;

		virtual ICommandBuffer& acquireCommandBuffer() = 0;
		virtual void submit(ICommandBuffer& cmd, TextureHandle presentTexture) = 0;
		virtual void submit(ICommandBuffer& cmd) = 0;

		virtual std::pair<uint32_t, uint32_t> getWindowSize() = 0;

		virtual Holder<BufferHandle> createBuffer(BufferDesc& desc, const char* debugName = "") = 0;
		virtual Holder<TextureHandle> createTexture(TextureDesc& desc, const char* debugName = "") = 0;
		virtual Holder<TextureHandle> createTextureView(TextureViewDesc& desc, const char* debugName = "") = 0;
		virtual Holder<RenderPipelineHandle> createRenderPipeline(RenderPipelineDesc& desc) = 0;
		virtual Holder<ShaderModuleHandle> createShaderModule(ShaderModuleDesc& desc) = 0;
		virtual Holder<DescriptorSetLayoutHandle> createDescriptorSetLayout(std::vector<DescriptorSetLayoutDesc>& desc) = 0;
		virtual Holder<SamplerHandle> createSampler(SamplerStateDesc& desc) = 0;
		virtual Holder<AccelStructHandle> createAccelerationStructure(AccelStructDesc& desc) = 0;
		virtual Holder<RayTracingPipelineHandle> createRayTracingPipeline(const RayTracingPipelineDesc& desc) = 0;

		virtual void destroy(BufferHandle handle) = 0;
		virtual void destroy(TextureHandle handle) = 0;
		virtual void destroy(RenderPipelineHandle handle) = 0;
		virtual void destroy(ShaderModuleHandle handle) = 0;
		virtual void destroy(DescriptorSetLayoutHandle handle) = 0;
		virtual void destroy(SamplerHandle handle) = 0;
		virtual void destroy(AccelStructHandle handle) = 0;
		virtual void destroy(RayTracingPipelineHandle handle) = 0;

		//swapchain functions
		virtual TextureHandle getCurrentSwapChainTexture() = 0;
		virtual Format getSwapChainTextureFormat() const = 0;
		virtual ColorSpace getSwapChainColorSpace() const = 0;
		virtual uint32_t getNumSwapchainImages() const = 0;
		virtual void recreateSwapchain(int newWidth, int newHeight) = 0;

		virtual uint64_t gpuAddress(BufferHandle handle, size_t offset = 0) = 0;
		virtual uint64_t gpuAddress(AccelStructHandle handle) = 0;

		virtual uint32_t getFrameBufferMSAABitMask() const = 0;
	};
}