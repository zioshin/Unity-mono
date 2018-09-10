use Cwd;
use Cwd 'abs_path';
use Getopt::Long;
use File::Basename;
use File::Path;
use File::Copy;

my $currentdir = getcwd();

my $target=0;

GetOptions('target=s'=>\$target);

my $baseLibTarget = "baselib::" . $target;

print ">>> Building baselib target $baseLibTarget\n";

chdir("external/baselib") eq 1 or die ("Failed to change to directory: external/baselib\n");

if ($^O eq "MSWin32")
{
	system("bee.exe", "b", "$baseLibTarget") eq 0 or die("Failed to build baselib target: $baseLibTarget\n");
}
else
{
	my $monoExecutable = "../../../../mono-build-deps/build/MonoBleedingEdge/builds/monodistribution/";
	if($^O eq "linux")
	{
		$monoExecutable .= "bin-linux32/mono";
	}
	elsif($^O eq 'darwin')
	{
		$monoExecutable .= "bin/mono";
	}

	if (-f $monoExecutable)
	{
		print ">>> Using Mono executable from $monoExecutable to run bee\n";
	}
	else
	{
		print ">>> Mono executable not found at $monoExecutable. Using the system Mono to run bee\n";
		$monoExecutable = "mono";

	}

	print ">>> Checking mono executable version\n";
	system("$monoExecutable", "--version");

	system("$monoExecutable", "bee.exe", "b", "$baseLibTarget") eq 0 or die("Failed to build baselib target: $baseLibTarget\n");
}

my $outputlib = "";
if ($target eq "mac64")
{
	$outputlib = "release_macosx64_nonlump/baselib.dylib";
}
elsif ($target eq "mac32")
{
	$outputlib = "release_macosx32_nonlump/baselib.dylib";
}
elsif ($target eq "win64")
{
	$outputlib = "release_win64/baselib.dll";
}
elsif ($target eq "win32")
{
	$outputlib = "release_win32/baselib.dll";
}
elsif ($target eq "linux_x64")
{
	$outputlib = "release_linux64_nonlump/baselib.so";
}
elsif ($target eq "linux_x86")
{
	$outputlib = "release_linux32_nonlump/baselib.so";
}
elsif ($target eq "android_arm32")
{
	$outputlib = "release_android_armv7/baselib.so";
}
elsif ($target eq "android_x86")
{
	$outputlib = "release_android_x86/baselib.so";
}
else
{
	die ("Unrecognized target: $target\n");
}

my $buildDestination = "$currentdir/support/.libs";
mkdir "$buildDestination";
copy("artifacts/baselib/$outputlib", $buildDestination) or die ("Failed copying artifacts/baselib/$outputlib to $buildDestination\n");

# Copy the output for unit tests only on some platforms (we don't run the unit tests everywhere)
if ($target eq "mac64" or $target eq "mac32" or $target eq "linux_x64" or $target eq "linux_x86")
{
	my $unitTestDestination = "$currentdir/tmp/lib";
	mkdir "$unitTestDestination";
	copy("artifacts/baselib/$outputlib", $unitTestDestination) or die ("Failed copying artifacts/baselib/$outputlib to $unitTestDestination\n");
}

chdir($currentdir);
