#pragma once
#include "Gaia/Renderer/Cameras/EditorCamera.h"
#include "Gaia/LoadMesh.h"
#include "Gaia/TimeSteps.h"
#include "Gaia/Events/Event.h"

namespace Gaia
{
	class EditorCamera;
	class LoadMesh;
	struct SceneDescriptor {
		std::string meshPath = "";
		uint32_t windowWidth = 0;
		uint32_t windowHeight = 0;

	};
	struct LightParameters {
		glm::vec3 color = glm::vec4(1.0);
		float intensity = 1.0;
		glm::vec3 direction = { 0.5,0.5,0.0 };
		float pad = 0.0;
	};

	class Scene final
	{
	public:
		Scene() = default;
		Scene(Scene&& other) : sceneDesc_(other.sceneDesc_)
		{
			mainCamera_.reset(other.mainCamera_.get());
			mesh_.reset(other.mesh_.get());

			other.mainCamera_.reset();
			other.mesh_.reset();
			other.sceneDesc_ = {};
		}
		Scene& operator=(Scene&) = delete;
		Scene& operator=(Scene&& other)
		{
			mainCamera_.reset(other.mainCamera_.get());
			mesh_.reset(other.mesh_.get());

			sceneDesc_ = other.sceneDesc_;

			other.mainCamera_.reset();
			other.mesh_.reset();
			other.sceneDesc_ = {};
			return *this;
		}
		Scene(const SceneDescriptor& desc);
		~Scene();
		static std::shared_ptr<Scene> create(const SceneDescriptor& desc);
		inline EditorCamera& getMainCamera() const { return *mainCamera_; }
		inline std::vector<Hierarchy>& getHirarchy() { return mesh_->m_hierarchy; }
		inline std::vector<glm::mat4>& getGlobalTransforms() { return mesh_->globalTransforms; }
		inline std::vector<glm::mat4>& getLocalTransforms() { return mesh_->localTransforms; }
		inline std::vector<std::string>& getNodeNames() { return mesh_->m_nodeNames; }
		inline std::unordered_map<int,SubMesh>& getMeshes() { return mesh_->m_subMeshes; }
		inline std::vector<Material>& getMaterials() { return mesh_->pbrMaterials; }
		inline std::vector<Texture>& getTextures() { return mesh_->gltfTextures; }

		glm::mat4 getLocalTransform(int nodeHierarchyIndex);
		void setTransform(int nodeHierarchyIndex, glm::mat4& newTransform);

		glm::mat4 getGlobalTransform(int nodeHierarchyIndex);
		void updateTransform(int nodeIndex);

		void update(TimeStep ts);
		void onEvent(Event& event);
		inline bool isTransformUpdated() {
			if (isTransformUpdated_)
			{
				isTransformUpdated_ = false;
				return true;
			}
			return isTransformUpdated_;
		}
		inline void updateSceneBounds() { mesh_->calculateSceneBounds(); }
		inline SceneBounds getSceneBounds() { return mesh_->sceneBounds_; }
	public:
		LightParameters lightParameter{};
		glm::vec3 LightPosition{ 0.0 , 0.0, 0.0};
	private:
		std::unique_ptr<EditorCamera> mainCamera_;
		std::unique_ptr<LoadMesh> mesh_;
		SceneDescriptor sceneDesc_;
		bool isTransformUpdated_ = false;
	};

}

