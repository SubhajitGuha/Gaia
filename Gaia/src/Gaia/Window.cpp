#include "pch.h"
#include "Window.h"
#include "Core.h"
#include "Window/WindowsWindow.h"

namespace Gaia {
	Window* Window::Create(const WindowProp& prop)
	{
		return new WindowsWindow();
	}
}