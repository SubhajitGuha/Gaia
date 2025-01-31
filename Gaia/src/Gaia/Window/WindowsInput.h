#pragma once
#include "Gaia/Core.h"
#include "Gaia/Input.h"
#include "GLFW/glfw3.h"

namespace Gaia {
	class WindowsInput : public Input
	{
	public:
		 bool IsKeyPressedImpl(int keyCode) override;
		 bool IsMouseButtonPressed(int Button) override;
		 std::pair<double, double> GetMousePos() override;
	};
}
