#pragma once

#include "Base.h"

#include "Window.h"
#include "Hydra/Core/LayerStack.h"
#include "Hydra/Events/Event.h"
#include "Hydra/Events/ApplicationEvent.h"

#include "Hydra/Core/Timestep.h"

#include "Hydra/ImGui/ImGuiLayer.h"


int main(int argc, char** argv);


namespace Hydra
{

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			HD_CORE_ASSERT(index < Count, "Index out of bounds");
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Hydra Application";
		std::string WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class Application
	{
	public:
		Application();Application(const ApplicationSpecification& specification);

		virtual ~Application();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		
		Window& GetWindow() { return *m_Window; }

		static Application& Get() { return *s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

	private:
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		ApplicationSpecification m_Specification;
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_LayerStack;
		float m_LastFrameTime = 0.0f;

	private:
		static Application* s_Instance;
		friend int ::main(int argc, char** argv);
	};

	// To be defined in CLIENT 
	Application* CreateApplication(ApplicationCommandLineArgs args);
}
