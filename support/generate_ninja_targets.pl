# This is a template for an optional script generating a list of non-standard build targets
# and/or ninja rules for a bundled or standalone extension.
# Please edit it according to your needs and rename it to generate_ninja_targets.pl
# If you don't need it, you can safely delete this file right now.

# This script may contain any perl code doing the following:
#
# - Print ninja commands like variable assignments, build commands for certain targets, or build rules.
# Rules which might be shared with other extensions may also go in a separate file rules.ninja
# placed in the same directory (support).  They will be included automatically.
#
# - Return a list of build targets to be included in top-level aliases like 'all' or 'clean',
#   for example:
#
#     return ( all => 'filename' )
#
# This script can find the configuration values of the extension in the global hash %ConfigVars.
# $ConfigVars{srcroot} always points to the top directory of the extension.
# The global variable $root always points to the top directory of polymake core system.

# Examples can be found in bundled extensions java, jreality, and javaview.
#
use vars '$extroot';

my $libname = "\${buildtop}/lib/libpolymake_oscarnumber.\${So}";
my $build_cmd="";
my @obj_files;
my $src_dir="$ConfigFlags{extroot}/external/jlpolymake_oscarnumber";
foreach my $src_file (glob "$src_dir/src/*.cpp") {
   my ($src_name, $obj_name)=basename($src_file, "cpp");
   $src_file =~ s/^\Q$root\E/\${root}/;
   my $obj_file="\${buildtop}/jlcxx/$obj_name.o";
   $build_cmd .= <<"---";
build $obj_file: cxxcompile $src_file
  CXXextraFLAGS=-I\${extroot}/include/app-wrappers -I\${extroot}/include/apps -I$src_dir/include -std=c++17
---
   push @obj_files, $obj_file;
}
$build_cmd .= <<"---";
build $libname: sharedmod @obj_files
  LDextraFLAGS=-Wl,--allow-undefined
  LIBSextra=-lcxxwrap_julia -ljulia
---

print "$build_cmd\n";


( all => $libname );
