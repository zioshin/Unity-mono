using System;
using NiceIO;

namespace MonoBuildProgram
{
	public class FileUtils
	{
		public static void CleanOrCreateDirectory(NPath dir)
		{
			if (!dir.IsRelative)
			{
				if (dir.DirectoryExists())
				{
					Console.WriteLine($">> Cleaning {dir}");
					dir.DeleteContents();
				}
				else
				{
					Console.WriteLine($">> Creating {dir}");
					dir.CreateDirectory();
				}
			}
			else
			{
				Console.WriteLine($">> Nothing to clean");
			}
		}

		public static void DeleteFileIfExists(NPath file)
		{
			if (file.FileExists())
			{
				Console.WriteLine($">> Deleting {file}");
				file.Delete();
			}
		}
		
		public static void CreateDirectoryIfNotExists(NPath dir)
		{
			if (!dir.DirectoryExists())
			{
				Console.WriteLine($">> Creating {dir}");
				dir.CreateDirectory();
			}
		}

		public static void AddToPathEnvVar(string path)
		{
			var name = "PATH";

			Console.WriteLine($">> Adding {path} to {name}");
			string currentValue = Environment.GetEnvironmentVariable(name);

			string newPath;
			if (Platform.IsWindows())
			{
				newPath = currentValue + path;
			}
			else
			{
				newPath = path + ":" + currentValue;
			}

			Environment.SetEnvironmentVariable(name, newPath);
			Console.WriteLine($"Environment {name}: {Environment.GetEnvironmentVariable(name)}");
			Console.WriteLine(Environment.NewLine);
		}

		public static NPath ProgramFilesx86()
		{
			if (8 == IntPtr.Size || (!String.IsNullOrEmpty(Environment.GetEnvironmentVariable("PROCESSOR_ARCHITEW6432"))))
			{
				return Environment.GetEnvironmentVariable("ProgramFiles(x86)");
			}
			return Environment.GetEnvironmentVariable("ProgramFiles");
		}
	}
}
