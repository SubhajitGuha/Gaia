#pragma once
#include "pch.h"
#include "Events/Event.h"

namespace Gaia
{
	struct WindowProp {
		std::string Title;
		uint32_t width, height;
		WindowProp(const std::string& s,const uint32_t& width,const uint32_t& height)
			:Title(s),width(width),height(height)
		{}
	};

	class Window {
	public:
		using EventCallbackFunc = std::function<void(Event&)>;
		virtual ~Window() {}
		virtual void OnUpdate() = 0;
		virtual unsigned int GetWidth() = 0;
		virtual unsigned int GetHeight() = 0;

		//window parameters
		virtual void SetCallbackEvent(const EventCallbackFunc&) = 0;
		virtual void SetVsync(bool enable) = 0;
		virtual bool b_Vsync()const = 0;
		virtual void* GetNativeWindow() = 0;//gets the GLFWwindow pointer

		static Window* Create(const WindowProp& prop);
	};
}