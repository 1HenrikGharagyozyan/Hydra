#pragma once

#include <string>

namespace Hydra
{

    class FileDialogs
    {
    public:
        // These returns empty strings if cancelled
        static std::string OpenFile(const char* filter);
        static std::string SaveFile(const char* filter);
    };

    class Time
    {
    public:
        static float GetTime();
    };

}