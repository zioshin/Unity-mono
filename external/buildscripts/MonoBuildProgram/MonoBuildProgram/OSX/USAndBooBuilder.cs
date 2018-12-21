using System;
using System.Text.RegularExpressions;
using NiceIO;

namespace MonoBuildProgram.OSX
{
	public class USAndBooBuilder : BaseBuilder
	{
		private NPath addToBuildResultsDir45;
		private NPath monoPrefix45;
		private NPath monoPrefixBin;
		private NPath booCheckout;
		private NPath unityScriptCheckout;
		private NPath unityScriptBuildDir;
		private NPath unityScriptLibDir;
		private NPath xBuild;
		private NPath mono;
		private string booRevision = "";
		private string unityScriptRevision = "";

		public USAndBooBuilder()
		{
			addToBuildResultsDir45 = AddToBuildResultsDir.Combine("lib").Combine("mono").Combine("4.5");
			monoPrefix45 = MonoPrefix.Combine("lib").Combine("mono").Combine("4.5");
			monoPrefixBin = MonoPrefix.Combine("bin");
			booCheckout = MonoRoot.Combine("boo").Combine("build");
			unityScriptCheckout = MonoRoot.Combine("unityscript").Combine("build");
			unityScriptBuildDir = MonoRoot.Combine("unityscript").Combine("build").Combine("build");
			unityScriptLibDir = MonoPrefix.Combine("lib").Combine("mono").Combine("unityscript");
		}


