using System;
using System.Collections.Generic;
using NiceIO;

namespace MonoBuildProgram.Android
{
	public class AndroidBuilder : BaseBuilder
	{
		private readonly List<string> configureParams = new List<string>();

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
				/*
				* Disable building of the class libraries by default when building the android runtime
				* since we don't care about a class library build in this situation (as of writing this at least)
				* but only if the test flag is not set.  If the test flag was set, we'd need to build the classlibs
				* in order to run the tests
				*/
				BuilderOptions.DisableMcs = BuilderOptions.Test ? false : true;

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

				BuildNecessaryTools();
				SetupAndroidBuildEnvironment();
				PrintAndroidEnvironment();

				PrepareForBuild(configureParams);

				//This step needs to run after configure. It generates the arm_dpimacros.h file, which is needed by the offset dumper
				RunMake("", MonoRoot.Combine("mono").Combine("arch").Combine("arm"));

				if (BuilderOptions.McsOnly)
				{
					RunMake($"-j{BuilderOptions.Jobs}", MonoRoot.Combine("mcs"));
				}
				else
				{
					RunMake($"-j{BuilderOptions.Jobs}", MonoRoot);
				}

				if (!BuilderOptions.DisableMcs)
				{
					Console.WriteLine(">> Skipping make install. We don't need to run this step when building the runtime on non-desktop platforms.");
				}
			}
		}

		public override void Artifacts()
		{
			if (BuilderOptions.Artifact)
			{
				var embedDirRoot = BuildsRoot.Combine("embedruntimes");
				NPath distDirArchBin = "";
				var embedDirArchDestination = embedDirRoot.Combine("android").Combine(BuilderOptions.AndroidArch);
				string versionsOutputFile = BuildsRoot.Combine($"versions-android-{BuilderOptions.AndroidArch}.txt");

				if (BuilderOptions.ArtifactsCommon)
				{
					Console.WriteLine(">> Skipping collecting common artifacts for linus runtime");
				}

				if (BuilderOptions.ArtifactsRuntime)
				{
					CreateArtifactsRuntime(embedDirArchDestination, distDirArchBin);
				}
				WriteVersionInformation(distDirArchBin, versionsOutputFile);
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

			Console.WriteLine($">> Copying {libMonoSgen} to {embedDirArchDestination}");
			libMonoSgen.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoBdwgc} to {embedDirArchDestination}");
			libMonoBdwgc.Copy(embedDirArchDestination);

			Console.WriteLine($">> Copying {libMonoPosixHelper} to {embedDirArchDestination}");
			libMonoPosixHelper.Copy(embedDirArchDestination);
		}

		private void SetupAndroidBuildEnvironment()
		{
			var ndkVersion = "r16b";
			var apiLevel = 16;
			var hostTriple = "";
			var platformRootPostfix = "";
			var useKraitPatch = 1;

			var kraitPatchPath = MonoRoot.Parent.Parent.Combine("android_krait_signal_handler").Combine("build");

			Environment.SetEnvironmentVariable("ANDROID_PLATFORM", $"android-{apiLevel}");

			if (BuilderOptions.AndroidArch.ToLower().Equals("armv7a"))
			{
				hostTriple = "arm-linux-androideabi";
				platformRootPostfix = "arm";
			}
			else if (BuilderOptions.AndroidArch.ToLower().Equals("x86"))
			{
				hostTriple = "i686-linux-android";
				platformRootPostfix = "x86";
				useKraitPatch = 0;
			}
			else
			{
				throw new Exception($"Unknown android architecture specified {BuilderOptions.AndroidArch}");
			}

			if (Platform.IsLinux())
				Environment.SetEnvironmentVariable("HOST_ENV", "linux");
			else if (Platform.IsOSX())
				Environment.SetEnvironmentVariable("HOST_ENV", "darwin");
			else
				Environment.SetEnvironmentVariable("HOST_ENV", "windows");

			Console.WriteLine(Environment.NewLine);
			Console.WriteLine($">> Android Platform = {Environment.GetEnvironmentVariable("ANDROID_PLATFORM")}");
			Console.WriteLine($">> Android NDK Version = {ndkVersion}");


			string ndkName;
			string ndkDir;
			if (Platform.IsLinux())
			{
				ndkDir = $"android-ndk-{ndkVersion}-linux";
				ndkName = $"android-ndk-{ndkVersion}-linux-x86_64.zip";
			}
			else if (Platform.IsOSX())
			{
				ndkDir = $"android-ndk-{ndkVersion}-darwin";
				ndkName = $"android-ndk-{ndkVersion}-darwin-x86_64.zip";
			}
			else
			{
				ndkDir = $"android-{ndkVersion}-windows";
				ndkName = $"android-ndk-{ndkVersion}-windows-x86.zip";
			}

			var depsNdkArchive = MonoBuildDeps.Combine(ndkDir).Combine(ndkName);
			var depsNdkFinal = MonoBuildDeps.Combine(ndkDir).Combine($"android-ndk-{ndkVersion}");
			//var depsNdkFinal = MonoBuildDeps.Combine($"android-ndk-{ndkVersion}");

			Console.WriteLine(Environment.NewLine);
			Console.WriteLine($">> Android NDK Archive = {depsNdkArchive}");
			Console.WriteLine($">> Android NDK Extraction Destination = {depsNdkFinal}");

			Environment.SetEnvironmentVariable("ANDROID_NDK_ROOT", depsNdkFinal);
			Console.WriteLine($">> ANDROID_NDK_ROOT set to = {Environment.GetEnvironmentVariable("ANDROID_NDK_ROOT")}");

			if (depsNdkFinal.DirectoryExists())
			{
				Console.WriteLine($">> Android NDK already extracted at {depsNdkFinal}");
			}
			else
			{
				Console.WriteLine($">> Android NDK needs to be to {depsNdkFinal}");

				string sevenZip;
				string sevenZipArgs;
				if (Platform.IsWindows())
				{
					//TODO: Test on windows
					sevenZip = BuildDeps.GetSevenZip();
					sevenZipArgs = $"x {depsNdkArchive} -o{MonoBuildDeps.Combine(ndkDir)}";
				}
				else
				{
					//TODO Check destination path, its being extracted to monobuilddeps directly, and other platforms
					sevenZip = "unzip";
					sevenZipArgs = $"{depsNdkArchive} -d {MonoBuildDeps.Combine(ndkDir)}";
					//sevenZipArgs = $"{depsNdkArchive} -d {MonoBuildDeps}";
				}
				Process.Run(sevenZip, sevenZipArgs, ".");
			}

			var ndkBuildFile = new NPath(Environment.GetEnvironmentVariable("ANDROID_NDK_ROOT")).Combine("ndk-build");
			if (!ndkBuildFile.FileExists())
			{
				throw new Exception(">> Something went wrong with the NDK extraction ...");
			}

			var androidNdkRoot = new NPath(Environment.GetEnvironmentVariable("ANDROID_NDK_ROOT"));
			var androidPlatformRoot = androidNdkRoot.Combine("platforms").Combine(Environment.GetEnvironmentVariable("ANDROID_PLATFORM")).Combine($"arch-{platformRootPostfix}");
			var androidToolchain = androidNdkRoot.Combine("toolchains").Combine($"{hostTriple}-clang");

			FileUtils.AddToPathEnvVar(androidToolchain.Combine("bin"));

			Console.WriteLine(">> Generating android toolchain ...");

			var makeToolChainFileName = androidNdkRoot.Combine("build").Combine("tools").Combine("make_standalone_toolchain.py");
			var makeToolChainArgs = $"--arch {platformRootPostfix} --api {apiLevel} --install-dir {androidToolchain}";
			Process.Run("python", $"{makeToolChainFileName} {makeToolChainArgs}", androidNdkRoot.Combine("build").Combine("tools"));

			Console.WriteLine($">> Android Arch = {BuilderOptions.AndroidArch}");
			Console.WriteLine($">> Android NDK Root = {androidNdkRoot}");
			Console.WriteLine($">> Android Platform Root = {androidPlatformRoot}");
			Console.WriteLine($">> Android Toolchain = {androidToolchain}");

			if (!androidToolchain.DirectoryExists())
			{
				throw new Exception("Failed to locate android toolchain");
			}

			if (!androidPlatformRoot.DirectoryExists())
			{
				throw new Exception("Failed to locate android platform root");
			}

			if (BuilderOptions.AndroidArch.ToLower().Equals("armv7a"))
			{
				Environment.SetEnvironmentVariable("CFLAGS", $"-DARM_FPU_VFP=1  -march=armv7-a -target armv7-none-linux-androideabi -DHAVE_ARMV6=1 -funwind-tables {Environment.GetEnvironmentVariable("CFLAGS")}");
				Environment.SetEnvironmentVariable("LDFLAGS", $"-Wl,--fix-cortex-a8 -Wl,-rpath-link={androidPlatformRoot.Combine("usr").Combine("lib")} {Environment.GetEnvironmentVariable("LDFLAGS")}");
			}

			var compilerSysroot = androidNdkRoot.Combine("sysroot");
			var archISystem = compilerSysroot.Combine("usr").Combine("include").Combine(hostTriple);
			var unifiedISystem = compilerSysroot.Combine("usr").Combine("include");

			Environment.SetEnvironmentVariable("CC", $"{androidToolchain.Combine("bin").Combine("clang")} -v -isystem {archISystem} -isystem {unifiedISystem}");
			Environment.SetEnvironmentVariable("CXX", $"{androidToolchain.Combine("bin").Combine("clang++")} -isystem {archISystem} -isystem {unifiedISystem}");
			Environment.SetEnvironmentVariable("CPP", $"{androidToolchain.Combine("bin").Combine("clang")} -E -isystem {archISystem} -isystem {unifiedISystem}");
			Environment.SetEnvironmentVariable("CXXCPP", $"{androidToolchain.Combine("bin").Combine("clang++")} -E -isystem {archISystem} -isystem {unifiedISystem}");
			Environment.SetEnvironmentVariable("CPATH", androidPlatformRoot.Combine("usr").Combine("include"));

			Environment.SetEnvironmentVariable("LD", androidToolchain.Combine("bin").Combine($"{hostTriple}-ld"));
			Environment.SetEnvironmentVariable("AS", androidToolchain.Combine("bin").Combine($"{hostTriple}-as"));
			Environment.SetEnvironmentVariable("AR", androidToolchain.Combine("bin").Combine($"{hostTriple}-ar"));
			Environment.SetEnvironmentVariable("RANLIB", androidToolchain.Combine("bin").Combine($"{hostTriple}-ranlib"));
			Environment.SetEnvironmentVariable("STRIP", androidToolchain.Combine("bin").Combine($"{hostTriple}-strip"));

			Environment.SetEnvironmentVariable("CFLAGS", $"-DANDROID -D__ANDROID_API__=16 -DPLATFORM_ANDROID -DLINUX -D__linux__ -DHAVE_USR_INCLUDE_MALLOC_H -D_POSIX_PATH_MAX=256 -DS_IWRITE=S_IWUSR -DHAVE_PTHREAD_MUTEX_TIMEDLOCK -fpic -g -ffunction-sections -fdata-sections {Environment.GetEnvironmentVariable("CFLAGS")}");
			Environment.SetEnvironmentVariable("CXXFLAGS", Environment.GetEnvironmentVariable("CFLAGS"));
			Environment.SetEnvironmentVariable("CPPFLAGS", Environment.GetEnvironmentVariable("CFLAGS"));

			if (useKraitPatch == 1)
			{
				Environment.SetEnvironmentVariable("LDFLAGS", $"-Wl,--wrap,sigaction -L{kraitPatchPath.Combine("obj").Combine("local").Combine("armeabi-v7a")} -lkrait-signal-handler {Environment.GetEnvironmentVariable("LDFLAGS")}");
			}
			Environment.SetEnvironmentVariable("LDFLAGS", $"--sysroot={androidPlatformRoot} -Wl,--no-undefined -Wl,--gc-sections -ldl -lm -llog -lc {Environment.GetEnvironmentVariable("LDFLAGS")}");


			if (useKraitPatch == 1)
			{
				if (kraitPatchPath.DirectoryExists())
				{
					Console.WriteLine($">> Krait patch repo exists at {kraitPatchPath}");
				}
				else
				{
					CloneKraitPatch(kraitPatchPath);
				}

				Process.Run(ndkBuildFile, "clean", kraitPatchPath);
				Process.Run(ndkBuildFile, "", kraitPatchPath);
			}

			configureParams.Add($"--host={hostTriple}");

			if (BuilderOptions.EnableCacheFile)
			{
				configureParams.Add($"--cache-file=android-{BuilderOptions.AndroidArch}.cache");
			}

			configureParams.Add("--disable-parallel-mark");
			configureParams.Add("--disable-shared-handles");
			configureParams.Add("--with-sigaltstack=no");
			configureParams.Add("--with-tls=pthread");
			configureParams.Add("--disable-visibility-hidden");
			configureParams.Add("mono_cv_uscore=yes");

			if (Platform.IsWindows())
			{
				configureParams.Add("ac_cv_header_zlib_h=no");
			}
		}

		private void PrintAndroidEnvironment()
		{
			Console.WriteLine(">> Android Environment");
			Console.WriteLine($"\tCC = {Environment.GetEnvironmentVariable("CC")}");
			Console.WriteLine($"\tCXX = {Environment.GetEnvironmentVariable("CXX")}");
			Console.WriteLine($"\tCPP = {Environment.GetEnvironmentVariable("CPP")}");
			Console.WriteLine($"\tCXXCPP = {Environment.GetEnvironmentVariable("CXXCPP")}");
			Console.WriteLine($"\tCPATH = {Environment.GetEnvironmentVariable("CPATH")}");
			Console.WriteLine($"\tLD = {Environment.GetEnvironmentVariable("LD")}");
			Console.WriteLine($"\tAS = {Environment.GetEnvironmentVariable("AS")}");
			Console.WriteLine($"\tAR = {Environment.GetEnvironmentVariable("AR")}");
			Console.WriteLine($"\tRANLIB = {Environment.GetEnvironmentVariable("RANLIB")}");
			Console.WriteLine($"\tSTRIP = {Environment.GetEnvironmentVariable("STRIP")}");
			Console.WriteLine($"\tCFLAGS = {Environment.GetEnvironmentVariable("CFLAGS")}");
			Console.WriteLine($"\tCXXFLAGS = {Environment.GetEnvironmentVariable("CXXFLAGS")}");
			Console.WriteLine($"\tCPPFLAGS = {Environment.GetEnvironmentVariable("CPPFLAGS")}");
			Console.WriteLine($"\tLDFLAGS = {Environment.GetEnvironmentVariable("LDFLAGS")}");
		}

		private void CloneKraitPatch(NPath kraitPatchPath)
		{
			var kraitPatchRepoUrl = "git://github.com/Unity-Technologies/krait-signal-handler.git";
			var branch = "master";

			Git.GitClone(kraitPatchRepoUrl, kraitPatchPath, branch);

			var kraitPatchRepoRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", kraitPatchPath);
			Console.WriteLine($">> Krait patch revision: {kraitPatchRepoRevision}");
		}
	}
}
