#include "pch.h"
#include "Scene.h"

namespace Gaia {
	Scene::Scene(const SceneDescriptor& desc) : sceneDesc_(desc)
	{
		mainCamera_ = std::make_unique<EditorCamera>(desc.windowWidth, desc.windowHeight);
		mesh_ = std::make_unique<LoadMesh>(desc.meshPath);
	}
	Scene::~Scene()
	{
	}
	std::shared_ptr<Scene> Scene::create(const SceneDescriptor& desc)
	{
		return std::make_shared<Scene>(desc);
	}
	void Scene::update(TimeStep ts)
	{
		mainCamera_->OnUpdate(ts);
	}
	void Scene::onEvent(Event& event)
	{
		mainCamera_->OnEvent(event);
	}
}