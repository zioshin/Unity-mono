use Cwd;
use Cwd 'abs_path';
use Getopt::Long;
use File::Basename;
use File::Path;

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
else
{
	die ("Unrecognized target: $target\n");
}

my $buildDestination = "$currentdir/support/.libs";
system("mkdir -p $buildDestination");
system("cp", "artifacts/baselib/$outputlib", $buildDestination) eq 0 or die ("Failed copying artifacts/baselib/$outputlib to $buildDestination\n");

my $unitTestDestination = "$currentdir/tmp/lib";
system("mkdir -p $unitTestDestination");
system("cp", "artifacts/baselib/$outputlib", $unitTestDestination) eq 0 or die ("Failed copying artifacts/baselib/$outputlib to $unitTestDestination\n");

chdir($currentdir);
