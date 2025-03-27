#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "pch.h"
#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include "ImGui/ImGuiLayer.h"
#include "glm/glm.hpp"
#include "Gaia/TimeSteps.h"
#include "Gaia/Renderer/GaiaRenderer.h"

namespace Gaia {
	//class Renderer;
	class Application { //set this class as dll export
	public:
		Application();

		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);

		void PushOverlay(Layer* Overlay);

		inline Window& GetWindow() { return *m_window; }

		static inline Application& Get() { return *getApplication; }

		inline ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		void setImGuiRenderingContext(IContext& context, TextureHandle renderTexture); 
	public:
		static TimeStep deltaTime;
		glm::vec3 v3;
		float v = 0;
		float r = 0;
	private:
		ImGuiLayer* m_ImGuiLayer = nullptr;
		ref<Window> m_window;
		//ref<Renderer> renderer;
		bool m_Running = true;
		bool closeWindow(WindowCloseEvent&);
		LayerStack m_layerstack;
		static Application* getApplication;
		float m_LastFrameTime = 0.0;
	};
	//define in client (not in engine dll)
	Application* CreateApplication();

}