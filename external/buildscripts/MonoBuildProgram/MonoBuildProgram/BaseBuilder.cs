using System;
using System.Collections.Generic;
using System.IO;
using NiceIO;

namespace MonoBuildProgram
{
	public class BaseBuilder
	{
		public static NPath MonoRoot = GetMonoRootDir();
		public static NPath MonoBuildToolsExtra = MonoRoot.Parent.Parent.Combine("mono-build-tools-extra").Combine("build");
		public static NPath MonoPrefix = MonoRoot.Combine("tmp").Combine("monoprefix");
		public static NPath BuildScriptsRoot = MonoRoot.Combine("external").Combine("buildscripts");
		public static NPath MonoBuildDeps = BuildScriptsRoot.Combine("artifacts").Combine("Stevedore");
		protected NPath BuildsRoot { get; }
		protected NPath AddToBuildResultsDir { get; }
		protected NPath IncludesRoot { get; }
		protected NPath SourcesRoot { get; }
		protected NPath DistributionDir { get; }
		
		protected string UnityMonoRevision = "";
		protected string UnityMonoBuildScriptsRevision = "";
		protected NPath ExistingExternalMonoRoot;
		protected string MonoHostArch = "";

		public BaseBuilder()
		{
			Console.WriteLine(">> Current working directory: " + Directory.GetCurrentDirectory());
			Console.WriteLine(">> Mono root directory: " + MonoRoot);
			Console.WriteLine(">> Mono build tools extra directory: " + MonoBuildToolsExtra);
			Console.WriteLine(">> MonoPrefix directory: " + MonoPrefix);
			Console.WriteLine(">> Build scripts directory: " + BuildScriptsRoot);
			Console.WriteLine(">> Mono build dependencies directory: " + MonoBuildDeps);

			BuildsRoot = MonoRoot.Combine("builds");
			Console.WriteLine(">> Builds directory: " + BuildsRoot);

			AddToBuildResultsDir = BuildScriptsRoot.Combine("add_to_build_results").Combine("monodistribution");
			Console.WriteLine(">> Add to build results directory: " + AddToBuildResultsDir);

			IncludesRoot = BuildsRoot.Combine("include");
			Console.WriteLine(">> Includes directory: " + IncludesRoot);

			SourcesRoot = BuildsRoot.Combine("source");
			Console.WriteLine(">> Sources directory: " + SourcesRoot);

			DistributionDir = BuildsRoot.Combine("monodistribution");
			Console.WriteLine(">> Distribution directory: " + DistributionDir);

			if (IsRunningOnBuildMachine())
			{
				Console.WriteLine(">> Running on build machine");
			}

			PrintRevisionInfo();
		}

		public virtual void Clean() { }

		public virtual void Build() { }

		public virtual void Artifacts() { }

		public void Test()
		{
			if (BuilderOptions.Test)
			{
				RunTests();
			}
			else
			{
				Console.WriteLine(">> Skipped running tests ...");
			}
		}

		private static NPath GetMonoRootDir()
		{
			var exePath = new NPath(System.Reflection.Assembly.GetEntryAssembly().Location);
			var monoRoot = exePath;

			//Assume "external" directory exists under monoRoot. 
			while (monoRoot.ToString().Contains("external"))
				monoRoot = monoRoot.Parent;

			return monoRoot;
		}

		protected bool IsRunningOnBuildMachine()
		{
			var buildMachine = Environment.GetEnvironmentVariable("UNITY_THISISABUILDMACHINE");
			if (buildMachine != null && buildMachine == "1")
				return true;
			return false;
		}

		private void PrintRevisionInfo()
		{
			Console.WriteLine(Environment.NewLine);

			UnityMonoRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", MonoRoot);
			Console.WriteLine($">> Mono revision: {UnityMonoRevision}");

			UnityMonoBuildScriptsRevision = Process.RunAndReturnOutput("git", "rev-parse HEAD", BuildScriptsRoot);
			Console.WriteLine($">> Build scripts revision: {UnityMonoBuildScriptsRevision}");
		}

