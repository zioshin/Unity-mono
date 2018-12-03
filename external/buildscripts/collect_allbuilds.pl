use lib ('external/buildscripts/perl_lib');
use Cwd 'abs_path';
use File::Basename;
use File::Copy::Recursive qw(dircopy rmove);
use File::Path;
use Tools qw(InstallNameTool);
use File::Copy;


my $monoroot = File::Spec->rel2abs(dirname(__FILE__) . "/../..");
my $monoroot = abs_path($monoroot);

my $path = "incomingbuilds/";

rmtree("collectedbuilds");
mkpath("collectedbuilds");

# Copy bareminimum into classlibs
dircopy("incomingbuilds/bareminimum/mono", "incomingbuilds/classlibs/osx/mono");
rmtree("incomingbuilds/bareminimum/mono");

my @folders = ();
opendir(DIR, $path) or die "cant find $path: $!";
# Sort the directories alphabetically so that classlibs comes before the
# OSX universal runtime (in the osx-i386 directory). Both builds produce the same
# files in some cases (notably libMonoPosixHelper.dylib), and we need the 
# universal runtime build to be second, since it produces a universal binary
# and the classlibs build produces a 32-bit binary only.  
my @files = sort readdir(DIR);
while (defined(my $file = shift @files)) {

	next if $file =~ /^\.\.?$/;
	if (-d "$path$file"){
		if (-f "$path$file/versions.txt") {
			system("cat $path$file/versions.txt >> collectedbuilds/versions-aggregated.txt");
		}
		dircopy("$path$file","collectedbuilds/") or die ("failed copying $path$file");
		push @folders,"$path$file";
	}
}
closedir(DIR);

# Copy classlibs into windows and linux. 
# Windows and linux runtime builds may have produced some files under collectedbuilds/<platform>/<arch>/mono. 
# We don't want them to be replaced when we copy over classlibs. 
# So, preserve them and copy back after the classlibs are copied

# Copy classlibs into windows x86
move('collectedbuilds/win/x86/mono', 'collectedbuilds/win/x86/mono-tmp');
dircopy("incomingbuilds/classlibs/osx/mono", "collectedbuilds/win/x86/mono");
dircopy("collectedbuilds/win/x86/mono-tmp/*", "collectedbuilds/win/x86/mono");
rmtree("collectedbuilds/win/x86/mono-tmp");

# Copy classlibs into windows x86_64
move('collectedbuilds/win/x86_64/mono', 'collectedbuilds/win/x86_64/mono-tmp');
dircopy("incomingbuilds/classlibs/osx/mono", "collectedbuilds/win/x86_64/mono");
dircopy("collectedbuilds/win/x86_64/mono-tmp/*", "collectedbuilds/win/x86_64/mono");
rmtree("collectedbuilds/win/x86_64/mono-tmp");

# Copy classlibs into linux x86
move('collectedbuilds/linux/x86/mono', 'collectedbuilds/linux/x86/mono-tmp');
dircopy("incomingbuilds/classlibs/osx/mono", "collectedbuilds/linux/x86/mono");
dircopy("collectedbuilds/linux/x86/mono-tmp/*", "collectedbuilds/linux/x86/mono");
rmtree("collectedbuilds/linux/x86/mono-tmp");

# Copy classlibs into linux x86_64
move('collectedbuilds/linux/x86_64/mono', 'collectedbuilds/linux/x86_64/mono-tmp');
dircopy("incomingbuilds/classlibs/osx/mono", "collectedbuilds/linux/x86_64/mono");
dircopy("collectedbuilds/linux/x86_64/mono-tmp/*", "collectedbuilds/linux/x86_64/mono");
rmtree("collectedbuilds/linux/x86_64/mono-tmp");

# Cleanup
unlink glob "collectedbuilds/linux/x86/mono/bin/*.bat";
unlink glob "collectedbuilds/linux/x86_64/mono/bin/*.bat";
unlink glob "collectedbuilds/osx/mono/bin/*.bat";

Cleanup("collectedbuilds/win/x86/mono/bin");
Cleanup("collectedbuilds/win/x86_64/mono/bin");

sub Cleanup
{
	my $dirname = shift;
	opendir(DH, $dirname);
	my @files = readdir(DH);
	closedir(DH);

	foreach my $file (@files)
	{
		# skip . and ..
		next if($file =~ /^\.$/);
		next if($file =~ /^\.\.$/);
	
		# Delete files with no extension, like shell executables
		if($file !~ /\./ )
		{
			my $working_dir_abs_path = File::Spec->rel2abs(dirname(__FILE__));
			unlink glob "$working_dir_abs_path/$dirname/$file";
		}
	}
}

system("find collectedbuilds -type f -name mono -exec chmod +x {} \\;") eq 0 or die("Failed chmodding");
system("find collectedbuilds -type f -name mono-sgen -exec chmod +x {} \\;") eq 0 or die("Failed chmodding");
system("find collectedbuilds -type f -name pedump -exec chmod +x {} \\;") eq 0 or die("Failed chmodding");

chdir("collectedbuilds");

rmove('versions-aggregated.txt', 'versions.txt');

open(MYFILE,">built_by_teamcity.txt");
print MYFILE "These builds were created by teamcity from svn revision $ENV{BUILD_VCS_NUMBER}\n";
print MYFILE "TC projectname was: $ENV{TEAMCITY_PROJECT_NAME}\n";
print MYFILE "TC buildconfigname was: $ENV{TEAMCITY_BUILDCONF_NAME}\n";
close(MYFILE);

system("zip -r builds.zip *") eq 0 or die("failed zipping up builds");

my $externalzip = "";
if($^O eq "linux")
{
	$externalzip = "$monoroot/../../mono-build-deps/build/7z/linux64/7za";
}
elsif($^O eq 'darwin')
{
	$externalzip = "$monoroot/../../mono-build-deps/build/7z/osx/7za";
}

if($^O eq "linux" || $^O eq 'darwin')
{
	if(-f $externalzip)
	{
		system("$externalzip a builds.7z * -x!builds.zip") eq 0 or die("failed 7z up builds");
	}
	else
	{
		#Use 7z installed on the machine. If its not installed, please install it.
		system("7z a builds.7z * -x!builds.zip") eq 0 or die("failed 7z up builds");
	}
}
else
{
	die("Unsupported platform for build collection.")
}