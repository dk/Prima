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

my $extraccflags = '-g -O';  # gcc allows both
$extraccflags = '-g -Wall -Werror';

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


my @all_defines = ("-DPERL_PATCHLEVEL=$Config{patchlevel}",
		   "-DPERL_SUBVERSION=$Config{subversion}",
		   "-DPRIMA_CORE=1",
		   "-DPERL_POLLUTE=1");
if ( defined( $ENV{ 'PRIMA_PARANOID_MALLOC'}) && $ENV{ 'PRIMA_PARANOID_MALLOC'} =~ /^(1|yes|on|true)$/i) {
    push @all_defines, "-DPARANOID_MALLOC=1";
}

my @cc;
push @cc, $Config{cc}, $extraccflags,
   $Config{cccdlflags}, $Config{ccflags},
   @inc_path,
   @all_defines
  ;

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

my $make = <<MAKE;
# Makefile for Prima project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.

project: prima

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

$make .= <<MAKE;
clean:
\trm -f $allclean auto/Prima/Prima.so

tags:
\tetags include/*.h include/unix/*.h *.c unix/*.c

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
open MAKE, ">Makefile"
  or die "Cannot create Makefile: $!\n";
print MAKE $make;
close MAKE;
print "\nAll done.  Now you can run make.\n";