		public void BuildNecessaryTools()
		{
			if (!String.IsNullOrEmpty(MonoBuildDeps))
			{
				Console.WriteLine(">> Building autoconf, texinfo, automake, and libtool if needed ...");

				var builtToolsDir = MonoBuildDeps.Combine("built-tools");
				FileUtils.CleanOrCreateDirectory(builtToolsDir);
				FileUtils.AddToPathEnvVar(builtToolsDir.Combine("bin"));

				BuildAutoConf(builtToolsDir);
				BuildTexInfo(builtToolsDir);
				BuildAutomake(builtToolsDir);
				BuildLibTool(builtToolsDir);
				DetermineExistingMonoPath();
				VerifyInstalledTools();
			}
		}

		private void BuildAutoConf(NPath builtToolsDir)
		{
			Console.WriteLine(">> Building autoconf");

			var autoconfVersion = "2.69";
			var autoconfDir = MonoBuildDeps.Combine("autoconf-src").Combine($"autoconf-{autoconfVersion}");
	
			if (!autoconfDir.DirectoryExists())
			{
				throw new Exception($"{autoconfDir} does not exist");
			}

			Process.Run($"{autoconfDir.Combine("configure")}", $"--prefix={builtToolsDir}", autoconfDir);
			Process.Run("make", "", autoconfDir);
			Process.Run("make", "install", autoconfDir);
		}

		private void BuildTexInfo(NPath builtToolsDir)
		{
			Console.WriteLine(">> Building texinfo");

			var texinfoVersion = "4.8";
			var texinfoDir = MonoBuildDeps.Combine("texinfo-src").Combine($"texinfo-{texinfoVersion}");

			if (!texinfoDir.DirectoryExists())
			{
				throw new Exception($"{texinfoDir} does not exist");
			}

			Process.Run($"{texinfoDir.Combine("configure")}", $"--prefix={builtToolsDir}", texinfoDir);
			Process.Run("make", "", texinfoDir);
			Process.Run("make", "install", texinfoDir);
		}

		private void BuildAutomake(NPath builtToolsDir)
		{
			Console.WriteLine(">> Building automake");

			var automakeVersion = "1.15";
			var automakeDir = MonoBuildDeps.Combine("automake-src").Combine($"automake-{automakeVersion}");

			if (!automakeDir.DirectoryExists())
			{
				throw new Exception($"{automakeDir} does not exist");
			}

			Process.Run($"{automakeDir.Combine("configure")}", $"--prefix={builtToolsDir}", automakeDir);
			Process.Run("make", "", automakeDir);
			Process.Run("make", "install", automakeDir);
		}

		private void BuildLibTool(NPath builtToolsDir)
		{
			Console.WriteLine(">> Building libtool");

			var libtoolVersion = "2.4.6";
			var libtoolDir = MonoBuildDeps.Combine("libtool-src").Combine($"libtool-{libtoolVersion}");

			if (!libtoolDir.DirectoryExists())
			{
				throw new Exception($"{libtoolDir} does not exist");
			}

			Process.Run($"{libtoolDir.Combine("configure")}", $"--prefix={builtToolsDir}", libtoolDir);
			Process.Run("make", "", libtoolDir);
			Process.Run("make", "install", libtoolDir);

			Environment.SetEnvironmentVariable("LIBTOOLIZE", builtToolsDir.Combine("bin").Combine("libtoolize"));
			Environment.SetEnvironmentVariable("LIBTOOL", builtToolsDir.Combine("bin").Combine("libtool"));
		}

