namespace Hydra
{
	// Routes through the engine's own logger via internal calls instead of
	// System.Console, so script output goes through the same logging pipeline
	// as the rest of the engine on every platform.
	public static class Log
	{
		public static void Trace(string message) => InternalCalls.NativeLog_Trace(message);
		public static void Info(string message) => InternalCalls.NativeLog_Info(message);
		public static void Warn(string message) => InternalCalls.NativeLog_Warn(message);
		public static void Error(string message) => InternalCalls.NativeLog_Error(message);
	}
}
