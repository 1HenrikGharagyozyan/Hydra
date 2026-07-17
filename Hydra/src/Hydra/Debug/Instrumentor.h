#pragma once

#include "Hydra/Core/Log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>


namespace Hydra
{

    using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

    struct ProfileResult
    {
        std::string Name;
        FloatingPointMicroseconds Start;
        FloatingPointMicroseconds ElapsedTime;
        std::thread::id ThreadID;
    };

    struct InstrumentationSession
    {
        std::string Name;
    };

    class Instrumentor
    {
    public:
        Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

        void BeginSession(const std::string& name, const std::string& filepath = "results.json")
        {
            std::lock_guard lock(m_Mutex);
            if (m_CurrentSession)
            {
                // If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead.  That's better than having badly formatted
				// profiling output.
				if (Log::GetCoreLogger())  // Edge case: BeginSession() might be before Log::Init()
					HD_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
				InternalEndSession();
            }

            m_OutputStream.open(filepath);
            if (m_OutputStream.is_open())
            {
                m_CurrentSession = new InstrumentationSession({name});
                WriteHeader();
            }
            else
            {
                if (Log::GetCoreLogger())  // Edge case: BeginSession() might be before Log::Init()
                    HD_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
            }
        }

        void EndSession()
        {
            std::lock_guard lock(m_Mutex);
            InternalEndSession();
        }

        void WriteProfile(const ProfileResult& result)
        {
            std::stringstream json;

            json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession) 
            {
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
        }

        static Instrumentor& Get()
        {
            static Instrumentor instance;
            return instance;
        }

    private:
        Instrumentor()
            : m_CurrentSession(nullptr)
        {
        }

        ~Instrumentor()
        {
            EndSession();
        }


        void WriteHeader()
        {
            m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
            m_OutputStream.flush();
        }

        void WriteFooter()
        {
            m_OutputStream << "]}";
            m_OutputStream.flush();
        }

        // Note: you must already own lock on m_Mutex before
		// calling InternalEndSession()
        void InternalEndSession()
        {
            if (m_CurrentSession)
            {
                WriteFooter();
                m_OutputStream.close();
                delete m_CurrentSession;
                m_CurrentSession = nullptr;
            }
        }

    private:
        std::mutex m_Mutex;
        InstrumentationSession* m_CurrentSession;
        std::ofstream m_OutputStream;
    };


    class InstrumentationTimer
    {
    public:
        InstrumentationTimer(const char* name)
            : m_Name(name)
            , m_Stopped(false)
        {
            m_StartTimepoint = std::chrono::steady_clock::now();
        }

        ~InstrumentationTimer()
        {
            if (!m_Stopped)
                Stop();
        }

        void Stop()
        {
            auto endTimepoint = std::chrono::steady_clock::now();
            auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
            auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() 
                                - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

            uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
            Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime,  std::this_thread::get_id() });

            m_Stopped = true;
        }

    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
        bool m_Stopped;
    };


    namespace InstrumentorUtils
    {

        template<size_t N>
        struct ChangeResult
        {
            char Data[N];
        };

        template<size_t N, size_t K>
        constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
        {
            ChangeResult<N> result = {};

            size_t srcIndex = 0;
            size_t dstIndex = 0;
            while (srcIndex < N)
            {
                size_t matchIndex = 0;
                while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
                    matchIndex++;
                if (matchIndex == K - 1)
                    srcIndex += matchIndex;
                result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
                srcIndex++;
            }
            return result;
        }

    }

}


#define HD_PROFILE 1

#if HD_PROFILE
    #if defined(__GNUC__) || defined(__clang__)
        #define HD_FUNC_SIG __PRETTY_FUNCTION__
    #elif defined(_MSC_VER)
        #define HD_FUNC_SIG __FUNCSIG__
    #elif defined(__cplusplus) && (__cplusplus >= 201103L)
        #define HD_FUNC_SIG __func__
    #else
        #define HD_FUNC_SIG "HD_FUNC_SIG unknown!"
    #endif


    #define HD_PROFILE_CONCAT_INNER(a, b) a##b
    #define HD_PROFILE_CONCAT(a, b) HD_PROFILE_CONCAT_INNER(a, b)


    #define HD_PROFILE_BEGIN_SESSION(name, filepath) \
        do { ::Hydra::Instrumentor::Get().BeginSession(name, filepath); } while(0)

    #define HD_PROFILE_END_SESSION() \
        do { ::Hydra::Instrumentor::Get().EndSession(); } while(0)


    #define HD_PROFILE_SCOPE(name) \
        constexpr auto HD_PROFILE_CONCAT(fixedName, __LINE__) = ::Hydra::InstrumentorUtils::CleanupOutputString(name, "__cdecl "); \
        ::Hydra::InstrumentationTimer HD_PROFILE_CONCAT(timer, __LINE__)(HD_PROFILE_CONCAT(fixedName, __LINE__).Data)


    #define HD_PROFILE_FUNCTION() HD_PROFILE_SCOPE(HD_FUNC_SIG)

#else

    #define HD_PROFILE_BEGIN_SESSION(name, filepath)
    #define HD_PROFILE_END_SESSION()
    #define HD_PROFILE_SCOPE(name)
    #define HD_PROFILE_FUNCTION()

#endif