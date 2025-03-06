#include "pch.h"
#include "VkPipeline.h"
#include "Gaia/Log.h"
#include "VkInitializers.h"
#include "Gaia/Core.h"

namespace Gaia {
	VkPipelineBuilder::VkPipelineBuilder()
	{
		m_inputAssembly = {};
		m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		m_vertexInput = {};
		m_vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		m_rasterizer = {};
		m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		m_colorBlendAttachment = {};

		m_multisampling = {};
		m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		m_pipelineLayout = {};
       

		m_depthStencil = {};
		m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		m_renderInfo = {};
		m_renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		m_shaderStages.clear();
	}

    VkPipelineBuilder::~VkPipelineBuilder()
    {
    }

	VkPipeline VkPipelineBuilder::CreateGraphicsPipeline(VkDevice device)
	{
		//
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// setup dummy color blending. We arent using transparent objects yet
		// the blending is just "no blend", but we do write to the color attachment
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &m_colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = &m_renderInfo;

        
		pipelineCreateInfo.stageCount = m_shaderStages.size();
		pipelineCreateInfo.pStages = m_shaderStages.data();
		pipelineCreateInfo.pVertexInputState = &m_vertexInput;
		pipelineCreateInfo.pInputAssemblyState = &m_inputAssembly;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &m_rasterizer;
		pipelineCreateInfo.pMultisampleState = &m_multisampling;
		pipelineCreateInfo.pColorBlendState = &colorBlending;
		pipelineCreateInfo.pDepthStencilState = &m_depthStencil;

		pipelineCreateInfo.layout = m_pipelineLayout;

		VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicInfo = {};
		dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.pDynamicStates = &state[0];
		dynamicInfo.dynamicStateCount = 2;

		pipelineCreateInfo.pDynamicState = &dynamicInfo;

		//for now no pipeline cache
		VkPipeline pipeline;
		auto res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,&pipeline);
		GAIA_ASSERT(res == VK_SUCCESS, "failed to create Graphics pipeline error code:- {}", res);
		return pipeline;
	}
    
    //> set_shaders
    void VkPipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader, const char* vertex_shader_entery,
        const char* frag_shader_entery)
    {
        m_shaderStages.clear();

        m_shaderStages.push_back(
            vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, vertex_shader_entery));

        m_shaderStages.push_back(
            vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, frag_shader_entery));
    }
    //< set_shaders
    //> set_topo
    void VkPipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
    {
        m_inputAssembly.topology = topology;
        // we are not going to use primitive restart on the entire tutorial so leave
        // it on false
        m_inputAssembly.primitiveRestartEnable = VK_FALSE;
    }
    //< set_topo

    //> set_poly
    void VkPipelineBuilder::set_polygon_mode(VkPolygonMode mode)
    {
        m_rasterizer.polygonMode = mode;
        m_rasterizer.lineWidth = 1.f;
    }
    //< set_poly

    //> set_cull
    void VkPipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace)
    {
        m_rasterizer.cullMode = cullMode;
        m_rasterizer.frontFace = frontFace;
    }
    //< set_cull

    //> set_multisample
    void VkPipelineBuilder::set_multisampling_none()
    {
        m_multisampling.sampleShadingEnable = VK_FALSE;
        // multisampling defaulted to no multisampling (1 sample per pixel)
        m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multisampling.minSampleShading = 1.0f;
        m_multisampling.pSampleMask = nullptr;
        // no alpha to coverage either
        m_multisampling.alphaToCoverageEnable = VK_FALSE;
        m_multisampling.alphaToOneEnable = VK_FALSE;
    }
    //< set_multisample

    //> set_noblend
    void VkPipelineBuilder::disable_blending()
    {
        // default write mask
        m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // no blending
        m_colorBlendAttachment.blendEnable = VK_FALSE;
    }
    //< set_noblend

    //> alphablend
    void VkPipelineBuilder::enable_blending_additive()
    {
        m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_colorBlendAttachment.blendEnable = VK_TRUE;
        m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void VkPipelineBuilder::enable_blending_alphablend()
    {
        m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_colorBlendAttachment.blendEnable = VK_TRUE;
        m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    //< alphablend

    void VkPipelineBuilder::set_vertex_input_binding(std::vector<VkVertexInputBindingDescription>& vertBindingDesc, std::vector<VkVertexInputAttributeDescription>& vertexAttribDesc)
    {
        m_vertexInput.vertexAttributeDescriptionCount = vertexAttribDesc.size();
        m_vertexInput.vertexBindingDescriptionCount = vertBindingDesc.size();
        m_vertexInput.pVertexAttributeDescriptions = vertexAttribDesc.data();
        m_vertexInput.pVertexBindingDescriptions = vertBindingDesc.data();
    }

    //> set_formats
    void VkPipelineBuilder::set_color_attachment_format(VkFormat format)
    {
        m_colorAttachmentformat = format;
        // connect the format to the renderInfo  structure
        m_renderInfo.colorAttachmentCount = 1;
        m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentformat;
    }

    void VkPipelineBuilder::set_depth_format(VkFormat format)
    {
        m_renderInfo.depthAttachmentFormat = format;
    }
    //< set_formats

    //> depth_disable
    void VkPipelineBuilder::disable_depthtest()
    {
        m_depthStencil.depthTestEnable = VK_FALSE;
        m_depthStencil.depthWriteEnable = VK_FALSE;
        m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        m_depthStencil.depthBoundsTestEnable = VK_FALSE;
        m_depthStencil.stencilTestEnable = VK_FALSE;
        m_depthStencil.front = {};
        m_depthStencil.back = {};
        m_depthStencil.minDepthBounds = 0.f;
        m_depthStencil.maxDepthBounds = 1.f;
    }
    //< depth_disable

    //> depth_enable
    void VkPipelineBuilder::enable_depthtest(bool depthWriteEnable, VkCompareOp op)
    {
        m_depthStencil.depthTestEnable = VK_TRUE;
        m_depthStencil.depthWriteEnable = depthWriteEnable;
        m_depthStencil.depthCompareOp = op;
        m_depthStencil.depthBoundsTestEnable = VK_FALSE;
        m_depthStencil.stencilTestEnable = VK_FALSE;
        m_depthStencil.front = {};
        m_depthStencil.back = {};
        m_depthStencil.minDepthBounds = 0.f;
        m_depthStencil.maxDepthBounds = 1.f;
    }
    //< depth_enable

    //> load_shader
    bool vkutil::load_shader_module(const char* filePath,
        VkDevice device,
        VkShaderModule* outShaderModule)
    {
        // open the file. With cursor at the end
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return false;
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
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }
    //< load_shader
}