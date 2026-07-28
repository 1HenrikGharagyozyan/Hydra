namespace Hydra
{
	public struct Vector3
	{
		public float X;
		public float Y;
		public float Z;

		public Vector3(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
		}
	}

	public class Entity
	{
		public float FloatVar { get; set; }

		public Entity()
		{
			Hydra.Log.Info("Main constructor!");
			Log("AAstroPhysiC", 8058);

			Vector3 pos = new Vector3(5, 2.5f, 1);
			Vector3 result = Log(pos);
			Hydra.Log.Info($"{result.X}, {result.Y}, {result.Z}");
			Hydra.Log.Info($"{InternalCalls.NativeLog_VectorDot(ref pos)}");
		}

		public void PrintMessage()
		{
			Hydra.Log.Info("Hello World from C#!");
		}

		public void PrintInt(int value)
		{
			Hydra.Log.Info($"C# says: {value}");
		}

		public void PrintInts(int value1, int value2)
		{
			Hydra.Log.Info($"C# says: {value1} and {value2}");
		}

		public void PrintCustomMessage(string message)
		{
			Hydra.Log.Info($"C# says: {message}");
		}

		private void Log(string text, int parameter)
		{
			InternalCalls.NativeLog(text, parameter);

		}
		private Vector3 Log(Vector3 parameter)
		{
			InternalCalls.NativeLog_Vector(ref parameter, out Vector3 result);
			return result;
		}

	}

}
