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
		inline LoadMesh& getMesh() const { return *mesh_; }
		void update(TimeStep ts);
		void onEvent(Event& event);
	private:
		std::unique_ptr<EditorCamera> mainCamera_;
		std::unique_ptr<LoadMesh> mesh_;
		SceneDescriptor sceneDesc_;
	};

}

