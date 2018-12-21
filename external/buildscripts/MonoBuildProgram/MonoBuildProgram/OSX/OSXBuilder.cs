using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using NiceIO;

namespace MonoBuildProgram.OSX
{
	public class OSXBuilder : BaseBuilder
	{
		private List<string> configureParams = new List<string>();
		private string monoBuildToolsExtraRevision = "";
		private string sdk = "10.11";

		public override void Clean()
		{
			if (BuilderOptions.Clean)
			{
				Console.WriteLine(">> Cleaning mono prefix ...");
				FileUtils.CleanOrCreateDirectory(MonoPrefix);
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
				if (BuilderOptions.DisableMcs)
					configureParams.Add("--disable-mcs-build");
				else
				{
					configureParams.Add("--with-unityjit=yes");
					configureParams.Add("--with-unityaot=yes");
				}

				configureParams.Add("--with-glib=embedded");
				configureParams.Add("--disable-nls");
				configureParams.Add("--disable-btls");
				configureParams.Add("--with-mcs-docs=no");
				configureParams.Add($"--prefix={MonoPrefix}");
				configureParams.Add("--with-monotouch=no");

				if (BuilderOptions.EnableCacheFile)
				{
					var cacheArch = BuilderOptions.Arch32 ? "i386" : "x86_64";
					configureParams.Add($"--cache-file=desktop-{cacheArch}.cache");
				}

				BuildNecessaryTools();
				SetupDarwinBuildEnvironment();
				PrintOSXEnvironment();

				configureParams.Add($"--host={MonoHostArch}-apple-darwin12.2.0");

				PrepareForBuild(configureParams);

				if (BuilderOptions.McsOnly)
				{
					RunMake($"-j{BuilderOptions.Jobs}", MonoRoot.Combine("mcs"));
				}
				else
				{
					RunMake($"-j{BuilderOptions.Jobs}", MonoRoot);
				}

				RunMake("install", MonoRoot);

				//Copy files from AddToBuildResults bin to MonoPrefix bin directory
				var monoPrefixBin = MonoPrefix.Combine("bin");
				var addToBuildResultsBin = AddToBuildResultsDir.Combine("bin");
				FileUtils.CreateDirectoryIfNotExists(MonoPrefix.Combine("bin"));
				addToBuildResultsBin.CopyFiles(monoPrefixBin, recurse: true);

				if (!BuilderOptions.DisableMcs)
				{
					CloneMonoBuildToolsExtra();
					MakeAdditionalProfiles();

					if (Platform.IsOSX())
					{
						new ProfileStubber().StubClassLibs();
					}
				}
			}
		}

		private void CloneMonoBuildToolsExtra()
		{
			var branch = "master";
			var monoBuildToolsExtraUrl = "git@gitlab.internal.unity3d.com:vm/mono-build-tools-extra.git";
			Git.GitClone(monoBuildToolsExtraUrl, MonoBuildToolsExtra, branch);

			monoBuildToolsExtraRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", MonoBuildToolsExtra);
			Console.WriteLine($">> mono-build-tools-extra revision: {monoBuildToolsExtraRevision}");
		}

