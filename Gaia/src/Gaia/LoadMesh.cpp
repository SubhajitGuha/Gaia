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
#include <filesystem>

#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace fs = std::filesystem;

namespace Gaia
{ 
	LoadMesh::LoadMesh()
	{
	}
	LoadMesh::LoadMesh(const std::string& Path)
	{
		//std::filesystem::path mesh_path(Path);
		//objectName = mesh_path.stem().string();
		m_path = Path;

		//if (m_LOD.size() == 0)
		//	m_LOD.push_back(this);
		LoadObj(Path);
	}
	LoadMesh::~LoadMesh()
	{
		//clear();
	}
	void LoadMesh::parse_scene_rec(tinygltf::Node& node, glm::mat4 nodeTransform, int parentIndex)
	{
		nodeTransform *= getTransform(parentIndex);
		if (node.mesh != -1)
		{
			transforms.push_back(nodeTransform);
			//LoadTransforms(node.mesh, nodeTransform);
			LoadVertexData(node.mesh);
		}
		//TODO camera, Lights..
		for (int children : node.children)
		{
			parse_scene_rec(model.nodes[children], nodeTransform, children);
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
		LoadTextures();
		LoadMatrials();
		/*transforms.resize(model.meshes.size());
		m_subMeshes.resize(model.meshes.size());*/

		for (auto scene : model.scenes)
		{
			for (auto node : scene.nodes)
			{
				glm::mat4 nodetransform = glm::mat4(1.0);
				parse_scene_rec(model.nodes[node], nodetransform, node);
			}
		}

	}
	void LoadMesh::LoadVertexData(int mesh_index)
	{
		uint32_t numPrimitives = model.meshes[mesh_index].primitives.size();

		SubMesh subMesh;
		//A mesh can have multiple material so we also treat them as seperate meshes but we use same mesh index
		for (const auto& glTFPrimitive : model.meshes[mesh_index].primitives)
		{
			uint32_t vertexCount = 0;
			uint32_t indexCount = 0;
			//subMesh.m_MaterialID = glTFPrimitive.material;
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
				//// Get buffer data for vertex color
				//if (glTFPrimitive.attributes.find("COLOR_0") != glTFPrimitive.attributes.end())
				//{
				//	auto componentType = LoadAccessor<float>(
				//		model.accessors[glTFPrimitive.attributes.find("COLOR_0")->second], colorBuffer);
				//	GAIA_ASSERT(componentType == GL_FLOAT, "unexpected component type");
				//}
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
					subMesh.Vertices.push_back(glm::vec3(position.x, position.y, position.z));

					// normal
					subMesh.Normal.push_back(glm::normalize(
						glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[vertexIterator * 3]) : glm::vec3(0.0f))));

					// color TODO
					/*auto vertexColor = colorBuffer ? glm::make_vec3(&colorBuffer[vertexIterator * 3]) : glm::vec3(1.0f);
					vertex.m_Color = glm::vec4(vertexColor.x, vertexColor.y, vertexColor.z, 1.0f) * diffuseColor;*/

					// uv
					subMesh.TexCoord.push_back(texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[vertexIterator * 2]) : glm::vec2(0.0f));

