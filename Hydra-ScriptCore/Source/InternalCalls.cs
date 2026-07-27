using System.Runtime.CompilerServices;

namespace Hydra
{
	// Declarations for the C++ functions registered via mono_add_internal_call
	// in ScriptGlue::RegisterFunctions(). Names/signatures must match exactly.
	internal static class InternalCalls
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Log_LogMessage(string message, int level);
	}
}
