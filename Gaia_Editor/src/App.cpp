#include "Gaia/Application.h"
#include "Gaia/EntryPoint.h"
#include "GaiaEditor.h"

class App :public Gaia::Application
{
public:
	App() {
		PushLayer(new GaiaEditor());
	}
	~App() {}
};

Gaia::Application* Gaia::CreateApplication() {
	return new App();
}

