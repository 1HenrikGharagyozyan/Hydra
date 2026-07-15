-- Hydra Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"]      = "%{wks.location}/Hydra/vendor/GLFW/include"
IncludeDir["Glad"]      = "%{wks.location}/Hydra/vendor/Glad/include"
IncludeDir["ImGui"]     = "%{wks.location}/Hydra/vendor/imgui"
IncludeDir["glm"]       = "%{wks.location}/Hydra/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Hydra/vendor/stb_image"
IncludeDir["spdlog"]    = "%{wks.location}/Hydra/vendor/spdlog/include"
IncludeDir["entt"]      = "%{wks.location}/Hydra/vendor/entt/include"
IncludeDir["yaml_cpp"]  = "%{wks.location}/Hydra/vendor/yaml-cpp/include"
IncludeDir["Box2d"]     = "%{wks.location}/Hydra/vendor/Box2d/include"
IncludeDir["ImGuizmo"]  = "%{wks.location}/Hydra/vendor/ImGuizmo"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

-- Library directories (for Windows)
LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

-- Libraries (for Windows)
Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"