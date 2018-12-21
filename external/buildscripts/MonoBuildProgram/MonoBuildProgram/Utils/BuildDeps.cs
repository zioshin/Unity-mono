using NiceIO;

namespace MonoBuildProgram
{
	public class BuildDeps
	{
		public static NPath GetSevenZip()
		{
			string sevenZip = "";
			if (Platform.IsWindows())
				sevenZip = BaseBuilder.MonoBuildDeps.Combine("7z").Combine("win64").Combine("7za.exe");
			else if (Platform.IsLinux())
				sevenZip = BaseBuilder.MonoBuildDeps.Combine("7z").Combine("linux64").Combine("7za");
			else if (Platform.IsOSX())
				sevenZip = BaseBuilder.MonoBuildDeps.Combine("7z").Combine("osx").Combine("7za");
			return sevenZip;
		}
	}
}