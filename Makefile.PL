#! /usr/bin/perl -w
#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#

#
# Create Makefile for a Prima compilation
#
# This is not a ``normal'' Makefile.PL file.
# It does not use MakeMaker.
#
require 5.00502;
use strict;
use Config;
use File::Find;
use File::Basename;
use IO::File;
use Cwd;
use vars qw( %make_trans @ovvars $dir_sep $path_sep );
use vars @ovvars = qw(
    $CC $CFLAGS $CDLFLAGS $COUTOFLAG $COUTEXEFLAG $CINCPATHFLAG $CDEFFLAG $COMPONLYFLAG $CLIBPATHFLAG
    $LD $LDFLAGS $LDOUTFLAG $LDLIBPATHFLAG $LDLIBFLAG
    $OBJ_EXT $LIB_EXT $EXE_EXT $DL_EXT
    @INCPATH
    @LIBPATH
    @LIBS
    $PLATFORM
    $PREFIX
    $INSTALL_LIB
    $INSTALL_DL
    $TMPDIR
    $NULLDEV
    $MAKE
    $RM
    $SHQUOTE
    $MAKETYPE
);

use vars qw(
    $Win32
    %DEFINES
    @alltml
    @allclean
    @allojects
    %alldeps
    @allinstall
    @alldirs
    $PrimaTarget
);

sub print_config
{
    foreach my $var ( sort keys %Config) {
	print "$var='", $Config{ $var} || '', "'\n";
    }
}

sub qd
{
    my ( $path_str) = @_;
    $path_str =~ s[/][$dir_sep]g;
    return $path_str;
}

sub quotemake
{
    return join '', map { $make_trans{ $MAKETYPE}->[ ord]} split //, $_[ 0];
}

sub tempfile
{
    my $mask = shift;
    my $name;
    my $n = 0;
    do {
	$name = sprintf $mask, $n++;
    } while ( -e $name);
    return $name;
}

sub cc_command_line
{
    my ( $srcf, $objf, $exef, $compile_only, $dl) = @_;
    my @cc = ( $CC, split ' ', $CFLAGS);
    push @cc, split ' ', $CDLFLAGS if $dl;
    push @cc, $COMPONLYFLAG if $compile_only;
    push @cc, map( { "$CINCPATHFLAG$_"} @INCPATH);
    push @cc, map( { "$CDEFFLAG$_" . ( $DEFINES{ $_} ? "=$DEFINES{ $_}" : '')} keys %DEFINES);
    push @cc, ( $compile_only ? "$COUTOFLAG$objf" : "$COUTEXEFLAG$exef");
    push @cc, "$COUTOFLAG$objf" if $Win32 && ! $compile_only;
    push @cc, $srcf;
    unless ( $compile_only) {
        push @cc, map { "$LDLIBFLAG$_"} @LIBS;
        push @cc, map { "$CLIBPATHFLAG$_"} @LIBPATH;
    }
    return @cc;
}

sub ld_command_line
{
    my ( $dstf) = shift;
    my @ld = ( $LD, split( ' ', $LDFLAGS));
    push @ld, map { "$LDLIBPATHFLAG$_"} @LIBPATH;
    push @ld, "$LDOUTFLAG$dstf", @_;
    push @ld, map { "$LDLIBFLAG$_"} @LIBS;
    return @ld;
}

sub compile
{
    my ( $text, $compile_only) = @_;
    my $tmpsrc = qd( tempfile( "$TMPDIR/pmts%04d.c"));
    my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$OBJ_EXT"));
    my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$EXE_EXT"));
    open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
    print TMPSRC $text;
    close TMPSRC;
    open OLDSTDOUT, ">&STDOUT" or die "STDOUT dup failed: $!";
    open OLDSTDERR, ">&STDERR" or die "STDERR dup failed: $!";
#    $NULLDEV = "/dev/tty";
    open STDOUT, ">$NULLDEV" or die "STDOUT redirect failed: $!";
    open STDERR, ">&STDOUT" or die "STDERR redirect failed: $!";
    my $rc = system grep { ! /^-W/} cc_command_line( $tmpsrc, $tmpo, $tmpexe, $compile_only || 0);
    close STDOUT;
    close STDERR;
    open STDOUT, ">&OLDSTDOUT" or die "STDOUT restoration failed: $!";
    open STDERR, ">&OLDSTDERR" or die "STDERR restoration failed: $!";
    close OLDSTDOUT;
    close OLDSTDERR;
    unlink $tmpsrc;
    unlink $tmpo if -w $tmpo;
    unlink $tmpexe if -w $tmpexe;
    return( $rc == 0);
}

