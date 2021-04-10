using System;
using DotNetBridge;

namespace MoonSharp
{
	public class SharpBridge
	{
		public void TestVersion()
		{
			var leavesBridge = new LeavesBridge();
			Console.WriteLine("Hello World! " + leavesBridge.GetScriptHandle());
		}
	}
}
