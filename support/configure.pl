# This is a template for a configuration script of a bundled or standalone extension.
# Please edit it according to your needs and rename it to configure.pl
# All subroutines defined below must stay here, even if they do nothing.


# List of variables used in the ninja rules that must be modified when building this extension.
# The complete list of recognized variables is contained in the module perllib/Polymake/Configure.pm.
# Usually you will have to modify only few of them, most probably CXXFLAGS, LDFLAGS, or LIBS.
# Please put just the bare names of the variables here, without '$' prefix.

@conf_vars=qw( CXXFLAGS LDFLAGS LIBS );


# This subroutine should augment the key sets of one or both hash maps passed by reference.
# These hash maps contain all options recognized by configure script.
# The first hash map is used for normal options like --docdir.
# The second hash map is used for trivalent options like --with-java / --without-java.
#
# The option disabling this extension completely (only applicable to bundled extensions)
# is added automatically and should not be mentioned here.
# Please specify just the bare option names, without '--' prefix.

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( julia ) }=();
}

sub usage {
   print STDERR "  --with-julia=PATH  installation path of julia, if non-standard\n";
}

sub proceed {
   my ($options)=@_;
   my ($julia_path, $julia_version, $julia_inc, $julia_lib, $julia_config);

   if (defined ($julia_path=$options->{julia})) {
      $julia_inc="$julia_path/include";
      $julia_lib=Polymake::Configure::get_libdir($julia_path, "julia");
   }
   if ($julia_inc) {
      if (-f "$julia_inc/julia/julia.h") {
         $CXXFLAGS = "-I$julia_inc -I$julia_inc/julia";
      } else {
         die "Invalid installation location of julia: header file $julia_inc/julia/julia.h does not exist\n";
      }
   }
   if ($julia_lib) {
      if (-f "$julia_lib/libjulia.$Config::Config{so}") {
         $LDFLAGS = "-L$julia_lib";
         $LDFLAGS .= " -Wl,-rpath,$julia_lib" unless $julia_lib =~ m#^/usr/lib#;
      } else {
         die "Invalid installation location of libjulia: library libjulia.$Config::Config{so} does not exist\n";
      }
   }

   # TODO: build test program, check version?

   $LIBS="-ljulia";
   return "$julia_version @ ".($julia_path//"system");
}
