#pragma once
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "imgui.h"
#include "Gaia/Layer.h"
#include "Gaia/Events/ApplicationEvent.h"
#include "Gaia/Events/KeyEvent.h"
#include "Gaia/Events/MouseEvent.h"
#include "Gaia/Renderer/GaiaRenderer.h"

namespace Gaia {
	class ImGuiLayer :public Layer
	{
	public:
		static std::shared_ptr<ImGuiLayer> create(IContext* context);
		explicit ImGuiLayer(IContext* context);
		~ImGuiLayer();

		 void OnAttach()override;
		 void OnDetach()override;
		 void OnImGuiRender() override;
		 virtual void OnEvent(Event& e) override;

		 void Begin();
		 void End();

		 void setRenderTarget(TextureHandle rt) { renderTarget_ = rt; }
	public:
		bool b_DispatchEvents = false;
		static ImFont* Font;
	private:
		void SetDarkThemeColors();
	private:
		IContext* context_;
		TextureHandle renderTarget_;
	};
}