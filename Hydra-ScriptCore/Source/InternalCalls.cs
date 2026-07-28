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
	}
}
