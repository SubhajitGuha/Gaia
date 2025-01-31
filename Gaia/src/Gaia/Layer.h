#pragma once
#include "pch.h"
#include "Core.h"
#include "Events/Event.h"
namespace Gaia {
	class Layer
	{
	public:
		Layer(const std::string s = "Layer");
		virtual ~Layer();
		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(float deltatime) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& e) {}

		inline std::string GetName()const { return m_DebugName; }
	private:
		std::string m_DebugName;

	};
}

