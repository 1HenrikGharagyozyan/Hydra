#include "hdpch.h" 
#include "Hydra/Utils/PlatformUtils.h"
#include "Hydra/Core/Application.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <memory>
#include <array>
#include <cstring> // For strlen
#include <filesystem> // For geting current directory


namespace Hydra 
{

    float Time::GetTime()
    {
        return glfwGetTime(); // TODO: Maybe Vulkan 
    }

    std::string FileDialogs::OpenFile(const char* filter)
    {
        std::string command = "zenity --file-selection --title=\"Select File\"";

        std::string currentDir = std::filesystem::current_path().string();
        command += " --filename=\"" + currentDir + "/\"";
        
        // Parse Windows-style filter ("Name\0*.extension\0") into Zenity format ("Name | *.extension")
        if (filter && strlen(filter) > 0)
        {
            std::string filterName(filter); 
            const char* specPtr = filter + filterName.length() + 1;
            if (*specPtr)
            {
                std::string filterSpec(specPtr);
                command += " --file-filter=\"" + filterName + " | " + filterSpec + "\"";
            }
        }
        else
            command += " --file-filter=\"*.hydra\""; // Default fallback

        std::string result;
        std::array<char, 256> buffer;
        
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) 
            return std::string(); // Error opening pipe

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
            result += buffer.data();

        if (!result.empty() && result.back() == '\n')
            result.pop_back();

        // If the user clicked "Cancel", zenity will return an empty string
        if (result.empty())
            return std::string();

        return result;
    }

    std::string FileDialogs::SaveFile(const char* filter)
    {
        std::string command = "zenity --file-selection --save --confirm-overwrite --title=\"Save File\"";
        std::string defaultExt = ".hydra"; // Fallback if the filter is empty

        std::string currentDir = std::filesystem::current_path().string();
        command += " --filename=\"" + currentDir + "/\"";

        // 1. Dynamically extract filter for the dialog and the default extension
        if (filter && strlen(filter) > 0)
        {
            std::string filterName(filter); // Reads until the first \0 (e.g., "Hydra Scene (*.hydra)")
            const char* specPtr = filter + filterName.length() + 1; // Move past the \0
            
            if (*specPtr)
            {
                std::string filterSpec(specPtr); // Reads the mask (e.g., "*.hydra")
                command += " --file-filter=\"" + filterName + " | " + filterSpec + "\"";

                // Extract the extension including the dot (e.g., from "*.hydra" get ".hydra")
                size_t dotPos = filterSpec.find_last_of('.');
                if (dotPos != std::string::npos)
                    defaultExt = filterSpec.substr(dotPos);
            }
        }
        else
            command += " --file-filter=\"*.hydra\"";

        std::string result;
        std::array<char, 256> buffer;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) 
            return std::string(); // Error opening pipe 

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
            result += buffer.data();

        if (!result.empty() && result.back() == '\n')
            result.pop_back();

        // If the user cancelled the save operation
        if (result.empty())
            return std::string();

        // Automatically append the extension if it is missing
        if (!defaultExt.empty())
        {
            if (result.size() < defaultExt.size() || result.compare(result.size() - defaultExt.size(), defaultExt.size(), defaultExt) != 0)
                result += defaultExt;
        }

        return result;
    }

}