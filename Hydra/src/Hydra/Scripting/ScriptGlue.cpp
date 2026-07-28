#include "hdpch.h"
#include "ScriptGlue.h"

#include "mono/metadata/object.h"


namespace Hydra 
{

#define HD_ADD_INTERNAL_CALL(Name) mono_add_internal_call("Hydra.InternalCalls::" #Name, (const void*)(Name))

	// Converts a MonoString to a std::string, always freeing the intermediate
	// UTF-8 buffer mono_string_to_utf8() allocates (mono_free(), not free()/
	// delete[] - it may come from Mono's own allocator).
	static std::string MonoStringToUTF8(MonoString* string)
	{
		if (string == nullptr)
			return std::string();

		char* cStr = mono_string_to_utf8(string);
		std::string str(cStr);
		mono_free(cStr);
		return str;
	}

	static void NativeLog(MonoString* string, int parameter)
	{
		std::cout << MonoStringToUTF8(string) << ", " << parameter << std::endl;
	}

	static void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
	{
		HD_CORE_WARN("Value: {0}", glm::to_string(*parameter));
		*outResult = glm::normalize(*parameter);
	}

	static float NativeLog_VectorDot(glm::vec3* parameter)
	{
		HD_CORE_WARN("Value: {0}", glm::to_string(*parameter));
		return glm::dot(*parameter, *parameter);
	}

	// Hydra.Log's C++ side - routes C# log calls through the engine's own
	// (client) logger instead of System.Console, on every platform.
	static void NativeLog_Trace(MonoString* message)
	{
		HD_TRACE(MonoStringToUTF8(message));
	}

	static void NativeLog_Info(MonoString* message)
	{
		HD_INFO(MonoStringToUTF8(message));
	}

	static void NativeLog_Warn(MonoString* message)
	{
		HD_WARN(MonoStringToUTF8(message));
	}

	static void NativeLog_Error(MonoString* message)
	{
		HD_ERROR(MonoStringToUTF8(message));
	}

	void ScriptGlue::RegisterFunctions()
	{
		HD_ADD_INTERNAL_CALL(NativeLog);
		HD_ADD_INTERNAL_CALL(NativeLog_Vector);
		HD_ADD_INTERNAL_CALL(NativeLog_VectorDot);

		HD_ADD_INTERNAL_CALL(NativeLog_Trace);
		HD_ADD_INTERNAL_CALL(NativeLog_Info);
		HD_ADD_INTERNAL_CALL(NativeLog_Warn);
		HD_ADD_INTERNAL_CALL(NativeLog_Error);
	}

}