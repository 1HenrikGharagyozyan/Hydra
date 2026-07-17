#pragma once

#include "Hydra/Core/PlatformDetection.h" 

#include <memory>

// ================= DLL EXPORT / IMPORT =================
#ifdef HD_DYNAMIC_LINK
    #ifdef HD_PLATFORM_WINDOWS
        #ifdef HD_BUILD_DLL
            #define HYDRA_API __declspec(dllexport)
        #else
            #define HYDRA_API __declspec(dllimport)
        #endif
    #elif defined(HD_PLATFORM_LINUX)
        #ifdef HD_BUILD_DLL
            #define HYDRA_API __attribute__((visibility("default")))
        #else
            #define HYDRA_API
        #endif
    #else
        #define HYDRA_API
    #endif
#else
    #define HYDRA_API
#endif

// ================= DEBUG ASSERTS =================
#ifdef HD_DEBUG
    #define HD_ENABLE_ASSERTS
#endif

#ifdef HD_PLATFORM_WINDOWS
    #define HD_DEBUGBREAK() __debugbreak()
#elif defined(HD_PLATFORM_LINUX)
    #include <signal.h>
    #define HD_DEBUGBREAK() raise(SIGTRAP)
#else
    #define HD_DEBUGBREAK()
#endif


#define HD_EXPAND_MACRO(x) x
#define HD_STRINGIFY_MACRO(x) #x


// ================= HELPERS =================
#define BIT(x) (1 << x)

#define HD_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Hydra 
{
    // Scope Wrapper
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Ref Wrapper
    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}

#include "Hydra/Core/Assert.h"
#include "Hydra/Core/Log.h"