		public void VerifyInstalledTools()
		{
			Console.WriteLine($">> Existing Mono : {ExistingExternalMonoRoot.Combine(ExistingExternalMonoRoot)}");
			FileUtils.AddToPathEnvVar(ExistingExternalMonoRoot.Combine(ExistingExternalMonoRoot));

			Console.WriteLine(">> mcs information :");
			Process.Run("which", "mcs", ".");
			Process.Run("mcs", "--version", ".");

			Console.WriteLine(">> Checking on some tools ... :");
			Process.Run("which", "autoconf", ".");
			Process.Run("autoconf", "--version", ".");

			Process.Run("which", "texi2dvi", ".");
			Process.Run("texi2dvi", "--version", ".");

			Process.Run("which", "automake", ".");
			Process.Run("automake", "--version", ".");

			Process.Run("which", "libtool", ".");
			Process.Run("libtool", "--version", ".");

			Process.Run("which", "libtoolize", ".");
			Process.Run("libtoolize", "--version", ".");

			Console.WriteLine($">> LIBTOOL before build = {Environment.GetEnvironmentVariable("LIBTOOL")}");
			Console.WriteLine($">> LIBTOOLIZE before build = {Environment.GetEnvironmentVariable("LIBTOOLIZE")}");
		}

		protected void DetermineExistingMonoPath()
		{
			if (String.IsNullOrEmpty(BuilderOptions.ExistingMono))
			{
				var existingExternalMbeRoot = MonoBuildDeps.Combine("MonoBleedingEdge");

				if (existingExternalMbeRoot.DirectoryExists())
				{
					Console.WriteLine(">> Existing external mono found at " + existingExternalMbeRoot);

					var existingExternalMonoBuildsDir = existingExternalMbeRoot.Combine("builds");
					if (existingExternalMonoBuildsDir.DirectoryExists())
					{
						Console.WriteLine(">> Existing external mono is already extracted " + existingExternalMonoBuildsDir);
					}
					else
					{
						Console.WriteLine(">> Extracting mono builds.zip ...");

						string sevenZip;
						string sevenZipArgs;
						if (Platform.IsWindows())
						{
							//TODO: Test on windows
							sevenZip = BuildDeps.GetSevenZip();
							sevenZipArgs = $"x {existingExternalMbeRoot.Combine("builds.zip")} -o{existingExternalMonoBuildsDir}";
						}
						else
						{
							sevenZip = "unzip";
							sevenZipArgs = $"{existingExternalMbeRoot.Combine("builds.zip")} -d {existingExternalMbeRoot.Combine("builds")}";
						}
						Process.Run(sevenZip, sevenZipArgs, ".");
					}
					ExistingExternalMonoRoot = existingExternalMonoBuildsDir.Combine("monodistribution");
				}
				else
				{
					Console.WriteLine(">> No external mono found.  Trusting a new enough mono is in your PATH");
				}
			}
			else
			{
				if (!new NPath(BuilderOptions.ExistingMono).DirectoryExists())
				{
					throw new Exception("Existing mono not found at " + BuilderOptions.ExistingMono);
				}
				ExistingExternalMonoRoot = BuilderOptions.ExistingMono;
			}

			DetermineMonoHostArch();
		}

		private void DetermineMonoHostArch()
		{
			string monoBinDir = "";
			if (Platform.IsLinux())
			{
				//TODO: Check architecture here
				MonoHostArch = BuilderOptions.Arch32 ? "i686" : "x86_64";
				monoBinDir = "bin-linux64";
			}
			else if (Platform.IsOSX())
			{
				MonoHostArch = BuilderOptions.Arch32 ? "i386" : "x86_64";
				monoBinDir = "bin";
			}
			else
			{
				MonoHostArch = "i686";
				monoBinDir = "bin-x64";
			}
			ExistingExternalMonoRoot = ExistingExternalMonoRoot.Combine(monoBinDir);

			Console.WriteLine($">> Existing Mono Root = {ExistingExternalMonoRoot}");
			Console.WriteLine($">> Mono Arch = {MonoHostArch}");
			//TODO ExistingExternalMono is still not set
		}

		protected void PrepareForBuild(List<string> configureParams)
		{
			DeleteConfigStatusFiles();
			GenerateMakeFiles(configureParams);
			if (BuilderOptions.McsOnly)
			{
				RunMake("clean", MonoRoot.Combine("mcs"));
			}
			else
			{
				RunMake("clean", MonoRoot);
			}
		}


