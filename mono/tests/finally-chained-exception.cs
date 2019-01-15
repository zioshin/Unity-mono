using System;
using System.Runtime.InteropServices;

public class Test {

	public static int Main () {
		try {
			try	{
				((object)null).GetHashCode();
			} finally {
				// OutputDebugStringW raises an exception that
				// system handles. Ensure we properly chain
				// nested exceptions during finally block processing
				OutputDebugStringW("This should not crash.");
			}
		} catch (NullReferenceException) {
		}

		return 0;
	}

	[DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
	static extern void OutputDebugStringW(string lpOutputString);
}
