#pragma once
#include "Gaia/GltfLoader/tiny_gltf.h"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "Gaia/Material.h"
#include <limits>

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
		std::vector<uint32_t> m_MaterialID; //storing material id per vertex
		std::vector<uint32_t> meshIndices; //storing mesh id per vertex

		/*VkBufferCreator vk_mesh_vertex_buffer;
		VkBufferCreator vk_mesh_index_buffer;*/

		//ref<VertexArray> VertexArray;
		uint32_t numVertices;
		//Bounds mesh_bounds; //sub_mesh bounds
	};
	struct Hierarchy
	{
		int parent = -1;
		int firstChild = -1;
		int nextSibling = -1;
		int level = -1;
	};
	struct SceneBounds
	{
		glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
	};
	class LoadMesh
	{
	public:
		
		LoadMesh();
		LoadMesh(const std::string& Path);
		void calculateSceneBounds();
		~LoadMesh();
		void clear() {

			gltfTextures.clear();
			m_subMeshes.clear();
			pbrMaterials.clear();
		}
		/*void CreateLOD(const std::string& Path, LoadType type = LoadType::LOAD_MESH);
		LoadMesh* GetLOD(int lodIndex);*/

	public:
		SceneBounds sceneBounds_;
		struct VertexAttributes {
			glm::vec4 Position;
			glm::vec2 TextureCoordinate;
			glm::vec3 Normal;
			glm::vec3 Tangent;
			uint32_t materialId;
			uint32_t meshId;
			VertexAttributes(const glm::vec4& Position, const glm::vec2& TextureCoordinate, const glm::vec3& normal = { 0,0,0 }, const glm::vec3& Tangent = { 0,0,0 }, const uint32_t& materialId = 0u, const uint32_t& meshId = 0)
			{
				this->Position = Position;
				this->TextureCoordinate = TextureCoordinate;
				Normal = normal;
				this->Tangent = Tangent;
				this->materialId = materialId;
				this->meshId = meshId;
			}
			VertexAttributes() = default;
		};
		std::string m_path;
		std::vector<std::string> m_nodeNames;
		std::unordered_map<int, SubMesh> m_subMeshes;
		std::vector<Material> pbrMaterials;
		std::vector<Texture> gltfTextures;
		std::vector<glm::mat4> localTransforms;
		std::vector<glm::mat4> globalTransforms;
		std::vector<Hierarchy> m_hierarchy;
	private:
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		int numMeshes = 0;//mesh count we get from gltf mesh
	private:
		int addNode(int parentIndex, int level); //adds a new node to the hierarchy and returns the new node index
		void parse_scene_rec(tinygltf::Node& node, glm::mat4 nodeTransform, int Index, int parentIndex, int level);
		void getTotalNodes(tinygltf::Node& node, int& totalNodes);
		void LoadObj(const std::string& Path);
		void LoadTextures();
		void LoadMatrials();
		glm::mat4 getTransform(int nodeIndex);
		void LoadVertexData(int mesh_index, int hierarchyIndex, int level);
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


