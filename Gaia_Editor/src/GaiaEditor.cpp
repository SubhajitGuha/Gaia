#include "GaiaEditor.h"
#include "Gaia/Application.h"
#include "Gaia/Window.h"
#include "Gaia/Renderer/Shadows.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"

GaiaEditor::GaiaEditor()
{
	Window& window = Application::Get().GetWindow();
	scene_ = Gaia::Scene::create(SceneDescriptor{
		.meshPath = "C:/Users/Subhajit/OneDrive/Documents/sponza.gltf",
		.windowWidth = window.GetWidth(),
		.windowHeight = window.GetHeight() 
		});
	renderer_ = Gaia::Renderer::create(window.GetNativeWindow(), *scene_);

	//init
	nodeSelection.resize(scene_->getHirarchy().size(), false);

	//imp set imgui rendering context.
	Application::Get().setImGuiRenderingContext(*renderer_->getContext(), renderer_->getRenderTarget());
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
	//std::fill(nodeSelection.begin(), nodeSelection.end(), false);

	ImGui::Begin("Light");
	if (ImGui::DragFloat3("Light Direction", &scene_->lightParameter.direction.x, 0.1))
	{
		Renderer::isFirstFrame = true;
	}
	ImGui::DragFloat3("Light Position", &scene_->LightPosition.x, 0.1);

	if (ImGui::ColorEdit3("Light Color", &scene_->lightParameter.color.x))
	{
		Renderer::isFirstFrame = true;
	}
	if (ImGui::DragFloat("Light Intensity", &scene_->lightParameter.intensity, 0.1))
	{
		Renderer::isFirstFrame = true;
	}
	ImGui::DragFloat("cascade lamda", &Shadows::lamda, 0.001);

	ImGui::End();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
	/*bool showDemo = true;
	ImGui::ShowDemoWindow(&showDemo);*/
	DrawHierarchy();
	DrawNodeProperties();
}

void GaiaEditor::OnEvent(Event& e)
{
}

void decomposeMatrix(glm::mat4& matrix, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
{
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat orient;
	glm::decompose(matrix, scale, orient, translation, skew, perspective);
	orient = glm::conjugate(orient);
	rotation = glm::eulerAngles(orient);
}

void GaiaEditor::traverseHierarchy(int index, std::vector<Hierarchy>& sceneHierarchy, std::vector<std::string>& nodeNames)
{
	if (index == -1)
		return;

	bool node_open;
	ImGuiTreeNodeFlags selectFlag = nodeSelection[index] ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;

	if(sceneHierarchy[index].firstChild == -1)
		node_open = ImGui::TreeNodeEx((void*)index, ImGuiTreeNodeFlags_Leaf | selectFlag, nodeNames[index].c_str());
	else
		node_open = ImGui::TreeNodeEx((void*)index, ImGuiTreeNodeFlags_OpenOnArrow | selectFlag, nodeNames[index].c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		if (currentSelectedNode != -1)
			nodeSelection[currentSelectedNode] = false; //make the previously selected node false
		nodeSelection[index] = true;
		glm::mat4 transform = scene_->getLocalTransform(index);
		decomposeMatrix(transform, nodeTranslation, nodeRotation, nodeScale);
		currentSelectedNode = index;
	}

	if (node_open)
	{
		traverseHierarchy(sceneHierarchy[index].firstChild, sceneHierarchy, nodeNames);
		ImGui::TreePop();
	}
	traverseHierarchy(sceneHierarchy[index].nextSibling, sceneHierarchy, nodeNames);
}
void GaiaEditor::DrawHierarchy()
{
	std::vector<Hierarchy>& sceneHierarchy = scene_->getHirarchy();
	std::vector<std::string>& nodeNames = scene_->getNodeNames();

	int index = 0;
	ImGui::Begin("Scene");
	{
		traverseHierarchy(index, sceneHierarchy, nodeNames);
	}
	ImGui::End();
}

bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f,const char* x = "X", const char* y = "Y", const char* z = "Z")
{
	ImGui::PushID(label.c_str());
	bool res = false;

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	if (res = res || ImGui::Button(x, buttonSize))
		values.x = resetValue;
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	bool res1 = ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	if (res = res || ImGui::Button(y, buttonSize))
		values.y = resetValue;
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	bool res2 = ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	if (res = res || ImGui::Button(z, buttonSize))
		values.z = resetValue;
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	bool res3 = ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return res || res1 || res2 || res3;
}

void GaiaEditor::DrawNodeProperties()
{
	const char* nodeName = currentSelectedNode != -1 ? scene_->getNodeNames()[currentSelectedNode].c_str() : "";
	ImGui::Begin("Node Properties");
	ImGui::Text(nodeName);
	bool isOpen = ImGui::TreeNodeEx("TRANSFORM", ImGuiTreeNodeFlags_OpenOnArrow |ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen);
	if (isOpen)
	{
		bool isTranslated = DrawVec3Control("Translation", nodeTranslation);
		bool isRotated = DrawVec3Control("Rotation", nodeRotation);
		bool isScaled = DrawVec3Control("Scale", nodeScale, 1.0f);
		if (isTranslated || isRotated || isScaled)
		{
			glm::mat4 rotateX = glm::eulerAngleX(nodeRotation.x);
			glm::mat4 rotateY = glm::eulerAngleY(nodeRotation.y);
			glm::mat4 rotateZ = glm::eulerAngleZ(nodeRotation.z);

			glm::mat4 rotation =
				rotateX * rotateY * rotateZ; // or some other order
			glm::mat4 transform = glm::translate(glm::mat4(1.0), nodeTranslation) * rotation * glm::scale(glm::mat4(1.0), nodeScale);
			if(currentSelectedNode != -1)
				scene_->setTransform(currentSelectedNode, transform);
		}
		ImGui::TreePop();
	}

	ImGui::End();
}
