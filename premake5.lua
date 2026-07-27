include "Dependencies.lua"
include "./vendor/premake/premake_customization/solution_items.lua"

workspace "Hydra"
    architecture "x86_64"
    startproject "HydraEditor"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    solution_items
    {
        ".editorconfig",
        "Dependencies.lua" -- Добавили сюда, чтобы файл было видно в Visual Studio
    }
    
    flags
    {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "vendor/premake"
    include "Hydra/vendor/Box2d"
    include "Hydra/vendor/GLFW"
    include "Hydra/vendor/Glad"
    include "Hydra/vendor/imgui"
    include "Hydra/vendor/yaml-cpp"
group ""

group "Core"
    include "Hydra/src/Hydra"
    include "Hydra-ScriptCore"
group ""

group "Tools"
    include "HydraEditor"
group ""

group "Misc"
    include "Sandbox"
group ""