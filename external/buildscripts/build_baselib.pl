use Cwd;
use Cwd 'abs_path';
use Getopt::Long;
use File::Basename;
use File::Path;
use File::Copy;

my $currentdir = getcwd();

my $target=0;

GetOptions('target=s'=>\$target);

my $baseLibTarget = $target . "::baselib";

print ">>> Building baselib target $baseLibTarget\n";

chdir("external/baselib") eq 1 or die ("Failed to change to directory: external/baselib\n");

system("./bee", "b", "$baseLibTarget") eq 0 or die("Failed to build baselib target: $baseLibTarget\n");

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
	$outputlib = "release_win64_nonlump/baselib.dll";
}
elsif ($target eq "win32")
{
	$outputlib = "release_win32_nonlump/baselib.dll";
}
else
{
	die ("Unrecognized target: $target\n");
}

my $buildDestination = "$currentdir/support/.libs";
mkdir "$buildDestination";
copy("artifacts/baselib/$outputlib", $buildDestination) or die ("Failed copying artifacts/baselib/$outputlib to $buildDestination\n");

if ($target eq "mac64" or $target eq "mac32")
{
	my $unitTestDestination = "$currentdir/tmp/lib";
	mkdir "$unitTestDestination";
	copy("artifacts/baselib/$outputlib", $unitTestDestination) or die ("Failed copying artifacts/baselib/$outputlib to $unitTestDestination\n");
}

chdir($currentdir);
