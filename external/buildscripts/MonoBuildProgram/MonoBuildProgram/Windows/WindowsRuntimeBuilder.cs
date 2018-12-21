using System;

namespace MonoBuildProgram.Windows
{
	public class WindowsRuntimeBuilder : BaseBuilder
	{
		private readonly string MsBuildVersion = "14.0";

		private string Architecture { get; }

		public WindowsRuntimeBuilder()
		{
			Architecture = BuilderOptions.Arch32 ? "win32" : "x64";
		}

		public override void Clean()
		{
			if (BuilderOptions.Clean)
			{
				Console.WriteLine(">> Cleaning mono prefix ...");
				MonoPrefix.DeleteContents();
			}
			else
			{
				Console.WriteLine(">> Skipping cleaning monoprefix ...");
			}
		}

		public override void Build()
		{
			if (BuilderOptions.Build)
			{
				string[] gc = { "bdwgc", "sgen" };
				foreach (var item in gc)
				{
					try
					{
						Compile(item);
					}
					catch (Exception e)
					{
						Console.WriteLine("Mono build failed!");
						Console.WriteLine(e);
						throw;
					}
				}
				CopyMsbuildOutputToMonoPrefix();
			}
		}

		public override void Artifacts()
		{
			if (BuilderOptions.Artifact)
			{
				CreateArtifacts();
			}
		}

		private void Compile(string gc)
		{
			Console.WriteLine(">> Compiling mono.sln ...");

			var msbuild = FileUtils.ProgramFilesx86().Combine("MSBuild").Combine(MsBuildVersion).Combine("Bin").Combine("MSBuild.exe");

			if (msbuild.FileExists())
			{
				var solution = MonoRoot.Combine("msvc").Combine("mono.sln");
				var target = BuilderOptions.Clean ? "/t:Clean,Build" : "/t:Build";
				var properties = $"/p:Configuration={BuilderOptions.Configuration};Platform={Architecture};MONO_TARGET_GC={gc}";

				Console.WriteLine($"{msbuild} {properties} {target} {solution}");

				Process.Run(msbuild, $"{properties} {target} {solution}", ".");
			}
			else
			{
				throw new Exception(msbuild + " does not exist");
			}
		}

		private void CopyMsbuildOutputToMonoPrefix()
		{
			// Copy over the VS built stuff that we want to use instead into the prefix directory
			FileUtils.CleanOrCreateDirectory(MonoPrefix);
			var monoPrefixBin = MonoPrefix.CreateDirectory("bin");
			var msvcBuildWithBdwgcBinDir = MonoRoot.Combine("msvc").Combine("build").Combine("bdwgc").Combine(Architecture).Combine("bin").Combine(BuilderOptions.Configuration.ToString());
			var msvcBuildWithSgenBinDir = MonoRoot.Combine("msvc").Combine("build").Combine("sgen").Combine(Architecture).Combine("bin").Combine(BuilderOptions.Configuration.ToString());

			msvcBuildWithBdwgcBinDir.Combine("mono-bdwgc.exe").Copy(monoPrefixBin);
			msvcBuildWithBdwgcBinDir.Combine("mono-2.0-bdwgc.dll").Copy(monoPrefixBin);
			msvcBuildWithBdwgcBinDir.Combine("mono-2.0-bdwgc.pdb").Copy(monoPrefixBin);
			msvcBuildWithBdwgcBinDir.Combine("MonoPosixHelper.dll").Copy(monoPrefixBin);
			msvcBuildWithBdwgcBinDir.Combine("MonoPosixHelper.pdb").Copy(monoPrefixBin);

			msvcBuildWithSgenBinDir.Combine("mono-sgen.exe").Copy(monoPrefixBin);
			msvcBuildWithSgenBinDir.Combine("mono-2.0-sgen.dll").Copy(monoPrefixBin);
			msvcBuildWithSgenBinDir.Combine("mono-2.0-sgen.pdb").Copy(monoPrefixBin);

			//sgen as default exe
			msvcBuildWithSgenBinDir.Combine("mono-sgen.exe").Copy(monoPrefixBin.Combine("mono.exe"));

			AddToBuildResultsDir.Combine("bin").CopyFiles(monoPrefixBin, recurse: true);
		}

		private void CreateArtifacts()
		{
			Console.WriteLine(">> Creating artifacts ...");

			var embedDirRoot = BuildsRoot.Combine("embedruntimes");
			var embedDirArchDestination = BuilderOptions.Arch32 ? embedDirRoot.Combine("win32") : embedDirRoot.Combine("win64");
			var distDirArchBin = BuilderOptions.Arch32 ? DistributionDir.Combine("bin") : DistributionDir.Combine("bin-x64");
			var versionsOutputFile = BuilderOptions.Arch32 ? BuildsRoot.Combine("versions-win32.txt") : BuildsRoot.Combine("versions-win64.txt");

			FileUtils.CleanOrCreateDirectory(embedDirArchDestination);
			FileUtils.CleanOrCreateDirectory(distDirArchBin);

			//embedruntimes directory setup
			Console.WriteLine($">> Creating embedruntimes directory at {embedDirArchDestination}");
			var monoPrefixBinDir = MonoPrefix.Combine("bin");
			monoPrefixBinDir.Combine("mono-2.0-bdwgc.dll").Copy(embedDirArchDestination);
			monoPrefixBinDir.Combine("mono-2.0-bdwgc.pdb").Copy(embedDirArchDestination);
			monoPrefixBinDir.Combine("mono-2.0-sgen.dll").Copy(embedDirArchDestination);
			monoPrefixBinDir.Combine("mono-2.0-sgen.pdb").Copy(embedDirArchDestination);
			monoPrefixBinDir.Combine("MonoPosixHelper.dll").Copy(embedDirArchDestination);
			monoPrefixBinDir.Combine("MonoPosixHelper.pdb").Copy(embedDirArchDestination);

			//monodistribution directory setup
			Console.WriteLine($">> Creating monodistribution directory at {distDirArchBin}");
			monoPrefixBinDir.Combine("mono-2.0-bdwgc.dll").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono-2.0-bdwgc.pdb").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono-2.0-sgen.dll").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono-2.0-sgen.pdb").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("MonoPosixHelper.dll").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("MonoPosixHelper.pdb").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono-sgen.exe").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono-bdwgc.exe").Copy(distDirArchBin);
			monoPrefixBinDir.Combine("mono.exe").Copy(distDirArchBin);

			WriteVersionInformation(distDirArchBin, versionsOutputFile);
		}
	}
}
