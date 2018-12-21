using System;
using System.Collections.Generic;
using NiceIO;

namespace MonoBuildProgram.Linux
{
	public class LinuxRuntimeBuilder : BaseBuilder
	{
		private List<string> configureParams = new List<string>();
		private List<string> commandPrefix = new List<string>();

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
				SetupLinuxBuildEnvironment();

				configureParams.Add($"--host={MonoHostArch}h-pc-linux-gnu");

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
					//TODO Check for linux
					//CloneMonoBuildToolsExtra();
					//MakeAdditionalProfiles();

					//if (Platform.IsOSX())
					//{
						//new ProfileStubber().StubClassLibs();
					//}
				}
			}
		}

		private void SetupLinuxBuildEnvironment()
		{
			if (IsRunningOnBuildMachine() || Environment.GetEnvironmentVariable("UNITY_USE_LINUX_SDK") != null)
			{
				var sdkVersion = "20170609";
				Console.WriteLine($">> Linux SDK Version = {sdkVersion}");

				var schroot = $"LinuxBuildEnvironment-{sdkVersion}";
				string[] linuxToolChain = new string[] { "schroot", "-c", schroot, "--" };

				//TODO: Check directories here
				var sdkName = $"linux-sdk-{sdkVersion}.tar.bz2";
				var depsSdkArchive = MonoBuildDeps.Combine($"linux-sdk-{sdkVersion}").Combine(sdkName);
				var depsSdkFinal = MonoBuildDeps.Combine($"linux-sdk-{sdkVersion}").Combine($"linux-sdk-{sdkVersion}");

				Console.WriteLine($">> Linux SDK Archive = {depsSdkArchive}");
				Console.WriteLine($">> Linux SDK Extraction Destination = {depsSdkFinal}");

				var linuxSdkRoot = depsSdkFinal;
				if (depsSdkFinal.DirectoryExists())
				{
					Console.WriteLine($">> Linux SDK already extracted at {depsSdkFinal}");
				}
				else
				{
					Console.WriteLine($">> Linux SDK needs to be extracted to {depsSdkFinal}");
					depsSdkFinal.CreateDirectory();
					Process.Run("tar", $"xafv {depsSdkArchive} '-C' {depsSdkFinal}", ".");
					Process.Run("cp", $"-R {depsSdkFinal}/linux-sdk-{sdkVersion} /etc/schroot", ".");
					var argument = $"s,^directory=.*,directory={depsSdkFinal}/{schroot}, \"{depsSdkFinal}/{schroot}.conf\" | sudo tee /etc/schroot/chroot.d/{schroot}.conf";
					Process.Run("sed", argument, ".");

					/*
					 * 	system('mkdir', '-p', $depsSdkFinal);
						system('tar', 'xaf', $depsSdkArchive, '-C', $depsSdkFinal) eq 0  or die("failed to extract Linux SDK\n");
						system('sudo', 'cp', '-R', "$depsSdkFinal/linux-sdk-$sdkVersion", '/etc/schroot');
						system("sed 's,^directory=.*,directory=$depsSdkFinal/$schroot,' \"$depsSdkFinal/$schroot.conf\" | sudo tee /etc/schroot/chroot.d/$schroot.conf") eq 0 or die("failed to deploy Linux SDK\n");
					
					 */
					//TODO Check destination path, its being extracted to monobuilddeps directly, and other platforms
					//var sevenZip = "unzip";
					//var sevenZipArgs = $"{depsSdkArchive} -d {depsSdkFinal}";
					//Process.Run(sevenZip, sevenZipArgs, ".");

					//var sevenZip = BuildDeps.GetSevenZip();
					//var severZipArgs = $"x {depsSdkArchive} -o{depsSdkFinal}";
					//Process.Run(sevenZip, severZipArgs, ".");

					//TODO: Check copy works
					//depsSdkFinal.Combine($"linux-sdk-{sdkVersion}").CopyFiles("/etc/schroot", recurse: true);

					//TODO:
					//system("sed 's,^directory=.*,directory=$depsSdkFinal/$schroot,' \"$depsSdkFinal/$schroot.conf\" | sudo tee /etc/schroot/chroot.d/$schroot.conf") eq 0 or die("failed to deploy Linux SDK\n");

					//TODO: Where is this used
					foreach (var item in linuxToolChain)
						commandPrefix.Add(item);

					Console.WriteLine($">> Linux SDK Root = {linuxSdkRoot}");
					Console.WriteLine($">> Linux Toolchain Command Prefix = {string.Join(" ", commandPrefix)}");

					//ConfigureParams.Add($"--host={MonoHostArch}h-pc-linux-gnu");

					//This causes crashes
					configureParams.Add("--disable-parallel-mark");

					string archflags = BuilderOptions.Arch32 ? "-m32" : "-fPIC";

					if (BuilderOptions.Configuration == BuilderOptions.BuildConfiguration.Debug)
					{
						Environment.SetEnvironmentVariable("CFLAGS", $"{archflags} -g -O0");
					}
					else
					{
						//Optimized for size
						Environment.SetEnvironmentVariable("CFLAGS", $"{archflags} -Os");
					}
				}
			}
		}

		public override void Artifacts()
		{
			if (BuilderOptions.Artifact)
			{
				var embedDirRoot = BuildsRoot.Combine("embedruntimes");

				var embedDirArchDestination = BuilderOptions.Arch32 ? embedDirRoot.Combine("linux32") : embedDirRoot.Combine("linux64");
				var distDirArchBin = BuilderOptions.Arch32
					? DistributionDir.Combine("bin-linux32")
					: DistributionDir.Combine("bin-linux64");
				var versionsOutputFile = BuilderOptions.Arch32
					? BuildsRoot.Combine("versions-linux32.txt")
					: BuildsRoot.Combine("versions-linux64.txt");


				if (BuilderOptions.ArtifactsCommon)
				{
					Console.WriteLine(">> Skipping collecting common artifacts for linus runtime");
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

		private void CreateArtifactsRuntime(NPath embedDirArchDestination, NPath distDirArchBin)
		{
			Console.WriteLine($">> Creating embedruntimes directory : {embedDirArchDestination}");
			FileUtils.CleanOrCreateDirectory(embedDirArchDestination);
			FileUtils.CleanOrCreateDirectory(distDirArchBin);

			var libMonoSgen = MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonosgen-2.0.so");
			var libMonoBdwgc = MonoRoot.Combine("mono").Combine("mini").Combine(".libs").Combine("libmonobdwgc-2.0.so");
			var libMonoPosixHelper = MonoRoot.Combine("support").Combine(".libs").Combine("libMonoPosixHelper.so");


			Console.WriteLine($">> Copying {libMonoSgen}");
			libMonoSgen.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoBdwgc}");
			libMonoBdwgc.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoPosixHelper}");
			libMonoPosixHelper.Copy(embedDirArchDestination);
			/*
			 *if ($buildMachine)
			{
				system("strip $embedDirArchDestination/libmonobdwgc-2.0.so") eq 0 or die("failed to strip libmonobdwgc-2.0.so (shared)\n");
				system("strip $embedDirArchDestination/libmonosgen-2.0.so") eq 0 or die("failed to strip libmonosgen-2.0.so (shared)\n");
				system("strip $embedDirArchDestination/libMonoPosixHelper.so") eq 0 or die("failed to strip libMonoPosixHelper (shared)\n");
			}
			*/

			NPath distDirArchEtc = BuilderOptions.Arch32
				? $"{DistributionDir.Combine("etc-linux32")}"
				: $"{DistributionDir.Combine("etc-linux64")}";
			FileUtils.CleanOrCreateDirectory(distDirArchEtc);
			FileUtils.CleanOrCreateDirectory(distDirArchBin);
			distDirArchEtc.Combine("mono").CreateDirectory();

			Process.Run("ln",
				$"-f {MonoRoot.Combine("mono").Combine("mini").Combine("mono-sgen")} {distDirArchBin.Combine("mono")}",
				".");
			Process.Run("ln",
				$"-f {MonoRoot.Combine("tools").Combine("pedump").Combine("pedump")} {distDirArchBin.Combine("pedump")}",
				".");
			MonoRoot.Combine("data").Combine("config").Copy(distDirArchEtc.Combine("mono").Combine("config"))
				.Files(recurse: true);
		}
	}
}
