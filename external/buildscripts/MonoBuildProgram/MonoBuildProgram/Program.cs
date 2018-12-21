using System;
using MonoBuildProgram.Android;
using MonoBuildProgram.Linux;
using MonoBuildProgram.OSX;
using MonoBuildProgram.Windows;

namespace MonoBuildProgram
{
	public class Program
	{
		public static int Main(string[] args)
		{
			int exitCode;

			if (!BuilderOptions.InitAndSetup(args))
				return 1;

			try
			{ 
				exitCode = Run();
			}
			catch (Exception e)
			{
				Console.WriteLine("monobuildprogram.exe didn't catch exception: " + e);
				throw;
			}

			return exitCode;
		}

		private static int Run()
		{
			if (BuilderOptions.WindowsRuntime)
			{
				if (Platform.IsWindows())
				{
					BaseBuilder windowsBuilder = new WindowsRuntimeBuilder();
					windowsBuilder.Clean();
					windowsBuilder.Build();
					windowsBuilder.Artifacts();
				}
				else
				{
					Console.WriteLine(">> Windows runtime can only be built on Windows");
				}
			}
			else if (BuilderOptions.BareMinimumRuntime)
			{
				if (Platform.IsWindows())
				{
					BaseBuilder bareMinimumBuilder = new WindowsBareMinimumBuilder();
					bareMinimumBuilder.Clean();
					bareMinimumBuilder.Build();
					bareMinimumBuilder.Artifacts();
				}
				else
				{
					Console.WriteLine(">> Bare minimum runtime can only be built on Windows");
				}
			}
			else if (BuilderOptions.AndroidRuntime)
			{
				BaseBuilder androidBuilder = new AndroidBuilder();
				androidBuilder.Clean();
				androidBuilder.Build();
				androidBuilder.Artifacts();
			}
			else if (BuilderOptions.OsxRuntime)
			{
				if (Platform.IsOSX())
				{
					BaseBuilder osxRuntimeBuilder = new OSXBuilder();
					osxRuntimeBuilder.Clean();
					osxRuntimeBuilder.Build();
					osxRuntimeBuilder.Artifacts();
					osxRuntimeBuilder.Test();
				}
				else
				{
					Console.WriteLine(">> OSX runtime can only be built on OSX");
				}
			}
			else if (BuilderOptions.Classlibs)
			{
				if (Platform.IsOSX())
				{
					BuilderOptions.DisableMcs = false;
					BuilderOptions.ArtifactsCommon = true;
					BuilderOptions.ArtifactsRuntime = true;
					BuilderOptions.BuildUsAndBoo = true;

					BaseBuilder classlibsBuilder = new OSXBuilder();
					classlibsBuilder.Clean();
					classlibsBuilder.Build();

					if (BuilderOptions.BuildUsAndBoo)
					{
						BaseBuilder usAndBooBuilder = new USAndBooBuilder();
						usAndBooBuilder.Clean();
						usAndBooBuilder.Build();
						usAndBooBuilder.Artifacts();
					}

					classlibsBuilder.Artifacts();
					classlibsBuilder.Test();
				}
				else
				{
					Console.WriteLine(">> OSX classlibs can only be built on OSX");
				}
			}
			else if (BuilderOptions.LinuxRuntime)
			{
				if (Platform.IsLinux())
				{
					BaseBuilder linuxRuntimeBuilder = new LinuxRuntimeBuilder();
					linuxRuntimeBuilder.Clean();
					linuxRuntimeBuilder.Build();
					linuxRuntimeBuilder.Artifacts();
					linuxRuntimeBuilder.Test();
				}
				else
				{
					Console.WriteLine(">> Linux runtime can only be built on Linux");
				}
			}
			return 0;
		}
	}
}
