#! /usr/bin/perl -w
#
# Create Makefile for a Prima compilation
#
# This is not a ``normal'' Makefile.PL file.
# It does not use MakeMaker.
#
require 5.005;
use strict;
use Config;
use File::Find;
use vars qw(@all_defines);

my $extraccflags = '-g -O';  # gcc allows both
$extraccflags = '-g -Wall -Werror';

my %known_header = ();

# trick to be avoided
die "This mechanism currently only works on UNIX platfroms\n"
  unless $Config{path_sep} eq ':';

die "This script should be launched from Prima root directory\n"
  unless -r 'include/apricot.h';

$| = 1;

my $pwd = `pwd`; chomp $pwd;

my @inc_path = ("-I$Config{incpath}/usr/include",
		"-I$Config{installarchlib}/CORE",
		"-Iinclude",
		"-Iinclude/generic",
		"-I/usr/X11R6/include",
		"-I/usr/local/include");


@all_defines = ("-DPERL_PATCHLEVEL=$Config{patchlevel}",
		"-DPERL_SUBVERSION=$Config{subversion}",
		"-DPRIMA_CORE=1",
		"-DPERL_POLLUTE=1");
if ( defined( $ENV{ 'PRIMA_PARANOID_MALLOC'}) && $ENV{ 'PRIMA_PARANOID_MALLOC'} =~ /^(1|yes|on|true)$/i) {
    push @all_defines, "-DPARANOID_MALLOC=1";
}

my @cc;
push @cc, $Config{cc}, $extraccflags,
   $Config{cccdlflags}, $Config{ccflags},
   @inc_path;

my @ld;
push @ld, $Config{cc}, $extraccflags,
  $Config{cccdlflags}, $Config{ccflags},
  (map { "-L$_" } split ' ', $Config{libpth}),
  '-L/usr/X11R6/lib', '-L/usr/local/lib', "-L$Config{installarchlib}/CORE",
  ;

my $alltml = '';
my $allo = '';
my $allclean = '';

sub cmake
{
  my $cfile = $_[0];
  print "Finding dependencies for $cfile...";
  my $extra = '';
  $extra = '-I/usr/X11R6/include' if $cfile =~ /^unix/;
  my $ofile = "$1.o" if $cfile =~ /^(.*)\.c$/;
  die "Internal error: illegal c file"
    unless defined $ofile;
  chomp(my $r = `@cc $extra -MM -MG $cfile`);
  for (`@cc -MM $_[0] 2>&1`) {
    next unless m{^.*:\d+:\s+(.*)\.(.*): No such file or directory$};
    if ($2 eq 'h') {
      my $s = $1;
      $r =~ s[\b$s\.h\b][include/generic/$s\.h];
    } elsif ($2 eq 'inc') {
      my $s = $1;
      $r =~ s[\b$s\.inc\b][include/generic/$s\.inc];
    }
  }
  $r =~ s[^.*\.o\s*:][$ofile:];
  $allo .= "$ofile ";
  $allclean .= "$ofile ";
  print "\n";
  $r .= "\n\t@cc $extra -c -o $ofile $cfile\n\n";
}

sub clsmake
{
   my $clsfile = $_[0];
   print "Finding dependencies for $clsfile...";
   my $classname = $1 if $clsfile =~ /^(.*)\.cls$/;
   die "Internal error: illegal cls file"
      unless defined $classname;
   my $mk = "include/generic/$classname.inc " . 
      "include/generic/$classname.h " . 
	 "include/generic/$classname.tml: \\\n" .
	    "$clsfile utils/gencls.pl ";
   $alltml .= "include/generic/$classname.tml ";
   $allclean .= "include/generic/$classname.h ";
   $allclean .= "include/generic/$classname.inc ";
   $allclean .= "include/generic/$classname.tml ";
   for (`perl utils/gencls.pl --depend $clsfile`) {
      next unless /^ancestor:\s+(.*)$/;
      $mk .= "include/generic/$1.h $1.cls ";
   }
   print "\n";
   $mk .= "\n\tperl utils/gencls.pl " .
      "--inc --h --tml $clsfile include/generic\n\n";
}

sub thunks
{
   $allclean .= "thunks.tinc ";
   <<THUNKS;
thunks.tinc: $alltml
\tperl utils/tmlink.pl -Iinclude/generic -othunks.tinc $alltml

THUNKS
}