		private void SetupDarwinBuildEnvironment()
		{
			string macVersion = "10.8";
			var macBuildEnvDir = MonoBuildDeps.Combine("MacBuildEnvironment");
			var macSdkPath = macBuildEnvDir.Combine("builds").Combine($"MacOSX{sdk}.sdk");

			if (!macSdkPath.DirectoryExists())
			{
				var sevenZip = "unzip";
				var sevenZipArgs = $"{macBuildEnvDir.Combine("builds.zip")} -d {macBuildEnvDir}";
				Process.Run(sevenZip, sevenZipArgs, ".");
			}
			else
			{
				Console.WriteLine(">> Mac build tool chain is already extracted ...");
			}

			//Set up mono for bootstrapping
			if (!ExistingExternalMonoRoot.DirectoryExists())
			{
				var monoInstalls = new NPath("/Library/Frameworks/Mono.framework/Versions");
				var monoVersions = new List<string>();

				if (monoInstalls.DirectoryExists())
				{
					foreach (var file in monoInstalls.Files())
					{
						if (!file.FileName.Equals("Current"))
						{
							monoVersions.Add(file.FileName);
							Console.WriteLine($">> Found mono version {file.FileName}");
						}
					}

					monoVersions.Sort();
					var monoVersionToUse = monoVersions[0];
					ExistingExternalMonoRoot = monoInstalls.Combine(monoVersionToUse);
				}
			}

			//TODO: what is this mcs and when is it used?
			var mcs = $"EXTERNAL_MCS={ExistingExternalMonoRoot.Combine("bin").Combine("mcs")}";
			Environment.SetEnvironmentVariable("CC", $"{macSdkPath.Parent.Combine("usr").Combine("bin").Combine("clang")}");
			Environment.SetEnvironmentVariable("CXX", $"{macSdkPath.Parent.Combine("usr").Combine("bin").Combine("clang++")}");
			Environment.SetEnvironmentVariable("MACSDKOPTIONS",
				$"-D_XOPEN_SOURCE -I{macBuildEnvDir.Combine("builds").Combine("usr").Combine("include")} -mmacosx-version-min={macVersion} -isysroot {macSdkPath}");
			Environment.SetEnvironmentVariable("CFLAGS",
				$"-D_XOPEN_SOURCE -I{macBuildEnvDir.Combine("builds").Combine("usr").Combine("include")} -mmacosx-version-min={macVersion} -isysroot {macSdkPath}");

			if (BuilderOptions.Configuration == BuilderOptions.BuildConfiguration.Debug)
			{
				Environment.SetEnvironmentVariable("CFLAGS", $"{Environment.GetEnvironmentVariable("CFLAGS")} -g -O0");
			}
			else
			{
				Environment.SetEnvironmentVariable("CFLAGS", $"{Environment.GetEnvironmentVariable("CFLAGS")} -Os");
			}

			Environment.SetEnvironmentVariable("CC", $"{Environment.GetEnvironmentVariable("CC")} -arch {MonoHostArch}");
			Environment.SetEnvironmentVariable("CXX", $"{Environment.GetEnvironmentVariable("CXX")} -arch {MonoHostArch}");

			//Add OSX specific autogen args
			//ConfigureParams.Add($"--host={MonoHostArch}-apple-darwin12.2.0");

			//Need to define because Apple's SIP gets in the way of us telling mono where to find this
			configureParams.Add(
				$"--with-libgdiplus={AddToBuildResultsDir.Combine("lib").Combine("libgdiplus.dylib")}");
			configureParams.Add("--enable-minimal=shared_perfcounters");
		}

		private void PrintOSXEnvironment()
		{
			Console.WriteLine(">> OSX Environment");

			Console.WriteLine($"\tPATH = {Environment.GetEnvironmentVariable("PATH")}");
			Console.WriteLine($"\tC_INCLUDE_PATH = {Environment.GetEnvironmentVariable("C_INCLUDE_PATH")}");
			Console.WriteLine($"\tCPLUS_INCLUDE_PATH = {Environment.GetEnvironmentVariable("CPLUS_INCLUDE_PATH")}");
			Console.WriteLine($"\tCFLAGS = {Environment.GetEnvironmentVariable("CFLAGS")}");
			Console.WriteLine($"\tCXXFLAGS = {Environment.GetEnvironmentVariable("CXXFLAGS")}");
			Console.WriteLine($"\tCC = {Environment.GetEnvironmentVariable("CC")}");
			Console.WriteLine($"\tCXX = {Environment.GetEnvironmentVariable("CXX")}");
			Console.WriteLine($"\tCPP = {Environment.GetEnvironmentVariable("CPP")}");
			Console.WriteLine($"\tCXXPP = {Environment.GetEnvironmentVariable("CXXPP")}");
			Console.WriteLine($"\tLD = {Environment.GetEnvironmentVariable("LD")}");
			Console.WriteLine($"\tLDFLAGS = {Environment.GetEnvironmentVariable("LDFLAGS")}");
		}

