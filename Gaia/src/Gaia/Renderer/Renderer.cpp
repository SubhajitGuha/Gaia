#include "pch.h"
#include "Renderer.h"
//#include "Vulkan/VulkanClasses.h"
#include "Gaia/Renderer/Vulkan/VkInitializers.h"

namespace Gaia
{
    Renderer::Renderer(void* window)
    {
        renderContext_ = std::make_unique<VulkanContext>(window);

        ShaderModuleDesc smVertexDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/vert_shader.spv", Stage_Vert);
        ShaderModuleDesc smFragDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/frag_shader.spv", Stage_Frag);
        vertexShaderModule = renderContext_->createShaderModule(smVertexDesc);
        fragmentShaderModule = renderContext_->createShaderModule(smFragDesc);

        RenderPipelineDesc desc{
            .topology = Topology_Triangle,
            .smVertex = vertexShaderModule,
            .smFragment = fragmentShaderModule,
            .depthFormat = Format_Z_F32,
            .cullMode = CullMode_None,
            .windingMode = WindingMode_CCW,
            .polygonMode = PolygonMode_Fill,
        };

        desc.colorAttachments[0].format = Format_RGBA_SRGB8;
        desc.colorAttachments[0].blendEnabled = false;
        desc.colorAttachments[0].alphaBlendOp = BlendOp_Add;
        desc.colorAttachments[0].rgbBlendOp = BlendOp_Add;
        desc.colorAttachments[0].srcAlphaBlendFactor = BlendFactor_One;
        desc.colorAttachments[0].dstAlphaBlendFactor = BlendFactor_One;
        desc.colorAttachments[0].srcRGBBlendFactor = BlendFactor_SrcAlpha;
        desc.colorAttachments[0].dstRGBBlendFactor = BlendFactor_OneMinusSrcAlpha;

        renderPipeline = renderContext_->createRenderPipeline(desc);
    }
    Renderer::~Renderer()
    {
        
        renderContext_.release();
        
    }
    std::shared_ptr<Renderer> Renderer::create(void* window)
    {
        GAIA_ASSERT(window, "Pass a valid window pointer");

        return std::make_shared<Renderer>(window);
    }

    void Renderer::update()
    {
    }

    void Renderer::windowResize(uint32_t width, uint32_t height)
    {
    }

    void Renderer::render()
    {
        VulkanContext& vulkanContext = *(VulkanContext*)renderContext_.get();
       TextureHandle swapchainTexture = vulkanContext.getCurrentSwapChainTexture();

        //begin recording
        const VulkanImmediateCommands::CommandBufferWrapper& wraper = vulkanContext.immediateCommands_->acquire();
        
        VulkanImage* swapchainImage = vulkanContext.texturesPool_.get(swapchainTexture);
        VkImageSubresourceRange subRange{
            .aspectMask = swapchainImage->getImageAspectFlags(),
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        swapchainImage->transitionLayout(wraper.cmdBuffer_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, subRange);
        VkClearValue clearVal{};
        clearVal.color = VkClearColorValue{ 1.0,1.0,0.0,1.0 };

        VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(swapchainImage->imageView_, &clearVal, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingInfo rInfo = vkinit::rendering_info({ vulkanContext.swapchain_->width_, vulkanContext.swapchain_->height_ }, &colorAttachment, nullptr);
        
        VkViewport viewport{};
        viewport.width = vulkanContext.swapchain_->width_;
        viewport.height = vulkanContext.swapchain_->height_;
        viewport.minDepth = 0.0;
        viewport.maxDepth = 1.0;

        VkRect2D scissor{};
        scissor.extent.width = vulkanContext.swapchain_->width_;
        scissor.extent.height = vulkanContext.swapchain_->height_;

        VkPipeline pipeline = vulkanContext.getPipeline(renderPipeline);

        vkCmdBeginRendering(wraper.cmdBuffer_, &rInfo);
        vkCmdBindPipeline(wraper.cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdSetViewport(wraper.cmdBuffer_, 0, 1, &viewport);
        vkCmdSetScissor(wraper.cmdBuffer_, 0, 1, &scissor);
        vkCmdDraw(wraper.cmdBuffer_, 3, 1, 0, 0);
        vkCmdEndRendering(wraper.cmdBuffer_);

        //submit
        vulkanContext.submit(wraper, swapchainTexture);
    }

}