sub have_header
{
    my $header = shift;
    my $present = compile( <<ENDP, 1);
#include <$header>
ENDP
    if ( $present) {
	my $defname = "HAVE_" . uc $header;
	$defname =~ s/\W/_/g;
	$DEFINES{ $defname} = 1;
    }
    return $present;
}

sub find_header
{
    my $header = shift;
    my ( $incpath, $present);
    foreach $incpath ( @_) {
	local @INCPATH = @INCPATH;
	push @INCPATH, $incpath if $incpath;
	$present = compile( <<ENDP, 1);
#include <$header>
ENDP
	return $incpath if $present;
    }
    return undef;
}

sub find_lib
{
    my $lib = shift;
    my ( $libpath, $present);
    local @LIBS = @LIBS;
    push @LIBS, $lib;
    foreach $libpath ( @_) {
	local @LIBPATH = @LIBPATH;
	push @LIBPATH, $libpath if $libpath;
	$present = compile( <<ENDP);
void
main()
{
}
ENDP
	return $libpath if $present;
    }
    return undef;
}

sub have_func
{
    my ( $funcname) = @_;
    print "Checking for function $funcname... ";
    my $rc = compile( <<ENDP);
void
main()
{
    ${ funcname}();
}
ENDP
    if ( $rc) {
	my $defname = "HAVE_" . uc $funcname;
	$defname =~ s/\W/_/g;
	$DEFINES{ $defname} = 1;
	print "yes\n";
    }
    else {
	print "no\n";
    }
    return $rc;
}

