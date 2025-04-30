#pragma once
#include "Gaia/Layer.h"
#include "Gaia/Scene/Scene.h"
#include "Gaia/Renderer/Renderer.h"
using namespace Gaia;

class  GaiaEditor :public Layer {
	
public:
	GaiaEditor();
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(float deltatime) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Event& e) override;

private:
	ref<Renderer> renderer_;
	ref<Scene> scene_;

	glm::vec3 nodeTranslation = glm::vec3(0.0);
	glm::vec3 nodeRotation = glm::vec3(0.0);
	glm::vec3 nodeScale = glm::vec3(1.0);
	int currentSelectedNode = -1;
	std::vector<bool> nodeSelection; //keeps track of whether the node is selected or not (if selected then heighlight)
private:
	void traverseHierarchy(int index, std::vector<Hierarchy>& sceneHierarchy, std::vector<std::string>& nodeNames);
	void DrawHierarchy();
	void DrawNodeProperties();
};