					// tangent
					glm::vec4 t = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[vertexIterator * 4]) : glm::vec4(0.0f);
					subMesh.Tangent.push_back(glm::vec3(t.x, t.y, t.z) * t.w);

					//need to construct it
					subMesh.BiTangent.push_back(glm::vec3(t.x, t.y, t.z) * t.w);
					subMesh.m_MaterialID.push_back((uint32_t)glTFPrimitive.material);
					subMesh.meshIndices.push_back(numMeshes);
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
						subMesh.indices.push_back(buf[index]);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(buffer);
					for (size_t index = 0; index < count; index++)
					{
						subMesh.indices.push_back(buf[index]);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				{
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer);
					for (size_t index = 0; index < count; index++)
					{
						subMesh.indices.push_back(buf[index]);
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
			subMesh.numVertices = subMesh.Vertices.size();
			m_subMeshes.push_back(std::move(subMesh));
		}
		numMeshes++;

	}
	void LoadMesh::LoadTextures()
	{
		fs::path meshPath = m_path;

		fs::path parentPath = meshPath.parent_path();

		size_t numTextures = model.images.size();
		for (uint32_t imageIndex = 0; imageIndex < numTextures; ++imageIndex)
		{
			Texture texture;
			fs::path imagePath = model.images[imageIndex].uri;

			fs::path absolutePath = parentPath / imagePath;
			tinygltf::Image& glTFImage = model.images[imageIndex];

			uint8_t* buffer;
			uint64_t bufferSize;
			texture.width = glTFImage.width;
			texture.height = glTFImage.height;
			texture.num_channels = glTFImage.component;

			if (glTFImage.component == 3)
			{
				bufferSize = glTFImage.width * glTFImage.height * 4;
				std::vector<uint8_t> imageData(bufferSize, 0);
				//buffer = new uint8_t[bufferSize]{};

				buffer = (uint8_t*)imageData.data();
				uint8_t* rgba = buffer;
				uint8_t* rgb = &glTFImage.image[0];
				for (int j = 0; j < glTFImage.width * glTFImage.height; ++j)
				{
					memcpy(rgba, rgb, sizeof(uint8_t) * 3);
					rgba += 4;
					rgb += 3;
				}
			}
			else
			{
				buffer = &glTFImage.image[0];
				bufferSize = glTFImage.image.size();
			}
			texture.textureData.resize(bufferSize);
			for (int i = 0; i < bufferSize; i++)
			{
				texture.textureData[i] = buffer[i];
			}
			gltfTextures.push_back(texture);
		}
	}
	void LoadMesh::LoadMatrials()
	{
		size_t numMaterials = model.materials.size();
		uint32_t materialIndex = 0u;
		for (int i = 0; i < numMaterials; i++)
		{
			tinygltf::Material glTFMaterial = model.materials[i];
			Material material = {};
			// diffuse color aka base color factor
			// used as constant color, if no diffuse texture is provided
			// else, multiplied in the shader with each sample from the diffuse texture
			if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
			{
				material.baseColorFactor =
					glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}

			// diffuse map aka basecolor aka albedo
			if (glTFMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
			{
				int diffuseTextureIndex = glTFMaterial.pbrMetallicRoughness.baseColorTexture.index;
				tinygltf::Texture& diffuseTexture = model.textures[diffuseTextureIndex];
				material.baseColorTexture = diffuseTexture.source;
			}
			else if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
			{
				int diffuseTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
				tinygltf::Texture& diffuseTexture = model.textures[diffuseTextureIndex];
				material.baseColorTexture = diffuseTexture.source;
			}

			// normal map
			if (glTFMaterial.normalTexture.index != -1)
			{
				int normalTextureIndex = glTFMaterial.normalTexture.index;
				tinygltf::Texture& normalTexture = model.textures[normalTextureIndex];
				material.normalStrength = glTFMaterial.normalTexture.scale;
				material.normalTexture = normalTexture.source;
			}

			// constant values for roughness and metallicness
			{
				material.roughnessFactor = glTFMaterial.pbrMetallicRoughness.roughnessFactor;
				material.metallicFactor = glTFMaterial.pbrMetallicRoughness.metallicFactor;
			}

			// texture for roughness and metallicness
			if (glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
			{
				int MetallicRoughnessTextureIndex = glTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
				tinygltf::Texture& metallicRoughnessTexture = model.textures[MetallicRoughnessTextureIndex];
				material.metallicRoughnessTexture = metallicRoughnessTexture.source;
			}

			pbrMaterials.push_back(material);
		}
	}
	void LoadMesh::LoadTransforms(int nodeIndex, glm::mat4& parentTransform)
	{
		tinygltf::Node& node = model.nodes[nodeIndex];
		if (node.matrix.size() == 16)
		{
			transforms[nodeIndex] = (glm::make_mat4x4(node.matrix.data()));
		}
		else {
			glm::vec3 translation = glm::vec3(0.0);
			glm::vec3 rotation = glm::vec3(0.0);
			glm::vec3 scale = glm::vec3(1.0);

			if (node.rotation.size() == 4)
			{
				float x = node.rotation[0];
				float y = node.rotation[1];
				float z = node.rotation[2];
				float w = node.rotation[3];

				glm::vec3 convert = glm::eulerAngles(glm::quat{ w,x,y,z });
				rotation = convert;
			}
			if (node.scale.size() == 3)
			{
				scale = { node.scale[0], node.scale[1], node.scale[2] };
			}
			if (node.translation.size() == 3)
			{
				translation = { node.translation[0], node.translation[1], node.translation[2] };
			}

			transforms[nodeIndex] = (glm::translate(glm::mat4(1.0), translation) * glm::toMat4(glm::quat(rotation)) * glm::scale(glm::mat4(1.0), scale));
		}
		transforms[nodeIndex] = parentTransform * transforms[nodeIndex];
	}
	glm::mat4 LoadMesh::getTransform(int nodeIndex)
	{
		tinygltf::Node& node = model.nodes[nodeIndex];
		glm::mat4 transform;
		if (node.matrix.size() == 16)
		{
			transform = (glm::make_mat4x4(node.matrix.data()));
		}
		else {
			glm::vec3 translation = glm::vec3(0.0);
			glm::vec3 rotation = glm::vec3(0.0);
			glm::vec3 scale = glm::vec3(1.0);

			if (node.rotation.size() == 4)
			{
				float x = node.rotation[0];
				float y = node.rotation[1];
				float z = node.rotation[2];
				float w = node.rotation[3];

				glm::vec3 convert = glm::eulerAngles(glm::quat{ w,x,y,z });
				rotation = convert;
			}
			if (node.scale.size() == 3)
			{
				scale = { node.scale[0], node.scale[1], node.scale[2] };
			}
			if (node.translation.size() == 3)
			{
				translation = { node.translation[0], node.translation[1], node.translation[2] };
			}

			transform = glm::translate(glm::mat4(1.0), translation) * glm::toMat4(glm::quat(rotation)) * glm::scale(glm::mat4(1.0), scale);
		}
		return transform;
	}
}