#include "pch.h"
#include "Renderer.h"
#include "Gaia/Scene/Scene.h"
#include "Gaia/Renderer/Vulkan/VulkanClasses.h"

namespace Gaia
{
    Renderer::Renderer(void* window, Scene& scene)
    {
        renderContext_ = std::make_unique<VulkanContext>(window);

        createGpuMeshTexturesAndBuffers(scene);

        SamplerStateDesc samplerDesc{
            .minFilter = SamplerFilter_Linear,
            .magFilter = SamplerFilter_Linear,
            .mipMap = SamplerMip_Linear,
            .maxAnisotropic = 16,
        };

        imageSampler = renderContext_->createSampler(samplerDesc);

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

        
        std::vector<DescriptorSetLayoutDesc> layoutDesc{
            DescriptorSetLayoutDesc{
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_UniformBuffer,
                .shaderStage = Stage_Vert | Stage_Frag,
                .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(mvpBuffer),
            },
             DescriptorSetLayoutDesc{
                .binding = 1,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_StorageBuffer,
                .shaderStage = Stage_Vert | Stage_Frag,
                .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(transformsBuffer),
            }
        };
         mvpMatrixDescriptorSetLayout = renderContext_->createDescriptorSetLayout(layoutDesc);

         std::vector<DescriptorSetLayoutDesc> meshLayoutDesc{
             DescriptorSetLayoutDesc{
             .binding = 0,
             .descriptorCount = 1,
             .descriptorType = DescriptorType_StorageBuffer,
             .shaderStage = Stage_Frag,
             .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(materialsBuffer),
             },
             DescriptorSetLayoutDesc{
             .binding = 1,
             .descriptorCount = static_cast<uint32_t>(glTfTextures.size()),
             .descriptorType = DescriptorType_CombinedImageSampler,
             .shaderStage = Stage_Frag,
             .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(glTfTextures),
             .sampler = imageSampler,
             }
         };
        meshDescriptorSet = renderContext_->createDescriptorSetLayout(meshLayoutDesc);

        ShaderModuleDesc smVertexDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/vert_shader.spv", Stage_Vert);
        ShaderModuleDesc smFragDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/frag_shader.spv", Stage_Frag);
        vertexShaderModule = renderContext_->createShaderModule(smVertexDesc);
        fragmentShaderModule = renderContext_->createShaderModule(smFragDesc);

        RenderPipelineDesc desc{
            .topology = Topology_Triangle,
            .smVertex = vertexShaderModule,
            .smFragment = fragmentShaderModule,
            .depthFormat = Format_Z_F32,
            .cullMode = CullMode_Back,
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
        desc.descriptorSetLayout[1] = meshDescriptorSet;

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
        vi.attributes[4].format = Format_R_UI32;
        vi.attributes[4].location = 4;
        vi.attributes[4].offset = offsetof(LoadMesh::VertexAttributes, materialId);
       
        vi.attributes[5].binding = 0;
        vi.attributes[5].format = Format_R_UI32;
        vi.attributes[5].location = 5;
        vi.attributes[5].offset = offsetof(LoadMesh::VertexAttributes, meshId);

        vi.inputBindings[0].stride = sizeof(LoadMesh::VertexAttributes);
        desc.vertexInput = vi;

        renderPipeline = renderContext_->createRenderPipeline(desc);
    }
    Renderer::~Renderer()
    {
        renderContext_.release();
    }
    void Renderer::createGpuMeshTexturesAndBuffers(Scene& scene)
    {
        LoadMesh& mesh = scene.getMesh();

        //lamda function that constructs a texture handle and copies the gltf texture data to there
        auto textureFunction = [&](Texture& gltfTexture, Format _textureFormat) {
            Texture& texture = gltfTexture;
            TextureDesc Desc{
                .type = TextureType_2D,
                .format = _textureFormat,
                .dimensions = {(uint32_t)texture.width, (uint32_t)texture.height, 1},
                .usage = TextureUsageBits_Sampled,
                .numMipLevels = 1,
                .storage = StorageType_Device,
                .generateMipmaps = false,
            };
            glTfTextures.push_back(renderContext_->createTexture(Desc));
            Holder<TextureHandle>& _textureHandle = glTfTextures.back();

            BufferDesc stagingBufferDesc{
                .usage_type = BufferUsageBits_Storage,
                .storage_type = StorageType_HostVisible,
                .size = texture.textureData.size(),
            };
            Holder<BufferHandle> stagingBufferHandle = renderContext_->createBuffer(stagingBufferDesc);

            //copy the texture data to the gpu texture
            {
                ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
                cmdBuffer.copyBuffer(stagingBufferHandle, texture.textureData.data(), texture.textureData.size());
                cmdBuffer.cmdTransitionImageLayout(_textureHandle, ImageLayout_TRANSFER_DST_OPTIMAL);
                cmdBuffer.cmdCopyBufferToImage(stagingBufferHandle, _textureHandle);
                cmdBuffer.cmdTransitionImageLayout(_textureHandle, ImageLayout_READ_ONLY_OPTIMAL);
                renderContext_->submit(cmdBuffer);
            }
            /* texture.textureData.clear();
             texture.textureData.shrink_to_fit();*/
            };

        int numGpuTextures = 0;
        for (int i = 0; i < mesh.pbrMaterials.size(); i++)
        {
            Material& pbrMaterial = mesh.pbrMaterials[i];

            //albedo texture
            if (pbrMaterial.baseColorTexture != -1)
            {
                textureFunction(
                    mesh.gltfTextures[pbrMaterial.baseColorTexture],
                    Format_RGBA_SRGB8);
                pbrMaterial.baseColorTexture = numGpuTextures;
                numGpuTextures++;
            }
            //normal Texture
            if (pbrMaterial.normalTexture != -1)
            {
                textureFunction(
                    mesh.gltfTextures[pbrMaterial.normalTexture],
                    Format_RGBA_UN8);
                pbrMaterial.normalTexture = numGpuTextures;
                numGpuTextures++;
            }
            //metallic roughness texture
            if (pbrMaterial.metallicRoughnessTexture != -1)
            {
                textureFunction(
                    mesh.gltfTextures[pbrMaterial.metallicRoughnessTexture],
                    Format_RGBA_UN8);
                pbrMaterial.metallicRoughnessTexture = numGpuTextures;
                numGpuTextures++;
            }
        }
        BufferDesc matBufferDesc{
            .usage_type = BufferUsageBits_Storage,
            .storage_type = StorageType_Device,
            .size = mesh.pbrMaterials.size() * sizeof(Material),
        };
        materialsBuffer = renderContext_->createBuffer(matBufferDesc);

        matBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> materialsBufferStaging = renderContext_->createBuffer(matBufferDesc);

        BufferDesc transformBufferDesc{
            .usage_type = BufferUsageBits_Storage,
            .storage_type = StorageType_Device,
            .size = mesh.transforms.size() * sizeof(glm::mat4),
        };
        transformsBuffer = renderContext_->createBuffer(transformBufferDesc);

        transformBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> transformBufferStaging = renderContext_->createBuffer(transformBufferDesc);

        //copy the staging buffers to device visible buffers
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(materialsBufferStaging, mesh.pbrMaterials.data(), matBufferDesc.size);
            cmdBuffer.copyBuffer(transformBufferStaging, mesh.transforms.data(), transformBufferDesc.size);
            cmdBuffer.cmdCopyBufferToBuffer(materialsBufferStaging, materialsBuffer);
            cmdBuffer.cmdCopyBufferToBuffer(transformBufferStaging, transformsBuffer);
            renderContext_->submit(cmdBuffer);
        }
    }
    std::shared_ptr<Renderer> Renderer::create(void* window, Scene& scene)
    {
        GAIA_ASSERT(window, "Pass a valid window pointer");

        return std::make_shared<Renderer>(window, scene);
    }

