#include "hdpch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Hydra/Core/UUID.h"
#include "Hydra/Core/KeyCodes.h"
#include "Hydra/Core/Input.h"

#include "Hydra/Scene/Scene.h"
#include "Hydra/Scene/Entity.h"

#include "mono/metadata/object.h"
#include "mono/metadata/reflection.h"

#include <box2d/box2d.h>

#if defined(__GNUC__) || defined(__clang__)
	#include <cxxabi.h>
#endif


namespace Hydra
{

	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;


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

	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		HD_CORE_ASSERT(s_EntityHasComponentFuncs.find(managedType) != s_EntityHasComponentFuncs.end());
		return s_EntityHasComponentFuncs.at(managedType)(entity);
	}

	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static void SpriteRendererComponent_GetColor(UUID entityID, glm::vec4* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<SpriteRendererComponent>().Color;
	}

	static void SpriteRendererComponent_SetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		entity.GetComponent<SpriteRendererComponent>().Color = *color;
	}

	static void CircleRendererComponent_GetColor(UUID entityID, glm::vec4* outColor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		*outColor = entity.GetComponent<CircleRendererComponent>().Color;
	}

	static void CircleRendererComponent_SetColor(UUID entityID, glm::vec4* color)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		entity.GetComponent<CircleRendererComponent>().Color = *color;
	}

	static void Rigidbody2DComponent_ApplyLinearImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		if (!rb2d.RuntimeBody)
			return;

		b2BodyId bodyId = *(b2BodyId*)rb2d.RuntimeBody;
		b2Body_ApplyLinearImpulse(bodyId, b2Vec2{ impulse->x, impulse->y }, b2Vec2{ point->x, point->y }, wake);
	}

	static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		HD_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		HD_CORE_ASSERT(entity);

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		if (!rb2d.RuntimeBody)
			return;

		b2BodyId bodyId = *(b2BodyId*)rb2d.RuntimeBody;
		b2Body_ApplyLinearImpulseToCenter(bodyId, b2Vec2{ impulse->x, impulse->y }, wake);
	}
	
	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

	// typeid(T).name() returns a human-readable "Hydra::TransformComponent" on
	// MSVC, but an Itanium-mangled "N5Hydra18TransformComponentE" (no ':' at
	// all) on GCC/Clang - stripping down to the part after the last ':' only
	// works on MSVC. Demangle first so this works on both.
	static std::string DemangleTypeName(const char* mangledName)
	{
#if defined(__GNUC__) || defined(__clang__)
		int status = 0;
		char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
		std::string result = (status == 0 && demangled != nullptr) ? demangled : mangledName;
		free(demangled);
		return result;
#else
		return mangledName;
#endif
	}

	template<typename... Component>
	static void RegisterComponent()
	{
		([]()
		{
			std::string typeName = DemangleTypeName(typeid(Component).name());
			size_t pos = typeName.find_last_of(':');
			std::string_view structName = pos == std::string::npos ? std::string_view(typeName) : std::string_view(typeName).substr(pos + 1);
			std::string managedTypename = fmt::format("Hydra.{}", structName);

			MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
			if (!managedType)
			{
				HD_CORE_ERROR("Could not find component type {}", managedTypename);
				return;
			}
			s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
		}(), ...);
	}

	template<typename... Component>
	static void RegisterComponent(ComponentGroup<Component...>)
	{
		RegisterComponent<Component...>();
	}

	void ScriptGlue::RegisterComponents()
	{
		RegisterComponent(ScriptableComponents{});
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

		HD_ADD_INTERNAL_CALL(Entity_HasComponent);
		HD_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		HD_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);

		HD_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColor);
		HD_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		HD_ADD_INTERNAL_CALL(CircleRendererComponent_GetColor);
		HD_ADD_INTERNAL_CALL(CircleRendererComponent_SetColor);

		HD_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulse);
		HD_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyLinearImpulseToCenter);

		HD_ADD_INTERNAL_CALL(Input_IsKeyDown);
	}

}