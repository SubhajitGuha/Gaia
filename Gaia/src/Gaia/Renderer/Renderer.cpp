#include "pch.h"
#include "Renderer.h"
#include "Gaia/Scene/Scene.h"
#include "Gaia/Renderer/Vulkan/VulkanClasses.h"

namespace Gaia
{
    Renderer::Renderer(void* window)
    {
        renderContext_ = std::make_unique<VulkanContext>(window);

        auto windowSize = renderContext_->getWindowSize();
        TextureDesc depthTexDesc{
            .type = TextureType_2D,
            .format = Format_Z_F32,
            .dimensions = {windowSize.first, windowSize.second, 1},
            .usage = TextureUsageBits_Attachment,
            .storage = StorageType_Device,
        };

        depthAttachment = renderContext_->createTexture(depthTexDesc);
        BufferDesc bufDesc = {
            .usage_type = BufferUsageBits_Uniform,
            .storage_type = StorageType_Device,
            .size = sizeof(MVPMatrices),
        };
        mvpBuffer = renderContext_->createBuffer(bufDesc);

        BufferDesc bufDesc2 = {
           .usage_type = BufferUsageBits_Uniform,
           .storage_type = StorageType_HostVisible,
           .size = sizeof(MVPMatrices),
        };
        mvpBufferStaging = renderContext_->createBuffer(bufDesc2);

        std::vector<DescriptorSetLayoutDesc> layoutDesc;
        layoutDesc.push_back(DescriptorSetLayoutDesc{
            .binding = 0,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_UniformBuffer,
            .shaderStage = Stage_Vert | Stage_Frag,
            .buffer = mvpBuffer,
            });
        mvpMatrixDescriptorSetLayout = renderContext_->createDescriptorSetLayout(layoutDesc);

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
        desc.colorAttachments[0].blendEnabled = true;
        desc.colorAttachments[0].alphaBlendOp = BlendOp_Add;
        desc.colorAttachments[0].rgbBlendOp = BlendOp_Add;
        desc.colorAttachments[0].srcAlphaBlendFactor = BlendFactor_One;
        desc.colorAttachments[0].dstAlphaBlendFactor = BlendFactor_One;
        desc.colorAttachments[0].srcRGBBlendFactor = BlendFactor_SrcAlpha;
        desc.colorAttachments[0].dstRGBBlendFactor = BlendFactor_OneMinusSrcAlpha;
        desc.descriptorSetLayout[0] = mvpMatrixDescriptorSetLayout;

        VertexInput vi{};
        vi.attributes[0].binding = 0;
        vi.attributes[0].format = Format_RGBA_F32;
        vi.attributes[0].location = 0;
        vi.attributes[0].offset = offsetof(LoadMesh::VertexAttributes, Position);

        vi.attributes[1].binding = 0;
        vi.attributes[1].format = Format_RG_F32;
        vi.attributes[1].location = 1;
        vi.attributes[1].offset = offsetof(LoadMesh::VertexAttributes, TextureCoordinate);

        vi.attributes[2].binding = 0;
        vi.attributes[2].format = Format_RGB_F32;
        vi.attributes[2].location = 2;
        vi.attributes[2].offset = offsetof(LoadMesh::VertexAttributes, Normal);

        vi.attributes[3].binding = 0;
        vi.attributes[3].format = Format_RGB_F32;
        vi.attributes[3].location = 3;
        vi.attributes[3].offset = offsetof(LoadMesh::VertexAttributes, Tangent);

        vi.attributes[4].binding = 0;
        vi.attributes[4].format = Format_RGB_F32;
        vi.attributes[4].location = 4;
        vi.attributes[4].offset = offsetof(LoadMesh::VertexAttributes, BiNormal);

        vi.inputBindings[0].stride = sizeof(LoadMesh::VertexAttributes);
        desc.vertexInput = vi;

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

    void Renderer::createStaticBuffers(Scene& scene) 
    {
        LoadMesh& mesh = scene.getMesh();
        vertexBuffer.resize(mesh.m_subMeshes.size());
        indexBuffer.resize(mesh.m_subMeshes.size());

        for (int k = 0; k < mesh.m_subMeshes.size(); k++)
        {
            std::vector<LoadMesh::VertexAttributes> buffer(mesh.m_subMeshes[k].Vertices.size());
            for (int i = 0; i < mesh.m_subMeshes[k].Vertices.size(); i++)
            {
                glm::vec3 transformed_normals = (mesh.m_subMeshes[k].Normal[i]);//re-orienting the normals (do not include translation as normals only needs to be orinted)
                glm::vec3 transformed_tangents = (mesh.m_subMeshes[k].Tangent[i]);
                glm::vec3 transformed_binormals = (mesh.m_subMeshes[k].BiTangent[i]);
                buffer[i] = (LoadMesh::VertexAttributes(glm::vec4(mesh.m_subMeshes[k].Vertices[i], 1.0), mesh.m_subMeshes[k].TexCoord[i], transformed_normals, transformed_tangents, transformed_binormals));
            }

            BufferDesc vertexBufferDesc{
                .usage_type = BufferUsageBits_Vertex,
                .storage_type = StorageType_Device,
                .size = buffer.size()*sizeof(LoadMesh::VertexAttributes),
            };
            vertexBuffer[k] = renderContext_->createBuffer(vertexBufferDesc);

            BufferDesc indexBufferDesc
            {
                .usage_type = BufferUsageBits_Index,
                .storage_type = StorageType_Device,
                .size = mesh.m_subMeshes[k].indices.size() * sizeof(uint32_t),
            };
            indexBuffer[k] = renderContext_->createBuffer(indexBufferDesc);

            //create temporary staging buffers
            vertexBufferDesc.storage_type = StorageType_HostVisible;
            Holder<BufferHandle> vertexBufferStaging = renderContext_->createBuffer(vertexBufferDesc);

            indexBufferDesc.storage_type = StorageType_HostVisible;
            Holder<BufferHandle> indexbufferStaging = renderContext_->createBuffer(indexBufferDesc);

            //copy the staging buffers to device visible buffers
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(vertexBufferStaging, buffer.data(), vertexBufferDesc.size);
            cmdBuffer.copyBuffer(indexbufferStaging, mesh.m_subMeshes[k].indices.data(), indexBufferDesc.size);
            cmdBuffer.cmdCopyBufferToBuffer(vertexBufferStaging, vertexBuffer[k]);
            cmdBuffer.cmdCopyBufferToBuffer(indexbufferStaging, indexBuffer[k]);
            renderContext_->submit(cmdBuffer);
        }
    }
    void Renderer::update(Scene& scene)
    {
       EditorCamera& camera = scene.getMainCamera();
       mvpData.view = camera.GetViewMatrix();
       mvpData.projection = camera.GetProjectionMatrix();

      ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
      cmdBuffer.copyBuffer(mvpBufferStaging, &mvpData, sizeof(MVPMatrices));
      cmdBuffer.cmdCopyBufferToBuffer(mvpBufferStaging, mvpBuffer);
      renderContext_->submit(cmdBuffer);
    }

    void Renderer::windowResize(uint32_t width, uint32_t height)
    {
    }

    void Renderer::render(Scene& scene)
    {
       LoadMesh& mesh = scene.getMesh();
       TextureHandle swapchainImageHandle = renderContext_->getCurrentSwapChainTexture();
       ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
       cmdBuffer.cmdTransitionImageLayout(swapchainImageHandle, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
       cmdBuffer.cmdTransitionImageLayout(depthAttachment, ImageLayout_DEPTH_ATTACHMENT_OPTIMAL);
       ClearValue clearVal = {
        .colorValue = {1.0,1.0,0.0,1.0},
        .depthClearValue = 1.0,
       };
       cmdBuffer.cmdBeginRendering(swapchainImageHandle, depthAttachment, &clearVal);
       cmdBuffer.cmdBindGraphicsPipeline(renderPipeline);
       std::pair<uint32_t, uint32_t> windowDimensions = renderContext_->getWindowSize();
       cmdBuffer.cmdSetViewport(Viewport{
           .width = (float)windowDimensions.first,
           .height = (float)windowDimensions.second,
           .minDepth = 0.0,
           .maxDepth = 1.0 });
       cmdBuffer.cmdSetScissor(Scissor{
           .width = windowDimensions.first,
           .height = windowDimensions.second,
           });
       cmdBuffer.cmdBindGraphicsDescriptorSets(0, renderPipeline, {mvpMatrixDescriptorSetLayout});
       for (int i = 0; i < vertexBuffer.size(); i++)
       {
           cmdBuffer.cmdBindVertexBuffer(0, vertexBuffer[i], 0);
           cmdBuffer.cmdBindIndexBuffer(indexBuffer[i], IndexFormat_U32, 0);

           cmdBuffer.cmdDrawIndexed(mesh.m_subMeshes[i].indices.size(), 1, 0, 0, 0);
       }
       cmdBuffer.cmdEndRendering();
       renderContext_->submit(cmdBuffer, swapchainImageHandle);
    }

}
