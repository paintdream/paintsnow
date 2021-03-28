using System;
using DotNetBridge;

namespace MoonSharp
{
	public class SharpBridge
	{
		public void TestVersion()
		{
			var leavesBridge = LeavesBridge.Instance;
			Console.WriteLine("Hello World! " + leavesBridge.GetScriptHandle());
		}
	}
}
