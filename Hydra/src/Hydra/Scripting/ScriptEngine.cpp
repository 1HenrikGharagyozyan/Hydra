#include "hdpch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"
#include "mono/utils/mono-publib.h"

#ifdef __linux__
	#include <dlfcn.h>
#endif


namespace Hydra 
{

	namespace Utils 
	{

		// TODO: move to FileSystem class
		static char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize)
		{
			std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

			if (!stream)
			{
				// Failed to open the file
				return nullptr;
			}

			std::streampos end = stream.tellg();
			stream.seekg(0, std::ios::beg);
			uint64_t size = end - stream.tellg();

			if (size == 0)
			{
				// File is empty
				return nullptr;
			}

			char* buffer = new char[size];
			stream.read((char*)buffer, size);
			stream.close();

			*outSize = (uint32_t)size;
			return buffer;
		}

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath)
		{
			uint32_t fileSize = 0;
			char* fileData = ReadBytes(assemblyPath, &fileSize);

			if (fileData == nullptr)
			{
				HD_CORE_ERROR("Failed to read C# assembly at '{}'", assemblyPath.string());
				return nullptr;
			}

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				HD_CORE_ERROR("Failed to open C# assembly image '{}': {}", assemblyPath.string(), mono_image_strerror(status));
				delete[] fileData;
				return nullptr;
			}

			std::string pathString = assemblyPath.string();
			MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
			mono_image_close(image);

			// Don't forget to free the file data
			delete[] fileData;

			return assembly;
		}

		void PrintAssemblyTypes(MonoAssembly* assembly)
		{
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

		static void LogMonoException(MonoObject* exception, const char* context)
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

		// mono_runtime_object_init() (the old, non-checked API) aborts the whole
		// process via an internal assert if a managed constructor throws, and
		// mono_runtime_invoke() silently swallows exceptions when its exc
		// out-param is null. Routing every call through mono_runtime_invoke()
		// with a real exception out-param lets us catch and log failures
		// (including the InnerException chain, since TypeInitializationException
		// always wraps the real cause there) instead of crashing blind.
		static bool CheckMonoException(MonoObject* exception, const char* context)
		{
			if (exception == nullptr)
				return false;

			LogMonoException(exception, context);

			MonoClass* exceptionClass = mono_object_get_class(exception);
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

#ifdef __linux__
		// Some Mono builds (confirmed on Ubuntu 24.04's packaged mono-complete
		// 6.8.0.105) route culture-aware number formatting - e.g. float.ToString(),
		// hit by any string interpolation over a float - through
		// DllImport("System.Native"). Classic Mono never ships that native
		// library; only .NET (Core/5+) does, as libSystem.Native.so. Without it,
		// the very first such call throws a DllNotFoundException wrapped in a
		// TypeInitializationException for 'Sys'.
		//
		// If a .NET runtime happens to be installed, dlopen() its copy of the
		// library directly by full path before mono_jit_init(). Once a library
		// is resident in the process, a later dlopen() by bare soname (which is
		// how Mono's own DllImport resolution works) finds the already-loaded
		// copy directly - unlike LD_LIBRARY_PATH, which glibc only parses once
		// at process startup, so setting it this late has no effect on
		// subsequent lookups. No-op (and Mono just throws as before) if nothing
		// is found.
		static void PreloadDotNetNativeLibrary()
		{
			static const char* searchRoots[] = {
				"/usr/lib/dotnet/shared/Microsoft.NETCore.App",
				"/usr/share/dotnet/shared/Microsoft.NETCore.App",
			};

			std::filesystem::path bestPath;
			std::string bestVersion;

			for (const char* root : searchRoots)
			{
				std::error_code ec;
				if (!std::filesystem::exists(root, ec))
					continue;

				for (const auto& entry : std::filesystem::directory_iterator(root, ec))
				{
					if (!entry.is_directory())
						continue;

					std::filesystem::path candidate = entry.path() / "libSystem.Native.so";
					if (!std::filesystem::exists(candidate, ec))
						continue;

					std::string version = entry.path().filename().string();
					if (bestVersion.empty() || version > bestVersion)
					{
						bestVersion = version;
						bestPath = candidate;
					}
				}
			}

			if (bestPath.empty())
				return;

			if (dlopen(bestPath.c_str(), RTLD_NOW | RTLD_GLOBAL) == nullptr)
			{
				HD_CORE_WARN("ScriptEngine: found but failed to preload '{}': {}", bestPath.string(), dlerror());
				return;
			}

			HD_CORE_TRACE("ScriptEngine: preloaded .NET runtime native library '{}'", bestPath.string());
		}
#endif

	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		ScriptClass EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, Ref<ScriptInstance>> EntityInstances;

		// Runtime
		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data = nullptr;

	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		InitMono();
		// Relative to the running app's working directory (e.g. HydraEditor/),
		// not the repo root - the assembly is built to <repo>/Hydra/Resources/Scripts.
		LoadAssembly("Resources/Scripts/Hydra-ScriptCore.dll");

		if (s_Data->CoreAssemblyImage == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to load Hydra-ScriptCore.dll, scripting is disabled");
			return;
		}

		// Optional: the actual project's own script assembly (built from
		// SandboxProject), referencing Hydra-ScriptCore. If it hasn't been
		// built yet, LoadAppAssembly()/LoadAssemblyClasses() no-op gracefully
		// and only the bare Hydra.Entity base class stays available.
		LoadAppAssembly("SandboxProject/Assets/Scripts/Binaries/Sandbox.dll");
		LoadAssemblyClasses();

		ScriptGlue::RegisterComponents();
		ScriptGlue::RegisterFunctions();

		// Retrieve and instantiate class
		s_Data->EntityClass = ScriptClass("Hydra", "Entity", true);

#if 0
		MonoObject* instance = s_Data->EntityClass.Instantiate();
		if (instance == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to instantiate Hydra.Entity, scripting is disabled");
			return;
		}

		// Call method
		MonoMethod* printMessageFunc = s_Data->EntityClass.GetMethod("PrintMessage", 0);
		s_Data->EntityClass.InvokeMethod(instance, printMessageFunc);

		// Call method with param
		MonoMethod* printIntFunc = s_Data->EntityClass.GetMethod("PrintInt", 1);

		int value = 5;
		void* param = &value;

		s_Data->EntityClass.InvokeMethod(instance, printIntFunc, &param);

		MonoMethod* printIntsFunc = s_Data->EntityClass.GetMethod("PrintInts", 2);
		int value2 = 508;
		void* params[2] =
		{
			&value,
			&value2
		};
		s_Data->EntityClass.InvokeMethod(instance, printIntsFunc, params);

		MonoString* monoString = mono_string_new(s_Data->AppDomain, "Hello World from C++!");
		MonoMethod* printCustomMessageFunc = s_Data->EntityClass.GetMethod("PrintCustomMessage", 1);
		void* stringParam = monoString;
		s_Data->EntityClass.InvokeMethod(instance, printCustomMessageFunc, &stringParam);
#endif
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		delete s_Data;
	}

	
	void ScriptEngine::InitMono()
	{
		// "mono/lib" is relative to the running app's working directory (e.g.
		// HydraEditor/), which doesn't have a mono/lib folder - Mono can't find
		// its own class libraries there and crashes. Point it at the actual
		// installed location instead.
#ifdef _WIN32
		mono_set_assemblies_path("C:/Program Files/Mono/lib");
#elif defined(__linux__)
		mono_set_assemblies_path("/usr/lib");
#elif defined(__APPLE__)
		mono_set_assemblies_path("/Library/Frameworks/Mono.framework/Versions/Current/lib");
#endif

#ifdef __linux__
		Utils::PreloadDotNetNativeLibrary();
#endif

		MonoDomain* rootDomain = mono_jit_init("HydraJITRuntime");
		HD_CORE_ASSERT(rootDomain);

		// Store the root domain pointer
		s_Data->RootDomain = rootDomain;
	}

	void ScriptEngine::ShutdownMono()
	{
		// mono_domain_unload() is left out on purpose: mono_domain_try_unload()
		// waits on an event that can hang/segfault, and since the whole process
		// exits right after this anyway there's nothing to gain from unloading
		// the app domain individually. mono_jit_cleanup() still has to run
		// though - without it Mono's background threads (Finalizer, SGen
		// worker, ...) are still alive when static teardown starts, racing
		// with it and crashing with a SIGSEGV on exit.
		s_Data->AppDomain = nullptr;

		if (s_Data->RootDomain != nullptr)
		{
			mono_jit_cleanup(s_Data->RootDomain);
			s_Data->RootDomain = nullptr;
		}
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Create an App Domain
		s_Data->AppDomain = mono_domain_create_appdomain("HydraScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		// Move this maybe
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath);
		if (s_Data->CoreAssembly == nullptr)
			return;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		// Utils::PrintAssemblyTypes(s_Data->CoreAssembly);
	}

	void ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppAssembly = Utils::LoadMonoAssembly(filepath);
		if (s_Data->AppAssembly == nullptr)
			return;

		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);
		// Utils::PrintAssemblyTypes(s_Data->AppAssembly);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.find(fullClassName) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		const auto& sc = entity.GetComponent<ScriptComponent>();
		if (ScriptEngine::EntityClassExists(sc.ClassName))
		{
			Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entity.GetUUID()] = instance;
			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Timestep ts)
	{
		UUID entityUUID = entity.GetUUID();
		HD_CORE_ASSERT(s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end());

		Ref<ScriptInstance> instance = s_Data->EntityInstances[entityUUID];
		instance->InvokeOnUpdate((float)ts);
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->SceneContext = nullptr;

		s_Data->EntityInstances.clear();
	}

	std::unordered_map<std::string, Ref<ScriptClass>> ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		s_Data->EntityClasses.clear();

		if (s_Data->AppAssemblyImage == nullptr)
		{
			HD_CORE_WARN("ScriptEngine: no app script assembly loaded (SandboxProject not built yet?) - only Hydra.Entity is available");
			return;
		}

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Hydra", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string fullName;
			if (strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, name);
			else
				fullName = name;

			MonoClass* monoClass = mono_class_from_name(s_Data->AppAssemblyImage, nameSpace, name);

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (isEntity)
			{
				s_Data->EntityClasses[fullName] = CreateRef<ScriptClass>(nameSpace, name);
				HD_CORE_TRACE("ScriptEngine: found script class '{}'", fullName);
			}
		}

		HD_CORE_INFO("ScriptEngine: {} script class(es) available", s_Data->EntityClasses.size());
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		if (monoClass == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: attempted to instantiate a null class");
			return nullptr;
		}

		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		if (instance == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: failed to allocate a {} instance", mono_class_get_name(monoClass));
			return nullptr;
		}

		MonoMethod* ctor = mono_class_get_method_from_name(monoClass, ".ctor", 0);
		if (ctor == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: {} has no parameterless constructor", mono_class_get_name(monoClass));
			return nullptr;
		}

		MonoObject* exception = nullptr;
		mono_runtime_invoke(ctor, instance, nullptr, &exception);
		if (Utils::CheckMonoException(exception, "constructor"))
			return nullptr;

		return instance;
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		MonoImage* image = isCore ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage;
		if (image == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: cannot look up {}.{} - {} assembly not loaded", classNamespace, className, isCore ? "core" : "app");
			return;
		}

		m_MonoClass = mono_class_from_name(image, classNamespace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		if (method == nullptr)
		{
			HD_CORE_ERROR("ScriptEngine: attempted to invoke a null method");
			return nullptr;
		}

		MonoObject* exception = nullptr;
		MonoObject* result = mono_runtime_invoke(method, instance, params, &exception);
		Utils::CheckMonoException(exception, mono_method_get_name(method));
		return result;
	}


	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);

		// Call Entity constructor
		{
			UUID entityID = entity.GetUUID();
			void* param = &entityID;
			m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, &param);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &ts;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

}