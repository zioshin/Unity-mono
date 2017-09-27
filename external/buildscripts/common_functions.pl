use Cwd;
use Cwd 'abs_path';
use Getopt::Long;
use File::Basename;
use File::Path;
use lib ('external/buildscripts', "../../Tools/perl_lib","perl_lib", 'external/buildscripts/perl_lib');
use Tools qw(InstallNameTool);

sub get_mono_root
{
    my $monoroot = File::Spec->rel2abs(dirname(__FILE__) . "/../..");
    my $monoroot = abs_path($monoroot);
    return $monoroot;
}

sub get_external_build_deps
{
    my ($buildDeps, $forceDefaultBuildDeps, $monoroot) = @_;
    my $externalBuildDeps = "";
    if ($buildDeps ne "" && not $forceDefaultBuildDeps)
    {
	$externalBuildDeps = $buildDeps;
    }
    else
    {
	$externalBuildDeps = "$monoroot/../../mono-build-deps/build";
    }
    # Only clean up the path if the directory exists, if it doesn't exist,
    # abs_path ends up returning an empty string
    $externalBuildDeps = abs_path($externalBuildDeps) if (-d $externalBuildDeps);
    
    return $externalBuildDeps;
}

sub get_external_build_deps_il2cpp
{
    my $monoroot = get_mono_root();
    return "$monoroot/../../il2cpp/build";
}

sub checkout_mono_build_deps_if_needed
{
    my ($externalBuildDeps, $checkoutonthefly) = @_;
    if (-d "$externalBuildDeps")
    {
	print(">>> External build deps found\n");
    }
    else
    {
	if (not $checkoutonthefly)
	{
	    print(">>> No external build deps found.  Might as well try to check them out.  If it fails, we'll continue and trust mono is in your PATH\n");
	}
	
	# Check out on the fly
	print(">>> Checking out mono build dependencies to : $externalBuildDeps\n");
	my $repo = "https://ono.unity3d.com/unity-extra/mono-build-deps";
	print(">>> Cloning $repo at $externalBuildDeps\n");
	my $checkoutResult = system("hg", "clone", $repo, "$externalBuildDeps");
	
	if ($checkoutOnTheFly && $checkoutResult ne 0)
	{
	    die("failed to checkout mono build dependencies\n");
	}
    }
}

sub checkout_il2cpp_if_needed
{
    my ($externalBuildDepsIl2Cpp, $checkoutonthefly) = @_;
    if (-d "$externalBuildDepsIl2Cpp")
    {
	print(">>> External il2cpp found\n");
    }
    else
    {
	if (!(-d "$externalBuildDepsIl2Cpp"))
	{
	    my $il2cpp_repo = "https://bitbucket.org/Unity-Technologies/il2cpp";
            print(">>> Cloning $il2cpp_repo at $externalBuildDepsIl2Cpp\n");
            $checkoutResult = system("hg", "clone", $il2cpp_repo, "$externalBuildDepsIl2Cpp");
	    
            if ($checkoutOnTheFly && $checkoutResult ne 0)
            {
                die("failed to checkout IL2CPP for the mono build dependencies\n");
            }
	}
    }
}

sub checkout_build_deps_if_needed
{
    my($externalBuildDeps, $externalBuildDepsIl2Cpp, $checkoutonthefly) = @_;
    checkout_mono_build_deps_if_needed($externalBuildDeps, $checkoutonthefly);
    checkout_il2cpp_if_needed($externalBuildDepsIl2Cpp, $checkoutonthefly);
}

sub compile_with_vs
{
    my($sln, $debug, $arch32) = @_;
    my $msbuild = $ENV{"ProgramFiles(x86)"}."/MSBuild/14.0/Bin/MSBuild.exe";
    
    my $config = $debug ? "Debug" : "Release";
    my $arch = $arch32 ? "Win32" : "x64";
    my $target = $clean ? "/t:Clean,Build" :"/t:Build";
    my $properties = "/p:Configuration=$config;Platform=$arch";
    
    print ">>> $msbuild $properties $target $sln\n\n";
    system($msbuild, $properties, $target, $sln) eq 0
	or die("MSBuild failed to build $sln\n");
}

return 1;
