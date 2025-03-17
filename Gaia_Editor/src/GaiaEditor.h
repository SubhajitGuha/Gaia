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
};