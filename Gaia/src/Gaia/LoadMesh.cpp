#include "pch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "LoadMesh.h"
#include "Log.h"
#include "Core.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Gaia/Renderer/Vulkan/Vulkan.h"

namespace Gaia
{ 
	LoadMesh::LoadMesh()
	{
		GlobalTransform = glm::mat4(1.0);
		this->CreateVulkanVertexDesc();
	}
	LoadMesh::LoadMesh(const std::string& Path)
	{
		GlobalTransform = glm::mat4(1.0);
		//std::filesystem::path mesh_path(Path);
		//objectName = mesh_path.stem().string();
		//m_path = (mesh_path.parent_path() / mesh_path.stem()).string() + extension; //temporary

		//if (m_LOD.size() == 0)
		//	m_LOD.push_back(this);
		this->CreateVulkanVertexDesc();
		LoadObj(Path);
	}
	LoadMesh::~LoadMesh()
	{
	}
	void LoadMesh::parse_scene_rec(tinygltf::Node& node)
	{
		if (node.mesh == -1)
			return;
		LoadVertexData(node.mesh);
		for (auto children : node.children)
		{
			parse_scene_rec(model.nodes[children]);
		}
	}
	void LoadMesh::LoadObj(const std::string& Path)
	{
		
		std::string err;
		std::string warn;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, Path);

		if (!warn.empty()) {
			GAIA_CORE_ERROR("Warn: {}", warn.c_str());
		}

		if (!err.empty()) {
			GAIA_CORE_ERROR("Err: {}", err.c_str());
		}

		if (!ret) {
			GAIA_CORE_ERROR("Failed to parse glTF");
			return;
		}

		for (auto scene : model.scenes)
		{
			for (auto node : scene.nodes)
			{

				parse_scene_rec(model.nodes[node]);
			}
		}
		/*Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(Path, aiProcess_OptimizeGraph | aiProcess_FixInfacingNormals | aiProcess_SplitLargeMeshes | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			GAIA_CORE_ERROR("ERROR::ASSIMP:: {} ", importer.GetErrorString());
			return;
		}

		ProcessMaterials(scene);
		ProcessNode(scene->mRootNode, scene);
		ProcessMesh();*/
		CreateStaticBuffers();

