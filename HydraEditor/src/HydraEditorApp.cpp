#include <Hydra.h>
#include <Hydra/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Hydra
{

	class HydraEditor : public Application
	{
	public:
		HydraEditor(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(new EditorLayer());
		}

		~HydraEditor()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "HydraEditor";
		spec.CommandLineArgs = args;

		return new HydraEditor(spec);
	}

}