		public override void Clean()
		{
			if (BuilderOptions.Clean)
			{
				FileUtils.CleanOrCreateDirectory(monoPrefix45);
				FileUtils.CleanOrCreateDirectory(unityScriptBuildDir);
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
					DetermineExistingMonoPath();
					mono = ExistingExternalMonoRoot.Combine("mono");
					xBuild = ExistingExternalMonoRoot.Combine("xbuild");

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
				CopyArtifacts();
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
			CloneBooAndUnityScript();

			var booProjectPath = booCheckout.Combine("src").Combine("booc").Combine("Booc.csproj");
			if (!booProjectPath.FileExists())
			{
				throw new Exception($"Unable to locate xbuild at {booProjectPath}");
			}

			CompileWithXBuild(booProjectPath);

			var xBuildOutputDir = booCheckout.Combine("ide-build");

			FileUtils.CleanOrCreateDirectory(monoPrefix45);
			foreach (var file in xBuildOutputDir.Files())
			{
				if (file.FileName.Contains("Boo.Lang") && file.HasExtension("dll"))
				{
					Console.WriteLine($">> Copying {file} to {monoPrefix45}");
					file.Copy(monoPrefix45);
				}
			}
			xBuildOutputDir.Combine("booc.exe").Copy(monoPrefix45);

			//TODO: There is nothing inside addToBuildResultsDir45.Files(), so for now just copying from BuildProgram.AddToBuildResultsDir.Combine("bin").Files()
			//addToBuildResultsDir45.CopyFiles(monoPrefix45, recurse: true);
			/*foreach (var file in addToBuildResultsDir45.Files())
			{
				file.Copy(monoPrefix45);
				FileFileUtils.Run("chmod", $"755 {monoPrefix45.Combine(file.FileName)}", ".");
			}*/

			//TODO this is not needed! ?
			/*foreach (var file in BuildProgram.AddToBuildResultsDir.Combine("bin").Files())
			{
				Console.WriteLine($">> Copying {file} to {monoPrefix45}");
				file.Copy(monoPrefix45);
				FileFileUtils.Run("chmod", $"755 {monoPrefix45.Combine(file.FileName)}", monoPrefix45);
			}*/

			//TODO Check if it exists or find existing mono - This should actually be run with built mono

			var booc = monoPrefix45.Combine("booc.exe");
			var extensionsDll = monoPrefix45.Combine("Boo.Lang.Extensions.dll");
			var extensionsSrc = booCheckout.Combine("src").Combine("Boo.Lang.Extensions");
			var compilerDll = monoPrefix45.Combine("Boo.Lang.Compiler.dll");
			Process.Run(mono, $"{booc} -debug- -out:{extensionsDll} -noconfig -nostdlib -srcdir:{extensionsSrc}  -r:System.dll -r:System.Core.dll -r:mscorlib.dll -r:Boo.Lang.dll -r:{compilerDll}", monoPrefix45);

			var usefulDll = monoPrefix45.Combine("Boo.Lang.Useful.dll");
			var usefulSrc = booCheckout.Combine("src").Combine("Boo.Lang.Useful");
			var parserDll = monoPrefix45.Combine("Boo.Lang.Parser");
			Process.Run(mono, $"{booc} -debug- -out:{usefulDll} -srcdir:{usefulSrc} -r:{parserDll}", monoPrefix45);

			var patternMatchingDll = monoPrefix45.Combine("Boo.Lang.PatternMatching.dll");
			var patternMatchingSrc = booCheckout.Combine("src").Combine("Boo.Lang.PatternMatching");
			Process.Run(mono, $"{booc} -debug- -out:{patternMatchingDll} -srcdir:{patternMatchingSrc}", monoPrefix45);

			var unityScriptLangDll = monoPrefix45.Combine("UnityScript.Lang.dll");
			var unityScriptLangSrc = unityScriptCheckout.Combine("src").Combine("UnityScript.Lang");
			Process.Run(mono, $"{booc} -debug- -out:{unityScriptLangDll} -srcdir:{unityScriptLangSrc}", monoPrefix45);

			var unityScriptDll = monoPrefix45.Combine("UnityScript.dll");
			var unityScriptSrc = unityScriptCheckout.Combine("src").Combine("UnityScript");
			Process.Run(mono, $"{booc} -debug- -out:{unityScriptDll} -srcdir:{unityScriptSrc} -r:{unityScriptLangDll} -r:{parserDll} -r:{patternMatchingDll}", monoPrefix45);

			var usExe = monoPrefix45.Combine("us.exe");
			var usSrc = unityScriptCheckout.Combine("src").Combine("us");
			Process.Run(mono, $"{booc} -debug- -out:{usExe} -srcdir:{usSrc} -r:{unityScriptLangDll} -r:{unityScriptDll} -r:{usefulDll}", monoPrefix45);

			FileUtils.CleanOrCreateDirectory(unityScriptBuildDir);

			//CopyArtifacts
			Console.WriteLine($">> Copying build artifacts to {unityScriptBuildDir}");
			foreach (var file in monoPrefix45.Files())
			{
				if (file.FileName.Contains("Boo.") || file.FileName.Contains("UnityScript."))
				{
					file.Copy(unityScriptBuildDir);
				}
			}

			usExe.Copy(unityScriptBuildDir);
			booc.Copy(unityScriptBuildDir);

			//Put unityscript and boo into their own directories that we can reference for compilation only in Unity
			FileUtils.CleanOrCreateDirectory(unityScriptLibDir);
			unityScriptBuildDir.CopyFiles(unityScriptLibDir, recurse: true);
		}


		private void CompileWithXBuild(NPath projectPath)
		{
			Console.WriteLine($">> Compiling {projectPath}");
			var target = "/t:Rebuild";
			Console.WriteLine($">> Running : {xBuild} {projectPath} {target}");
			Process.Run(xBuild, $"{projectPath} {target}", ".");
		}

		private void CopyArtifacts()
		{
			var monoPrefixLibMonoDir = MonoPrefix.Combine("lib").Combine("mono").Combine("4.5");
			var unityJitLibMono = MonoPrefix.Combine("lib").Combine("mono").Combine("unityjit");
			FileUtils.CleanOrCreateDirectory(unityJitLibMono);

			Console.WriteLine(">> Copying Unity Script and Boo Dlls from 4.5 profile to unityjit profile ...");
			monoPrefixLibMonoDir.CopyFiles(unityJitLibMono, recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "Boo.*\\.dll"));
			monoPrefixLibMonoDir.CopyFiles(unityJitLibMono, recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "UnityScript.*\\.dll"));

			monoPrefixLibMonoDir.Combine("booc.exe").Copy(unityJitLibMono);
			monoPrefixLibMonoDir.Combine("us.exe").Copy(unityJitLibMono);
		}

	}
}
