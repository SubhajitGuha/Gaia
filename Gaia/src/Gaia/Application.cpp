#include "pch.h"
#include "Application.h"
#include "Log.h"
#include "Window/WindowsInput.h"
#include "GaiaCodes.h"
#include"Layer.h"
//#include "Gaia/Renderer/Renderer.h"

//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define HZ_BIND_FN(x) std::bind(&Application::x,this,std::placeholders::_1)
/*
std::bind returns a function pointer that can be used as an argument in SetCallBackEvent(), also we are using std::bind
to pass the 'this' pointer as an argument of onEvent while also retriving a function pointer,
with out std::bind it is not possible to call OnEvent with an argument while also retreiving a function pointer
*/

namespace Gaia {

	Application* Application::getApplication;
	uint32_t Application::frameNum = 0;
	TimeStep Application::deltaTime(0.0);
	Application::Application()
	{
		getApplication = this;
		m_window = ref<Window>(Window::Create({ "Gaia", 1920, 1200 }));
		m_window->SetCallbackEvent(HZ_BIND_FN(OnEvent));

	}
	//ImGui layer is created when we set the rendering context
	void Application::setImGuiRenderingContext(IContext& context, TextureHandle renderTarget)
	{
		m_ImGuiLayer = new ImGuiLayer(&context);
		m_ImGuiLayer->setRenderTarget(renderTarget);
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		//m_ImGuiLayer->OnDetach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispach(e);
		dispach.Dispatch<WindowCloseEvent>(HZ_BIND_FN(closeWindow));
		/*dispach.Dispatch<WindowResizeEvent>([](WindowResizeEvent e) {
			Renderer::WindowResize(e.GetWidth(), e.GetHeight());
			return false; });*/
		dispach.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) {frameNum = 0;  return true; });
		dispach.Dispatch<MouseButtonPressed>([&](MouseButtonPressed& e) {frameNum = 0; return true; });

		GAIA_CORE_TRACE(e);
		for (auto it = m_layerstack.end(); it != m_layerstack.begin();)
		{
			if (e.m_Handeled)
				break;
			(*--it)->OnEvent(e);
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_layerstack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* Overlay)
	{
		m_layerstack.PushOverlay(Overlay);
		Overlay->OnAttach();
	}


	bool Application::closeWindow(WindowCloseEvent& EventClose)
	{
		m_Running = false;
		return true;
	}

	void Application::Run()
	{
		while (m_Running) //render loop
		{
			m_window->OnUpdate();

			float time = glfwGetTime();
			deltaTime = time - m_LastFrameTime;//this is the delta time (time btn last and present frame or time required to render a frame)
			m_LastFrameTime = time;

			//layers render layer and game layers
			for (Layer* layer : m_layerstack)
				layer->OnUpdate(deltaTime);

			GAIA_ASSERT(m_ImGuiLayer!=nullptr, "Gui layer cannot be null, make sure to set gui layer in the client side")
			//for ImguiLayers
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_layerstack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();
			frameNum++;
		}
		m_ImGuiLayer->OnDetach();
	}


}