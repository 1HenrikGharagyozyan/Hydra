namespace Hydra
{
	// Mirrors Hydra::ScriptLogLevel in ScriptGlue.cpp
	public enum LogLevel
	{
		Trace = 0,
		Info = 1,
		Warn = 2,
		Error = 3
	}

	// Routes through the engine's own logger instead of System.Console, so
	// script output shows up alongside the rest of the engine's log.
	public static class Log
	{
		public static void Trace(string message) => InternalCalls.Log_LogMessage(message, (int)LogLevel.Trace);
		public static void Info(string message) => InternalCalls.Log_LogMessage(message, (int)LogLevel.Info);
		public static void Warn(string message) => InternalCalls.Log_LogMessage(message, (int)LogLevel.Warn);
		public static void Error(string message) => InternalCalls.Log_LogMessage(message, (int)LogLevel.Error);
	}
}
