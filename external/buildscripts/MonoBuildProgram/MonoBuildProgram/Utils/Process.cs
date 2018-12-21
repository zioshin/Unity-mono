using System;
using System.Diagnostics;

namespace MonoBuildProgram
{
	public class Process
	{
		public static string RunAndReturnOutput(string filename, string arguments, string workingDir)
		{
			var proc = ProcessStartInfo(filename, arguments, workingDir);
			proc.Start();
			var output = proc.StandardOutput.ReadToEnd();
			proc.WaitForExit();
			return output;
		}

		public static int Run(string filename, string arguments, string workingDir)
		{
			var proc = ProcessStartInfo(filename, arguments, workingDir);
			proc.OutputDataReceived += (sender, args) => Console.WriteLine("{0}", args.Data);
			proc.ErrorDataReceived += (sender, args) => Console.WriteLine("{0}", args.Data);
			proc.Start();
			proc.BeginOutputReadLine();
			proc.BeginErrorReadLine();
			proc.WaitForExit();
			return proc.ExitCode;
		}

		private static System.Diagnostics.Process ProcessStartInfo(string filename, string arguments, string workingDir)
		{
			var proc = new System.Diagnostics.Process
			{
				StartInfo = new ProcessStartInfo
				{
					FileName = filename,
					Arguments = arguments,
					UseShellExecute = false,
					RedirectStandardOutput = true,
					RedirectStandardError = true,
					CreateNoWindow = true,
					WorkingDirectory = workingDir
				}
			};
			return proc;
		}
	}
}