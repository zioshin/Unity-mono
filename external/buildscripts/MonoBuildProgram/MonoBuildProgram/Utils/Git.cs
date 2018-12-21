using System;
using NiceIO;

namespace MonoBuildProgram
{
	public class Git
	{
		public static void GitClone(string repoUrl, NPath repoDir, string branch = "master")
		{
			if (repoDir.DirectoryExists())
				repoDir.DeleteContents();

			Console.WriteLine($">> Cloning {repoUrl} to {repoDir} and checking out branch {branch}");
			Process.Run("git", $"clone --recurse-submodules --branch {branch} {repoUrl} {repoDir}", ".");
		}
	}
}