sub setup_variables
{
    die "Oops! OS/2 isn't supported!\n" if $^O =~ /os2/i;
    if ( $Win32 = ( $^O =~ /mswin32/i)) { # XXX Correct for NT.
	$PLATFORM = 'win32';
    }
    else {
	$PLATFORM = 'unix';
    }
    $CC = $Config{ cc};
    $CFLAGS = $Config{ ccflags} . ( $Win32 ? " -W3 -WX" : " -Wall -Werror");
    $CDLFLAGS = "$Config{ cccdlflags} $Config{ ccdlflags}";
    $COUTOFLAG = $Win32 ? '-Fo' : '-o';
    $COUTEXEFLAG = $Win32 ? '-Fe' : '-o';
    $CINCPATHFLAG = '-I';
    $CDEFFLAG = '-D';
    $COMPONLYFLAG = '-c';
    $CLIBPATHFLAG = $Win32 ? '/link /LIBPATH:' : '-L';
    $LD = $Config{ ld};
    $LDFLAGS = "$Config{ ldflags} $Config{ lddlflags}";
    $LDOUTFLAG = $Win32 ? '/OUT:' : '-o';
    $LDLIBPATHFLAG = $Win32 ? '/LIBPATH:' : '-L';
    $LDLIBFLAG = $Win32 ? '' : '-l';
    $OBJ_EXT = $Config{ _o};
    $LIB_EXT = $Config{ _a};
    $EXE_EXT = $Config{ _exe};
    $DL_EXT = "." . $Config{ dlext};
    @INCPATH = (
		"include",
		qd( "include/generic"),
		$Config{installarchlib} . qd( "/CORE"),
	       );
    @LIBPATH = split ' ', $Config{ libpth};
    @LIBS = map { s/^$LDLIBFLAG//; $_} split ' ', $Config{ libs};
    %DEFINES = (
		PERL_PATCHLEVEL => $Config{ patchlevel},
		PERL_SUBVERSION => $Config{ subversion},
		PRIMA_CORE => 1,
		PERL_POLLUTE => 1,
	       );
    $DEFINES{ PARANOID_MALLOC} = 1
	if ( $ENV{ PRIMA_PARANOID_MALLOC}
	     && $ENV{ PRIMA_PARANOID_MALLOC} =~ /^1|yes|on|true$/i);
    while ( $CFLAGS =~ s/-D(\w+)(?:=(\S+))?\s*//) {
	my ( $defname, $defvalue) = ( $1, $2 || '');
	$DEFINES{ $defname} = $defvalue;
    }

    if ( defined $ENV{ PRIMA_DEVEL}) {
	$CFLAGS .= $Win32 ? " -Zi" : " -g -O";
	$LDFLAGS .= $Win32 ? "" : " -g";
    }
    else {
	$CFLAGS .= " " . ( $Config{ optimize} || '');
    }

    $PREFIX = $Config{ installsitearch};
    $INSTALL_LIB = $PREFIX . qd( "/Prima");
    $INSTALL_DL = $PREFIX . qd( "/auto/Prima");

    $TMPDIR = $ENV{ TMPDIR} || $ENV{ TEMPDIR} || ( $Win32 ? ( $ENV{ TEMP} || "$ENV{ SystemDrive}\\TEMP") : "/tmp");
    $NULLDEV = $Win32 ? "NUL" : "/dev/null";
    $MAKETYPE = ( $MAKE = $Config{ make}) =~ /^nmake$/i ? 'nmake' : 'make';
    $RM = $Config{ rm};
    $SHQUOTE = $Win32 ? '"' : "'";
    $PrimaTarget = qd( "auto/Prima/Prima$DL_EXT");
}

sub setup_paths
{
    unless ( $Win32) {
	push @INCPATH, ( qd( "/usr/include"), qd( "/usr/local/include"));
	my $incpath = find_header( qd( "X11/Xlib.h"), qd( "/usr/X11R6/include"), qd( "/usr/X11/include"));
	die "Prima needs X11 headers for compilation! Perhaps, you have non-standard path to X11?\n" unless defined $incpath;
	print "X11 includes found in $incpath\n";
	push @INCPATH, $incpath;
	my $libpath = find_lib( "X11", "/usr/X11R6/lib", "/usr/X11/lib");
	die "Prima needs X11 libraries for compilation! Perhaps, you have non-standard path to X11?\n" unless defined $libpath;
	print "X11 libraries found in $libpath\n";
	push @LIBPATH, $libpath;
	push @LIBS, 'X11';

	if ( $incpath = find_header( "gif_lib.h", "", map { "$_/giflib"} @INCPATH)) {
	    print "Found gif_lib.h\n";
	    push @INCPATH, $incpath;
	    $DEFINES{ HAVE_GIFLIB} = 1;
	    $libpath = find_lib( 'gif', '', '/usr/local/lib');
	    die "GifLib libraries not found!\n" unless defined $libpath;
	    push @LIBS, 'gif';
	    if ( $libpath) {
		print "Adding $libpath to LIBPATH.\n";
		push @LIBPATH, $libpath;
	    }
	}
    }
    else {
        my $libpath = find_lib( $Config{ libperl}, "", qd( "$Config{ archlib}/CORE"));
        die "Cannot find perl library" unless defined $libpath;
        print "Perl library found in ", ( $libpath || "default LIBPATH"), "\n";
        push @LIBPATH, $libpath if $libpath;
        push @LIBS, $Config{ libperl};

        $libpath = find_lib( "prigraph$LIB_EXT", "");
        die "Prima/Win32 requires GBM to work" unless defined $libpath;
        push @LIBS, "prigraph$LIB_EXT";
    }
}

sub setup_defines
{
    have_func( 'strcasecmp') || have_func( 'stricmp');
}

sub setup_misc
{
    # Setting up translation tables for quoting string for different make utilities.
    my ( $make_tbl, $nmake_tbl);
    $make_tbl = [ map { chr} 0..255];
    $nmake_tbl = [ map { chr} 0..255];
    foreach ( ':', ';', '#', '(', ')', '$', '^', '\\', '{', '}', '!', '@', '-') {
	$nmake_tbl->[ ord] = "^$_";
    }
    $make_tbl->[ ord '$'] = '$$';
    $make_tbl->[ ord '#'] = '\\#';
    %make_trans = (
		   make => $make_tbl,
		   nmake => $nmake_tbl,
		  );
}

sub process_commandline
{
    my %overrideable = map { /^(.)(.*)$/; $2 => $1} @ovvars; # Script variables which may be overridden.
    foreach my $arg ( @ARGV) {
	if ( $arg =~ /^\s*(\w+)\s*(\+?)\=(.*)$/) {
	    my ( $varname, $setmode, $varval) = ( $1, $2 || '', $3);
	    die "Unknown variable $varname" unless $overrideable{ $varname};
	    if ( $overrideable{ $varname} eq '$') {
		if ( $setmode eq '+') {
		    eval "\$$varname = \$$varname . \$varval";
		}
		else {
		    eval "\$$varname = \$varval";
		}
		die $@ if $@;
	    }
	    elsif ( $overrideable{ $varname} eq '@') {
		my @values = split /$path_sep/, $varval;
		if ( $setmode eq '+') {
		    eval "push \@$varname, \@values";
		}
		else {
		    eval "\@$varname = \@values";
		}
		die $@ if $@;
	    }
	}
	elsif ( $arg =~ /^-(D|U)(\w+)(?:(\+?)=(.*))?$/) {
	    my ( $defmode, $defname, $setmode, $value) = ( $1, $2, $3 || '', $4 || '');
	    if ( $defmode eq 'U') {
		delete $DEFINES{ $defname} if exists $DEFINES{ $defname};
	    }
	    else {
		if ( $setmode eq '+') {
		    $DEFINES{ $defname} = $DEFINES{ $defname} . $value;
		}
		else {
		    $DEFINES{ $defname} = $value;
		}
	    }
	}
	else {
	    die "Unknown command line argument or wrong syntax: '$arg'";
	}
    }
}

sub find_file
{
    my ( $fname, $dir) = @_;
    my $pathname = qd( "$dir/$fname");
    if ( -e $pathname) {
	return ( $pathname, 1);
    }
    opendir D, $dir or die "Cannot open dir $dir: $!";
    my @entries = map { qd( "$dir/$_")} grep { /^[^.]/ && -d qd( "$dir/$_")} readdir D;
    closedir D;
    my $found;
    foreach my $entry ( @entries) {
	( $pathname, $found) = find_file( $fname, $entry);
	return ( $pathname, 1) if $found;
    }
    return ( $fname, 0);
}

sub canon_name
{
    my ( $fname) = @_;
    my $qdirsep = quotemeta( $dir_sep);
    $fname =~ s{[^$qdirsep]+$qdirsep\.\.(?:$qdirsep|\Z)}{}
	while $fname =~ /(?:$qdirsep|\A)\.\.(?:$qdirsep|\Z)/;
    $fname =~ s{(?:(?<=$qdirsep)|(?<=\A))\.(?=$qdirsep|\Z)$qdirsep?}{}g;
    return $fname;
}

sub find_cdeps
{
    my ( $cfile, $deps, $included) = @_;

    $deps ||= {};
    $included ||= {};

    return () if exists $deps->{ $cfile};
    $deps->{ $cfile} = [];
    return @{ $alldeps{ $cfile}} if exists $alldeps{ $cfile};
    $alldeps{ $cfile} = [];
    return () unless -f $cfile;

    my $cf = new IO::File;
    open $cf, "<$cfile" or die "Cannot open $cfile: $!";
    while ( <$cf>) {
	chomp;
	next unless /^\s*\#\s*include\s+"([^\"]+)"/;
	my $incfile = $1;
	my $found = 0;
	( $incfile, $found) = find_file( $incfile, ".");
	$incfile = qd( "include/generic/$incfile") unless $found;
	$incfile = canon_name( $incfile);
	unless ( exists $included->{ $incfile}) {
	    push @{ $alldeps{ $cfile}}, $incfile;
	    push @{ $deps->{ $cfile}}, $incfile;
	    $included->{ $incfile} = 1;
	}
	my @subdeps = find_cdeps( $incfile, $deps, $included);
	push @{ $deps->{ $cfile}}, @subdeps;
	push @{ $alldeps{ $cfile}}, @subdeps;
    }
    close $cf;
    return @{ $deps->{ $cfile}};
}

sub cmake
{
    my ( $cfile) = @_;
    print "Finding dependencies for $cfile...\n";
    my $ofile = "$1$OBJ_EXT" if $cfile =~ /^(.*)\.c$/;
    die "Internal error: illegal c file"
	unless defined $ofile;
    push @allclean, qd( $ofile);
    push @allojects, qd( $ofile);
    my @deps = find_cdeps( $cfile);
    return(
	   "$ofile: $cfile @deps\n\t" .
	   join( " ", cc_command_line( $cfile, $ofile, "", 1, 1)) .
	   "\n\n"
	  );
}

sub clsmake
{
    my ( $clsfile) = @_;
    print "Finding dependencies for $clsfile...";
    my $classname = $1 if $clsfile =~ /^(.*)\.cls$/;
    die "Internal error: illegal cls file"
	unless defined $classname;
    my $mk = qd( "include/generic/$classname.inc " .
		 "include/generic/$classname.h " .
		 "include/generic/$classname.tml") . ": " .
		"$clsfile " . qd( "utils/gencls.pl ");
    push @alltml, qd( "include/generic/$classname.tml");
    push @allclean, (
		     qd( "include/generic/$classname.h"),
		     qd( "include/generic/$classname.inc"),
		     qd( "include/generic/$classname.tml")
		    );
    push @allinstall, qd( "include/generic/$classname.h"), $INSTALL_LIB . qd( "/CORE/generic");
    for (`perl utils/gencls.pl --depend $clsfile`) {
	next unless /^ancestor:\s+(.*)$/;
	$mk .= qd( "include/generic/$1.h $1.cls ");
    }
    print "\n";
    $mk .= "\n\tperl utils/gencls.pl " .
	"--inc --h --tml $clsfile include/generic\n\n";
}

#
# Here we go...
#

$dir_sep = ( $path_sep = $Config{ path_sep}) eq ':' ? '/' : '\\';

die "This script should be launched from Prima root directory\n"
    unless -r qd( "include/apricot.h") && -d "Prima";

$| = 1;

print "Setting up working environment.\n";
setup_variables;
setup_paths;
setup_defines;
setup_misc;
process_commandline;

push @alldirs, qd( "include/generic"), qd( "auto/Prima");

my $make = <<MAKE;
# Makefile for Prima project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.

all: prima

MAKE

while (<*.cls>) {
  $make .= clsmake( $_);
}
while (<*.c>) {
  $make .= cmake( $_);
}
while (<$PLATFORM/*.c>) {
  $make .= cmake( $_);
}

my $thunks_tinc = qd( "include/generic/thunks.tinc");
push @allclean, $thunks_tinc;

print "Writing Makefile...";
$make =~ tr!/!!s;
open MAKE, ">Makefile" or die "Creation of Makefile failed: $!";
print MAKE $make;

my $CPSCRIPT = quotemake( 'die qq(Even number of parameters expected) unless ( $#ARGV % 2); while ( scalar @ARGV) { ( $src, $dst) = ( shift @ARGV, shift @ARGV); print qq(Installing $src -> $dst\\n); next unless -f $src; die qq(Destination must be a directory) if -e $dst && ! -d $dst; mkpath $dst unless -d $dst; copy( $src, $dst) or die qq(Copy failed: $!); }');
my $RMSCRIPT = quotemake( 'unlink @ARGV');
print MAKE <<MAKE;
$thunks_tinc: @alltml
\tperl utils/tmlink.pl -Iinclude/generic -o$thunks_tinc @alltml

RM=$^X -e ${ SHQUOTE}${ RMSCRIPT}${ SHQUOTE}

clean:
\t\$(RM) @allclean $PrimaTarget

prima: Makefile dirs $PrimaTarget

$PrimaTarget: @allojects
\t${ \eval{ return join ' ', ld_command_line( $PrimaTarget, @allojects)}}

Makefile: Makefile.PL
\t\@echo Cleaning up...
\t\@$MAKE clean
\t\@echo Rebuilding Makefile...
\t\@$^X Makefile.PL
\t\@$MAKE
\t\@echo You are safe to ignore the following error...
\t\@false

CP=$^X -MFile::Copy -MFile::Path -MFile::Basename -e ${ SHQUOTE}${ CPSCRIPT}${ SHQUOTE}

install: all
\t\@\$(CP) \\
\t$PrimaTarget $INSTALL_DL \\
MAKE

finddepth( sub { print MAKE "\t", qd( "$File::Find::dir/$_ $PREFIX/$File::Find::dir"), " \\\n" unless -d $_}, 'Prima');
while ( scalar @allinstall) {
    my ( $src, $dst) = ( shift @allinstall, shift @allinstall);
    print MAKE "\t$src $dst \\\n";
}
while ( <include/*.h>) {
    my ( $dsth) = $_;
    $dsth =~ s/^include/CORE/;
    print MAKE "\t$_ ", dirname( qd( "$INSTALL_LIB/$dsth")), " \\\n";
}
while ( <include/$PLATFORM/*.h>) {
    my ( $dsth) = $_;
    $dsth =~ s/^include/CORE/;
    print MAKE "\t$_ ", dirname( qd( "$INSTALL_LIB/$dsth")), " \\\n";
}
print MAKE "\tPrima.pm $PREFIX\n";

print MAKE <<MAKE;

MD=perl -MFile::Path -e ${ SHQUOTE}mkpath [ \@ARGV]${SHQUOTE}

dirs: @alldirs

@alldirs:
\t\@echo Creating directories...
\t\@\$(MD) @alldirs

MAKE

close MAKE;
print "\nAll done.  Now you can run ${ MAKE}.\n";
