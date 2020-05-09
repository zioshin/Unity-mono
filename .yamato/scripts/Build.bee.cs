using System;
using Bee.Core;
using Bee.Stevedore;
using Unity.BuildSystem.NativeProgramSupport;

namespace BuildProgram
{
    internal static class Program
    {
        private static void Main()
        {
            StevedoreGlobalSettings.Instance = new StevedoreGlobalSettings
            {
                Manifest = { "manifest.stevedore" }
            };

            if (Platform.HostPlatform is MacOSXPlatform)
            {
                Console.WriteLine($">>> Registering artifact");
                Backend.Current.Register(new StevedoreArtifact("mono-iOS-simulator-runtime"));
            }
        }
    }
}
