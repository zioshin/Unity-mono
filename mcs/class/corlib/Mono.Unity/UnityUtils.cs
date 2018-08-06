#if UNITY

using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace Mono.Unity
{
	internal static class UnityUtils
	{
		[MethodImplAttribute (MethodImplOptions.InternalCall)]
		extern static string GetUnityPlatformName();

		public static string GetPlatformName()
		{
			return GetUnityPlatformName();
		}
	}
}

#endif // UNITY