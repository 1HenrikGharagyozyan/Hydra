project "Hydra-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"
	targetdir ("%{wks.location}/Hydra/Resources/Scripts")
	objdir ("%{wks.location}/Hydra/Resources/Scripts/Intermediates")
	files
	{
		"**.cs"
	}
	
	filter "configurations:Debug"
		optimize "Off"
		symbols "Default"
	filter "configurations:Release"
		optimize "On"
		symbols "Default"
	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"