using System;
using System.IO;

namespace MonoBuildProgram
{
	public class Platform
	{
		public static bool IsWindows()
		{
			switch (Environment.OSVersion.Platform)
			{
				case PlatformID.Win32NT:
				case PlatformID.Win32S:
				case PlatformID.Win32Windows:
				case PlatformID.WinCE:
					return true;
			}
			return false;
		}

		public static bool IsLinux()
		{
			return !IsWindows() && Directory.Exists("/proc");
		}

		public static bool IsOSX()
		{
			return !IsWindows() && !IsLinux();
		}
	}
}
