#pragma once
#include "Application.h"

extern Gaia::Application* Gaia::CreateApplication();
int main(int* argc, char** argv) {

	Gaia::Log::init();
	auto app = Gaia::CreateApplication();

	app->Run();

	delete app;

	return 0;
}
