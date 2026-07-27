#include "hdpch.h"
#include "ScriptEngine.h"
#include "ScriptGlue.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/utils/mono-publib.h>


namespace Hydra
{

    struct ScriptEngineData
    {
        MonoDomain* RootDomain = nullptr;
        MonoDomain* AppDomain = nullptr;

        MonoAssembly* CoreAssembly = nullptr;
    };

    static ScriptEngineData* s_Data = nullptr;

    void ScriptEngine::Init()
    {
        s_Data = new ScriptEngineData();
        
        InitMono();
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();

        delete s_Data;
    }


	char* ReadBytes(const std::string& filepath, uint32_t* outSize)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint32_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			return nullptr;
		}

		char* buffer = new char[size];
		stream.read((char*)buffer, size);
		stream.close();

		*outSize = size;
		return buffer;
	}

	MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath)
	{
		uint32_t fileSize = 0;
		char* fileData = ReadBytes(assemblyPath, &fileSize);

		if (fileData == nullptr)
		{
			HD_CORE_ERROR("Failed to read C# assembly at '{}'", assemblyPath);
			return nullptr;
		}

		// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			HD_CORE_ERROR("Failed to open C# assembly image '{}': {}", assemblyPath, mono_image_strerror(status));
			delete[] fileData;
			return nullptr;
		}

		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
		mono_image_close(image);

		// Don't forget to free the file data
		delete[] fileData;

		return assembly;
	}

	// mono_runtime_object_init() (the old, non-checked API) aborts the whole
	// process via an internal assert if the managed constructor throws.
	// Routing every call through mono_runtime_invoke() with a real exception
	// out-param instead lets us catch and log the failure and keep running.
	// Logs a single exception object's type + Message, with no InnerException traversal.
	void LogMonoException(MonoObject* exception, const char* context)
	{
		MonoClass* exceptionClass = mono_object_get_class(exception);
		MonoProperty* messageProp = mono_class_get_property_from_name(exceptionClass, "Message");
		MonoObject* messageObj = messageProp ? mono_property_get_value(messageProp, exception, nullptr, nullptr) : nullptr;

		if (messageObj != nullptr)
		{
			char* message = mono_string_to_utf8((MonoString*)messageObj);
			HD_CORE_ERROR("ScriptEngine: {} ({}): {}", context, mono_class_get_name(exceptionClass), message);
			mono_free(message);
		}
		else
		{
			HD_CORE_ERROR("ScriptEngine: {} ({})", context, mono_class_get_name(exceptionClass));
		}
	}

	bool CheckMonoException(MonoObject* exception, const char* context)
	{
		if (exception == nullptr)
			return false;

		LogMonoException(exception, context);

		// TypeInitializationException (and others) wrap the real failure in
		// InnerException - without walking it, all we know is "some static
		// constructor threw", not why.
		MonoClass* exceptionClass = mono_object_get_class((MonoObject*)exception);
		MonoProperty* innerProp = mono_class_get_property_from_name(exceptionClass, "InnerException");
		MonoObject* inner = innerProp ? mono_property_get_value(innerProp, exception, nullptr, nullptr) : nullptr;

		int depth = 0;
		while (inner != nullptr && depth < 8)
		{
			LogMonoException(inner, "  caused by");

			MonoClass* innerClass = mono_object_get_class(inner);
			MonoProperty* nextProp = mono_class_get_property_from_name(innerClass, "InnerException");
			inner = nextProp ? mono_property_get_value(nextProp, inner, nullptr, nullptr) : nullptr;
			depth++;
		}

		return true;
	}

	void PrintAssemblyTypes(MonoAssembly* assembly)
	{
		if (assembly == nullptr)
		{
			HD_CORE_ERROR("Cannot print types of a null assembly");
			return;
		}

		MonoImage* image = mono_assembly_get_image(assembly);
		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

			HD_CORE_TRACE("{}.{}", nameSpace, name);
		}
	}

	void ScriptEngine::InitMono()
	{
        // Кроссплатформенная установка путей к базовым библиотекам Mono (.NET)
#ifdef _WIN32
        // Стандартный путь установки Mono на Windows
        mono_set_assemblies_path("C:/Program Files/Mono/lib");
#elif defined(__linux__)
        // Системный путь к библиотекам Mono на Linux (Ubuntu/Debian/Arch)
        mono_set_assemblies_path("/usr/lib");
#elif defined(__APPLE__)
        // На случай, если решишь портировать на macOS
        mono_set_assemblies_path("/Library/Frameworks/Mono.framework/Versions/Current/lib");
#else
        #error "Unknown platform for Mono setup!"
#endif

		MonoDomain* rootDomain = mono_jit_init("HydraJITRuntime");
		HD_CORE_ASSERT(rootDomain);

		// Store the root domain pointer
		s_Data->RootDomain = rootDomain;

		// Create an App Domain
		s_Data->AppDomain = mono_domain_create_appdomain("HydraScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		// Must happen before any managed code that calls one of these runs,
		// so register right after the domains are set up.
		ScriptGlue::RegisterFunctions();

		// Move this maybe
		// NOTE: relative to the working directory of the running app (e.g. HydraEditor/),
		// not the repo root - the assembly is built to <repo>/Hydra/Resources/Scripts.
		s_Data->CoreAssembly = LoadCSharpAssembly("../Hydra/Resources/Scripts/Hydra-ScriptCore.dll");
		if (s_Data->CoreAssembly == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to load Hydra-ScriptCore.dll, scripting is disabled");
			return;
		}

		PrintAssemblyTypes(s_Data->CoreAssembly);

		MonoImage* assemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		MonoClass* monoClass = mono_class_from_name(assemblyImage, "Hydra", "Main");
		if (monoClass == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to find Hydra.Main in Hydra-ScriptCore.dll");
			return;
		}

		// 1. create an object and call its constructor.
		// NOTE: mono_runtime_object_init() aborts the process on a failed ctor
		// instead of returning an error, so invoke .ctor via mono_runtime_invoke()
		// (which gives us a real exception out-param) instead.
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		if (instance == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to allocate an instance of Hydra.Main");
			return;
		}

		MonoObject* exception = nullptr;

		MonoMethod* ctorFunc = mono_class_get_method_from_name(monoClass, ".ctor", 0);
		if (ctorFunc == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: Hydra.Main has no parameterless constructor");
			return;
		}

		mono_runtime_invoke(ctorFunc, instance, nullptr, &exception);
		if (CheckMonoException(exception, "Hydra.Main..ctor"))
			return;

		// 2. call function
		MonoMethod* printMessageFunc = mono_class_get_method_from_name(monoClass, "PrintMessage", 0);
		mono_runtime_invoke(printMessageFunc, instance, nullptr, &exception);
		CheckMonoException(exception, "Hydra.Main.PrintMessage");

		// 3. call function with param
		MonoMethod* printIntFunc = mono_class_get_method_from_name(monoClass, "PrintInt", 1);

		int value = 5;
		void* param = &value;

		exception = nullptr;
		mono_runtime_invoke(printIntFunc, instance, &param, &exception);
		CheckMonoException(exception, "Hydra.Main.PrintInt");

		MonoMethod* printIntsFunc = mono_class_get_method_from_name(monoClass, "PrintInts", 2);
		int value2 = 508;
		void* params[2] =
		{
			&value,
			&value2
		};
		exception = nullptr;
		mono_runtime_invoke(printIntsFunc, instance, params, &exception);
		CheckMonoException(exception, "Hydra.Main.PrintInts");

		MonoString* monoString = mono_string_new(s_Data->AppDomain, "Hello World from C++!");
		MonoMethod* printCustomMessageFunc = mono_class_get_method_from_name(monoClass, "PrintCustomMessage", 1);
		void* stringParam = monoString;
		exception = nullptr;
		mono_runtime_invoke(printCustomMessageFunc, instance, &stringParam, &exception);
		CheckMonoException(exception, "Hydra.Main.PrintCustomMessage");

		// Proves the C# -> C++ internal call bridge (ScriptGlue) works: this
		// calls Hydra.Log.Info() in C#, which calls back into Log_LogMessage().
		MonoMethod* logViaEngineFunc = mono_class_get_method_from_name(monoClass, "LogViaEngine", 0);
		exception = nullptr;
		mono_runtime_invoke(logViaEngineFunc, instance, nullptr, &exception);
		CheckMonoException(exception, "Hydra.Main.LogViaEngine");
	}

	void ScriptEngine::ShutdownMono()
	{
		// Leaving Mono's JIT/GC running and just nulling these pointers was
		// letting the process start static teardown while Mono's background
		// threads (Finalizer, SGen worker, ...) were still alive, racing with
		// it and crashing with a SIGSEGV on exit.
		//
		// mono_domain_unload() specifically is fragile here: mono_domain_try_unload()
		// waits on an event that can hang/segfault (confirmed via gdb - crash was
		// inside mono_domain_try_unload -> mono_os_event_wait_one). Since the whole
		// process is exiting right after this anyway, there is nothing to gain from
		// unloading the app domain individually - go straight to a full runtime
		// shutdown via mono_jit_cleanup(), which stops the background threads.
		if (s_Data->RootDomain != nullptr)
		{
			mono_jit_cleanup(s_Data->RootDomain);
			s_Data->RootDomain = nullptr;
		}

		s_Data->AppDomain = nullptr;
	}

}