		m_Mesh.clear();
		m_Mesh.shrink_to_fit();
	}
	auto AssimpToGlmMatrix = [&](const aiMatrix4x4& from) {
		glm::mat4 to;
		to[0][0] = from.a1; to[0][1] = from.a2; to[0][2] = from.a3; to[0][3] = from.a4;
		to[1][0] = from.b1; to[1][1] = from.b2; to[1][2] = from.b3; to[1][3] = from.b4;
		to[2][0] = from.c1; to[2][1] = from.c2; to[2][2] = from.c3; to[2][3] = from.c4;
		to[3][0] = from.d1; to[3][1] = from.d2; to[3][2] = from.d3; to[3][3] = from.d4;

		return std::move(to);
		};
	void LoadMesh::ProcessNode(aiNode* Node, const aiScene* scene)
	{
		GlobalTransform *= AssimpToGlmMatrix(Node->mTransformation);
		for (int i = 0; i < Node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[Node->mMeshes[i]];
			m_Mesh.push_back(mesh);
		}

		for (int i = 0; i < Node->mNumChildren; i++)
		{
			ProcessNode(Node->mChildren[i], scene);
		}
	}
	void LoadMesh::ProcessMesh()
	{
		for (int i = 0; i < m_Mesh.size(); i++)
		{
			unsigned int material_ind = m_Mesh[i]->mMaterialIndex;
			m_subMeshes[material_ind].numVertices = m_Mesh[i]->mNumVertices;

			for (int k = 0; k < m_Mesh[i]->mNumVertices; k++)
			{
				aiVector3D aivertices = m_Mesh[i]->mVertices[k];
				glm::vec4 pos = GlobalTransform * glm::vec4(aivertices.x, aivertices.y, aivertices.z, 1.0);
				//Bounds mesh_bounds(glm::vec3(pos.x, pos.y, pos.z));
				//m_subMeshes[material_ind].mesh_bounds.Union(mesh_bounds);
				m_subMeshes[material_ind].Vertices.push_back({ pos.x,pos.y,pos.z });

				if (m_Mesh[i]->HasNormals()) {
					aiVector3D ainormal = m_Mesh[i]->mNormals[k];
					glm::vec4 norm = GlobalTransform * glm::vec4(ainormal.x, ainormal.y, ainormal.z, 0.0);
					m_subMeshes[material_ind].Normal.push_back({ norm.x,norm.y,norm.z });
				}

				glm::vec2 coord(0.0f);
				if (m_Mesh[i]->mTextureCoords[0])
				{
					coord.x = m_Mesh[i]->mTextureCoords[0][k].x;
					coord.y = m_Mesh[i]->mTextureCoords[0][k].y;

					m_subMeshes[material_ind].TexCoord.push_back(coord);
				}
				else
					m_subMeshes[material_ind].TexCoord.push_back(coord);

				if (m_Mesh[i]->HasTangentsAndBitangents())
				{
					aiVector3D tangent = m_Mesh[i]->mTangents[k];
					aiVector3D bitangent = m_Mesh[i]->mBitangents[k];
					glm::vec4 tan = GlobalTransform * glm::vec4(tangent.x, tangent.y, tangent.z, 0.0);
					glm::vec4 bitan = GlobalTransform * glm::vec4(bitangent.x, bitangent.y, bitangent.z, 0.0);

					m_subMeshes[material_ind].Tangent.push_back({ tan.x, tan.y, tan.z });
					m_subMeshes[material_ind].BiTangent.push_back({ bitan.x,bitan.y,bitan.z });
					//m_Mesh[i]->biTange
				}
				else
				{
					m_subMeshes[material_ind].Tangent.push_back({ 0,0,0 });
					m_subMeshes[material_ind].BiTangent.push_back({ 0,0,0 });
				}
			}
			//total_bounds.Union(m_subMeshes[material_ind].mesh_bounds);
			for (int k = 0; k < m_Mesh[i]->mNumFaces; k++)
			{
				aiFace face = m_Mesh[i]->mFaces[k];
				for (int j = 0; j < face.mNumIndices; j++)
					m_subMeshes[material_ind].indices.push_back(face.mIndices[j]);
			}
		}
	}
	void LoadMesh::ProcessMaterials(const aiScene* scene)
	{
		int NumMaterials = scene->mNumMaterials;
		m_subMeshes.resize(NumMaterials);
	}
	void LoadMesh::CalculateTangent()
	{
	}
	void LoadMesh::CreateStaticBuffers()
	{
		for (int k = 0; k < m_subMeshes.size(); k++)
		{
			std::vector<VertexAttributes> buffer(m_subMeshes[k].Vertices.size());
			//m_subMeshes[k].VertexArray = VertexArray::Create();

			for (int i = 0; i < m_subMeshes[k].Vertices.size(); i++)
			{
				glm::vec3 transformed_normals = (m_subMeshes[k].Normal[i]);//re-orienting the normals (do not include translation as normals only needs to be orinted)
				glm::vec3 transformed_tangents = (m_subMeshes[k].Tangent[i]);
				glm::vec3 transformed_binormals = (m_subMeshes[k].BiTangent[i]);
				buffer[i] = (VertexAttributes(glm::vec4(m_subMeshes[k].Vertices[i], 1.0), m_subMeshes[k].TexCoord[i], transformed_normals, transformed_tangents, transformed_binormals));
			}
			
			uint32_t buffer_size = buffer.size() * sizeof(VertexAttributes);
			m_subMeshes[k].vk_mesh_vertex_buffer.CreateBuffer(Vulkan::GetVulkanContext()->m_device, Vulkan::GetVulkanContext()->m_allocator, buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_subMeshes[k].vk_mesh_vertex_buffer.CopyBufferCPUToGPU(Vulkan::GetVulkanContext()->m_allocator, buffer.data(), buffer_size);

			uint32_t indx_buffer_size = m_subMeshes[k].indices.size() * sizeof(uint32_t);
			m_subMeshes[k].vk_mesh_index_buffer.CreateBuffer(Vulkan::GetVulkanContext()->m_device, Vulkan::GetVulkanContext()->m_allocator, indx_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
			m_subMeshes[k].vk_mesh_index_buffer.CopyBufferCPUToGPU(Vulkan::GetVulkanContext()->m_allocator, m_subMeshes[k].indices.data(), indx_buffer_size);

		}

	}
	void LoadMesh::LoadVertexData(int mesh_index)
	{
		uint32_t numPrimitives = model.meshes[mesh_index].primitives.size();
		m_subMeshes.resize(numPrimitives);

		uint32_t primitiveIndex = 0;
		for (const auto& glTFPrimitive : model.meshes[mesh_index].primitives)
		{
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;

			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* colorBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* tangentsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const uint32_t* jointsBuffer = nullptr;
				const float* weightsBuffer = nullptr;

				int jointsBufferDataType = 0;

				// Get buffer data for vertex positions
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
				{
					auto componentType =
						LoadAccessor<float>(model.accessors[glTFPrimitive.attributes.find("POSITION")->second],
							positionBuffer, &vertexCount);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}
				// Get buffer data for vertex color
				if (glTFPrimitive.attributes.find("COLOR_0") != glTFPrimitive.attributes.end())
				{
					auto componentType = LoadAccessor<float>(
						model.accessors[glTFPrimitive.attributes.find("COLOR_0")->second], colorBuffer);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
				{
					auto componentType = LoadAccessor<float>(
						model.accessors[glTFPrimitive.attributes.find("NORMAL")->second], normalsBuffer);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}
				// Get buffer data for vertex tangents
				if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
				{
					auto componentType = LoadAccessor<float>(
						model.accessors[glTFPrimitive.attributes.find("TANGENT")->second], tangentsBuffer);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
				{
					auto componentType = LoadAccessor<float>(
						model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second], texCoordsBuffer);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}

				// Get buffer data for joints
				if (glTFPrimitive.attributes.find("JOINTS_0") != glTFPrimitive.attributes.end())
				{
					jointsBufferDataType = LoadAccessor<uint32_t>(
						model.accessors[glTFPrimitive.attributes.find("JOINTS_0")->second], jointsBuffer);
					GAIA_ASSERT((jointsBufferDataType == GL_BYTE) || (jointsBufferDataType == GL_UNSIGNED_BYTE),
						"unexpected component type");
				}
				// Get buffer data for joint weights
				if (glTFPrimitive.attributes.find("WEIGHTS_0") != glTFPrimitive.attributes.end())
				{
					auto componentType = LoadAccessor<float>(
						model.accessors[glTFPrimitive.attributes.find("WEIGHTS_0")->second], weightsBuffer);
					GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				}

				// Append data to model's vertex buffer
				/*m_subMeshes[primitiveIndex].Vertices.resize(vertexCount);
				m_subMeshes[primitiveIndex].Normal.resize(vertexCount);
				m_subMeshes[primitiveIndex].Tangent.resize(vertexCount);
				m_subMeshes[primitiveIndex].BiTangent.resize(vertexCount);
				m_subMeshes[primitiveIndex].numVertices=vertexCount;*/

				for (size_t vertexIterator = 0; vertexIterator < vertexCount; ++vertexIterator)
				{
					SubMesh vertex{};
					// position
					auto position = positionBuffer ? glm::make_vec3(&positionBuffer[vertexIterator * 3]) : glm::vec3(0.0f);
					m_subMeshes[primitiveIndex].Vertices.push_back(glm::vec3(position.x, position.y, position.z));

					m_subMeshes[primitiveIndex].numVertices = vertexCount;
					// normal
					m_subMeshes[primitiveIndex].Normal.push_back(glm::normalize(
						glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[vertexIterator * 3]) : glm::vec3(0.0f))));

					// color TODO
					/*auto vertexColor = colorBuffer ? glm::make_vec3(&colorBuffer[vertexIterator * 3]) : glm::vec3(1.0f);
					vertex.m_Color = glm::vec4(vertexColor.x, vertexColor.y, vertexColor.z, 1.0f) * diffuseColor;*/

					// uv
					m_subMeshes[primitiveIndex].TexCoord.push_back(texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[vertexIterator * 2]) : glm::vec2(0.0f));

					// tangent
					glm::vec4 t = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[vertexIterator * 4]) : glm::vec4(0.0f);
					m_subMeshes[primitiveIndex].Tangent.push_back(glm::vec3(t.x, t.y, t.z) * t.w);

					//need to construct it
					m_subMeshes[primitiveIndex].BiTangent.push_back(glm::vec3(t.x, t.y, t.z) * t.w);

					// joint indices and joint weights TODO
					/*if (jointsBuffer && weightsBuffer)
					{
						switch (jointsBufferDataType)
						{
						case GL_BYTE:
						case GL_UNSIGNED_BYTE:
							vertex.m_JointIds = glm::ivec4(
								glm::make_vec4(&(reinterpret_cast<const int8_t*>(jointsBuffer)[vertexIterator * 4])));
							break;
						case GL_SHORT:
						case GL_UNSIGNED_SHORT:
							vertex.m_JointIds = glm::ivec4(
								glm::make_vec4(&(reinterpret_cast<const int16_t*>(jointsBuffer)[vertexIterator * 4])));
							break;
						case GL_INT:
						case GL_UNSIGNED_INT:
							vertex.m_JointIds = glm::ivec4(
								glm::make_vec4(&(reinterpret_cast<const int32_t*>(jointsBuffer)[vertexIterator * 4])));
							break;
						default:
							LOG_CORE_CRITICAL("data type of joints buffer not found");
							break;
						}
						vertex.m_Weights = glm::make_vec4(&weightsBuffer[vertexIterator * 4]);
					}*/
					
				}

				// calculate tangents
				if (!tangentsBuffer)
				{
					//CalculateTangents();
				}
			}
			// Indices
			{
				const uint32_t* buffer;
				uint32_t count = 0;
				auto componentType = LoadAccessor<uint32_t>(model.accessors[glTFPrimitive.indices], buffer, &count);

				//indexCount += count;

				// glTF supports different component types of indices
				switch (componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				{
					const uint32_t* buf = buffer;
					for (size_t index = 0; index < count; index++)
					{
						m_subMeshes[primitiveIndex].indices.push_back(buf[index]);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(buffer);
					for (size_t index = 0; index < count; index++)
					{
						m_subMeshes[primitiveIndex].indices.push_back(buf[index]);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				{
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer);
					for (size_t index = 0; index < count; index++)
					{
						m_subMeshes[primitiveIndex].indices.push_back(buf[index]);
					}
					break;
				}
				default:
				{
					GAIA_ASSERT(false, "unexpected component type, index component type not supported!");
					return;
				}
				}
			}

			primitiveIndex++;
		}

	}
	void LoadMesh::CreateVulkanVertexDesc()
	{
		m_vertexAttribDesc.resize(5);

		m_vertexAttribDesc[0].binding = 0;
		m_vertexAttribDesc[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		m_vertexAttribDesc[0].location = 0;
		m_vertexAttribDesc[0].offset = offsetof(VertexAttributes, Position);

		m_vertexAttribDesc[1].binding = 0;
		m_vertexAttribDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
		m_vertexAttribDesc[1].location = 1;
		m_vertexAttribDesc[1].offset = offsetof(VertexAttributes, TextureCoordinate);

		m_vertexAttribDesc[2].binding = 0;
		m_vertexAttribDesc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_vertexAttribDesc[2].location = 2;
		m_vertexAttribDesc[2].offset = offsetof(VertexAttributes, Normal);

		m_vertexAttribDesc[3].binding = 0;
		m_vertexAttribDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_vertexAttribDesc[3].location = 3;
		m_vertexAttribDesc[3].offset = offsetof(VertexAttributes, Tangent);


		m_vertexAttribDesc[4].binding = 0;
		m_vertexAttribDesc[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		m_vertexAttribDesc[4].location = 4;
		m_vertexAttribDesc[4].offset = offsetof(VertexAttributes, BiNormal);

		m_vertexBindingDesc.resize(1);
		m_vertexBindingDesc[0].binding = 0;
		m_vertexBindingDesc[0].stride = sizeof(VertexAttributes);
		m_vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}
}