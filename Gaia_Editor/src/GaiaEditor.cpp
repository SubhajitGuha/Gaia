#include "GaiaEditor.h"
#include "Gaia/Application.h"
#include "Gaia/Window.h"

GaiaEditor::GaiaEditor()
{
	Window& window = Application::Get().GetWindow();
	renderer_ = Gaia::Renderer::create(window.GetNativeWindow());
	scene_ = Gaia::Scene::create(SceneDescriptor{
		.meshPath = "C:/Users/Subhajit/Downloads/cube.gltf",
		.windowWidth = window.GetWidth(),
		.windowHeight = window.GetHeight() 
		});
	renderer_->createStaticBuffers(*scene_);
}

void GaiaEditor::OnAttach()
{
}

void GaiaEditor::OnDetach()
{
}

void GaiaEditor::OnUpdate(float deltatime)
{
	scene_->update(deltatime);
	renderer_->update(*scene_);

	renderer_->render(*scene_);
}

void GaiaEditor::OnImGuiRender()
{
}

void GaiaEditor::OnEvent(Event& e)
{
}