		public override void Artifacts()
		{
			if (BuilderOptions.Artifact)
			{
				var embedDirRoot = BuildsRoot.Combine("embedruntimes");

				//Note these tmp directories will get merged into a single 'osx' directory later by a parent script TODO: Collate script
				var embedDirArchDestination = embedDirRoot.Combine($"osx-tmp-{MonoHostArch}");
				var distDirArchBin = DistributionDir.Combine($"bin-osx-tmp-{MonoHostArch}");

				//TODO: check this
				var versionsOutputFile = BuilderOptions.Arch32
					? BuildsRoot.Combine("versions-osx32.txt")
					: BuildsRoot.Combine("versions-osx64.txt");

				if (BuilderOptions.ArtifactsCommon)
				{
					CreateArtifactsCommon(distDirArchBin);
				}

				if (BuilderOptions.ArtifactsRuntime)
				{
					CreateArtifactsRuntime(embedDirArchDestination, distDirArchBin);
				}

				if (distDirArchBin.DirectoryExists())
				{
					Process.Run("chmod", $"-R 755 {distDirArchBin}", distDirArchBin);
					WriteVersionInformation(distDirArchBin, versionsOutputFile);
				}
			}
		}

		private void CreateArtifactsCommon(NPath distDirArchBin)
		{
			FileUtils.CleanOrCreateDirectory(distDirArchBin);

			Console.WriteLine(">> Creating common artifacts ...");
			Console.WriteLine($">> Distribution directory = {DistributionDir}");

			var distDirLib = DistributionDir.Combine("lib");
			var distDirLibmono = distDirLib.Combine("mono");
			if (!distDirLibmono.DirectoryExists())
			{
				distDirLibmono.CreateDirectory();
			}
			else
			{
				FileUtils.CleanOrCreateDirectory(distDirLibmono);
			}

			Console.WriteLine(">> Creating normal profile artifacts ...");

			AddToBuildResultsDir.Combine("bin").Copy(DistributionDir).Files(recurse: true);
			MonoPrefix.Combine("lib").Combine("mono").Copy(distDirLib).Files(recurse: true);

			if (!Platform.IsOSX())
			{
				//On OSX we build a universal binary for 32-bit and 64-bit in the mono executable. The class library build
				//only creates the 64-bit slice, so we don't want to end up with a single slice binary in the output.
				//If we do, it will step on the universal binary produced but the OSX runtime build.
				MonoPrefix.Combine("bin").CopyFiles(DistributionDir, recurse: true);
			}

			MonoPrefix.Combine("etc").Copy(DistributionDir).Files(recurse: true);

			MonoBuildDeps.Combine("reference-assemblies").Combine("unity")
				.CopyFiles(distDirLibmono.Combine("unity"), recurse: true);
			MonoBuildDeps.Combine("reference-assemblies").Combine("unity_web")
				.CopyFiles(distDirLibmono.Combine("unity_web"), recurse: true);

			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("2.0-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "Boo.*\\.dll"));
			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("2.0-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "UnityScript.*\\.dll"));

			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("4.0-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "Boo.*\\.dll"));
			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("4.0-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "UnityScript.*\\.dll"));

			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("4.5-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "Boo.*\\.dll"));
			MonoBuildDeps.Combine("reference-assemblies").Combine("unity").CopyFiles(distDirLibmono.Combine("4.5-api"),
				recurse: true, fileFilter: p => Regex.IsMatch(p.FileName, "UnityScript.*\\.dll"));

			//now remove nunit from a couple places (but not all, we need some of them)
			foreach (var dir in distDirLibmono.Combine("gac").Directories())
			{
				if (dir.FileName.Contains("nunit"))
					dir.Delete();
			}

			var zippedClassLibs = MonoRoot.Combine("ZippedClasslibs.tar.gz");
			if (zippedClassLibs.FileExists())
			{
				zippedClassLibs.Delete();
			}

			Console.WriteLine(">> Creating ZippedClasslibs.tar.gz ...");
			//TODO: Check Architecture
			var zipName = BuilderOptions.Arch32 ? "ZippedClasslibs_osx32.tar.gz" : "ZippedClasslibs_osx64.tar.gz";
			//TODO: use unzip?
			Process.Run("tar", $"-pzcvf {zipName} {BuildsRoot}", MonoRoot);

		}

		private void CreateArtifactsRuntime(NPath embedDirArchDestination, NPath distDirArchBin)
		{
			Console.WriteLine($">> Creating embedruntimes directory : {embedDirArchDestination}");
			FileUtils.CleanOrCreateDirectory(embedDirArchDestination);
			FileUtils.CleanOrCreateDirectory(distDirArchBin);

			//TODO: there must be another way to create hard links
			var libMonoSgenDylib = MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonobdwgc-2.0.dylib");
			var libMonoBdwgcDylib = MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonosgen-2.0.dylib");
			var libMonoPosixHelperDylib = MonoRoot.Combine("support").Combine(".libs").Combine("libMonoPosixHelper.dylib");

			Console.WriteLine($">> Copying {libMonoSgenDylib} to {embedDirArchDestination}");
			libMonoSgenDylib.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoBdwgcDylib} to {embedDirArchDestination}");
			libMonoBdwgcDylib.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoPosixHelperDylib} to {embedDirArchDestination}");
			libMonoPosixHelperDylib.Copy(embedDirArchDestination);


			/*Process.Run("ln",
				$"-f {BuildProgram.MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonobdwgc-2.0.dylib")} {embedDirArchDestination.Combine("libmonobdwgc-2.0.dylib")}",
				".");
			Process.Run("ln",
				$"-f {BuildProgram.MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonosgen-2.0.dylib")} {embedDirArchDestination.Combine("libmonosgen-2.0.dylib")}",
				".");
			Process.Run("ln",
				$"-f {BuildProgram.MonoRoot.Combine("mono").Combine("support").Combine(".libs").Combine("libMonoPosixHelper.dylib")} {embedDirArchDestination.Combine("libMonoPosixHelper.dylib")}",
				".");*/

			//TODO
			//InstallNameTool("$embedDirArchDestination/libmonobdwgc-2.0.dylib", "\@executable_path/../Frameworks/MonoEmbedRuntime/osx/libmonobdwgc-2.0.dylib");
			//InstallNameTool("$embedDirArchDestination/libmonosgen-2.0.dylib", "\@executable_path/../Frameworks/MonoEmbedRuntime/osx/libmonosgen-2.0.dylib");
			//InstallNameTool("$embedDirArchDestination/libMonoPosixHelper.dylib", "\@executable_path/../Frameworks/MonoEmbedRuntime/osx/libMonoPosixHelper.dylib");

			Console.WriteLine(">> Copying mono public headers ...");
			FileUtils.CleanOrCreateDirectory(IncludesRoot.Combine("mono"));
			MonoPrefix.Combine("include").Combine("mono-2.0").Combine("mono")
				.CopyFiles(IncludesRoot.Combine("mono"), recurse: true);

			MonoRoot.Combine("mono").Combine("mini").Combine("mono").Copy(distDirArchBin);
			MonoRoot.Combine("tools").Combine("pedump").Combine("pedump").Copy(distDirArchBin);
			/*Process.Run("ln",
				$"-f {BuildProgram.MonoRoot.Combine("mono").Combine("mini").Combine("mono")} {distDirArchBin.Combine("mono")}", ".");
			Process.Run("ln",
				$"-f {BuildProgram.MonoRoot.Combine("tools").Combine("pedump").Combine("pedump")} {distDirArchBin.Combine("pedump")}",
				".");*/
		}
	}
}
