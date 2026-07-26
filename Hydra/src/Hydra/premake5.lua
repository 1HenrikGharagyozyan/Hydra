project "Hydra"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hdpch.h"
	pchsource "../hdpch.cpp"

	files
	{
		"../hdpch.h",
		"../hdpch.cpp",
		"**.h",
		"**.cpp",
		"../Platform/OpenGL/**.h",
		"../Platform/OpenGL/**.cpp",
		"%{IncludeDir.stb_image}/**.h",
		"%{IncludeDir.stb_image}/**.cpp",
		"%{IncludeDir.glm}/glm/**.hpp",
		"%{IncludeDir.glm}/glm/**.inl",

		"%{IncludeDir.ImGuizmo}/src/ImGuizmo.h",
		"%{IncludeDir.ImGuizmo}/src/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
		"HD_DYNAMIC_LINK",
		"HD_BUILD_DLL"
	}

	includedirs
	{
		"..",
		"src",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.Box2d}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}/src",
		"%{IncludeDir.Mono}"
	}

	links
	{
		"Box2d",
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp"
	}

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	removefiles
	{
		"src/Platform/**"
	}

	filter "system:windows"
		systemversion "latest"
		defines { "HD_PLATFORM_WINDOWS" }
		files
		{
			"../Platform/Windows/**.h",
			"../Platform/Windows/**.cpp"
		}
		links
		{
			"opengl32.lib",
			"%{Library.Mono}"
		}

	filter "system:linux"
		systemversion "latest"
		defines { "HD_PLATFORM_LINUX" }
		pic "On"
		files
		{
			"../Platform/Linux/**.h",
			"../Platform/Linux/**.cpp"
		}
		libdirs
		{
			"%{LibraryDir.Mono}"
		}

		links
		{
			"GL",
			"pthread",
			"dl",
			"X11",
			"shaderc_shared",
			"spirv-cross-glsl",
			"spirv-cross-core",
			"mono-2.0"
		}

	filter "configurations:Debug"
		defines "HD_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "HD_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "HD_DIST"
		runtime "Release"
		optimize "on"