sub module_clsmake
{
   my ($module,$clsfile,$incdir,$name) = @_;
   unless (-d $incdir) {
      system "mkdirhier $incdir";
      die "cannot create necessary directory $incdir: $!"
	 unless -d $incdir;
   }
   print "Finding dependencies for $module ($clsfile)...";
   my $mk = "$incdir/$name.inc " . 
      "$incdir/$name.h: \\\n" . 
	 "$clsfile utils/gencls.pl ";
   $allclean .= "$incdir/$name.h ";
   $allclean .= "$incdir/$name.inc ";
   $known_header{"$name.h"} = "$incdir/$name.h";
   $known_header{"$name.inc"} = "$incdir/$name.inc";
   for (`perl utils/gencls.pl --depend $clsfile`) {
      next unless /^ancestor:\s+(.*)$/;
      $mk .= "include/generic/$1.h $1.cls ";
   }
   print "\n";
   $mk .= "\n\tperl utils/gencls.pl " .
      "--inc --h $clsfile $incdir\n\n";
}

sub module_cmake
{
   my ($module,$cfile,$dir,$all_inc,$objects) = @_;
   print "Finding dependencies for $module ($cfile)...";
   my $extra = '';
   $extra = '-I/usr/X11R6/include ' if $cfile =~ /^unix/;
   $extra .= $all_inc;
   $cfile =~ m!/([^/]*).c$!;
   my $name = $1;
   my $ofile = "$dir/$name.o";
   chomp(my $r = `@cc $extra -MM -MG $cfile`);
   for (`@cc $extra -MM $cfile 2>&1`) {
      next unless m{^.*:\d+:\s+(.*)\.(.*): No such file or directory$};
      if ($known_header{"$1.$2"}) {
         my $s = "$1.$2";
	 $r =~ s[\b$s\b][$known_header{$s}];
      } elsif ($2 eq 'h') {
	 my $s = $1;
	 $r =~ s[\b$s\.h\b][include/generic/$s\.h];
      } elsif ($2 eq 'inc') {
	 my $s = $1;
	 $r =~ s[\b$s\.inc\b][include/generic/$s\.inc];
      }
   }
   $r =~ s[^.*\.o\s*:][$ofile:];
   $$objects .= "$ofile ";
   $allclean .= "$ofile ";
   print "\n";
   $r .= "\n\t@cc $extra -c -o $ofile $cfile\n\n";
}