		private void GenerateMakeFiles(List<string> configureParams)
		{
			string autogenArgs = "";
			var autogenPath = MonoRoot.Combine("autogen.sh");

			Console.WriteLine(">> Configure parameters are : ");

			foreach (var item in configureParams)
			{
				Console.WriteLine("\t" + item);
				autogenArgs = autogenArgs + " " + item;
			}

			Console.WriteLine(Environment.NewLine);
			Console.WriteLine(">> Calling autogen in mono");

			Process.Run(autogenPath, autogenArgs, MonoRoot);
		}

		private void DeleteConfigStatusFiles()
		{
			//Having these files around can cause "source directory already configured". So delete them.
			FileUtils.DeleteFileIfExists(MonoRoot.Combine("config.status"));
			FileUtils.DeleteFileIfExists(MonoRoot.Combine("eglib").Combine("config.status"));
			FileUtils.DeleteFileIfExists(MonoRoot.Combine("libgc").Combine("config.status"));
		}

		protected void RunMake(string arguments, NPath directory)
		{
			Console.WriteLine($">> Calling make {arguments} in {directory} ...");
			Process.Run("make", arguments, directory);
		}

		protected void MakeAdditionalProfiles()
		{
			var additionalProfiles = new[] { "unityjit", "unityaot" };

			foreach (var profile in additionalProfiles)
			{
				RunMake($"PROFILE={profile}", MonoRoot.Combine("mcs"));

				var profileDestDir = MonoPrefix.Combine("lib").Combine("mono").Combine(profile);
				var facadesDir = profileDestDir.Combine("Facades").CreateDirectory();

				FileUtils.CleanOrCreateDirectory(profileDestDir);

				Console.WriteLine($">> Copying {profile} to {profileDestDir}");
				MonoRoot.Combine("mcs").Combine("class").Combine("lib").Combine(profile)
					.CopyFiles(profileDestDir, recurse: false, fileFilter: p => p.FileName.Contains(".dll"));

				Console.WriteLine($">> Copying Facades to {facadesDir}");
				MonoRoot.Combine("mcs").Combine("class").Combine("lib").Combine(profile).Combine("Facades")
					.CopyFiles(facadesDir, recurse: false, fileFilter: p => p.FileName.Contains(".dll"));
			}
		}

		protected void WriteVersionInformation(NPath distDirArchBin, string versionsOutputFile)
		{
			string monoVersion = "";

			if (distDirArchBin.DirectoryExists())
			{
				var mono = Platform.IsWindows() ? distDirArchBin.Combine("mono.exe") : distDirArchBin.Combine("mono");
				if (mono.FileExists())
				{
					monoVersion = Process.RunAndReturnOutput(distDirArchBin.Combine("mono"), "--version", ".");
				}
			}

			Console.WriteLine($">> Creating version file : {versionsOutputFile}");
			using (StreamWriter writer = new StreamWriter(versionsOutputFile))
			{
				writer.WriteLine($"mono-version = {monoVersion}");
				writer.WriteLine($"unity-mono-revision = {UnityMonoRevision}");
				writer.WriteLine($"unity-mono-build-scripts-revision = {UnityMonoBuildScriptsRevision}");
				writer.Write(DateTime.Now.ToString("yyyy-MM-ddTHH:mm:sszzz"));
			}
		}

		protected void RunTests()
		{
			if (BuilderOptions.RuntimeTests)
			{
				var runtimeTestsDir = MonoRoot.Combine("mono").Combine("mini");
				Console.WriteLine($">> Calling make check in {runtimeTestsDir}\n");
				Process.Run("make", "check", runtimeTestsDir);
			}
			else
			{
				Console.WriteLine(">> Skipping runtime unit tests");
			}

			if (BuilderOptions.ClasslibTests)
			{
				if (BuilderOptions.DisableMcs)
				{
					Console.WriteLine(">> Skipping classlib unit tests because building the class libs was disabled");
				}
				else
				{
					var runtimeTestsDir = MonoRoot.Combine("mcs").Combine("class");
					Console.WriteLine($">> Calling make run-test in {runtimeTestsDir}\n");
					Process.Run("make", "run-test", runtimeTestsDir);
				}
			}
			else
			{
				Console.WriteLine(">> Skipping classlib unit tests");
			}
		}
	}

}

