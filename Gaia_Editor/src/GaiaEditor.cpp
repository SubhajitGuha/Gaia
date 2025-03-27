#include "GaiaEditor.h"
#include "Gaia/Application.h"
#include "Gaia/Window.h"
#include "Gaia/Renderer/Shadows.h"
#include "imgui.h"

GaiaEditor::GaiaEditor()
{
	Window& window = Application::Get().GetWindow();
	scene_ = Gaia::Scene::create(SceneDescriptor{
		.meshPath = "C:/Users/Subhajit/OneDrive/Documents/bistro_ext.gltf",
		.windowWidth = window.GetWidth(),
		.windowHeight = window.GetHeight() 
		});
	renderer_ = Gaia::Renderer::create(window.GetNativeWindow(), *scene_);
	//imp set imgui rendering context.
	Application::Get().setImGuiRenderingContext(*renderer_->getContext(), renderer_->getRenderTarget());
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
	ImGui::Begin("Light");
	ImGui::DragFloat3("Light Direction", &scene_->lightParameter.direction.x,0.1);
	ImGui::DragFloat3("Light Position", &scene_->LightPosition.x, 0.1);

	ImGui::ColorEdit3("Light Color", &scene_->lightParameter.color.x);
	ImGui::DragFloat("Light Intensity", &scene_->lightParameter.intensity, 0.1);
	ImGui::DragFloat("cascade lamda", &Shadows::lamda, 0.001);

	ImGui::End();
}

void GaiaEditor::OnEvent(Event& e)
{
}
