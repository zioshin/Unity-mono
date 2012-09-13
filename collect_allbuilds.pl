use lib ('./perl_lib');
use File::Copy::Recursive qw(dircopy);
use File::Path;
use Tools qw(InstallNameTool);

my $path = "incomingbuilds/";

rmtree("collectedbuilds");
mkpath("collectedbuilds");

my @folders = ();
opendir(DIR, $path) or die "cant find $path: $!";
while (defined(my $file = readdir(DIR))) {
	next if $file =~ /^\.\.?$/;
	if (-d "$path$file"){
		dircopy("$path$file","collectedbuilds/") or die ("failed copying $path$file");
		push @folders,"$path$file";
	}
}
closedir(DIR);

system("chmod +x collectedbuilds/monodistribution/bin/mono") eq 0 or die("Failed chmodding");
chdir("collectedbuilds");

open(MYFILE,">built_by_teamcity.txt");
print MYFILE "These builds were created by teamcity from svn revision $ENV{BUILD_VCS_NUMBER}\n";
print MYFILE "TC projectname was: $ENV{TEAMCITY_PROJECT_NAME}\n";
print MYFILE "TC buildconfigname was: $ENV{TEAMCITY_BUILDCONF_NAME}\n";
close(MYFILE);

system("zip -r builds.zip *") eq 0 or die("failed zipping up builds");
