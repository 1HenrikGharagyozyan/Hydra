#include "hdpch.h"
#include "ScriptGlue.h"

#include "Hydra/Core/Log.h"

#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/utils/mono-publib.h>

namespace Hydra
{

	static std::string MonoStringToUTF8(MonoString* monoString)
	{
		if (monoString == nullptr)
			return std::string();

		char* utf8 = mono_string_to_utf8(monoString);
		std::string result(utf8);
		mono_free(utf8);
		return result;
	}

	// Mirrors Hydra.LogLevel in Hydra-ScriptCore/Source/Log.cs
	enum class ScriptLogLevel : int32_t
	{
		Trace = 0,
		Info = 1,
		Warn = 2,
		Error = 3
	};

	static void Log_LogMessage(MonoString* message, int32_t level)
	{
		std::string msg = MonoStringToUTF8(message);

		switch ((ScriptLogLevel)level)
		{
			case ScriptLogLevel::Trace: HD_TRACE(msg); break;
			case ScriptLogLevel::Info:  HD_INFO(msg);  break;
			case ScriptLogLevel::Warn:  HD_WARN(msg);  break;
			case ScriptLogLevel::Error: HD_ERROR(msg); break;
			default:
				HD_CORE_WARN("Log_LogMessage: unknown log level {} for message '{}'", level, msg);
				break;
		}
	}

#define HD_ADD_INTERNAL_CALL(name) mono_add_internal_call("Hydra.InternalCalls::" #name, (void*)name)

	void ScriptGlue::RegisterFunctions()
	{
		HD_ADD_INTERNAL_CALL(Log_LogMessage);
	}

}
