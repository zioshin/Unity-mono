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

system("./bee", "b", "$baseLibTarget") eq 0 or die("Failed to build baselib target: $baseLibTarget\n");

my $outputlib = "";
if ($target eq "mac64")
{
	$outputlib = "release_macosx64/baselib.dylib";
}
elsif ($target eq "mac32")
{
	$outputlib = "release_macosx32/baselib.dylib";
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
	$outputlib = "release_linux64/baselib.so";
}
elsif ($target eq "linux_x86")
{
	$outputlib = "release_linux32/baselib.so";
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

# Don't try to copy the output now. On Linx we build baselib very early, before the output directory exists.
# This is necessary because the system Mono won't run bee properly with the SDK we use to build Mono. Later in
# the build process for Linux, build.pl will reach into the artifacts directory and pull out what it needs.
if ($target ne "linux_x64" and $target ne "linux_x86")
{
	my $buildDestination = "$currentdir/support/.libs";
	mkdir "$buildDestination";
	copy("artifacts/baselib/$outputlib", $buildDestination) or die ("Failed copying artifacts/baselib/$outputlib to $buildDestination\n");
}

# Copy the output for unit tests only on some platforms (we don't run the unit tests everywhere)
if ($target eq "mac64" or $target eq "mac32")
{
	my $unitTestDestination = "$currentdir/tmp/lib";
	mkdir "$unitTestDestination";
	copy("artifacts/baselib/$outputlib", $unitTestDestination) or die ("Failed copying artifacts/baselib/$outputlib to $unitTestDestination\n");
}

chdir($currentdir);