    void Renderer::createStaticBuffers(Scene& scene) 
    {
        LoadMesh& mesh = scene.getMesh();
        vertexBuffer.resize(mesh.m_subMeshes.size());
        indexBuffer.resize(mesh.m_subMeshes.size());

        for (int k = 0; k < mesh.m_subMeshes.size(); k++)
        {
            SubMesh& subMesh = mesh.m_subMeshes[k];
            std::vector<LoadMesh::VertexAttributes> buffer(mesh.m_subMeshes[k].Vertices.size());
            for (int i = 0; i < subMesh.Vertices.size(); i++)
            {
                glm::vec3 transformed_normals = (subMesh.Normal[i]);//re-orienting the normals (do not include translation as normals only needs to be orinted)
                glm::vec3 transformed_tangents = (subMesh.Tangent[i]);
                glm::vec3 transformed_binormals = (subMesh.BiTangent[i]);
                buffer[i] = (LoadMesh::VertexAttributes(glm::vec4(subMesh.Vertices[i], 1.0), subMesh.TexCoord[i], transformed_normals, transformed_tangents,subMesh.m_MaterialID[i], subMesh.meshIndices[i]));
            }

            BufferDesc vertexBufferDesc{
                .usage_type = BufferUsageBits_Vertex,
                .storage_type = StorageType_Device,
                .size = buffer.size() * sizeof(LoadMesh::VertexAttributes),
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

            numIndicesPerMesh.push_back(mesh.m_subMeshes[k].indices.size());

            //copy the staging buffers to device visible buffers
            {
                ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
                cmdBuffer.copyBuffer(vertexBufferStaging, buffer.data(), vertexBufferDesc.size);
                cmdBuffer.copyBuffer(indexbufferStaging, mesh.m_subMeshes[k].indices.data(), indexBufferDesc.size);
                cmdBuffer.cmdCopyBufferToBuffer(vertexBufferStaging, vertexBuffer[k]);
                cmdBuffer.cmdCopyBufferToBuffer(indexbufferStaging, indexBuffer[k]);
                renderContext_->submit(cmdBuffer);
            }

        }

        mesh.clear();
    }
    void Renderer::update(Scene& scene)
    {
        LoadMesh& mesh = scene.getMesh();

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
       TextureHandle swapchainImageHandle = renderContext_->getCurrentSwapChainTexture();
       ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
       cmdBuffer.cmdTransitionImageLayout(swapchainImageHandle, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
       cmdBuffer.cmdTransitionImageLayout(depthAttachment, ImageLayout_DEPTH_ATTACHMENT_OPTIMAL);
       ClearValue clearVal = {
        .colorValue = {0.3,0.3,0.3,1.0},
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
       cmdBuffer.cmdBindGraphicsDescriptorSets(0, renderPipeline, {mvpMatrixDescriptorSetLayout, meshDescriptorSet});
       for (int i = 0; i < vertexBuffer.size(); i++)
       {
           cmdBuffer.cmdBindVertexBuffer(0, vertexBuffer[i], 0);
           cmdBuffer.cmdBindIndexBuffer(indexBuffer[i], IndexFormat_U32, 0);

           cmdBuffer.cmdDrawIndexed(numIndicesPerMesh[i], 1, 0, 0, 0);
       }
       cmdBuffer.cmdEndRendering();
       renderContext_->submit(cmdBuffer, swapchainImageHandle);
    }

}
