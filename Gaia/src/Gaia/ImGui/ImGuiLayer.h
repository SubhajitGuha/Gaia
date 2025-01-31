#pragma once
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "imgui.h"
#include "Gaia/Layer.h"
#include "Gaia/Events/ApplicationEvent.h"
#include "Gaia/Events/KeyEvent.h"
#include "Gaia/Events/MouseEvent.h"

namespace Gaia {
	class ImGuiLayer :public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		 void OnAttach()override;
		 void OnDetach()override;
		 void OnImGuiRender() override;
		 virtual void OnEvent(Event& e) override;

		 void Begin();
		 void End();
	public:
		bool b_DispatchEvents = false;
		static ImFont* Font;
	private:
		void SetDarkThemeColors();
	private:
	};
}