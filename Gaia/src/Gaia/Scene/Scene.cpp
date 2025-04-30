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

	glm::mat4 Scene::getLocalTransform(int nodeHierarchyIndex)
	{
		return mesh_->localTransforms[nodeHierarchyIndex];
	}

	void Scene::setTransform(int nodeHierarchyIndex, glm::mat4& newTransform)
	{
		isTransformUpdated_ = true;
		mesh_->localTransforms[nodeHierarchyIndex] = newTransform;
		updateTransform(nodeHierarchyIndex);
	}

	glm::mat4 Scene::getGlobalTransform(int nodeHierarchyIndex)
	{
		return mesh_->globalTransforms[nodeHierarchyIndex];
	}

	void Scene::updateTransform(int nodeIndex)
	{
		if (nodeIndex == -1)
		{
			return;
		}
		Hierarchy& current_hierarchy = mesh_->m_hierarchy[nodeIndex];
		if (current_hierarchy.parent != -1)
			mesh_->globalTransforms[nodeIndex] = mesh_->globalTransforms[current_hierarchy.parent] * mesh_->localTransforms[nodeIndex];
		else
			mesh_->globalTransforms[nodeIndex] = mesh_->localTransforms[nodeIndex];

		updateTransform(current_hierarchy.firstChild);
		updateTransform(current_hierarchy.nextSibling);
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