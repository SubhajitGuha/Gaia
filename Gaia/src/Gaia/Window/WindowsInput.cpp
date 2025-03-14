#include "pch.h"
#include "WindowsInput.h"
#include "Gaia/Application.h"

namespace Gaia {
	Input* Input::m_Input = new WindowsInput();

	bool WindowsInput::IsKeyPressedImpl(int keyCode)
	{
		
		int status = glfwGetKey((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow(), keyCode);
		return status==GLFW_PRESS || status==GLFW_REPEAT;
	}

	bool WindowsInput::IsMouseButtonPressed(int Button)
	{
		
		int status = glfwGetMouseButton((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow(), Button);
		return status == GLFW_PRESS;
	}

	std::pair<double, double> WindowsInput::GetMousePos()
	{
		std::pair<double, double> coordinate;
		glfwGetCursorPos((GLFWwindow*) Application::Get().GetWindow().GetNativeWindow(),&coordinate.first,&coordinate.second);
		return coordinate;
	}

}