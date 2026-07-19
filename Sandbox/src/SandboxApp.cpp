#include <Hydra.h>
#include <Hydra/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "ExampleLayer.h"

class Sandbox : public Hydra::Application
{
public:
	Sandbox(const Hydra::ApplicationSpecification& specification)
		: Hydra::Application(specification)
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}

	~Sandbox()
	{
	}
};

Hydra::Application* Hydra::CreateApplication(ApplicationCommandLineArgs args)
{
	ApplicationSpecification spec;
	spec.Name = "Sandbox";
	spec.WorkingDirectory = "../HydraEditor";
	spec.CommandLineArgs = args;

	return new Sandbox(spec);
}