#if UNITY

using System;
using System.Reflection;
using NUnit.Framework;

namespace MonoTests.Mono.Unity
{
	[TestFixture]
	public class UnityUtilsTest
	{
		[Test]
		public void VerifyPlatformIsNotUnknown()
		{
			Assert.That(GetInternalUnityUtilsMethod("GetPlatformName").Invoke(null, new object[] {}),
				Is.Not.EqualTo("Unknown Platform"));
		}

		// We would like to use InteralsVisibleTo in mscorlib to allow this test code to
		// call internal methods. However, InternalsVisibleTo exposes conflcits between the
		// internal type System.Diagnostics.Assert and NUnit.Assert in most tests. Therefore,
		// we settle for reflection instead.
		MethodInfo GetInternalUnityUtilsMethod(string name)
		{
			const string unityUtilsTypeName = "Mono.Unity.UnityUtils";
			var unityUtilsType = typeof(int).Assembly.GetType(unityUtilsTypeName);
			Assert.That(unityUtilsType, Is.Not.Null, string.Format("Unable to reflection the type {0}",
				unityUtilsType));

			var method = unityUtilsType.GetMethod(name);
			Assert.That(method, Is.Not.Null, string.Format("Unable to reflect the method {0} in type {1}",
				name, unityUtilsTypeName));

			return method;
		}
	}
}

#endif // UNITY