#pragma once
#include "Gaia/GltfLoader/tiny_gltf.h"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "Gaia/Material.h"

#define GL_BYTE 0x1400           // 5120
#define GL_UNSIGNED_BYTE 0x1401  // 5121
#define GL_SHORT 0x1402          // 5122
#define GL_UNSIGNED_SHORT 0x1403 // 5123
#define GL_INT 0x1404            // 5124
#define GL_UNSIGNED_INT 0x1405   // 5125
#define GL_FLOAT 0x1406          // 5126
#define GL_2_BYTES 0x1407        // 5127
#define GL_3_BYTES 0x1408        // 5128
#define GL_4_BYTES 0x1409        // 5129
#define GL_DOUBLE 0x140A         // 5130

struct aiMesh;
struct aiNode;
struct aiScene;
namespace Gaia
{
	struct SubMesh
	{
		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normal;
		std::vector<glm::vec3> Tangent;
		std::vector<glm::vec3> BiTangent;
		std::vector<glm::vec2> TexCoord;
		std::vector<uint32_t> indices;

		/*VkBufferCreator vk_mesh_vertex_buffer;
		VkBufferCreator vk_mesh_index_buffer;*/

		//ref<VertexArray> VertexArray;
		uint64_t m_MaterialID;
		uint32_t numVertices;
		//Bounds mesh_bounds; //sub_mesh bounds
	};
	class LoadMesh
	{
	public:
		LoadMesh();
		LoadMesh(const std::string& Path);
		~LoadMesh();
		void clear() {
			/*for (uint8_t*& texture : glTfTextures)
			{
				delete[] texture;
				texture = nullptr;
			}*/

			gltfTextures.clear();
			m_subMeshes.clear();
			pbrMaterials.clear();
		}
		/*void CreateLOD(const std::string& Path, LoadType type = LoadType::LOAD_MESH);
		LoadMesh* GetLOD(int lodIndex);*/

	public:
		struct VertexAttributes {
			//glm::vec3 Position;
			glm::vec4 Position;
			glm::vec2 TextureCoordinate;
			glm::vec3 Normal;
			glm::vec3 Tangent;
			uint32_t materialId;
			VertexAttributes(const glm::vec4& Position, const glm::vec2& TextureCoordinate, const glm::vec3& normal = { 0,0,0 }, const glm::vec3& Tangent = { 0,0,0 }, const uint32_t& materialId = 0u)
			{
				this->Position = Position;
				this->TextureCoordinate = TextureCoordinate;
				Normal = normal;
				this->Tangent = Tangent;
				this->materialId = materialId;
			}
			VertexAttributes() = default;
			//may more ..uv coord , tangents , normals..
		};
		std::string m_path;
		std::vector<SubMesh> m_subMeshes;
		std::vector<Material> pbrMaterials;
		//gltf textures
		std::vector<Texture> gltfTextures;
		//Bounds total_bounds; //total mesh bounds
		glm::mat4 GlobalTransform;
		/*std::vector<VkVertexInputAttributeDescription> m_vertexAttribDesc;
		std::vector < VkVertexInputBindingDescription> m_vertexBindingDesc;*/
		//uint64_t uuid;
	private:
		//VmaAllocation m_allocation;
		std::string extension = ".asset";
		std::string objectName;
		std::vector<LoadMesh*> m_LOD;
		/*ref<BufferLayout> bl;
		ref<VertexBuffer> vb;
		ref<IndexBuffer> ib;*/
		std::vector<aiMesh*> m_Mesh;
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		
	private:
		void parse_scene_rec(tinygltf::Node& node);
		void LoadObj(const std::string& Path);
		void ProcessNode(aiNode* Node, const aiScene* scene);
		void ProcessMesh();
		void ProcessMaterials(const aiScene* scene);
		void CalculateTangent();
		void CreateStaticBuffers();
		void CreateVulkanVertexDesc();
		void LoadTextures();
		void LoadMatrials();

		void LoadVertexData(int mesh_index);
		template <typename T>
		int LoadAccessor(const tinygltf::Accessor& accessor, const T*& pointer, uint32_t* count = nullptr, int* type = nullptr)
		{
			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			pointer =
				reinterpret_cast<const T*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			if (count)
			{
				*count = static_cast<uint32_t>(accessor.count);
			}
			if (type)
			{
				*type = accessor.type;
			}
			return accessor.componentType;
		}
	};
}


