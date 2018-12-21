using System;
using System.IO;
using MonoBuildProgram.Utils.Utils;
using NiceIO;

namespace MonoBuildProgram.Windows
{
	public class WindowsBareMinimumBuilder : BaseBuilder
	{
		private string MsBuildVersion = "14.0";
		private NPath monoDistroLibmono;
		private NPath outputDir;
		private NPath booCheckout;
		private NPath unityScriptCheckout;
		private string booRevision;
		private string unityScriptRevision;

		public WindowsBareMinimumBuilder()
		{
			monoDistroLibmono = DistributionDir.Combine("lib").Combine("mono");
			outputDir = new NPath(Environment.GetEnvironmentVariable("TEMP")).Combine("output").Combine("BareMinimum");
			booCheckout = MonoRoot.Combine("boo").Combine("build");
			unityScriptCheckout = MonoRoot.Combine("us").Combine("build");
		}

		public override void Clean()
		{
			if (BuilderOptions.Clean)
			{
				FileUtils.CleanOrCreateDirectory(outputDir);
				FileUtils.CleanOrCreateDirectory(BuildsRoot);
			}
			else
			{
				Console.WriteLine(">> Skipping clean ...");
			}
		}

		public override void Build()
		{
			if (BuilderOptions.Build)
			{
				CloneBooAndUnityScript();

				try
				{
					BuildUnityScriptForUnity();
				}
				catch (Exception e)
				{
					Console.WriteLine("Build unity script failed!");
					Console.WriteLine(e);
					throw;
				}
				
			}
		}

		public override void Artifacts()
		{
			if (BuilderOptions.Artifact)
			{
				CreateArtifacts();
				WriteVersionInfo();
			}
		}

		private void CloneBooAndUnityScript()
		{
			var branch = "unity-trunk";
			var booRepoUrl = "git://github.com/Unity-Technologies/boo.git";
			Git.GitClone(booRepoUrl, booCheckout, branch);

			var unityscriptRepoUrl = "git://github.com/Unity-Technologies/unityscript.git";
			Git.GitClone(unityscriptRepoUrl, unityScriptCheckout, branch);

			booRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", booCheckout);
			Console.WriteLine($">> Boo revision: {booRevision}");

			unityScriptRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", unityScriptCheckout);
			Console.WriteLine($">> UnityScript revision: {unityScriptRevision}");
		}

		private void BuildUnityScriptForUnity()
		{
			var booProjectPath = booCheckout.Combine("src").Combine("booc").Combine("Booc.csproj");
			if (!booProjectPath.FileExists())
			{
				throw new Exception($"Unable to locate boo project at {booProjectPath}");
			}

			var commonDefines = "NO_SERIALIZATION_INFO,NO_SYSTEM_PROCESS,NO_ICLONEABLE,MSBUILD,IGNOREKEYFILE";
			var optionalArguments = $"/property:TargetFrameworkVersion=4.0 /property:DefineConstants=\"{commonDefines}\" /property:OutputPath={outputDir}/wp8";
			Compile(booProjectPath, optionalArguments);

			optionalArguments = $"/property:TargetFrameworkVersion=4.0 /property:DefineConstants=\"{commonDefines},NO_SYSTEM_REFLECTION_EMIT\" /property:OutputPath={outputDir}/wsa";
			Compile(booProjectPath, optionalArguments);

			RunBooc();
		}

		private void Compile(NPath projectPath, string optionalArguments)
		{
			Console.WriteLine($">> Compiling {projectPath}");

			var msbuild = FileUtils.ProgramFilesx86().Combine("MSBuild").Combine(MsBuildVersion).Combine("Bin").Combine("MSBuild.exe");
			if (msbuild.FileExists())
			{
				var target = "/t:Rebuild";
				var config = $"/p:Configuration={BuilderOptions.Configuration}";
				var properties = "/p:AssemblyOriginatorKeyFile= /p:SignAssembly=false /p:MonoTouch=True";
				Console.WriteLine($"{msbuild} {projectPath} {properties} {target} {config} {optionalArguments}");
				Process.Run(msbuild, $"{projectPath} {properties} {target} {config} {optionalArguments}", ".");
			}
			else
			{
				throw new Exception(msbuild + " does not exist");
			}
		}

		private void RunBooc()
		{
			var booc = outputDir.Combine("wsa").Combine("booc");
			var extensionsDll = outputDir.Combine("wsa").Combine("Boo.Lang.Extensions.dll");
			var extensionsSrc = booCheckout.Combine("src").Combine("Boo.Lang.Extensions");
			var compilerDll = outputDir.Combine("wsa").Combine("Boo.Lang.Compiler.dll");
			Process.Run(booc, $"-debug- -out:{extensionsDll} -srcdir:{extensionsSrc} -r:{compilerDll}", ".");

			var usefulDll = outputDir.Combine("wsa").Combine("Boo.Lang.Useful.dll");
			var usefulSrc = booCheckout.Combine("src").Combine("Boo.Lang.Useful");
			var parserDll = outputDir.Combine("wsa").Combine("Boo.Lang.Parser");
			Process.Run(booc, $"-debug- -out:{usefulDll} -srcdir:{usefulSrc} -r:{parserDll}", ".");

			var patternMatchingDll = outputDir.Combine("wsa").Combine("Boo.Lang.PatternMatching.dll");
			var patternMatchingSrc = booCheckout.Combine("src").Combine("Boo.Lang.PatternMatching");
			Process.Run(booc, $"-debug- -out:{patternMatchingDll} -srcdir:{patternMatchingSrc}", ".");

			var unityScriptLangDll = outputDir.Combine("UnityScript.Lang.dll");
			var unityScriptLangSrc = unityScriptCheckout.Combine("src").Combine("UnityScript.Lang");
			Process.Run(booc, $"-debug- -out:{unityScriptLangDll} -srcdir:{unityScriptLangSrc} -r:{extensionsDll}", ".");
		}

		private void CreateArtifacts()
		{
			FileUtils.CleanOrCreateDirectory(monoDistroLibmono);
			var bareMiniumDistroDir = monoDistroLibmono.CreateDirectory("bare-minumum");
			var wsaDistroDir = bareMiniumDistroDir.CreateDirectory("wsa");
			var wp8DistroDir = bareMiniumDistroDir.CreateDirectory("wp8");

			outputDir.Combine("wsa").Combine("Boo.Lang.dll").Copy(wsaDistroDir);
			outputDir.Combine("wsa").Combine("Boo.Lang.pdb").Copy(wsaDistroDir);
			outputDir.Combine("wp8").Combine("Boo.Lang.dll").Copy(wp8DistroDir);
			outputDir.Combine("wp8").Combine("Boo.Lang.pdb").Copy(wp8DistroDir);
			outputDir.Combine("UnityScript.Lang.dll").Copy(bareMiniumDistroDir);
		}

		private void WriteVersionInfo()
		{
			var versionsOutputFile = BuildsRoot.Combine("versions.txt");
			Console.WriteLine($">> Writing version information to {versionsOutputFile}");

			using (var writer = new StreamWriter(versionsOutputFile))
			{
				writer.Write($"boo-revision = {booRevision}");
				writer.WriteLine($"unity-script-revision = {unityScriptRevision}");
				writer.Write(DateTime.Now.ToString("yyyy-MM-ddTHH:mm:sszzz"));
			}
		}
	}
}
