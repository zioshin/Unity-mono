using System;

namespace MonoBuildProgram
{
	public class ProfileStubber
	{
		public void StubClassLibs()
		{
			if (!CanBuildProfileStubber())
				return;

			BuildProfileStubber();
			RunProfileStubber();
		}

		private void BuildProfileStubber()
		{
			Console.WriteLine(">> Building profile stubber ...");

			var result = Process.Run("xbuild", $"{BaseBuilder.MonoBuildToolsExtra.Combine("mono-build-tools-extra.sln")} /p:Configuration=Release", "." );
			if (result != 0)
			{
				throw new Exception(">> Failed to build ProfileStubber utility");
			}
		}

		private void RunProfileStubber()
		{
			var referenceProfileName = "4.7.1-api";
			var profileRoot = BaseBuilder.MonoPrefix.Combine("lib").Combine("mono");
			var referenceProfile = profileRoot.Combine(referenceProfileName);
			
			var additionalProfiles = new[] {"unityjit", "unityaot"};

			foreach (var profile in additionalProfiles)
			{
				Console.WriteLine($">> Modifying the {profile} profile to match the .NET {referenceProfileName} ...");

				var exe = BaseBuilder.MonoBuildToolsExtra.Combine("build").Combine("ProfileStubber.exe");
				var arguments = $"--reference-profile={referenceProfile} --stub-profile={profileRoot}/{profile}";
				var result = Process.Run("mono", $"{exe} {arguments}", ".");

				if (result != 0)
				{
					throw new Exception("Failed to stub the unityaot profile");
				}
			}
		}

		private bool CanBuildProfileStubber()
		{
			if (!Platform.IsOSX())
			{
				Console.WriteLine(">> The ProfileStubber is only built and run in the class library build on macOS");
				return false;
			}
			return true;
		}
	}
}
