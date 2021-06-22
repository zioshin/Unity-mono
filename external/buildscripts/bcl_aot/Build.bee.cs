using System;
using System.Collections.Generic;
using System.Linq;
using Bee.Core;
using Bee.Tools;
using NiceIO;

class Build
{
	static void Main()
	{
		NPath root = "../../..";
		NPath monoDir = $@"{root}/incomingbuilds\win64\monodistribution\bin-x64";

		NPath unityJitRelative = @"monodistribution\lib\mono\unityjit-win32";
		NPath classLibDir = $@"{root}/incomingbuilds\classlibs\{unityJitRelative}";
		NPath outputDir = $@"{root}/incomingbuilds\aot-classlibs-win64\{unityJitRelative}";

		// TODO: this is the version yamato has right now. Do stevedore here.
		NPath msvcRoot = @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30037";

		var paths = new NPath[]
		{
			monoDir,
			$@"{msvcRoot}\bin\Hostx64\x64",
		};

		var _envVar = new Dictionary<string, string>
		{
			{"PATH", paths.Select(p => p.ToString(SlashMode.Native)).Append(Environment.GetEnvironmentVariable("PATH")).SeparateWith(";")},
			{"MONO_PATH", classLibDir.ToString(SlashMode.Native)}
		};

		NPath libpath = $@"{msvcRoot}\lib\x64";
		var libpathstr = $"/LIBPATH:\\\"{libpath.InQuotesResolved(SlashMode.Native)}\\\"";

		var classLibs = classLibDir.Files("*.dll");
		foreach (var classLib in classLibs)
		{
			var nativeTarget = classLib.ChangeExtension($".{classLib.Extension}.dll");
			var outputFile = outputDir.Combine(nativeTarget.FileName);

			Backend.Current.AddAction("MonoAot",
				new[] {nativeTarget},
				classLibs,
				monoDir.Combine("mono.exe").InQuotes(),
				new[] {$"--aot=keep-temps,ld-flags={libpathstr}", "--llvm", classLib.InQuotesResolved()},
				environmentVariables: _envVar);
			Backend.Current.SetupCopyFile(outputFile, nativeTarget);
		}
	}
}
