#include "pch.h"
#include "Renderer.h"
#include "Gaia/Scene/Scene.h"
#include "Gaia/Renderer/Vulkan/VulkanClasses.h"
#include "Shadows.h"
#include "ddgi.h"
#include "ddgi.h"

#include "Gaia/Input.h"
#include "Gaia/Application.h"

namespace fs = std::filesystem;
namespace Gaia
{
    bool Renderer::isFirstFrame = true;
    VertexInput Renderer::vertexInput = VertexInput{};

    void Renderer::onFirstFrame(Scene& scene)
    {
        //GBuffer render pipeline
        {
            ShaderModuleDesc smVertexDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/g_buffer.vert.spv", Stage_Vert);
            ShaderModuleDesc smFragDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/g_buffer.frag.spv", Stage_Frag);
            vertexShaderModule = renderContext_->createShaderModule(smVertexDesc);
            fragmentShaderModule = renderContext_->createShaderModule(smFragDesc);

            RenderPipelineDesc rps{
                .topology = Topology_Triangle,
                .smVertex = vertexShaderModule,
                .smFragment = fragmentShaderModule,
                .depthFormat = Format_Z_F32,
                .cullMode = CullMode_Back,
                .windingMode = WindingMode_CCW,
                .polygonMode = PolygonMode_Fill,
            };

            //albedo
            rps.colorAttachments[0].format = Format_RGBA_UN8;
            rps.colorAttachments[0].blendEnabled = false;
            rps.colorAttachments[0].alphaBlendOp = BlendOp_Add;
            rps.colorAttachments[0].rgbBlendOp = BlendOp_Add;
            rps.colorAttachments[0].srcAlphaBlendFactor = BlendFactor_One;
            rps.colorAttachments[0].dstAlphaBlendFactor = BlendFactor_One;
            rps.colorAttachments[0].srcRGBBlendFactor = BlendFactor_SrcAlpha;
            rps.colorAttachments[0].dstRGBBlendFactor = BlendFactor_OneMinusSrcAlpha;

            //metallic roughness
            rps.colorAttachments[1].format = Format_RGBA_UN8;
            rps.colorAttachments[1].blendEnabled = false;

            //normal
            rps.colorAttachments[2].format = Format_RGBA_F16;
            rps.colorAttachments[2].blendEnabled = false;


            rps.descriptorSetLayout[0] = mvpMatrixDescriptorSetLayout;
            rps.descriptorSetLayout[1] = meshDescriptorSet;

            rps.vertexInput = vertexInput;

            renderPipelineGBuffer = renderContext_->createRenderPipeline(rps);
        }


        //gi render pipeline
        {
            ShaderModuleDesc smGIVertexDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/global_illumination.vert.spv", Stage_Vert);
            ShaderModuleDesc smGIFragDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/global_illumination.frag.spv", Stage_Frag);
            Holder<ShaderModuleHandle> giVertexShaderModule = renderContext_->createShaderModule(smGIVertexDesc);
            Holder<ShaderModuleHandle> giFragmentShaderModule = renderContext_->createShaderModule(smGIFragDesc);

            RenderPipelineDesc giRps{
             .topology = Topology_Triangle,
             .smVertex = giVertexShaderModule,
             .smFragment = giFragmentShaderModule,
             .depthFormat = Format_Invalid,
             .cullMode = CullMode_None,
             .windingMode = WindingMode_CCW,
             .polygonMode = PolygonMode_Fill,
            };
            giRps.colorAttachments[0].format = Format_RGBA_UN8;
            giRps.colorAttachments[0].blendEnabled = false;

            giRps.descriptorSetLayout[0] = mvpMatrixDescriptorSetLayout;
            giRps.descriptorSetLayout[1] = gBufferDescSetLayout;
            giRps.descriptorSetLayout[2] = ddgi_->getProbeDSL(); //get the DDGI probe irradiance and depth images DSL
            giRps.descriptorSetLayout[3] = ddgi_->getDDGIParametersDSL();

            renderPipelineGI = renderContext_->createRenderPipeline(giRps);
        }

        //deferred render pipeline
        {
            ShaderModuleDesc smVertexDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/deferred.vert.spv", Stage_Vert);
            ShaderModuleDesc smFragDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/deferred.frag.spv", Stage_Frag);
            Holder<ShaderModuleHandle> giVertexShaderModule = renderContext_->createShaderModule(smVertexDesc);
            Holder<ShaderModuleHandle> giFragmentShaderModule = renderContext_->createShaderModule(smFragDesc);

            RenderPipelineDesc deffRpDesc{
            .topology = Topology_Triangle,
            .smVertex = giVertexShaderModule,
            .smFragment = giFragmentShaderModule,
            .depthFormat = Format_Invalid,
            .cullMode = CullMode_None,
            .windingMode = WindingMode_CCW,
            .polygonMode = PolygonMode_Fill,
            };
            deffRpDesc.colorAttachments[0].format = Format_RGBA_SRGB8;
            deffRpDesc.colorAttachments[0].blendEnabled = false;

            deffRpDesc.descriptorSetLayout[0] = mvpMatrixDescriptorSetLayout;
            deffRpDesc.descriptorSetLayout[1] = shadowDescSetLayout;
            deffRpDesc.descriptorSetLayout[2] = gBufferDescSetLayout; //get the DDGI probe irradiance and depth images DSL
            deffRpDesc.descriptorSetLayout[3] = giOutputDescSetLayout;

            renderPipelineDeferred = renderContext_->createRenderPipeline(deffRpDesc);
        }
    }

