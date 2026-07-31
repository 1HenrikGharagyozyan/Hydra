using System;
using System.Runtime.CompilerServices;

namespace Hydra
{
	// Declarations for the C++ functions registered via mono_add_internal_call
	// in ScriptGlue::RegisterFunctions(). Names/signatures must match exactly.
	internal static class InternalCalls
	{
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog(string text, int parameter);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Vector(ref Vector3 parameter, out Vector3 result);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static float NativeLog_VectorDot(ref Vector3 parameter);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Trace(string message);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Info(string message);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Warn(string message);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Error(string message);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_GetColor(ulong entityID, out Vector4 color);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetColor(ulong entityID, ref Vector4 color);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void CircleRendererComponent_GetColor(ulong entityID, out Vector4 color);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void CircleRendererComponent_SetColor(ulong entityID, ref Vector4 color);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_ApplyLinearImpulse(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(ulong entityID, ref Vector2 impulse, bool wake);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Input_IsKeyDown(KeyCode keycode);
	}
}