sub modules
{
   my @cls = ();
   my $mk = '';
   print "Getting list of external modules...";
   find( sub { push @cls, $File::Find::name if /\.cls$/; }, 'modules/');
   @cls = sort { length $a <=> length $b } @cls;
   # class name
   #   modules/IPA/Geometry/Geometry.cls => IPA::Geometry
   #   modules/IPA/IPA.cls => IPA
   # shared library name/location
   #   modules/IPA/Geometry/Geometry.cls => auto/IPA/Geometry/Geometry.so
   #   modules/IPA/IPA.cls => auto/IPA/IPA.so
   # dir
   #   modules/IPA/Geometry/Geometry.cls => modules/IPA/Geometry/
   #   modules/IPA/IPA.cls => modules/IPA/
   # incdir
   #   modules/IPA/Geometry/Geometry.cls => include/generic/IPA/Geometry/
   #   modules/IPA/IPA.cls => include/generic/IPA/
   my (@class,@share,@dir,@incdir,@name,@pmdir);
   for my $cls (@cls) {
      ($_ = $cls) =~ s!^modules/(.*)\.cls$!auto/${1}.so!;
      push @share, $_;
      $_ =~ s!/[^/]*$!!;
      unless (-d) {
	 system "mkdirhier $_";
	 die "cannot create necessary directory $_: $!"
	    unless -d;
      }
      ($_ = $cls) =~ s!^modules/(.*)/[^/]*\.cls$!include/generic/${1}!;
      push @incdir, $_;
      ($_ = $cls) =~ s!^modules/(.*)/[^/]*\.cls!$1!;
      s!/!::!g;
      push @class, $_;
      s!::!/!g; s!/?[^/]*$!!;
      push @pmdir, "scripts/modules/$_";
      unless (-d "scripts/modules/$_") {
	 system "mkdirhier scripts/modules/$_";
	 die "cannot create necessary directory scripts/modules/$_: $!"
            unless -d "scripts/modules/$_";
      }
      ($_ = $cls) =~ s!/[^/]*\.cls!!;
      push @dir, $_;
      $cls =~ m!/([^/]*)\.cls!;
      push @name, $1;
   }
   print "\n";
   my $all_incs = '';
   for (@incdir) { $all_incs .= "-I$_ "; }
   for (@dir) { $all_incs .= "-I$_ "; }
   my $all_modules = '';
   while (@cls) {
      $mk .= module_clsmake $class[0], $cls[0], $incdir[0], $name[0];
      my $objects = '';
      while (<$dir[0]/*.c>) {
	 $mk .= module_cmake $class[0], $_, $dir[0], $all_incs, \$objects;
      }
      while (<$dir[0]/*.pm>) {
	 m!([^/]*\.pm)$!;
	 my $n = $1;
	 $mk .= <<MAKE;
$pmdir[0]/$n: $dir[0]/$n
\tinstall -c -m 0644 $dir[0]/$n $pmdir[0]/$n

MAKE
         $allclean .= "$pmdir[0]/$n ";
         $all_modules .= "$pmdir[0]/$n ";
      }
      $allclean .= "$share[0] ";
      $all_modules .= "$share[0] ";
      $mk .= <<MAKE;
$share[0]: $objects auto/Prima/Prima.so
\t@ld -Lauto/Prima -shared -o $share[0] $objects $Config{libs} -lperl -lX11 -lgif

MAKE
      shift @class, shift @cls, shift @incdir, shift @name, shift @dir, shift @share, shift @pmdir;
   }
   $mk .= <<MAKE;
all_modules: $all_modules

MAKE
}

sub check_function
{
   my ($func) = shift;
   print "checking $func()...";
   my (@p,@i);
   while (@_) {
      push @p, shift;
      push @i, shift;
   }
   my $code = "int main( int argc, char **argv) {\n";
   my $var = "bugeaaa";
   my $call = "   $func( ";
   while (@p) {
      if ($p[0] =~ /^\s*char\s*\*\s*$/) {
	 $i[0] =~ s!\\!\\\\!g;
	 $i[0] =~ s!\"!\\\"!g;
	 $code .= "   $p[0] $var = \"$i[0]\";\n";
      } else {
	 if ($i[0] ne '') {
	    $code .= "   $p[0] $var = $i[0];\n";
	 } else {
	    $code .= "   $p[0] $var;\n";
	 }
      }
      shift @p, shift @i;
      if (@p) {
	 $call .= "$var, ";
      } else {
	 $call .= "$var);";
      }
      $var++;
   }
   $code .= "\n$call\n   return 0;\n}\n";
   my $name = "/tmp/pmplt$$";
   open C, "> $name.c" or die $!;
   print C $code;
   close C;
   my $r = system("$Config{cc} -o $name $name.c >/dev/null 2>&1") == 0;
   unlink  "$name.c", "$name";
   if ($r) {
      print " found!";
      push @all_defines, "-DPRIMA_" . uc($func) . "_PRESENT=1";
   } else {
      print " not found!";
   }
   print "\n";
}


check_function "stricmp", "char *" => '', "char *" => '';
check_function "strcasecmp", "char *" => '', "char *" => '';
check_function "trunc", "double" => 1.5;
push @cc, @all_defines;

my $make = <<MAKE;
# Makefile for Prima project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.

project: prima all_modules

MAKE

while (<*.cls>) {
  $make .= clsmake( $_);
}
while (<*.c>) {
  $make .= cmake( $_);
}
while (<unix/*.c>) {
  $make .= cmake( $_);
}

$make .= thunks;
$make .= modules;

$make .= <<MAKE;
clean:
\trm -f $allclean auto/Prima/Prima.so tags TAGS

.PHONY: tags

tags:
\t/usr/local/bin/ctags -ip *.c \`find include -name "*.h" -print\` \\
\t\t	/usr/X11R6/include/X11/X*.h unix/*.c \\
\t\t	/usr/X11R6/include/X11/keysymdef.h
\t/usr/local/bin/ctags -ip -e *.c \`find include -name "*.h" -print\` \\
\t\t	/usr/X11R6/include/X11/X*.h unix/*.c \\
\t\t	/usr/X11R6/include/X11/keysymdef.h

calltree:
.if !defined(func)
\t\@echo "The calltree target requires a function parameter,"
\t\@echo "e.g.: \\"make calltree func=apc_font_pick\\""
.else
\t\@cflow -X stdio.h -X ctypes.h -X errno.h -X stdlib.h -r "\${func}" @inc_path @all_defines *.c unix/*.c 2> /dev/null
.endif

mtree:
\t\@echo Making tree specification...
\t\@rm -f treespec.1 treespec
\t\@mtree -c >treespec.1
\t\@perl -pe 's/treespec\.1(.*)\$\$/treespec\$\$1 nochange/; s/(type=dir.*)\$\$/\$\$1 nochange/;' treespec.1 >treespec
\t\@rm -f treespec.1

comptree:
\t\@echo Tree differences:
\t\@mtree -e -f treespec | perl -ne 'print "\$\$1\\n" if /^(\\S.*):\\s*\$\$/;'

Makefile: Makefile.PL
\t\@echo Rebuilding Makefile...
\t\@perl Makefile.PL
\t\@make
\t\@echo You are safe to ignore the following error...
\t\@false

prima: Makefile auto/Prima/Prima.so

auto/Prima/Prima.so: $allo
\t@ld -shared -o auto/Prima/Prima.so $allo $Config{libs} -lperl -lX11 -lgif
MAKE

print "Writing Makefile...";
$make =~ tr!/!!s;
open MAKE, ">Makefile"
  or die "Cannot create Makefile: $!\n";
print MAKE $make;
close MAKE;
print "\nAll done.  Now you can run make.\n";