    Renderer::Renderer(void* window, Scene& scene)
    {
        //set up the vertex attributes
        vertexInput.attributes[0].binding = 0;
        vertexInput.attributes[0].format = Format_RGBA_F32;
        vertexInput.attributes[0].location = 0;
        vertexInput.attributes[0].offset = offsetof(LoadMesh::VertexAttributes, Position);

        vertexInput.attributes[1].binding = 0;
        vertexInput.attributes[1].format = Format_RG_F32;
        vertexInput.attributes[1].location = 1;
        vertexInput.attributes[1].offset = offsetof(LoadMesh::VertexAttributes, TextureCoordinate);

        vertexInput.attributes[2].binding = 0;
        vertexInput.attributes[2].format = Format_RGB_F32;
        vertexInput.attributes[2].location = 2;
        vertexInput.attributes[2].offset = offsetof(LoadMesh::VertexAttributes, Normal);

        vertexInput.attributes[3].binding = 0;
        vertexInput.attributes[3].format = Format_RGB_F32;
        vertexInput.attributes[3].location = 3;
        vertexInput.attributes[3].offset = offsetof(LoadMesh::VertexAttributes, Tangent);

        vertexInput.attributes[4].binding = 0;
        vertexInput.attributes[4].format = Format_R_UI32;
        vertexInput.attributes[4].location = 4;
        vertexInput.attributes[4].offset = offsetof(LoadMesh::VertexAttributes, materialId);

        vertexInput.attributes[5].binding = 0;
        vertexInput.attributes[5].format = Format_R_UI32;
        vertexInput.attributes[5].location = 5;
        vertexInput.attributes[5].offset = offsetof(LoadMesh::VertexAttributes, meshId);

        vertexInput.inputBindings[0].stride = sizeof(LoadMesh::VertexAttributes);

        renderContext_ = std::make_unique<VulkanContext>(window);

        auto windowSize = renderContext_->getWindowSize();

        TextureDesc gBuffRtDesc{
            .type = TextureType_2D,
            .format = Format_RGBA_UN8,
            .dimensions = {windowSize.first, windowSize.second, 1},
            .usage = TextureUsageBits_Attachment | TextureUsageBits_Sampled,
            .storage = StorageType_Device,
        };
        giTexture = renderContext_->createTexture(gBuffRtDesc);
        gBufferAlbedo = renderContext_->createTexture(gBuffRtDesc);
        gBufferMetallicRoughness = renderContext_->createTexture(gBuffRtDesc);
        gBuffRtDesc.format = Format_RGBA_F16;
        gBufferNormal = renderContext_->createTexture(gBuffRtDesc);

        TextureDesc rtDesc{
           .type = TextureType_2D,
           .format = Format_RGBA_SRGB8,
           .dimensions = {windowSize.first, windowSize.second, 1},
           .usage = TextureUsageBits_Attachment,
           .storage = StorageType_Device,
        };
        renderTarget_ = renderContext_->createTexture(rtDesc);

        rayTraceOutput = { static_cast<uint32_t>(windowSize.first * rayTraceOutputScale) , static_cast<uint32_t>(windowSize.second * rayTraceOutputScale)};
        
        TextureDesc rayTracingTexDesc{
            .type = TextureType_2D,
            .format = Format_RGBA_F32,
            .dimensions = {rayTraceOutput.first, rayTraceOutput.second, 1},
            .usage = TextureUsageBits_Storage | TextureUsageBits_Sampled,
            .storage = StorageType_Device,
        };
        rtOutputTexture = renderContext_->createTexture(rayTracingTexDesc);

        //change the image layout to general
        {
            ICommandBuffer& cmdBuf = renderContext_->acquireCommandBuffer();
            cmdBuf.cmdTransitionImageLayout(rtOutputTexture, ImageLayout_GENERAL);
            renderContext_->submit(cmdBuf);
        }

        createGpuMeshTexturesAndBuffers(scene);
        
        //create vertex, index buffers and build the acceleration structure
        createStaticBuffers(scene);

        SamplerStateDesc samplerDesc{
            .minFilter = SamplerFilter_Linear,
            .magFilter = SamplerFilter_Linear,
            .mipMap = SamplerMip_Linear,
            .maxAnisotropic = 16,
        };

        imageSampler = renderContext_->createSampler(samplerDesc);

        SamplerStateDesc shadowSampler{
            .wrapU = SamplerWrap_ClampBorder,
            .wrapV = SamplerWrap_ClampBorder,
            .wrapW = SamplerWrap_ClampBorder,
            .depthCompareOp = CompareOp_LessEqual,
            .depthCompareEnabled = false ,
        };
        shadowImageSampler = renderContext_->createSampler(shadowSampler);

        TextureDesc depthTexDesc{
            .type = TextureType_2D,
            .format = Format_Z_F32,
            .dimensions = {windowSize.first, windowSize.second, 1},
            .usage = TextureUsageBits_Attachment | TextureUsageBits_Sampled,
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

        BufferDesc lightParamBufferDesc{
            .usage_type = BufferUsageBits_Uniform,
           .storage_type = StorageType_Device,
           .size = sizeof(LightParameters),
        };
        lightParameterBuffer = renderContext_->createBuffer(lightParamBufferDesc);

        lightParamBufferDesc.storage_type = StorageType_HostVisible;
        lightParameterBufferStaging = renderContext_->createBuffer(lightParamBufferDesc);

        BufferDesc lightMatrixBufferDesc{
           .usage_type = BufferUsageBits_Uniform,
          .storage_type = StorageType_Device,
          .size = sizeof(LightData) * MAX_SHADOW_CASCADES,
        };
        lightMatricesBuffer = renderContext_->createBuffer(lightMatrixBufferDesc);

        lightMatrixBufferDesc.storage_type = StorageType_HostVisible;
        lightMatricesBufferStaging = renderContext_->createBuffer(lightMatrixBufferDesc);
        

        //create descriptor sets
        std::vector<DescriptorSetLayoutDesc> layoutDesc{
            DescriptorSetLayoutDesc{
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_UniformBuffer,
                .shaderStage = Stage_Vert | Stage_Frag | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(mvpBuffer),
            },
             DescriptorSetLayoutDesc{
                .binding = 1,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_StorageBuffer,
                .shaderStage = Stage_Vert | Stage_Frag | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(transformsBuffer),
            },
            DescriptorSetLayoutDesc{
                .binding = 2,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_UniformBuffer,
                .shaderStage = Stage_Frag | Stage_Vert | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(lightParameterBuffer),
            },
           
        };
         mvpMatrixDescriptorSetLayout = renderContext_->createDescriptorSetLayout(layoutDesc);

         std::vector<DescriptorSetLayoutDesc> meshLayoutDesc{
             DescriptorSetLayoutDesc{
                 .binding = 0,
                 .descriptorCount = 1,
                 .descriptorType = DescriptorType_StorageBuffer,
                 .shaderStage = Stage_Frag | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                 .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(materialsBuffer),
             },
             DescriptorSetLayoutDesc{
                 .binding = 1,
                 .descriptorCount = static_cast<uint32_t>(glTfTextures.size()),
                 .descriptorType = DescriptorType_CombinedImageSampler,
                 .shaderStage = Stage_Frag | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                 .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(glTfTextures),
                 .sampler = imageSampler,
             },
         };
        meshDescriptorSet = renderContext_->createDescriptorSetLayout(meshLayoutDesc);

        std::vector<DescriptorSetLayoutDesc> rayTraceImageDSLDesc{
            DescriptorSetLayoutDesc{
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = DescriptorType_CombinedImageSampler,
                .shaderStage = Stage_Frag | Stage_RayGen | Stage_ClosestHit | Stage_Miss,
                .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(rtOutputTexture),
                .sampler = imageSampler,
            },
        };
        rayTraceImageDescSetLayout = renderContext_->createDescriptorSetLayout(rayTraceImageDSLDesc);

        //create the components
        ShadowDescriptor shadowDesc{
            .numCascades = 4,
            .maxShadowMapRes = 4096,
            .minShadowMapRes = 128,
        };
        shadows_ = Shadows::create(shadowDesc, this, scene);

        std::vector<DescriptorSetLayoutDesc> shadowDslDesc{
              DescriptorSetLayoutDesc{
               .binding = 0,
               .descriptorCount = 1,
               .descriptorType = DescriptorType_UniformBuffer,
               .shaderStage = Stage_Frag | Stage_Vert,
               .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(lightMatricesBuffer),
              },
              DescriptorSetLayoutDesc{
                 .binding = 1,
                 .descriptorCount = (uint32_t)shadows_->shadowCascadeTextures_.size(),
                 .descriptorType = DescriptorType_CombinedImageSampler,
                 .shaderStage = Stage_Frag,
                 .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(shadows_->shadowCascadeTextures_),
                 .sampler = shadowImageSampler,
             }
        };
        shadowDescSetLayout = renderContext_->createDescriptorSetLayout(shadowDslDesc);

        std::vector<DescriptorSetLayoutDesc> accelStructDSLDesc{
             DescriptorSetLayoutDesc{
              .binding = 0,
              .descriptorCount = 1,
              .descriptorType = DescriptorType_AccelerationStructure,
              .shaderStage = Stage_Frag | Stage_Vert | Stage_RayGen | Stage_ClosestHit,
              .accelStructure = TLAS
            },
        };
        accelStructureDescSetLayout = renderContext_->createDescriptorSetLayout(accelStructDSLDesc);
        
        std::vector<DescriptorSetLayoutDesc> geometryBufferAddressDSLDesc{
            DescriptorSetLayoutDesc{
             .binding = 0,
             .descriptorCount = 1,
             .descriptorType = DescriptorType_StorageBuffer,
             .shaderStage = Stage_Frag | Stage_Vert | Stage_RayGen | Stage_ClosestHit,
             .buffer = DescriptorSetLayoutDesc::getResource<BufferHandle>(geometryBufferAddress),
           },
           DescriptorSetLayoutDesc{
             .binding = 1,
             .descriptorCount = 1,
             .descriptorType = DescriptorType_StorageImage,
             .shaderStage = Stage_Frag | Stage_Vert | Stage_RayGen | Stage_ClosestHit,
             .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(rtOutputTexture),
           },
        };

        geometryBufferAddressDescSetLayout = renderContext_->createDescriptorSetLayout(geometryBufferAddressDSLDesc);

        //GBuffer descriptor sets
        //transition to a correct layout
        {
            ICommandBuffer& cmdBuf = renderContext_->acquireCommandBuffer();
            cmdBuf.cmdTransitionImageLayout(depthAttachment, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuf.cmdTransitionImageLayout(gBufferAlbedo, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuf.cmdTransitionImageLayout(gBufferMetallicRoughness, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuf.cmdTransitionImageLayout(gBufferNormal, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuf.cmdTransitionImageLayout(giTexture, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            renderContext_->submit(cmdBuf);
        }
        std::vector<DescriptorSetLayoutDesc> gBufferDSLDesc{
           DescriptorSetLayoutDesc{
            .binding = 0,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_CombinedImageSampler,
            .shaderStage = Stage_Frag | Stage_Vert,
            .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(depthAttachment),
            .sampler = imageSampler,
          },
           DescriptorSetLayoutDesc{
            .binding = 1,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_CombinedImageSampler,
            .shaderStage = Stage_Frag | Stage_Vert,
            .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(gBufferAlbedo),
            .sampler = imageSampler,
          },
          DescriptorSetLayoutDesc{
            .binding = 2,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_CombinedImageSampler,
            .shaderStage = Stage_Frag | Stage_Vert,
            .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(gBufferMetallicRoughness),
            .sampler = imageSampler,
          },
          DescriptorSetLayoutDesc{
            .binding = 3,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_CombinedImageSampler,
            .shaderStage = Stage_Frag | Stage_Vert,
            .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(gBufferNormal),
            .sampler = imageSampler,
          },
        };

        gBufferDescSetLayout = renderContext_->createDescriptorSetLayout(gBufferDSLDesc);

        std::vector<DescriptorSetLayoutDesc> giDSLDesc
        {
             DescriptorSetLayoutDesc{
            .binding = 0,
            .descriptorCount = 1,
            .descriptorType = DescriptorType_CombinedImageSampler,
            .shaderStage = Stage_Frag | Stage_Vert,
            .texture = DescriptorSetLayoutDesc::getResource<TextureHandle>(giTexture),
            .sampler = imageSampler,
          },
        };
        giOutputDescSetLayout = renderContext_->createDescriptorSetLayout(giDSLDesc);

        ShaderModuleDesc rayGenShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/rayGen.rgen.spv", Stage_RayGen);
        ShaderModuleDesc missShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/rayMiss.rmiss.spv", Stage_Miss);
        ShaderModuleDesc shadowMissShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/shadowMiss.rmiss.spv", Stage_Miss);
        ShaderModuleDesc closestHitShaderDesc("E:/Gaia/Gaia/src/Gaia/Renderer/Shaders/closestHit.rchit.spv", Stage_ClosestHit);

        rayGenShaderModule = renderContext_->createShaderModule(rayGenShaderDesc);
        missShaderModule = renderContext_->createShaderModule(missShaderDesc);
        shadowMissShaderModule = renderContext_->createShaderModule(shadowMissShaderDesc);
        closestHitShaderModule = renderContext_->createShaderModule(closestHitShaderDesc);

        RayTracingPipelineDesc rtpd{
            .smRayGen = rayGenShaderModule,
            .smClosestHit = closestHitShaderModule,
        };
        rtpd.smMiss[0] = missShaderModule;
        rtpd.smMiss[1] = shadowMissShaderModule;

        rtpd.descriptorSetLayout[0] = accelStructureDescSetLayout;
        rtpd.descriptorSetLayout[1] = geometryBufferAddressDescSetLayout;
        rtpd.descriptorSetLayout[2] = mvpMatrixDescriptorSetLayout;
        rtpd.descriptorSetLayout[3] = meshDescriptorSet;

        rtPipeline = renderContext_->createRayTracingPipeline(rtpd);

        ddgi_ = DDGI::create(this, scene);
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

        std::vector<LoadMesh::VertexAttributes> buffer;
        std::vector<uint32_t> indicesBuffer;
        std::vector<uint32_t> indicesBufferRT;

        int totalIndices = 0;
        for (int k = 0; k < mesh.m_subMeshes.size(); k++)
        {
            SubMesh& subMesh = mesh.m_subMeshes[k];
            for (int i = 0; i < subMesh.Vertices.size(); i++)
            {
                glm::vec3 transformed_normals = (subMesh.Normal[i]);//re-orienting the normals (do not include translation as normals only needs to be orinted)
                glm::vec3 transformed_tangents = (subMesh.Tangent[i]);
                glm::vec3 transformed_binormals = (subMesh.BiTangent[i]);
                buffer.push_back(LoadMesh::VertexAttributes(glm::vec4(subMesh.Vertices[i], 1.0), subMesh.TexCoord[i], transformed_normals, transformed_tangents,subMesh.m_MaterialID[i], subMesh.meshIndices[i]));
            }
            for (int i = 0; i < subMesh.indices.size(); i++)
            {  
                indicesBuffer.push_back(subMesh.indices[i] + totalIndices); //offset the indices
                indicesBufferRT.push_back(subMesh.indices[i]); //for RT indices I dont offset the indices
            }
            totalIndices += subMesh.Vertices.size();
        }
        BufferDesc vertexBufferDesc{
            .usage_type = BufferUsageBits_Vertex | BufferUsageBits_AccelStructBuildInputReadOnly,
            .storage_type = StorageType_Device,
            .size = buffer.size() * sizeof(LoadMesh::VertexAttributes),
        };
        vertexBuffer = renderContext_->createBuffer(vertexBufferDesc);

        BufferDesc indexBufferDesc
        {
            .usage_type = BufferUsageBits_Index | BufferUsageBits_AccelStructBuildInputReadOnly,
            .storage_type = StorageType_Device,
            .size = indicesBuffer.size() * sizeof(uint32_t),
        };
        indexBuffer = renderContext_->createBuffer(indexBufferDesc);
        indexBufferRT = renderContext_->createBuffer(indexBufferDesc); //buffer sizes are same

        //create temporary staging buffers
        vertexBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> vertexBufferStaging = renderContext_->createBuffer(vertexBufferDesc);

        indexBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> indexbufferStaging = renderContext_->createBuffer(indexBufferDesc);
        Holder<BufferHandle> indexbufferStagingRT = renderContext_->createBuffer(indexBufferDesc);

        numIndicesPerMesh = indicesBuffer.size();

        //copy the staging buffers to device visible buffers
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(vertexBufferStaging, buffer.data(), vertexBufferDesc.size);
            cmdBuffer.copyBuffer(indexbufferStaging, indicesBuffer.data(), indexBufferDesc.size);
            cmdBuffer.copyBuffer(indexbufferStagingRT, indicesBufferRT.data(), indexBufferDesc.size);

            cmdBuffer.cmdCopyBufferToBuffer(vertexBufferStaging, vertexBuffer);
            cmdBuffer.cmdCopyBufferToBuffer(indexbufferStaging, indexBuffer);
            cmdBuffer.cmdCopyBufferToBuffer(indexbufferStagingRT, indexBufferRT);
            renderContext_->submit(cmdBuffer);
        }

        //need a 3x4 transform buffer for blas build
        BufferDesc blasTransform{
            .usage_type = BufferUsageBits_AccelStructBuildInputReadOnly,
            .storage_type = StorageType_HostVisible,
            .size = sizeof(glm::mat3x4),
        };
        Holder<BufferHandle> transformBufferBLAS = renderContext_->createBuffer(blasTransform);
        glm::mat3x4 tdata(1.0);
        {
            auto& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(transformBufferBLAS, &tdata[0][0], sizeof(glm::mat3x4));
            renderContext_->submit(cmdBuffer);
        }

        std::vector<MeshGPUBufferAddress> gpuAddresses;
        //create blas for every sub meshes and also record the gpu addresses
        uint32_t offset = 0;
        uint32_t index_offset = 0;
        for (int k = 0; k < mesh.m_subMeshes.size(); k++)
        {
            SubMesh& subMesh = mesh.m_subMeshes[k];
            AccelStructDesc blasDesc{
                .type = AccelStructType_BLAS,
                .geometryType = AccelStructGeomType_Triangles,
                .geometryFlags = AccelStructGeometryFlagBits_Opaque,
                .vertexFormat = Format_RGB_F32,
                .vertexBufferAddress = renderContext_->gpuAddress(vertexBuffer, sizeof(LoadMesh::VertexAttributes) * offset),
                .vertexStride = sizeof(LoadMesh::VertexAttributes),
                .numVertices = static_cast<uint32_t>(subMesh.Vertices.size()),
                .indexFormat = IndexFormat_U32,
                .indexBufferAddress = renderContext_->gpuAddress(indexBufferRT, sizeof(uint32_t) * index_offset),
                .transformBufferAddress = renderContext_->gpuAddress(transformBufferBLAS),
                .buildrange = {.primitiveCount = static_cast<uint32_t>(subMesh.indices.size()/3) },
                .buildFlags = AccelStructBuildFlagBits_PreferFastTrace
            };
            gpuAddresses.push_back({.vertexBufferAddress = blasDesc.vertexBufferAddress, .indexBufferAddress=blasDesc.indexBufferAddress});
            BLAS.push_back(renderContext_->createAccelerationStructure(blasDesc));
            offset += subMesh.Vertices.size();
            index_offset += subMesh.indices.size();
        }

        //copy the gpuAddresses buffer
        BufferDesc gpuAddBufferDesc{
            .usage_type = BufferUsageBits_Storage,
            .storage_type = StorageType_Device,
            .size = gpuAddresses.size() * sizeof(MeshGPUBufferAddress),
        };
        geometryBufferAddress = renderContext_->createBuffer(gpuAddBufferDesc);

        gpuAddBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> geometryBufferAddStaging = renderContext_->createBuffer(gpuAddBufferDesc);

        {
            ICommandBuffer& cmdBuf = renderContext_->acquireCommandBuffer();
            cmdBuf.copyBuffer(geometryBufferAddStaging, gpuAddresses.data(), gpuAddBufferDesc.size);
            cmdBuf.cmdCopyBufferToBuffer(geometryBufferAddStaging, geometryBufferAddress);
            renderContext_->submit(cmdBuf);
        }
        
        // Use transform matrices from the glTF nodes and convert to
        std::vector<glm::mat3x4> transformMatrices;
        for (auto transform : mesh.transforms) {
            glm::mat3x4 transformMatrix{};
            auto m = glm::mat3x4(glm::transpose(transform));
            memcpy(&transformMatrix, (void*)&m, sizeof(glm::mat3x4));
            transformMatrices.push_back(transformMatrix);
        }
        std::vector<AccelStructInstance> instances;
        for (int i = 0; i < mesh.m_subMeshes.size(); i++)
        {
            SubMesh& subMesh = mesh.m_subMeshes[i];
            instances.push_back({
                .transform = transformMatrices[subMesh.meshIndices[0]],
                .flags = AccelStructInstanceFlagBits_TriangleFacingCullDisable,
                .accelerationStructureReference = renderContext_->gpuAddress(BLAS[i]),
                });
        }
        
        BufferDesc instBufferDesc{
            .usage_type = BufferUsageBits_AccelStructBuildInputReadOnly,
            .storage_type = StorageType_Device,
            .size = sizeof(AccelStructInstance) * instances.size(),
        };
        instancesBuffer = renderContext_->createBuffer(instBufferDesc);

        instBufferDesc.storage_type = StorageType_HostVisible;
        Holder<BufferHandle> instanceBufferStaging = renderContext_->createBuffer(instBufferDesc);

        //copy the instances buffer
        {
            auto& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(instanceBufferStaging, instances.data(), instBufferDesc.size);
            cmdBuffer.cmdCopyBufferToBuffer(instanceBufferStaging, instancesBuffer);
            renderContext_->submit(cmdBuffer);
        }

        AccelStructDesc tlasDesc{
             .type = AccelStructType_TLAS,
             .geometryType = AccelStructGeomType_Instances,
             .instancesBufferAddress = renderContext_->gpuAddress(instancesBuffer),
             .buildrange = {.primitiveCount = static_cast<uint32_t>(instances.size())},
             .buildFlags = AccelStructBuildFlagBits_PreferFastTrace | AccelStructBuildFlagBits_AllowUpdate,
        };
        TLAS = renderContext_->createAccelerationStructure(tlasDesc);

        mesh.clear();
    }
    void Renderer::update(Scene& scene)
    {
        if (Input::IsButtonPressed(0) || Input::IsButtonPressed(1) || Input::IsButtonPressed(2))
            Application::frameNum = 0;
        shadows_->update(scene);
        ddgi_->update(scene);
        LoadMesh& mesh = scene.getMesh();

        EditorCamera& camera = scene.getMainCamera();
        mvpData.view = camera.GetViewMatrix();
        mvpData.projection = camera.GetProjectionMatrix();
        mvpData.viewDir = camera.GetViewDirection();
        mvpData.camPos = camera.GetCameraPosition();
        mvpData.frameNumber = Application::frameNum;

        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.copyBuffer(mvpBufferStaging, &mvpData, sizeof(MVPMatrices));
            cmdBuffer.copyBuffer(lightParameterBufferStaging, &scene.lightParameter.color, sizeof(LightParameters));
            cmdBuffer.copyBuffer(lightMatricesBufferStaging, &shadows_->lightData_[0], sizeof(LightData) * MAX_SHADOW_CASCADES);

            cmdBuffer.cmdCopyBufferToBuffer(mvpBufferStaging, mvpBuffer);
            cmdBuffer.cmdCopyBufferToBuffer(lightParameterBufferStaging, lightParameterBuffer);
            cmdBuffer.cmdCopyBufferToBuffer(lightMatricesBufferStaging, lightMatricesBuffer);
            renderContext_->submit(cmdBuffer);
        }

    }

    void Renderer::windowResize(uint32_t width, uint32_t height)
    {
    }

    void Renderer::render(Scene& scene)
    {
        if (isFirstFrame)
        {
            onFirstFrame(scene);
        }
        shadows_->render(scene);
        ddgi_->render(scene);
        auto windowSize = renderContext_->getWindowSize();
        ////hardware acc ray tracing
        //{
        //    ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
        //    cmdBuffer.cmdTransitionImageLayout(rtOutputTexture, ImageLayout_GENERAL);
        //    cmdBuffer.cmdBindRayTracingPipeline(rtPipeline);
        //    cmdBuffer.cmdBindRayTracingDescriptorSets(0, rtPipeline, { accelStructureDescSetLayout, geometryBufferAddressDescSetLayout, mvpMatrixDescriptorSetLayout, meshDescriptorSet });
        //    cmdBuffer.cmdTraceRays(rayTraceOutput.first, rayTraceOutput.second, 1, rtPipeline);
        //    renderContext_->submit(cmdBuffer);
        //}
        /*if(Application::frameNum == 0)
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.cmdTransitionImageLayout(rtOutputTexture, ImageLayout_TRANSFER_SRC_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(renderTarget_, ImageLayout_TRANSFER_DST_OPTIMAL);
            cmdBuffer.cmdBlitImage(rtOutputTexture, renderTarget_);
            renderContext_->submit(cmdBuffer);
        }
        else 
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.cmdTransitionImageLayout(rtOutputTexture, ImageLayout_TRANSFER_SRC_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(renderTarget_, ImageLayout_TRANSFER_DST_OPTIMAL);
            cmdBuffer.cmdBlitImage(rtOutputTexture, renderTarget_);
            renderContext_->submit(cmdBuffer);
        }*/
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.cmdTransitionImageLayout(gBufferAlbedo, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferMetallicRoughness, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferNormal, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(depthAttachment, ImageLayout_DEPTH_ATTACHMENT_OPTIMAL);
            ClearValue clearVal = {
             .colorValue = {0.3,0.3,0.3,1.0},
             .depthClearValue = 1.0,
            };

            cmdBuffer.cmdBeginRendering({gBufferAlbedo, gBufferMetallicRoughness, gBufferNormal}, depthAttachment, &clearVal);
            cmdBuffer.cmdBindGraphicsPipeline(renderPipelineGBuffer);
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
            cmdBuffer.cmdBindGraphicsDescriptorSets(0, renderPipelineGBuffer, {
                mvpMatrixDescriptorSetLayout,
                meshDescriptorSet,
                });
            //draw the batched mesh
            {
                cmdBuffer.cmdBindVertexBuffer(0, vertexBuffer, 0);
                cmdBuffer.cmdBindIndexBuffer(indexBuffer, IndexFormat_U32, 0);

                cmdBuffer.cmdDrawIndexed(numIndicesPerMesh, 1, 0, 0, 0);
            }
            cmdBuffer.cmdEndRendering();
            renderContext_->submit(cmdBuffer);
        }

        //global illumination pass
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.cmdTransitionImageLayout(giTexture, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(ddgi_->getProbeIrradianceImage(), ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(ddgi_->getProbeDepthImage(), ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(depthAttachment, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferAlbedo, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferMetallicRoughness, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferNormal, ImageLayout_SHADER_READ_ONLY_OPTIMAL);

            ClearValue clearVal = {
             .colorValue = {1.0,1.0,0.3,1.0},
             .depthClearValue = 1.0,
            };
            cmdBuffer.cmdBeginRendering(giTexture, {}, &clearVal);
            cmdBuffer.cmdBindGraphicsPipeline(renderPipelineGI);
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
            cmdBuffer.cmdBindGraphicsDescriptorSets(0, renderPipelineGI, {
                mvpMatrixDescriptorSetLayout,
                gBufferDescSetLayout,
                ddgi_->getProbeDSL(),
                ddgi_->getDDGIParametersDSL(),
                });
            cmdBuffer.cmdDraw(3, 1, 0, 0);
            cmdBuffer.cmdEndRendering();
            renderContext_->submit(cmdBuffer);
        }
        //deferred render pass
        {
            ICommandBuffer& cmdBuffer = renderContext_->acquireCommandBuffer();
            cmdBuffer.cmdTransitionImageLayout(renderTarget_, ImageLayout_COLOR_ATTACHMENT_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(giTexture, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(depthAttachment, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferAlbedo, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferMetallicRoughness, ImageLayout_SHADER_READ_ONLY_OPTIMAL);
            cmdBuffer.cmdTransitionImageLayout(gBufferNormal, ImageLayout_SHADER_READ_ONLY_OPTIMAL);

            ClearValue clearVal = {
             .colorValue = {0.5,0.6,1.0,1.0},
             .depthClearValue = 1.0,
            };
            cmdBuffer.cmdBeginRendering(renderTarget_, {}, &clearVal);
            cmdBuffer.cmdBindGraphicsPipeline(renderPipelineDeferred);
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
            cmdBuffer.cmdBindGraphicsDescriptorSets(0, renderPipelineDeferred, {
                mvpMatrixDescriptorSetLayout,
                shadowDescSetLayout,
                gBufferDescSetLayout,
                giOutputDescSetLayout,
                });
            cmdBuffer.cmdDraw(3, 1, 0, 0);
            cmdBuffer.cmdEndRendering();
            renderContext_->submit(cmdBuffer);
        }

        if (Renderer::isFirstFrame)
            Renderer::isFirstFrame = false;
    }

}
