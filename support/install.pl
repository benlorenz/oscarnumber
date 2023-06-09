# This is a template for an optional script installing non-standard artifacts
# like java archives or other resources built in a bundled or standalone extension.
# Please edit it according to your needs and rename it to install.pl
# If you don't need it, you can safely delete this file right now.

# This script can use any subroutines defined in the main install.pl script
# (to be found in the support directory of the core polymake system)
# like make_dir, copy_dir, or copy_file.

# The configuration values of the extension can be found in the global hash %ConfigFlags.
# The global variable $ext_root points to the top directory of the extension.
# The global variable $buildir points to the build directory of the extension, usually
# it will be $ext_root/build/Opt .
# The global variable $root points to the top directory of the core polymake system.

# A bundled extension should install all binaries specific for a system architecture
# into an appropriate subdirectory of $InstallArch and everything else into subdirectories
# of $InstallTop.  For a standalone extension, the corresponding destination directories
# are $ExtArch and $ExtTop.

my @libs = glob("$buildtop/lib/libpolymake_oscarnumber.*");
copy_file(@libs, "$InstallArch/lib/", mode=> 0555);
my @apps = glob("$buildtop/apps/*");
for my $dir (@apps) {
   my $app = basename($dir);
   make_dir("$ExtTop/apps/$app");
   system("touch $ExtTop/apps/$app/.empty");
}
