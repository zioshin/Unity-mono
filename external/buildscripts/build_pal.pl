require "./common_functions.pl";

my $monoroot = get_mono_root();
my $externalBuildDeps = get_external_build_deps("", true, $monoroot);
my $externalBuildDepsIl2Cpp = get_external_build_deps_il2cpp();

checkout_build_deps_if_needed($externalBuildDeps, $externalBuildDepsIl2Cpp, true);

my $premakeDir = "$externalBuildDeps/premake";
my $premakePkg = "premake-5.0.0-alpha12";
my $premakeCmd = "$premakeDir/premake5";

my $isWindows = 0;

chdir($premakeDir);
if ($^O eq "linux")
{
    $premakePkg = "$premakePkg-linux.tar.gz";
    system("tar xzf $premakeDir/$premakePkg") eq 0 or die("Failed to unzip $premakePkg\n");
}
elsif ($^O eq 'darwin')
{
    $premakePkg = "$premakePkg-macosx.tar.gz";
    system("tar xzf $premakeDir/$premakePkg") eq 0 or die("Failed to unzip $premakePkg\n");
}
else
{
    $premakePkg = "$premakePkg-windows.zip";
    my $sevenZip = "$externalBuildDeps/7z/win64/7za.exe";
    system($sevenZip, "x", "$premakeDir/$premakePkg", "-o$premakeDir", "-y") eq 0 or die("Failed to unzip $premakePkg\n");
    $premakeCmd = "$premakeCmd.exe";
    $isWindows = 1;
}

chdir($monoroot);

my $premakeFile = "$monoroot/msvc/unity-pal/unity-pal-premake.lua";

if ($isWindows)
{
    system("$premakeCmd", "vs2015", "--file=$premakeFile") eq 0 or die("Failed to premake $premakeFile\n");
    compile_with_vs("$monoroot/msvc/unity-pal/unity-pal.sln", false, false);
}
else
{
    system("$premakeCmd", "gmake", "--file=$premakeFile");
    chdir("$monoroot/msvc/unity-pal");
    system("make");
}
