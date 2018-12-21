using System;
using System.Linq;
using MonoBuildProgram.Utils.Utils.Options;
using NiceIO;

namespace MonoBuildProgram
{
	[ProgramOptions]
	public static class BuilderOptions
	{
		[HelpDetails("Windows runtime")]
		public static bool WindowsRuntime;

		[HelpDetails("Bare minimum runtime")]
		public static bool BareMinimumRuntime;

		[HelpDetails("Linux runtime")]
		public static bool LinuxRuntime;

		[HelpDetails("OSX runtime")]
		public static bool OsxRuntime;

		[HelpDetails("Android runtime")]
		public static bool AndroidRuntime;

		[HelpDetails("Classlibs")]
		public static bool Classlibs;

		[HelpDetails("Build UnityScript and Boo (default=false)")]
		public static bool BuildUsAndBoo;

		[HelpDetails("Build (default=true)")]
		public static bool Build;

		[HelpDetails("Clean (default=true)")]
		public static bool Clean;

		[HelpDetails("Test (default=false)")]
		public static bool Test;

		[HelpDetails("Run runtime tests (default=true)")]
		public static bool RuntimeTests;

		[HelpDetails("Run classlib tests (default=true)")]
		public static bool ClasslibTests;

		[HelpDetails("Create artifacts (default=true)")]
		public static bool Artifact;

		[HelpDetails("Create common artifacts (default=false)")]
		public static bool ArtifactsCommon;

		[HelpDetails("Create runtime artifacts (default=true)")]
		public static bool ArtifactsRuntime;

		[HelpDetails("Build configuration. Debug|Release. (default=Release)")]
		public static BuildConfiguration Configuration;

		[HelpDetails("Arch32. default=true)")]
		public static bool Arch32;

		[HelpDetails("AndroidArchitecture. x86|armv7a. (default=x86)")]
		public static string AndroidArch;

		[HelpDetails("Path to root directory of existing mono (default=null)")]
		public static NPath ExistingMono;

		[HelpDetails("Disable mcs (default=false)")]
		public static bool DisableMcs;

		[HelpDetails("Mcs only (default=false)")]
		public static bool McsOnly;

		[HelpDetails("Number of jobs for Make (default=8)")]
		public static int Jobs;

		[HelpDetails("Enable cache file (default=false)")]
		public static bool EnableCacheFile;

		public static void SetToDefaults()
		{
			BuildUsAndBoo = false;
			Clean = true;
			Build = true;
			Test = false;
			RuntimeTests = true;
			ClasslibTests = true;
			Artifact = true;
			ArtifactsCommon = false;
			ArtifactsRuntime = true;
			Configuration = BuildConfiguration.Release;
			Arch32 = true;
			AndroidArch = "x86";
			ExistingMono = null;
			DisableMcs = false;
			McsOnly = false;
			Jobs = 8;
			EnableCacheFile = false;
		}

		public static bool InitAndSetup(string[] args)
		{
			SetToDefaults();

			if (OptionsParser.HelpRequested(args) || args.Length == 0)
			{
				OptionsParser.DisplayHelp(typeof(Program).Assembly, false);
				return false;
			}

			var unknownArgs = OptionsParser.Prepare(args, typeof(Program).Assembly, false).ToList();

			if (unknownArgs.Count > 0)
			{
				Console.WriteLine("Unknown arguments : ");
				foreach (var remain in unknownArgs)
				{
					Console.WriteLine("\t {0}", remain);
				}

				return false;
			}

			return true;
		}

		public enum BuildConfiguration
		{
			Debug,
			Release,
		}

	}
}
