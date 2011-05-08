#! /usr/bin/perl -w
#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
#  $Id$
#

BEGIN {
	unshift @INC, '.';
};

my $END; 
END {
	print $END if defined $END;
};

require 5.00502;
use strict;
use Config;
my %Config = %Config::Config; # original %Config is read-only
use File::Find;
use File::Basename;
use File::Path;
use File::Copy;
use Cwd;
use DynaLoader;
use Prima::Gencls;
use ExtUtils::Packlist;
use vars qw( %make_trans @ovvars @path_expand_ovvars $dir_sep $path_sep @no_compat );
use vars @ovvars = qw(
	$CC $CFLAGS $CCFLAGS $CCCDLFLAGS $CDLFLAGS $C_FLAGS $CDEBUGFLAGS $CDLFLAGS $COUTOFLAG $COUTEXEFLAG $CINCPATHFLAG
	$CDEFFLAG $COMPONLYFLAG $CLIBPATHFLAG $CLINKPREFIX
	$LD $LDFLAGS $LD_FLAGS $LDOUTFLAG $LDLIBPATHFLAG $LDLIBFLAG
	$LIB $LIBOFLAG
	$OBJ_EXT $LIB_EXT $LIB_PREFIX $EXE_EXT $DL_EXT $SCRIPT_EXT
	@INCPATH
	@LIBPATH
	$LIBS
	$INC
	$PLATFORM
	$COMPILER
	$PREFIX
	$X11BASE
	$INSTALLSCRIPT
	$INSTALLSITEARCH
	$INSTALL_BASE
	$INSTALL_LIB
	$INSTALL_DL
	$INSTALL_EXAMPLES
	$INSTALL_MAN3
	$INSTALL_MAN1
	$TMPDIR
	$NULLDEV
	$MAKE
	$RM
	$SHQUOTE
	$MAKETYPE
	$DEFFILE
	$DISTNAME
	$DL_LOAD_FLAGS
	$DEBUG
	$WITH_XFT
	$WITH_ICONV
	$WITH_GTK2
	$AUTOMATED_RUN
	$CYGWIN_X11
);

use vars @path_expand_ovvars = qw(
	@INCPATH
	@LIBPATH
	$INC
	$LIBS
	$COMPILER
	$PREFIX
	$X11BASE
	$INSTALLSCRIPT
	$INSTALLSITEARCH
	$INSTALL_BASE
	$INSTALL_LIB
	$INSTALL_DL
	$INSTALL_EXAMPLES
	$INSTALL_MAN3
	$INSTALL_MAN1
	$TMPDIR
	$MAKE
	$RM
	$DEFFILE
);

@no_compat = qw(
	 SITELIBEXP     INSTALLDIRS
         PERLPREFIX     SITEPREFIX      VENDORPREFIX
         INST_ARCHLIB                   INSTALLARCHLIB  INSTALLVENDORARCH
         INST_LIB       INSTALLPRIVLIB  INSTALLSITELIB      INSTALLVENDORLIB
         INST_BIN       INSTALLBIN      INSTALLSITEBIN      INSTALLVENDORBIN
         INST_SCRIPT                    INSTALLSITESCRIPT   INSTALLVENDORSCRIPT
         INST_MAN1DIR   INSTALLMAN1DIR  INSTALLSITEMAN1DIR  INSTALLVENDORMAN1DIR
         INST_MAN3DIR   INSTALLMAN3DIR  INSTALLSITEMAN3DIR  INSTALLVENDORMAN3DIR
);

use vars qw(
	$CWD
	$Win32
	$Win64
	$cygwin
	$NeedX11
	$OS2
	$OS2DLLF
	$useGC
	%cc_specs
	@LIBS
	%DEFINES
	%PASSIVE_CODECS
	@ACTIVE_CODECS
	%USER_VARS
	%USER_VARS_ADDONS
	%USER_VARS_LINKS
	%USER_DEFINES
	%USER_DEFINES_ADDONS
	%overrideable
	$ARGV_STR
	@alltml
	@alltmldeps
	@allclean
	@allrealclean
	@allojects
	%alldeps
	@allinstall
	@allbins
	@alldirs
	@allman
	@headers
	@footers
	@Makefile_deps
	$PrimaLib
	$PrimaTarget
	@Prima_exports
	$extralibs
	@executables
	$install_manuals
	$packlist
	%cache_find_files
	$binary_prereq
	$win32_use_dlltool
	$compiler_version
);

my $see_makefile_log = "(see also makefile.log for details)";

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

sub qtilde
{
	my $path = shift;
	return $path unless $path =~ s/^~//;
	die "** path '~$path' begins with '~' but no HOME is set\n" unless exists $ENV{HOME};
	return $ENV{HOME} . $path;
}

sub quoted_split
{
	my @neww = ();
	my $text = shift;
	while ($text =~ m<
		"((?:[^\"\\]*\\.[^\"\\]*)*)"\s*
	| (\S+)\s*
	| \s+
	>gx) {
		if ( defined $1) {
			push @neww, $1;
		} elsif ( defined $2) {
			push @neww, $2;
		}
	}
	return @neww;
}

sub fatal
{
	open ERRLOG, ">". qd "$CWD/makefile.log";
	print ERRLOG @_;
	close ERRLOG;
	die @_;
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
	my @cc = ( $CC, split(' ', $CFLAGS), split(' ', $CCFLAGS));
	push @cc, split ' ', $CCCDLFLAGS if $dl;
	push @cc, split ' ', $CDLFLAGS if $dl && !$compile_only;
	push @cc, $COMPONLYFLAG if $compile_only;
	push @cc, map( { "$CINCPATHFLAG$_"} @INCPATH);
#	for ( keys %DEFINES) {
#		next unless $DEFINES{$_};
#		push @cc, "$CDEFFLAG$_=$DEFINES{$_}";
#	}
	push @cc, "${ CDEFFLAG}HAVE_CONFIG_H=1";
	push @cc, ( $compile_only ? "$COUTOFLAG$objf" : "$COUTEXEFLAG$exef");
	push @cc, "$COUTOFLAG$objf" if $COMPILER =~ /^(msvc|bcc32)$/ && !$compile_only;
	push @cc, map { "$CLIBPATHFLAG$_"} @LIBPATH unless $compile_only || ( $COMPILER eq 'msvc');
	push @cc, $srcf;
	return @cc if $compile_only;
	push @cc, $CLINKPREFIX;
	push @cc, map { "\"$CLIBPATHFLAG\\\"$_\\\"\"" } @LIBPATH if $COMPILER eq 'msvc';
	push @cc, map { "$LDLIBFLAG$_"} @LIBS;
	return @cc;
}

sub ld_command_line
{
	my ( $dstf) = shift;
	my @ld = ( $LD, split( ' ', $LDFLAGS));
	if ( $COMPILER eq 'bcc32') {
		push @ld, '-L"' . join( ';', @LIBPATH) . '" c0d32.obj ';
		push @ld, @_;
		push @ld, ",", $dstf, ", ,", @LIBS, ",", "win32\\Prima.def,";
	}
	else {
		push @ld, map { 
			m/\s/ ? 
				"$SHQUOTE$LDLIBPATHFLAG$_$SHQUOTE" :
				"$LDLIBPATHFLAG$_"
		} @LIBPATH;
		push @ld, map { "-Wl,-R$_" } @LIBPATH if $^O =~ /netbsd/;
		push @ld, "$LDOUTFLAG$dstf", @_;
		push @ld, map { "$LDLIBFLAG$_"} @LIBS;
		push @ld, "os2\\Prima.def" if $OS2;
		if ( $Win32) {
			push @ld, "/def:win32\\Prima.def" if $COMPILER eq 'msvc';
			push @ld, "win32/Prima.def" if $COMPILER eq 'gcc';
		}
	}
	return @ld;
}

sub null_output
{
	open OLDSTDOUT, ">&STDOUT" or die "STDOUT dup failed: $!";
	open OLDSTDERR, ">&STDERR" or die "STDERR dup failed: $!";
#	$NULLDEV = ( $Win32 || $OS2) ? "CON" : "/dev/tty";
#	$NULLDEV = ( $Win32 || $OS2) ? "NUL" : "/dev/null";
	if ( $^O !~ /linux/) {
		close STDOUT;
		close STDERR;
	}
	open STDOUT, ">>$NULLDEV" or fatal "STDOUT redirect failed: $!";
	open STDERR, ">&STDOUT" or fatal "STDERR redirect failed: $!";
}

sub restore_output
{
	if ( $^O !~ /linux/) {
		close STDOUT;
		close STDERR;
	}
	open STDOUT, ">&OLDSTDOUT" or fatal "STDOUT restoration failed: $!";
	open STDERR, ">&OLDSTDERR" or fatal "STDERR restoration failed: $!";
	close OLDSTDOUT;
	close OLDSTDERR;
}

sub compile
{
	my ( $text, $compile_only, @extra) = @_;
	my $tmpsrc = qd( tempfile( "$TMPDIR/pmts%04d.c"));
	my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$OBJ_EXT"));
	my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$EXE_EXT"));
	my @tmpextras = ( $tmpsrc, $tmpsrc);
	$tmpextras[0] =~ s/\.[^\.+]$/.ilk/;
	$tmpextras[1] =~ s/\.[^\.+]$/.pdb/;

	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed $see_makefile_log";
	print TMPSRC $text;
	close TMPSRC;

	null_output;
	my @cc = grep !/^-W(all|error|\d)/i, cc_command_line( $tmpsrc, $tmpo, $tmpexe, $compile_only || 0);
	push @cc, @extra;
	print STDERR "@cc\n";
	my $rc = system("@cc");
	restore_output;
	unlink $tmpsrc;
	unlink $tmpo if -w $tmpo;
	unlink $tmpexe if -w $tmpexe;
	unlink $_ for @tmpextras;
	return( $rc == 0);
}

sub compile_and_run
{
	my ( $text) = @_;
	my $tmpsrc = qd( tempfile( "$TMPDIR/pmts%04d.c"));
	my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$OBJ_EXT"));
	my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$EXE_EXT"));
	my @tmpextras = ( $tmpsrc, $tmpsrc);
	$tmpextras[0] =~ s/\.[^\.+]$/.ilk/;
	$tmpextras[1] =~ s/\.[^\.+]$/.pdb/;

	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed $see_makefile_log";
	print TMPSRC $text;
	close TMPSRC;

	null_output;
	my @cc = grep !/^-W(all|error|\d)/i, cc_command_line( $tmpsrc, $tmpo, $tmpexe, 0);
	print STDERR "@cc\n";
	my $rc = system("@cc");
	restore_output;
	unlink $tmpsrc;
	unlink $tmpo if -w $tmpo;
	my $ret = `$tmpexe`;
	chomp $ret;
	unlink $tmpexe if -w $tmpexe;
	unlink $_ for @tmpextras;
	return $ret;
}

sub have_header
{
	my $header = shift;
	(my $defname = "HAVE_" . uc $header) =~ s/\W/_/g;
	return $DEFINES{$defname} if exists $DEFINES{$defname};
	my @pre_headers = map { "#include <$_>\n" } @_;
	print "Checking for presence of $header... ";
	my $present = compile( <<EOF, 1);
@pre_headers
#include <$header>
EOF
	$DEFINES{ $defname} = undef;
	$DEFINES{ $defname} = 1 if $present;
	print( $present ? "yes" : "no", "\n");
	return $present;
}

sub find_header
{
	my $header = shift;
	my $options = ref($_[0]) eq 'HASH' ? shift : {};
	my ( $incpath, $present);
	foreach $incpath ( @_) {
		local @INCPATH = @INCPATH;
		push @INCPATH, $incpath if $incpath;
		my $code = $options->{Code} || <<EOF;
#include <$header>
EOF
		$present = compile( $code, 1);
		return $incpath if $present;
	}
	return undef;
}

sub find_lib
{
	my ( $lib, $inc) = ( shift, shift );
	my ( $libpath, $present);

	if ( $OS2 && !length $inc) { # link386 is broken - warns, not dies if no library found
		if ( -f $lib) {
			$lib =~ s/(\\|\/[^\\|\/]*)$//g;
			return $lib;
		}
		for ( @LIBPATH) {
			return $_ if -f "$_/$lib";
			return $_ if -f "$_/$lib.lib";
			return $_ if -f "$_/$lib.a";
		}
		return undef;
	}
	
	local @LIBS = @LIBS;
	push @LIBS, $lib;
	foreach $libpath ( @_) {
		local @LIBPATH = (@LIBPATH, $libpath) if $libpath;
		$present = compile( <<EOF);
$inc

int
main()
{
   return 0;
}
EOF
		return $libpath if $present;
	}
	return undef;
}

sub have_func
{
	my ( $funcname, @headers) = @_;
	die "have_func() without any header is deprecated $see_makefile_log" unless @headers;
	my $defname = "HAVE_" . uc $funcname;
	my @srchead = map { "#include <$_>\n"} @headers;
	$defname =~ s/\W/_/g;
	print "Checking for function $funcname... ";
	my $rc = compile( <<EOF);
@srchead

int
main()
{
    void * ixi = $funcname;
    return 0;
}
EOF
	if ( $rc) {
		$DEFINES{ $defname} = 1;
		print "yes\n";
	} else {
		$DEFINES{ $defname} = undef;
		print "no\n";
	}
	return $rc;
}

sub have_define
{
	my ( $defname) = @_;
	my $cwd = cwd;
	chdir $TMPDIR;
	my $tmpsrc = qd( tempfile( "pmts%04d.c"));
	my $tmpo = $tmpsrc;
	if ( $COMPILER eq 'msvc') {
		$tmpo =~ s/\.c$/\.obj/;
	} else {
		$tmpo =~ s/\.c$/\.o/;
	}
	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed $see_makefile_log";
	print TMPSRC <<EOF;
int
main()
{
#if defined( $defname)
return 0;
#else
0error No
#endif
}
EOF
	close TMPSRC;
	null_output;
	my $ccrc = system( "$CC -c $tmpsrc");
	restore_output;
	unlink $tmpsrc, $tmpo;
	chdir $cwd;
	return $ccrc == 0;
}

sub have_type
{
	my ( $type, @headers) = @_;
	(my $defname = "HAVE_" . uc $type) =~ s/\W/_/g;
	return 1 if $DEFINES{$defname};
	print "Checking for presence of type ${ type}... ";
	my @srchead = map { "#include <$_>\n"} @headers;
	my $rc = compile( <<EOF);
@srchead

int
main()
{
	${ type} foo;
	return 0;
}
EOF
	if ( $rc) {
		$DEFINES{ $defname} = 1;
		print "yes\n";
	}
	else {
		$DEFINES{ $defname} = undef;
		print "no\n";
	}
	return $rc;
}

sub have_types_in
{
	my ($hdr, @types) = @_;
	return unless have_header( $hdr);
	my $found = 1;
	for my $type (@types) {
		$found = $found && have_type($type, $hdr);
		last unless $found;
	}
	return $found;
}

sub have_funcs_in
{
	my ($hdr, @funcs) = @_;
	return unless have_header( $hdr);
	my $found = 1;
	for my $func (@funcs) {
		$found = $found && have_func( $func, $hdr);
		last unless $found;
	}
	return $found;
}


sub find_inline
{
	print "Checking for inline... ";
	for ( qw( inline __inline inline__ __inline__
				INLINE __INLINE INLINE__ __INLINE__)) {
		my $i = $_;    
		my $rc = compile( <<EOF);

$_ void a( void) {};

int
main()
{
	return 0;
}
EOF
		if ( $rc) {
			$DEFINES{__INLINE__} = $i;
			print "$i\n";
			return;
		}
	}
	print "none found\n";
	$DEFINES{__INLINE__} = 'static'; # nasty hack, but better than #pragma inline
}

sub setvar
{
	my ( $varname) = shift;
	die "Tried to set non-overrideable variable $varname" unless $overrideable{ $varname};
	if ( $overrideable{ $varname} eq '$') {
		if ( defined $USER_VARS{ $varname}) {
			eval "\$$varname = \$USER_VARS{ \$varname}";
		}
		elsif ( defined $USER_VARS_LINKS{ $varname}
			&& exists $USER_VARS{ $USER_VARS_LINKS{ $varname}->{ src}}) {
			eval "\$$varname = sprintf \"$USER_VARS_LINKS{ $varname}->{ format}\", \$$USER_VARS_LINKS{ $varname}->{ src}";
		}
		else {
			eval "\$$varname = join( '', \@_)";
		}
		die $@ if $@;
		if ( defined $USER_VARS_ADDONS{ $varname}) {
			eval "\$$varname .= join( '', \@{ \$USER_VARS_ADDONS{ \$varname}})";
		}
		die $@ if $@;
	}
	elsif ( $overrideable{ $varname} eq '@') {
		if ( defined $USER_VARS{ $varname}) {
			eval "\@$varname = \@{\$USER_VARS{ \$varname}}";
		}
		else {
			eval "\@$varname = \@_";
		}
		die $@ if $@;
		if ( defined $USER_VARS_ADDONS{ $varname}) {
				eval "push \@$varname, \@{ \$USER_VARS_ADDONS{ \$varname}}";
		}
		die $@ if $@;
	}
	else {
		die "Unsupported type of variable";
	}
}

sub env_true
{
	my ( $var) = @_;
	return( $ENV{ $var} && $ENV{ $var} =~ /^1|yes|on|true$/i);
}

sub suck_symbols
{
	my $fn = shift;
	open F, $fn or die "Cannot open $fn:$!\n";
	local $/;
	my $x = <F>;
	close F;
	return ( $x =~ m/\bextern\s+\w+(?:\s*\*\s*)?\s+(\w+)\s*\(.*?;/gs );
}

sub setup_variables
{
	setvar( 'CYGWIN_X11', 0);
	$Win32 = 0 if $CYGWIN_X11;

	my $platform;
	$NeedX11 = 0;
	if ( $Win32) {              
		$platform = 'win32';
	}
	elsif ( $OS2) {
		$platform = 'os2';
	}
	else {
		$platform = 'unix';
		$NeedX11 = 1;
	}
	setvar( 'PLATFORM', $platform);

	%cc_specs = (
		cc => {
			warnflags => "",
			cflags => "",
			coutoflag => '-o ',
			coutexeflag => '-o ',
			cdebugflags => '-g -O',
			clinkprefix => '',
			clibpathflag => '-L',
			ldflags => '',
			ldoutflag => '-o ',
			ldlibpathflag => '-L',
			ldlibflag => '-l',
			ldlibext  => '',
			lddebugflags => '-g',
			name => "CC",
			lib  => '',
			liboflag => '',
		},
		gcc => {
			warnflags => "",
			cflags => "",
			coutoflag => '-o ',
			coutexeflag => '-o ',
			cdebugflags => '-g -O -Wall',
			clinkprefix => '',
			clibpathflag => '-L',
			ldflags => '',
			ldoutflag => '-o ',
			ldlibpathflag => '-L',
			ldlibflag => '-l',
			ldlibext  => '',
			lddebugflags => '-g',
			name => "GNU",
			lib  => '',
			liboflag => '',
		},
		hpcc => {
			warnflags => "", #"+w1",
			cflags => "-Ae",
			coutoflag => '-o ',
			coutexeflag => '-o ',
			cdebugflags => '-g +O2 +Onolimit',
			clinkprefix => '',
			clibpathflag => '-L',
			ldflags => '',
			ldoutflag => '-o ',
			ldlibpathflag => '-L',
			ldlibflag => '-l',
			ldlibext  => '',
			lddebugflags => '-g',
			name => "HP-UX C-ANSI-C",
			lib  => '',
			liboflag => '',
		},
		irixcc  => {
			warnflags => "-fullwarn",
			cflags => "-diag_error 1035 -woff 1011,1042,1048,1140,1164,1174,1209,1506,1515,1552",
			coutoflag => '-o',
			coutexeflag => '-o',
			cdebugflags => '-g -O0',
			clinkprefix => '',
			clibpathflag => '-L',
			ldflags => '',
			ldoutflag => '-o',
			ldlibpathflag => '-L',
			ldlibflag => '-l',
			ldlibext  => '',
			lddebugflags => '-g',
			name => "IRIX Native",
			lib  => '',
			liboflag => '',
		},
		emx => {
			warnflags => "-Wall",
			cflags => "",
			coutoflag => '-o',
			coutexeflag => '-o',
			cdebugflags => '-g -O',
			clinkprefix => '',
			clibpathflag => '-L',
			ldflags => '',
			ldoutflag => '-o',
			ldlibpathflag => '-L',
			ldlibflag => '-l',
			ldlibext  => '',
			lddebugflags => '-g',
			lib  => 'emxomf',
			liboflag => '-o',
			name => "EMX",
		},
		msvc => {
			warnflags => "-W3",
			cflags => "-nologo",
			coutoflag => '-Fo',
			coutexeflag => '-Fe',
			clinkprefix => '/link',
			clibpathflag => '/LIBPATH:',
			cdebugflags => '-Zi',
			ldflags => '',
			ldoutflag => '/OUT:',
			ldlibpathflag => '/LIBPATH:',
			ldlibflag => '',
			ldlibext  => '.lib',
			lddebugflags => '/DEBUG',
			name => "Microsoft Visual C++",
			lib  => 'lib',
			liboflag => '-out:',
		},
		bcc32 => {
			warnflags => "-w0", # Borland is the only compiler which doesn't allow of geting rid of warnings.
			cflags => "-tWM",
			coutoflag => '-o',
			coutexeflag => '-e',
			clinkprefix => '',
			clibpathflag => '-L',
			cdebugflags => '-v -y',
			ldflags => '',
			ldoutflag => '',
			ldlibpathflag => '-L',
			ldlibflag => '',
			ldlibext  => '',
			lddebugflags => '-v',
			lib  => '',
			liboflag => '',
			name => "Borland C++",
		},
	);

	setvar( 'TMPDIR', $ENV{ TMPDIR} || $ENV{ TEMPDIR} || ( $Win32 ? ( $ENV{ TEMP} || "$ENV{ SystemDrive}\\TEMP") : "/tmp"));
	setvar( 'NULLDEV', "$CWD/makefile.log");
	setvar( 'DL_LOAD_FLAGS', ( $Win32 || $OS2) ? 0 : -1);
	setvar( 'SHQUOTE', $Win32 ? '"' : "'");

	setvar( 'CC', $Config{ cc});
	print "Determining compiler type... ";

	# setting preliminary $COMPILER so have_define() can run
	if ( $Win32) {
		$COMPILER = ( $CC =~ /cl(\.exe)?$/i ) ? 'msvc' : $CC;
	} else {
		$COMPILER = $CC;
	}

	if ( defined $USER_VARS{ COMPILER}) {
		$COMPILER = $USER_VARS{ COMPILER};
		die "Compiler type $COMPILER is unknown" unless defined $cc_specs{ $COMPILER};
	}
	elsif ( $^O =~ /irix/ && $CC =~ /^(\S*\/)?cc\b/) {
		# this code should be put first, as cc call does not treat #error as error,
		# and have_define() is therefore always true
		$COMPILER = 'irixcc';
	}
	elsif ( $^O eq "hpux" && $CC =~ /^(\S*\/)?cc\b/) {
		# this code should be put first, as cc call does not treat #error as error,
		# and have_define() is therefore always true
		$COMPILER = 'hpcc'; 
	} 
	elsif ( have_define( "__EMX__")) {
		$COMPILER = 'emx';
	}
	elsif ( have_define( "__GNUC__")) {
		$COMPILER = 'gcc';
	}
	elsif ( have_define( "__BORLANDC__")) {
		$COMPILER = 'bcc32';
	}
	elsif ( have_define( "_MSC_VER")) {
		$COMPILER = 'msvc';
	} elsif ( $CC =~ /^(\S*\/)?cc\b/ ) {
		$COMPILER = 'cc';
	} else {
		print "($CC is unknown, assuming COMPILER=cc) ";
		$COMPILER = 'cc';
	}
	print "$cc_specs{ $COMPILER}->{ name}\n";
	$win32_use_dlltool = $Win32 && ( $COMPILER eq 'gcc') && !$cygwin;

	# 5.8.0 hack
	$Config{PATCHLEVEL} = $Config{PERL_PATCHLEVEL} if !defined $Config{PATCHLEVEL} || !length $Config{PATCHLEVEL};
	$Config{SUBVERSION} = $Config{PERL_SUBVERSION} if !defined $Config{SUBVERSION} || !length $Config{SUBVERSION};
	# Checking some Config.pm - were empty for ActiveState 626
	$Config{PATCHLEVEL} = $Config{patchlevel} if !defined $Config{PATCHLEVEL} || !length $Config{PATCHLEVEL};
	$Config{SUBVERSION} = $Config{subversion} if !defined $Config{SUBVERSION} || !length $Config{SUBVERSION};
	$Config{ld} = (($COMPILER eq 'msvc') ? 'link' : 'ld'), print("** warning: malformed Config.pm\n")
		unless length $Config{ld};

	my $ccflags   = $Config{ ccflags};
	my $warnflags = $cc_specs{ $COMPILER}->{ warnflags};
	# my $ldflags = $COMPILER eq 'emx' ? "" : "$Config{ ldflags} ";
	my $ldflags = "";
	my $lddlflags = $Config{ lddlflags};
	setvar( 'DEBUG', defined($ENV{ PRIMA_DEVEL}) ? $ENV{ PRIMA_DEVEL} : 0);
	if ( $COMPILER eq 'msvc' && (( $ccflags =~ /TP/) || ($ccflags =~ /O\d/))) { # M$VC++ protection
		$warnflags =~ s/W3/W1/;        # lower warning level
		$ccflags   =~ s/\-W3/\-W1/;        # lower warning level
		$warnflags =~ s/(-|\/)WX//;    # and dismiss 'warnings as errors'; ( for its stupidity in the c++ case)
		$ldflags =~ s/(-|\/)nodefaultlib//g;
		$lddlflags =~ s/(-|\/)nodefaultlib//g;
		$ccflags =~ s/(-|\/)O\d//g if $DEBUG; # remove optimization
	}
	setvar( 'C_FLAGS',  "$ccflags $warnflags $cc_specs{ $COMPILER}->{ cflags} " . ( $Config{ optimize} || '') . " ");
	setvar( 'CFLAGS',  "$ccflags $warnflags $cc_specs{ $COMPILER}->{ cflags}" . ( $DEBUG ? " $cc_specs{ $COMPILER}->{ cdebugflags}" : " " . ( $Config{ optimize} || '')) . " ");
	setvar( 'CCFLAGS',  '');
	if ( $COMPILER eq 'gcc' && $Config{ cccdlflags} =~ /-K\S/) {
		# remove known proprietary compiler junk flags
		$Config{ cccdlflags} =~ s/-K\S*//;
	}
	setvar( 'CCCDLFLAGS', $Config{ cccdlflags});
	setvar( 'CDLFLAGS', $Config{ ccdlflags});
	setvar( 'COUTOFLAG', $cc_specs{ $COMPILER}->{ coutoflag});
	setvar( 'COUTEXEFLAG', $cc_specs{ $COMPILER}->{ coutexeflag});
	setvar( 'CINCPATHFLAG', '-I');
	setvar( 'CDEFFLAG', '-D');
	setvar( 'COMPONLYFLAG', '-c');
	setvar( 'CLINKPREFIX', $cc_specs{ $COMPILER}->{ clinkprefix});
	setvar( 'CLIBPATHFLAG', $cc_specs{ $COMPILER}->{ clibpathflag});
	setvar( 'LD', $Config{ ld});
	setvar( 'LD_FLAGS', "$ldflags $lddlflags $cc_specs{ $COMPILER}->{ ldflags} ");
	setvar( 'LDFLAGS', "$ldflags $lddlflags $cc_specs{ $COMPILER}->{ ldflags}" . ( $DEBUG ? " $cc_specs{ $COMPILER}->{ lddebugflags}" : "") . " ");
	setvar( 'LDOUTFLAG', $cc_specs{ $COMPILER}->{ ldoutflag});
	setvar( 'LDLIBPATHFLAG', $cc_specs{ $COMPILER}->{ ldlibpathflag});
	setvar( 'LDLIBFLAG', $cc_specs{ $COMPILER}->{ ldlibflag});
	setvar( 'OBJ_EXT', $Config{ _o});
	setvar( 'LIB_EXT', ( $cygwin ? '.dll' : '') . $Config{ _a});
	setvar( 'LIB_PREFIX', $cygwin ? 'lib' : '');
	setvar( 'EXE_EXT', $Config{ _exe});
	setvar( 'LIB',     $cc_specs{ $COMPILER}->{ lib});
	setvar( 'LIBOFLAG', $cc_specs{ $COMPILER}->{ liboflag});

	setvar( 'SCRIPT_EXT',
				(( $Win32 && !$cygwin) ? '.bat' : ( $OS2 ? '.cmd' : '')));
	setvar( 'DL_EXT', "." . $Config{ dlext});
	setvar( 'INC', ''); # compat mode INC
	setvar( 'INCPATH', (
		"include",
		qd( "include/generic"),
		qtilde($Config{archlib}) . qd( "/CORE"),
		( map { s/-I//; $_ } split ' ', $INC)
	));
	my @libpaths = quoted_split( $Config{ libpth});
	push( @libpaths, '/usr/local/lib') if $platform eq 'unix';  # or perl makefile.pl LIBPATH+=/usr/local/lib
	setvar( 'LIBPATH', @libpaths);
	my @libs = map { s/^$LDLIBFLAG//; $_} quoted_split($Config{ libs});
	# compat mode LIBS
	setvar( 'LIBS', '');
	for ( split ' ', $LIBS) {
		if ( /^-l(.*)/) {
			push @libs, $1;
		} elsif ( /^-L(.*)/) {
			push @LIBPATH, $1;
		} else {
			$LDFLAGS .= " $_";
		}
	}
	#    push ( @libs, 'pmprintf') if $OS2 && defined $ENV{PRIMA_PMPRINTF};
	if ( $useGC) {
		push @libs, 'leak';
	}
	if (
		$COMPILER eq 'gcc' and 
		$LD ne 'g++' # g++ and gcc together are weird kind - g++ does -lgcc itself
	) {
		push @libs, 'gcc';
	}
	my @flibs;
	
	if ( $Win32 && $COMPILER eq 'gcc') {
		$Config{libperl} =~ s/^lib(.*)\.a$/$1/i;
		push @libs, qw(gdi32 mpr winspool comdlg32); # add more when appropriate
	}
	if ( $CYGWIN_X11) {
		$Config{libperl} =~ s/^lib(.*)\.a$/$1/i;
	}

	open F, 'Prima.pm' or die "Cannot open Prima.pm:$!\n";
	my ($ver1, $ver2);
	while (<F>) {
		next unless m/\$VERSION[^\.\d]*(\d+)\.([_\d]+)/;
		$ver1 = $1, $ver2 = $2, last;
	}
	close F;
	die "Cannot find VERSION string in Prima.pm\n" unless defined $ver1;
	print "Version: $ver1.$ver2\n";
	setvar( 'DISTNAME', "Prima-$ver1.$ver2");

# here starts lot of execs...
	setup_compiler();

	for my $lib (@libs) {
		print "Checking for library $lib... ";
		my $path = find_lib($lib, '', '');
		if ( defined $path) {
			push @flibs, $lib;
			print "yes";
			print ", in $path" if length($path);
			print "\n";
		} else {
			print "no\n";
			$END .= "*** Warning (probably harmless): `$lib' library not found\n";
		}
	}

	@LIBS = @flibs;
	%DEFINES = (
		PRIMA_VERSION => $ver1,
		PRIMA_SUBVERSION => $ver2,
		PERL_PATCHLEVEL => $Config{ PATCHLEVEL},
		PERL_SUBVERSION => $Config{ SUBVERSION},
		PRIMA_CORE => 1,
		PERL_POLLUTE => 1,
		PRIMA_DEBUG  => $DEBUG,
	);
	$DEFINES{PRIMA_PLATFORM} = $OS2 ? 1 : ( $Win32 ? 2 : 3);
	if ( env_true( 'PRIMA_PARANOID_MALLOC')) {
		$DEFINES{ PARANOID_MALLOC} = 1;
	}
	if ( $useGC) {
		$DEFINES{ USE_GC} = 1;
	}
	while ( $CFLAGS =~ s/-D(\w+)(?:=(\S+))?\s*//) {
		my ( $defname, $defvalue) = ( $1, $2 || 1);
		$DEFINES{ $defname} = $defvalue;
	}
	while ( $CCFLAGS =~ s/-D(\w+)(?:=(\S+))?\s*//) {
		my ( $defname, $defvalue) = ( $1, $2 || 1);
		$DEFINES{ $defname} = $defvalue;
	}
	
	if ( $COMPILER eq 'msvc') {
		# crazy activestate/msvc stuff
		if ( $compiler_version < 1400) {
			$DEFINES{_CRT_SECURE_NO_DEPRECATE} = 1;
			# to be invoked on command line
			$CFLAGS .= " -D_CRT_SECURE_NO_DEPRECATE";
		}
		if ( $compiler_version >= 1600 ) {
			delete $DEFINES{_USE_32BIT_TIME_T};
			$DEFINES{_CRT_SECURE_NO_WARNINGS} = 1;
			$CFLAGS .= " -D_CRT_SECURE_NO_WARNINGS";
			$CFLAGS .= " /wd4244"; # warning C4244: '=' : conversion from 'Bool' to 'char', possible loss of data
			$CFLAGS .= " /wd4267"; # warning C4267: '=' : conversion from 'size_t' to 'int', possible loss of data
			$CFLAGS .= " /wd4018"; # warning C4018: '<' : signed/unsigned mismatch";
		}
	}

	# find common denominator with installsitearch and installscript
	my @a = split( '\\\\|\/', $Config{installscript});
	my @b = split( '\\\\|\/', $Config{installsitearch});
	my $i = 0;
	while ( defined $a[$i] && defined $b[$i]) {
		last if $a[$i] ne $b[$i];
		$i++;
	}
	my $prefix = qd(($i ? join( '/', @a[0..$i-1]) : ''));
	$USER_VARS{PREFIX} ||= $USER_VARS{INSTALL_BASE} if exists $USER_VARS{INSTALL_BASE};
	setvar( 'PREFIX',           $prefix);
	setvar( 'INSTALLSCRIPT',    $PREFIX . qd( '/' . join( '/', @a[$i .. $#a])));
	setvar( 'INSTALLSITEARCH',  $PREFIX . qd( '/' . join( '/', @b[$i .. $#b])));
	setvar( 'INSTALL_LIB',      $INSTALLSITEARCH . qd( "/Prima"));
	setvar( 'INSTALL_DL',       $INSTALLSITEARCH . qd( "/auto/Prima"));
	setvar( 'INSTALL_EXAMPLES', $INSTALLSITEARCH . qd( "/Prima/examples"));
	if ( exists $USER_VARS{PREFIX}) {
		setvar( 'INSTALL_MAN1', $INSTALLSITEARCH . qd( "/man/man1"));
		setvar( 'INSTALL_MAN3', $INSTALLSITEARCH . qd( "/man/man3"));
	} else {
		setvar( 'INSTALL_MAN1', $Config{installman1dir});
		setvar( 'INSTALL_MAN3', $Config{installman3dir});
	}
	setvar( 'X11BASE');

	setvar( 'MAKETYPE', lc ( $MAKE = $Config{ make}));
	# Note that Borland's make utility also named 'make'. But its
	# usage is deprecated, thus we can ignore it.
	#
	# In Linux GNU make also has name 'make'. But Makefile been
	# generated by this script is compatible with it. If it named
	# 'gmake' then we force $MAKETYPE to contain 'make'.
	$MAKETYPE = 'make' if $MAKETYPE =~ /gmake|pmake/;
	die "Unknown make utility '$MAKETYPE'" unless $MAKETYPE =~ /(nmake|dmake|make)/;
	$MAKETYPE = $1;

	setvar( 'RM', $Config{ rm});
	$OS2DLLF = 'Prima';
	$OS2DLLF = &DynaLoader::mod2fname([$OS2DLLF]) if $OS2 && defined &DynaLoader::mod2fname;
	$PrimaLib    = qd( "auto/Prima/${LIB_PREFIX}Prima$LIB_EXT");
	$PrimaTarget = qd( "auto/Prima/$OS2DLLF$DL_EXT");
	$extralibs = '';
	$extralibs = qd("auto/Prima/Prima.lib") if $OS2;
	setvar( 'DEFFILE', $Win32 ? qd( "win32/Prima.def") : ( $OS2 ? qd( "os2/Prima.def") : ''));
	setvar( 'AUTOMATED_RUN', 0);
	@executables = qw(
		Prima/VB/VB.pl
		Prima/VB/cfgmaint.pl
	);
	my $unix = (!$Win32 && !$OS2);
	setvar('WITH_ICONV', $unix);
	setvar('WITH_XFT', $unix);
	setvar('WITH_GTK2', $unix);
	$install_manuals = $cygwin || (!$Win32 && !$OS2);
	$install_manuals = 0 if !length( $INSTALL_MAN1) || !length($INSTALL_MAN3);
	$binary_prereq = '';

	@Prima_exports = qw(
boot_Prima build_dynamic_vmt build_static_vmt call_perl call_perl_indirect
clean_perl_call_method clean_perl_call_pv create_mate create_object
ctx_remap_def cv_call_perl debug_write duplicate_string eval gimme_the_mate
gimme_the_vmt kind_of kill_zombies notify_perl Object_create Object_destroy parse_hv
plist_create plist_destroy prima_mallocz pop_hv_for_REDEFINED protect_object
push_hv push_hv_for_REDEFINED query_method sv_call_perl sv_query_method
unprotect_object perl_error
);
	push @Prima_exports, grep { /^(apc|list|prima)/ } suck_symbols('include/apricot.h');
	push @Prima_exports, suck_symbols('include/img.h');
	push @Prima_exports, suck_symbols('include/img_conv.h');
	my %g = map { $_ => 1 } @Prima_exports;
	delete @g{qw(prima_utf8_to_uv prima_uv_to_utf8)};
	@Prima_exports = sort keys %g;
}

sub setup_compiler
{
	print "Checking if can compile... ";
	compile('int a;', 1) or die "no $see_makefile_log\n";
	print "yes\n";

	print "Checking if can link... ";
	compile( <<EOF, 0) or die "no $see_makefile_log\n";
int
main()
{
   return 0;
}
EOF
	print "yes\n";

	if ( $COMPILER eq 'msvc' ) {
		print "Checking MSVC version... ";
		$compiler_version = compile_and_run(<<'MSCVER');
#include <stdio.h>
int main() { 
	printf("%d\n", _MSC_VER); 
	return 0;
}
MSCVER
		print "$compiler_version\n";
	}
	
	if ($Win32) {
		print "Checking windows subsystem...";
		$Win64 = have_define('_WIN64');
		print $Win64 ? " 64" : " 32";
		print " bits\n";
	}

}

sub setup_X11
{
	# find X11 include files
	print "Checking for X11 headers...";
	push @INCPATH, "$X11BASE/include" if defined($X11BASE) and -d "$X11BASE/include";
	for ( 'local/', 'freeware/', 'gnu/', 'opt/') {
		push @INCPATH, qd( "/usr/${_}include") if -d "/usr/${_}include";
	}
	my $incpath = find_header( qd( "X11/Xlib.h"), 
		qd( "/usr/X11R6/include"), 
		qd( "/usr/X11/include"), 
		qd("/usr/X/include"), 
		qd("/usr/openwin/include"),
		qd("/opt/X11/include")
	);

	unless ( defined $incpath) {
		print "no\n";
		return unless $NeedX11;
		warn 
			"Prima needs X11 headers for compilation! ".
			"Set X11BASE='/path/to/X' or INCPATH+='/path/to/X/include' ".
			"if you have a non-standard path to X. $see_makefile_log\n";
		exit(0);
	}

	print "yes";
	if ( -d $incpath) {
		print ", in $incpath";
		push @INCPATH, $incpath;
	}
	print "\n";

	# find X11 libraries
	my @libpath = ( "X11", '', 
		(defined($X11BASE) ? "$X11BASE/lib" : ()), 
		"/usr/X11R6/lib", 
		"/usr/X11/lib",
		"/usr/X/lib",
		"/usr/openwin/lib",
		"/opt/X11/lib"
	);
	# using /usr/X11R6/lib64 ?
	unshift @libpath, map { s/lib$/lib64/; $_ } grep { /lib$/ } @libpath
		if $Config{intsize} == 8;

	print "Checking for library X11... ";
	my $libpath = find_lib( @libpath);
	unless ( defined $libpath) {
		print "no\n";
		unless ( $NeedX11) {
			pop @INCPATH if @INCPATH and $INCPATH[-1] eq $incpath;
			return;
		}
		warn 
			"Prima needs X11 libraries for compilation! ".
			"Set X11BASE='/path/to/X' or LIBPATH+='/path/to/X/lib' ".
			"if you have a non-standard path to X. $see_makefile_log\n";
		exit(0);
	}

	print "yes";
	if ( -d $libpath) {
		print ", in $libpath";
		push @LIBPATH, $libpath;
	}
	print "\n";

	if ( $NeedX11) {
		push @LIBS, 'X11';
		if (defined find_lib( "Xext", '', '')) {
			print "Xext library found.\n";
			push @LIBS, "Xext";
		}
	}
}

sub setup_perl
{
	if ( $Win32 || $cygwin || $OS2) {
		print "Checking for perl library... ";
		my $libperl = $Config{ libperl};
		$libperl =~ s/\.dll$//i if $cygwin;
		$libperl =~ s/\.lib$// if $Win32 and $COMPILER eq 'gcc' and not $cygwin;
		my $libpath = find_lib( $libperl, '', "", qtilde(qd( "$Config{ archlib}/CORE")));
		unless ( defined $libpath) {
			print "no\n";
			die "Cannot find perl library $see_makefile_log\n" 
		}
		print "yes";
		print ", in $libpath" if length($libpath);
		print "\n";
		push @LIBPATH, $libpath if $libpath;
		push @LIBS, $libperl;
	}
}

sub setup_defines
{
	have_header( "io.h");
	have_header( "unistd.h");
	have_header( "strings.h");

	my @int_types = qw(int8_t int16_t int32_t);
	my @uint_types = qw(uint8_t uint16_t uint32_t uint64_t);
	my @u_int_types = qw(u_int8_t u_int16_t u_int32_t u_int64_t);
	have_types_in( "sys/types.h", @int_types)
		|| have_types_in( "sys/bitypes.h", @int_types)
		|| have_types_in( "sys/inttypes.h", @int_types)
		|| have_types_in( "stdint.h", @int_types);
	have_types_in( "sys/types.h", @uint_types)
		|| have_types_in( "sys/bitypes.h", @uint_types)
		|| have_types_in( "sys/inttypes.h", @uint_types)
		|| have_types_in( "stdint.h", @uint_types);
	have_types_in( "sys/types.h", @u_int_types)
		|| have_types_in( "sys/bitypes.h", @u_int_types)
		|| have_types_in( "sys/inttypes.h", @u_int_types)
		|| have_types_in( "stdint.h", @u_int_types);

	if ( $NeedX11) {
		have_header( "sys/ipc.h", "sys/types.h");
		have_header( "sys/shm.h", "sys/types.h");
		have_header( "X11/extensions/shape.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
		have_header( "X11/extensions/XShm.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
	}
	if ( $OS2 && defined $ENV{ PRIMA_PMPRINTF}) {
		if ( have_header( "pmprintf.h", "os2.h")) {
				my $libpath = find_lib( 'pmprintf', '', "");
				die "There is no library for pmprintf  $see_makefile_log\n" unless defined $libpath;
				push @LIBS, 'pmprintf';
		}
		else {
				warn "PRIMA_PMPRINTF defined, but there is no pmprintf.h in include path";
		}
	}
	if ( $useGC) {
		have_header( 'gc.h');
	}

	have_funcs_in( 'strings.h', 'strcasecmp');
	have_funcs_in( 'strings.h', 'strncasecmp');
	have_funcs_in( 'strings.h', 'strcasestr');
	have_funcs_in( 'string.h', 'stricmp');
	have_funcs_in( 'string.h', 'strnicmp');
	have_funcs_in( 'stdio.h', 'snprintf');
	have_funcs_in( 'stdio.h', '_snprintf');
	have_funcs_in( 'stdlib.h', 'reallocf');
	have_funcs_in( 'strings.h', 'bzero');
	if ( $Win32) {
		have_type( "BOOLEAN", "windows.h");
	}
	find_inline();

	foreach my $defname ( keys %USER_DEFINES) {
		if ( defined $USER_DEFINES{ $defname}) {
			$DEFINES{ $defname} = $USER_DEFINES{ $defname};
		}
		else {
			delete $DEFINES{ $defname} if exists $DEFINES{ $defname};
		}
	}
	foreach my $defname ( keys %USER_DEFINES_ADDONS) {
		$DEFINES{ $defname} = ( $DEFINES{ $defname} || '') . join( '', @{ $USER_DEFINES_ADDONS{ $defname}});
	}
}

sub setup_xft
{
	if ( $WITH_XFT) {
		my $HAVE_XFT = 0;
		my $NEED_XFT = 4;
		my @pre_xft_libs = @LIBS;

		my @ft_incpaths = ( "",
			( defined($X11BASE) ? qd("$X11BASE/include/freetype2") : ()),
			qd("/usr/include/freetype2"),
			qd("/usr/X11R6/include/freetype2"),
			qd("/usr/X11/include/freetype2"),
			qd("/usr/X/include/freetype2"),
			qd("/usr/openwin/include/freetype2"),
			qd("/opt/X11/include/freetype2"),
			qd("/usr/local/include/freetype2"),
			qd("/usr/gnu/include/freetype2"),
			qd("/usr/freeware/include/freetype2"),
			qd("/usr/opt/include/freetype2")
		);
		my $have_ft2build_h = find_header( qd("ft2build.h"), @ft_incpaths );

		print "Checking for presence of freetype/freetype.h... ";
		my $incpath;
		if ($have_ft2build_h) {
			$DEFINES{HAVE_FT2BUILD_H} = undef;
			$incpath = find_header( 
				qd("freetype/freetype.h"), {
					Code => <<EOF,
#include "ft2build.h"
#include FT_FREETYPE_H
EOF
				},
				@ft_incpaths,
			);
		} else {
			$incpath = find_header( qd("freetype/freetype.h"), @ft_incpaths);
		}
		if (defined $incpath) {
			print "yes";
			print ", in $incpath" if $incpath;
			print "\n";
			push @INCPATH, $incpath if $incpath;
			print "Checking for presence of libfreetype... ";
			if ( defined find_lib( 'freetype', '', '')) {
				push @LIBS, 'freetype';
				$HAVE_XFT++;
				print "yes\n";
			} else {
				print "no\n";
				$DEFINES{HAVE_FREETYPE_FREETYPE_H} = undef;
			}
		} else {
			print "no\n";
		}

		if ( have_header( "fontconfig/fontconfig.h")) {
			print "Checking for presence of libfontconfig... ";
			if ( defined find_lib( 'fontconfig', '', '')) {
				push @LIBS, 'fontconfig';
				$HAVE_XFT++;
				print "yes\n";
			} else {
				$DEFINES{HAVE_FONTCONFIG_FONTCONFIG_H} = undef;
				print "no\n";
			}
		}

		if ( have_header( "X11/extensions/Xrender.h", "X11/X.h", 
				"X11/Xlib.h", "X11/extensions/Xext.h")) {
			print "Checking for presence of libXrender... ";
			if ( defined find_lib( 'Xrender', '', '')) {
				push @LIBS, 'Xrender';
				$HAVE_XFT++;
				print "yes\n";
			} else {
				$DEFINES{HAVE_X11_EXTENSIONS_XRENDER_H} = undef;
				print "no\n";
			}
		}

		if ( have_header( "X11/Xft/Xft.h", "X11/X.h", "X11/Xlib.h", 
				"X11/extensions/Xext.h", "X11/extensions/Xrender.h")) {
			print "Checking for presence of libXft... ";
			if ( defined find_lib( 'Xft', '', '')) {
				print "yes\n";
				push @LIBS, 'Xft';
				$HAVE_XFT++;
			} else {
				print "no\n";
				$DEFINES{HAVE_X11_XFT_XFT_H} = undef;
			}
		}

		$WITH_XFT = 0 unless $HAVE_XFT == $NEED_XFT;
		@LIBS = @pre_xft_libs unless $WITH_XFT;
	}

	$WITH_ICONV = 0 unless $WITH_XFT; # iconv is used for xft only
	if ( $WITH_ICONV && have_header( "iconv.h")) {
		print "Checking for presence of libiconv... ";
		if ( defined find_lib( 'iconv', '', '')) {
			push @LIBS, 'iconv';
			print "yes\n";
		} else {
			$DEFINES{HAVE_ICONV_H} = undef;
			$WITH_ICONV = 0;
			print "no\n";
		}
	} else {
		$WITH_ICONV = 0;
	}
	
	if ( $WITH_GTK2) {
		print "Checking for presence of gtk2... ";
		my $pkg_config = `which pkg-config`;
		chomp $pkg_config;
		unless ( -f $pkg_config) {
			$WITH_GTK2 = 0;
			print "no pkg-config, no gtk2\n";
		}

		if ( $WITH_GTK2) {
			my $vers = `pkg-config --modversion gtk+-2.0`;
			chomp $vers;
			if ( $vers =~ /^[\d\.]+$/) {
				$DEFINES{GTK_VERSION} = "\"$vers\"";
				print "yes, $vers\n";
			} else {
				$WITH_GTK2 = 0;
				print "no\n";
			}
		}

		if ( $WITH_GTK2) {
			my @saveinc = @INCPATH;
			my @savelib = @LIBS;

			my $inc = `pkg-config --cflags-only-I gtk+-2.0`;
			chomp $inc;
			my %inc = map { $_ => 1 } @INCPATH;
			push @INCPATH, $_ for grep { not exists $inc{$_}} $inc =~ /-I(\S+)/g;

			my $lib = `pkg-config --libs-only-l gtk+-2.0`;
			chomp $lib;
			my %lib = map { $_ => 1 } @LIBS;
			push @LIBS, $_ for grep { not exists $lib{$_}} $lib =~ /-l(\S+)/g;

			# now, try to compile with GTK. I've got lots of CPAN build failures
			# because GTK wasn't willing to compile or god knows what.
			print "Checking if can compile and link with gtk2... ";
			my $ok = compile( "#include <gtk/gtk.h>\nint main() { return 0; }\n");
			if ( $ok) {
				$DEFINES{WITH_GTK2} = 1;
				print "yes\n";
			} else {
				$WITH_GTK2 = 0;
				@LIBS     = @savelib;
				@INCPATH  = @saveinc;
				print "no\n";
			}
		}
	}
	
	print "Using Xft library\n" if $WITH_XFT;
	print "Using iconv library\n" if $WITH_ICONV;
	print "Using gtk2 library\n" if $WITH_GTK2;
}       

sub generate_win32_def
{
	open PRIMADEF, ">$DEFFILE" or die "Cannot create $DEFFILE: $!";
	print PRIMADEF <<EOF;
LIBRARY Prima
EXPORTS
EOF
	if ( $COMPILER eq 'bcc32') {
		print PRIMADEF map { "\t_$_\n\t$_=_$_\n"} @Prima_exports;
	}
	else {
		print PRIMADEF map { "\t$_\n\t_$_ = $_\n"} @Prima_exports;
	}
	close PRIMADEF;
}

sub generate_os2_def
{
	open PRIMADEF, ">$DEFFILE" or die "Cannot create $DEFFILE: $!";
	print PRIMADEF <<EOF;
LIBRARY $OS2DLLF INITINSTANCE TERMINSTANCE
DESCRIPTION 'Prima Guts version 2.00, Copyright (C) the Protein Laboratory 1997'
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
EOF
	print PRIMADEF map { "\t$_\n"} @Prima_exports;
	close PRIMADEF;
}


sub setup_codecs
{
	# see if Prima::codecs:: is installed
	my ( $prereq, $have_binary_prereq);
	$prereq = 'win32' if $Win32 and not $cygwin;
	$prereq = 'win64' if $Win64 and not $cygwin;
	$prereq = 'os2'   if $OS2; 
	if ( $prereq) {
		print "Checking for Prima::codecs::$prereq... ";
		eval "use Prima::codecs::$prereq;";
		unless ( $@) {
			print "yes\n";
			$have_binary_prereq++;
			push @LIBPATH, qd("$Config{installsitearch}/Prima/codecs/$prereq/lib");
			push @INCPATH, qd("$Config{installsitearch}/Prima/codecs/$prereq/include");
		} else {
			print "no\n";
		}
	}

	# finding image codecs
	my %libs = map { $_ => 1 } @LIBS;
	my @codecs;
	my @builtin_codecs;
	while ( <img/codec_*.c>) {
		if ( m/prigraph/) {
			unshift @codecs, $_; # put it first
		} elsif ( m/codec_(bmp)/) {
			push @builtin_codecs, $1;
		} else {
			push @codecs, $_;
		}
	}
	
	my @codec_libpath = qd( $Config{installsitearch});
	my @warn_codecs;
	for my $cx ( @codecs) {
		my @inc;
		my $foundlib;
		$cx =~ m/codec_(.*)\.c$/i;
		my ( $fn, $lib, $codec) = ( $cx, $1, $1);
		next unless open F, $fn;
		while(<F>) {
			push @inc, $_ if m/^\s*#include\s*\</;
		}
		close F;

	AGAIN:
		$lib.='.lib' if ( $COMPILER eq 'msvc') || $OS2;
		print "Checking for $codec library... ";
		if ( 
			$libs{$lib} || 
			defined ( $foundlib = find_lib( $lib, join('', @inc), '', @codec_libpath))
		) {
			if ( defined $foundlib and length $foundlib) {
				push @LIBPATH, $foundlib;
				@codec_libpath = ();
			}
			push( @ACTIVE_CODECS, $codec);
			# In gcc, order of libs matters. libXpm requires libgdi32, and
			# has to be mentioned _after_ it to work.
			unshift( @LIBS, $lib) unless $libs{$lib};
			print "yes";
			print ", in $foundlib" if defined($foundlib) and length($foundlib);
			print "\n";
		} elsif ( $codec eq 'ungif') {
			$lib = $codec = 'gif';
			print "no\n";
			goto AGAIN;
		} else {
			$PASSIVE_CODECS{$fn} = 1;
			push @warn_codecs, $codec unless $codec eq 'prigraph';
			print "no\n";
		}
	}

	unless ( @ACTIVE_CODECS) {
		$binary_prereq = $prereq;
		$END .= <<NOCODECS;

** No image codecs found. 

Note that in this configuration Prima will not be 
able to work with graphic files. Please follow the 
instructions in README file.
NOCODECS
		$END .= <<BROKEN_CODECS if $have_binary_prereq;

** Prima::codecs::$binary_prereq is found in 
$Config{installsitearch}, but is broken. Please reinstall it.

BROKEN_CODECS
		$END .= <<NOCODECS_BIN if $binary_prereq;

If you are under CPAN shell and are asked to install 
Prima::codecs::$binary_prereq dependency, do so. Otherwise,
install it manually.
NOCODECS_BIN

		$END .= <<NOCODECS_CYGWIN if $cygwin;

Install these libraries and re-run Makefile.PL

NOCODECS_CYGWIN
	} elsif ( @warn_codecs) {
		$END .= <<NOCODECS;
          
** Warning: the following image libraries weren't found:

@warn_codecs

Note that in this configuration Prima will not be 
able to work with the corresponding image formats.
Please follow the instructions in README file.

NOCODECS
	}
	push @ACTIVE_CODECS, @builtin_codecs;

	# make prigraph last so it'll have a chance to check which codecs it doesn't 
	# need to support
	@codecs = grep { $_ ne 'prigraph' } @ACTIVE_CODECS;
	@ACTIVE_CODECS = ( @codecs, 'prigraph' ) unless @codecs == @ACTIVE_CODECS;
}

sub setup_misc
{
	push @Prima_exports, 'bzero' unless $DEFINES{HAVE_BZERO};
	# Setting up translation tables for quoting string for different make utilities.
	my ( $make_tbl, $nmake_tbl, $dmake_tbl);
	$make_tbl = [ map { chr} 0..255];
	$dmake_tbl = [ map { chr} 0..255];
	$nmake_tbl = [ map { chr} 0..255];
	foreach ( ':', ';', '#', '(', ')', '^', '\\', '{', '}', '!', '@', '-') {
		$nmake_tbl->[ ord] = "^$_";
	}
	$nmake_tbl->[ ord '%'] = '%%';
	$nmake_tbl->[ ord '$'] = $make_tbl->[ ord '$'] = '$$';
	$dmake_tbl->[ ord '#'] = '\\#';
	$make_tbl->[ ord '#'] = '\\#';
	%make_trans = (
		make => $make_tbl,
		nmake => $nmake_tbl,
		dmake => $dmake_tbl,
	);
	generate_win32_def if $Win32;
	generate_os2_def if $OS2;

	# check if we need dl_load_flags(0x01)
	if ( $DL_LOAD_FLAGS < 0) {

		print "Determining dl_load_flags... ";

		my $c1  = qd( tempfile( "$TMPDIR/pmts%04d.c"));
		$c1 =~ m/pmts([^\.]*).c$/;
		my ( $n1, $n2) = ( $1, sprintf("%04d", 1 + $1));

		my $o1  = qd( "$TMPDIR/pmts$n1$OBJ_EXT");
		my $o2  = qd( "$TMPDIR/pmts$n2$OBJ_EXT");
		my $dl1 = qd( "$TMPDIR/pmts$n1$DL_EXT");
		my $dl2 = qd( "$TMPDIR/pmts$n2$DL_EXT");
		my @ex = map { qd("$TMPDIR/pmts$_")} map { ("$n1$_", "$n2$_") } ('.ilk', '.pdb');

		open TMPSRC, ">$c1" or die "Creation of temporary file $c1 failed";
		print TMPSRC <<D;
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

int test( void ) { return 1; }

XS(boot_pmts$n1) { 
   dXSARGS;
   XSRETURN(1);
}
D
		close TMPSRC;

		my $c2  = qd( tempfile( "$TMPDIR/pmts%04d.c"));
		open TMPSRC, ">$c2" or die "Creation of temporary file $c2 failed";
		print TMPSRC <<D;
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

extern int test ( void );

XS(boot_pmts$n2) {
   dXSARGS;
   test();
   XSRETURN(1);
}
D
		close TMPSRC;

		my @cc1 = grep !/^-W(all|error|\d)/i, cc_command_line( $c1, $o1, $dl1, 1, 1);
		my @cc2 = grep !/^-W(all|error|\d)/i, cc_command_line( $c2, $o2, $dl2, 1, 1);
		my @ld1 = ld_command_line( $dl1, $o1);
		my @ld2 = ld_command_line( $dl2, $o2);
		my $dlpl = "$^X -e '" . (join ' ', split("\n", <<DN)) . "'";
require DynaLoader;
unshift \@INC, q($TMPDIR);

package pmts$n1;
\@ISA = q(DynaLoader);
sub dl_load_flags{0x00}
bootstrap pmts$n1 0;

package pmts$n2;
\@ISA = q(DynaLoader);
bootstrap pmts$n2 0;
DN
		
		null_output;
		print STDERR "@cc1\n";
		goto FAIL if system("@cc1");
		print STDERR "@ld1\n";
		goto FAIL if system("@ld1");
		print STDERR "@cc2\n";
		goto FAIL if system("@cc2");
		print STDERR "@ld2\n";
		goto FAIL if system("@ld2");
		print STDERR "$dlpl\n";
		my $ok_0 = system( $dlpl );
		$dlpl =~ s/0x00/0x01/;
		print STDERR "$dlpl\n";
		my $ok_1 = system( $dlpl );

		if ( $ok_0 != 0 && $ok_1 == 0) {
			$DL_LOAD_FLAGS = 1;
		} elsif ( $ok_0 == 0 && $ok_1 == 0) {
			$DL_LOAD_FLAGS = 0;
		}
FAIL:
		unlink ( $c1, $c2, $o1, $o2, $dl1, $dl2, @ex );
		restore_output;
		if ( $DL_LOAD_FLAGS < 0 ) {
			print <<DLERR;
unknown
** warning: set DL_LOAD_FLAGS=1 if your system requires RTLD_GLOBAL
DLERR
			$DL_LOAD_FLAGS = 0;
		} else {
			printf("0x%02x\n", $DL_LOAD_FLAGS);
		}
	}

	$packlist = ExtUtils::Packlist-> new();
}

sub process_commandline
{
	%overrideable = map { /^(.)(.*)$/; $2 => $1} @ovvars; # Script variables which may be overridden.
	my %expand = map { /^.(.*)$/; $1 => 1 } @path_expand_ovvars;
	my %no_compat = map { $_ => 1 } @no_compat;
	$ARGV_STR = join( " ", map { "\"$_\""} @ARGV);
	foreach my $arg ( @ARGV) {
		if ( $arg =~ /^\s*(\w+)\s*(\+?)\=(.*)$/) {
			my ( $varname, $setmode, $varval) = ( $1, $2 || '', $3);
			if ( $no_compat{ $varname}) {
				print "Variable $varname is not supported. Expect wrong behavior\n";
				next;
			}
			die "Unknown variable $varname" unless $overrideable{ $varname};
			$varval =~ s/~/$ENV{HOME}/g if $expand{ $varname};
			if ( $overrideable{ $varname} eq '$') {
				if ( $setmode eq '+') {
					push @{ $USER_VARS_ADDONS{ $varname}}, $varval;
				} else {
					$USER_VARS{ $varname} = $varval;
				}
				die $@ if $@;
			}
			elsif ( $overrideable{ $varname} eq '@') {
				my @values = split /$path_sep/, $varval;
				if ( $expand{ $varname}) {
					s/~/$ENV{HOME}/g for @values;
				}
				if ( $setmode eq '+') {
					push @{ $USER_VARS_ADDONS{ $varname}}, @values;
				} else {
					$USER_VARS{ $varname} = \@values;
				}
				die $@ if $@;
			}
		}
		elsif ( $arg =~ /^-(D|U)(\w+)(?:(\+?)=(.*))?$/) {
			my ( $defmode, $defname, $setmode, $value) = ( $1, $2, $3 || '', $4 || '');
			if ( $defmode eq 'U') {
				$USER_DEFINES{ $defname} = undef; # I.e. it will exists in the hash...
			}
			else {
				if ( $setmode eq '+') {
					push @{ $USER_DEFINES_ADDONS{ $defname}}, $value;
				} else {
					$USER_DEFINES{ $defname} = $value;
				}
			}
		}
		else {
				die "Unknown command line argument or wrong syntax: '$arg'";
		}
	}
}

sub _find_file
{
	my ( $fname, $dir) = @_;
	my ( $pathname, $found);
	$pathname = qd( "$dir/$fname");
	return $pathname if -e $pathname;
	opendir D, $dir or die "Cannot open dir $dir: $!";
	my @entries = map { qd( "$dir/$_")} grep { /^[^.]/ && -d qd( "$dir/$_")} readdir D;
	closedir D;
	foreach my $entry ( @entries) {
		$pathname = _find_file( $fname, $entry);
		next unless defined $pathname;
		return $pathname;
	}
	return undef;
}

sub find_file
{
	my ( $fname) = @_;
	$fname =~ s/\\/\//g;
	$fname = qd($fname);
	return $cache_find_files{$fname} if exists $cache_find_files{$fname};
	return $cache_find_files{$fname} = _find_file( $fname, '.');
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

	local *CF;
	open CF, "<$cfile" or die "Cannot open $cfile: $!";
	while ( <CF>) {
		chomp;
		next unless /^\s*\#\s*include\s+"([^\"]+)"/;
		my $incfile = $1;
		my $i = find_file( $incfile);
		$incfile = defined($i) ? $i : qd( "include/generic/$incfile");
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
	close CF;
	return @{ $deps->{ $cfile}};
}

sub cmake
{
	my ( $cfile) = @_;
	print "Finding dependencies for $cfile...\n";
	my $ofile = "$1$OBJ_EXT" if $cfile =~ /^(.*)\.c$/;
	die "Internal error: illegal c file"
		unless defined $ofile;
	$cfile = qd( $cfile);
	$ofile = qd( $ofile);
	push @allclean, $ofile;
	push @allojects, $ofile;
	my @deps = find_cdeps( $cfile);
	return(
		"$ofile: Makefile $cfile @deps\n\t" .
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
	my $mk = qd( "include/generic/$classname.h") . ": Makefile " .
		"$clsfile " . qd( "utils/gencls.pl ") .
		qd( "Prima/Gencls.pm ");
	push @alltml, qd( "include/generic/$classname.tml");
	push @alltmldeps, qd( "include/generic/$classname.h");
	push @allclean, (
		qd( "include/generic/$classname.h"),
		qd( "include/generic/$classname.inc"),
		qd( "include/generic/$classname.tml")
	);
	push @allinstall, qd( "include/generic/$classname.h"), $INSTALL_LIB . qd( "/CORE/generic");
	my @ancestors = gencls( $clsfile, depend => 1);
	$mk .= qd( "include/generic/$_.h $_.cls ") foreach @ancestors;
	print "\n";
	$mk .= "\n\t$^X -I. utils/gencls.pl --inc --h --tml $clsfile include/generic\n\n";
}

sub qqd
{
	my ( $path_str) = @_;
	$path_str =~ s[/][$dir_sep]g;
	$path_str =~ s[\\][\\\\]g;
	return $path_str;
}

sub manname
{
	my ( $name, $section, $prefix) = @_;
	$prefix = quotemeta( $prefix );
	my $ds = quotemeta($dir_sep);
	$name =~ s/^$prefix(?:[\\\/$ds])//;
	$name =~ s/[\\\/$ds]/::/g;
	$name =~ s/\.[^\.]*$/\.$section/;
	return (( $section == 3 ) ? $INSTALL_MAN3 : $INSTALL_MAN1) . $dir_sep . $name;
}

sub _quote
{
	my $name = shift;
	$name =~ s/'/\\'/g;
	return \ "'$name'";
}

sub create_config_pm
{
	my $_cwd = cwd;
	my $cwd = qqd($_cwd);
	my $ifs = ($Win32 && !$cygwin) ? '\\\\' : '/';
	my @ip = map { qqd($_) } @INCPATH;
	$ip[0] = "$cwd${ifs}include";
	$ip[1] = "$cwd${ifs}include${ifs}generic";
	my $ipp = join(',', map {"\'$_\'"} @ip);
	$ip[0] = qqd("$INSTALLSITEARCH/Prima/CORE");
	$ip[1] = qqd("$INSTALLSITEARCH/Prima/CORE/generic");
	my $ippi = join(',', map {"\'$_\'"} @ip);
	my @cdefs = ('HAVE_CONFIG_H=1');
	my $cdefs = join( ',', map {"'$_'"} @cdefs);
	my $lddef = ( $COMPILER eq 'msvc') ? '/def:' : '',
	my @libpath = @LIBPATH;
	my @libs = @LIBS;
	my $perllib = pop @libs;
	if ( $perllib =~ m/^(.*)\/([^\/]*)$/) {
		my $pfile = $2;
		push @libpath, $1;
		$pfile =~ s/$LIB_EXT$//;
		push @libs,  $pfile;
	} else {
		push @libs, $perllib;
	}
	unless ( $PLATFORM eq 'unix' or $COMPILER eq 'gcc') {
		push @libpath, "$_cwd/auto/Prima";
		push @libs, 'Prima' . $cc_specs{$COMPILER}->{ldlibext};
	}
	my $libpath = qqd(join( ',', map {"'$_'"} @libpath));
	unless ( $PLATFORM eq 'unix' or $COMPILER eq 'gcc') {
		$libpath[-1] = "$INSTALLSITEARCH/auto/Prima";
	}
	my $libpathi = qqd(join( ',', map {"'$_'"} @libpath));
	my $ldlibs  = qqd(join( ',', map {"'$_'"} @libs));
	my $perl    = qqd($^X);
	my $pl = qqd( $PrimaLib);
	my $pt = qqd( $PrimaTarget);
	my $iscr = qqd($INSTALLSCRIPT);
	my $isa  = qqd($INSTALLSITEARCH);

	my $define = join(' ', map { "-D$_" } @cdefs);
	my $inc    = join(' ', map { "-I$_" } @ip);
	my $libs   = '';
	if ( $cygwin) {
		$libs = "-L$INSTALL_DL -lPrima";
	} elsif ( $Win32 || $OS2) {
		$libs = "$INSTALLSITEARCH/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT";
	}

	open F, "> Prima/Config.pm" or die "cannot open Prima/Config.pm:$!\n";
	print F <<CONFIG;
# This file was automatically generated.
# Do not edit, you'll loose your changes anyway.
package Prima::Config;
use vars qw(%Config %Config_inst);

%Config_inst = (
	incpaths              => [ $ippi ],
	gencls                => '$iscr${ifs}gencls$SCRIPT_EXT',
	tmlink                => '$iscr${ifs}tmlink$SCRIPT_EXT',
	libname               => '$isa${ifs}auto${ifs}Prima${ifs}Prima$LIB_EXT',
	dlname                => '$isa${ifs}$pt',
	ldpaths               => [$libpathi],

	libs                  => '$libs',
	define                => '$define',
	inc                   => '$inc',
);

%Config = (
	ifs                   => '$ifs',
	quote                 => '\\$SHQUOTE',
	platform              => '$PLATFORM',
	compiler              => '$COMPILER',
	incpaths              => [ $ipp ],
	platform_path         => ${_quote("$cwd${ifs}$PLATFORM")},
	gencls                => '\\$SHQUOTE$perl\\$SHQUOTE $cwd${ifs}utils${ifs}gencls.pl',
	tmlink                => '\\$SHQUOTE$perl\\$SHQUOTE $cwd${ifs}utils${ifs}tmlink.pl',
	scriptext             => '$SCRIPT_EXT',
	genclsoptions         => '--tml --h --inc',
	cc                    => ${_quote($CC)},
	cflags                => ${_quote("-c $C_FLAGS $CCFLAGS")},
	cdebugflags           => ${_quote($cc_specs{$COMPILER}->{cdebugflags})},
	cincflag              => ${_quote($CINCPATHFLAG)},
	cobjflag              => ${_quote($COUTOFLAG)},
	cdefflag              => ${_quote($CDEFFLAG)},
	cdefs                 => [$cdefs],
	objext                => ${_quote($OBJ_EXT)},
	lib                   => ${_quote($LIB)},
	liboutflag            => ${_quote($LIBOFLAG)},
	libext                => ${_quote($LIB_EXT)},
	libprefix             => ${_quote($LIB_PREFIX)},
	libname               => ${_quote("$cwd${ifs}$pl")},
	libs                  => ${_quote("$cwd${ifs}$pl")},
	dlname                => ${_quote("$cwd${ifs}$pt")},
	dlext                 => ${_quote($DL_EXT)},
	ld                    => ${_quote($LD)},
	ldflags               => ${_quote($LD_FLAGS)},
	lddefflag             => ${_quote($lddef)},
	lddebugflags          => ${_quote($cc_specs{$COMPILER}->{lddebugflags})},
	ldoutflag             => ${_quote($LDOUTFLAG)},
	ldlibflag             => ${_quote($LDLIBFLAG)},
	ldlibpathflag         => ${_quote($LDLIBPATHFLAG)},
	ldpaths               => [$libpath],
	ldlibs                => [$ldlibs],
	ldlibext              => ${_quote($cc_specs{$COMPILER}->{ldlibext})},
	inline                => ${_quote($DEFINES{__INLINE__})},
	perl                  => ${_quote($perl)},
	dl_load_flags         => $DL_LOAD_FLAGS,

	libs                  => '$libs',
	define                => '$define',
	inc                   => '$inc',
);

1;
CONFIG
	close F;
}

sub create_codecs_c
{
	open F, "> img/codecs.c" or die "cannot open img/codecs.c:$!\n";

	my $def1 = join("\n", map { "extern void apc_img_codec_$_(void);"} @ACTIVE_CODECS);
	my $def2 = join("\n", map { "\tapc_img_codec_$_();"} @ACTIVE_CODECS);

	print F <<CONTENT;
/*
  This file was automatically generated.
  Do not edit, you'll loose your changes anyway.
*/

#include "img.h"

#ifdef __cplusplus
extern "C" {
#endif

$def1

void
prima_cleanup_image_subsystem(void)
{
	apc_img_done();
}

void
prima_init_image_subsystem(void)
{
	apc_img_init();
$def2
}

#ifdef __cplusplus
}
#endif

CONTENT

	close F;
}


#
# Here we go...
#

$useGC = env_true( 'PRIMA_USE_GC');

$Win32 = ( $^O =~ /mswin32|cygwin/i);
$cygwin = ( $^O =~ /cygwin/i);
$OS2 = ( $^O =~ /os2/i);
$dir_sep = (( ( $path_sep = $Config{ path_sep}) eq ':') || ( defined $Config{ emxpath})) ? '/' : '\\';

if ( $#ARGV >= 0 && $ARGV[ 0] =~ /^\-\-cp(bin)?$/) {
	my $isbin = defined $1;
	shift @ARGV;
	die qq(Even number of parameters expected) unless ( $#ARGV % 2);
	while ( scalar @ARGV) {
		my ( $src, $dst) = ( shift @ARGV, shift @ARGV);
		print qq(Installing $src -> $dst\n);
		next unless -f $src;
		if ( $isbin) {
			my $dstdir = dirname( $dst);
			mkpath $dstdir if $dstdir && ! -d $dstdir;
			unlink $dst;
			open SRCPL, "<$src" or die "Cannot open $src: $!";
			open DSTPL, ">$dst" or die "Cannot create $dst: $!";
			chmod 0755, $dst unless ($Win32 && !$cygwin) || $OS2;
			if ( $Win32 && !$cygwin) {
				print DSTPL <<EOF;
\@rem = '--*-Perl-*--
\@echo off
if "%OS%" == "Windows_NT" goto WinNT
$Config{perlpath} -w -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
$Config{perlpath} -w -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\\system32\\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
\@rem ';
#!perl
EOF
			}
			elsif ( $OS2) {
				my $perlpath = $Config{perlpath};
				$perlpath =~ s/(perl)(\.exe)?$/$1__$2/i if $perlpath =~ /perl(\.exe)?$/i;
				print DSTPL <<EOF;
extproc $perlpath -wS
EOF
			} else {
				print DSTPL <<EOF;
#!$Config{perlpath} -w
EOF
			}
			my $filestart = 1;
			while ( <SRCPL>) {
				next if $filestart && /^\#\!/;
				$filestart = 0;
				print DSTPL;
			}
			if ( $Win32 && !$cygwin) {
				print DSTPL <<EOF;
__END__
:endofperl
EOF
			}
			close SRCPL;
			close DSTPL;
		}
		else {
			die qq(Destination must be a directory) if -e $dst && ! -d $dst;
			mkpath $dst unless -d $dst;
			copy( $src, $dst) or die qq(Copy failed: $!);
		}
	}
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--md') {
	shift @ARGV;
	mkpath \@ARGV;
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--rm') {
	shift @ARGV;
	unlink @ARGV;
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--rmdir') {
	shift @ARGV;
	rmdir for @ARGV;
}
elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--updateconfig') {
	shift @ARGV;
	my ( $fn_cfg, $fn_prima) = ( shift @ARGV, shift @ARGV);
	open F, $fn_cfg or die "cannot open $fn_cfg:$!\n";
	open FF, "> $fn_cfg.tmp" or die "cannot open $fn_cfg.tmp:$!\n";
	my ( $c_state, $ci_state) = (0,0);
	my %ci;

	print FF <<HEADER;
# This file was automatically generated.
package Prima::Config;
use vars qw(%Config);

%Config = (
HEADER
	while ( <F>) {
		if ( $ci_state == 0) {
			if ( m/\%Config_inst = \(/) {
				$ci_state = 1;
			}
		} elsif ( $ci_state == 1) {
			if ( m/^\);/) {
				$ci_state = 0;
			} else {
				$ci{$1} = $_ if m/^\s*(\S+)\s*/;
			}
		}
		if ( $c_state == 0) {
			if ( m/\%Config = \(/) {
				$c_state = 1;
			}
		} elsif ( $c_state == 1) {
			if ( m/^\);/) {
				$c_state = 0;
			} else {
				if ( m/^\s*(\S+)\s*/ && exists $ci{$1}) {
					print FF $ci{$1};
				} else {
					print FF $_;
				}
			}
		}
	}
print FF <<FOOTER;
);

1;
FOOTER
	close FF;
	close F;
	unlink $fn_cfg;
	rename "$fn_cfg.tmp", $fn_cfg;

	if ( defined $fn_prima) {
		open F, $fn_prima or die "cannot open $fn_prima:$!\n";
		local $/;
		my $ct = <F>;
		close F;
		$ct =~ s/(dl_load_flags\s*\{\s*)0x00/${1}0x01/;
		open F, "> $fn_prima" or die "cannot write $fn_prima:$!\n";
		print F $ct;
		close F;
	}
} elsif ( $#ARGV >= 2 && $ARGV[ 0] eq '--dist') {
	my $type = lc $ARGV[1];
	my $cwd = cwd();
	my $distname = $ARGV[2];

	sub clean_dist
	{
		my @dirs;
		return unless -d $distname;
		print "Cleaning...\n";
		finddepth( sub {
			my $f = "$File::Find::dir/$_";
			-d($f) ? push(@dirs, $f) : unlink($f);
		}, "$cwd/$distname");
		rmdir $_ for sort {length($b) <=> length($a)} @dirs;
		rmdir $distname;
	}

	sub cleanup
	{
		clean_dist;
		warn("$_[0]:$!\n") if defined $_[0];
		exit(0);
	}

	clean_dist;
	my @dirs;
	my @files;
	finddepth( sub {
		return if $_ eq '.' || 
			($_ eq 'Makefile' && $File::Find::dir eq $cwd) || 
			$_ eq 'makefile.log' || 
			m/^\./;
		return if /\.(pdb|ncb|opt|dsp|dsw)$/i; # M$VC
		my $f = "$File::Find::dir/$_";
		return if $f =~ /include.generic|CVS|\.git/;
		if ($type eq 'bin') {
			return if $f =~ /\.(c|cls|h)$/i;
			return if $f =~ /$cwd.(img|include|os2|win32|unix|Makefile.PL)/i;
		} else {
			return if $f =~ /auto/;
		}
		if ( -d $f) {
			$f =~ s/^$cwd/$distname/;
			push @dirs, $f;
		} else {
			return if $f =~ m/$Config{_o}$/;
			push @files, $f;
		}
	}, $cwd);

	print "Creating directories...\n";
	for ( @dirs) {
		next if -d $_;
		cleanup( "Can't mkdir $_") unless mkpath $_;
	}

	print "Copying files...\n";
	for ( @files) {
		my $f = $_;
		$f =~ s/^$cwd/$distname/;
		cleanup("Error copying $_ to $_") unless copy $_, $f;
	}

	if ( $type eq 'bin') {
		my $os_suffix = $^O;
		$os_suffix =~ s/\s/_/g;
		my $zipname = "$distname-$os_suffix.zip";
		unlink $zipname;
		unlink "$distname/$zipname";
		system "zip -r $zipname $distname";
	} elsif ( $type eq 'zip') {
		my $zipname = "$distname.zip";
		unlink $zipname;
		unlink "$distname/$zipname";
		system "zip -r $zipname $distname";
	} else { # tar dist
		my $tarname = "$distname.tar";
		unlink $tarname;
		unlink "$distname/$tarname";
		system "tar -cv -f $tarname $distname"
	}

	clean_dist;
} elsif ( $#ARGV >= 0 && $ARGV[ 0] eq '--help') {
	print <<HELP;

$0 - configuration script for Prima

syntax:
    perl Makefile.PL [options] 

compiler/linker options:
    COMPILER          - one of (cc,gcc,hpcc,irixcc,emx,msvc,bcc32), selects 
                        environment profile. Detected automatically if unset.
    CC, CFLAGS        - compiler and its flags
    LD, LDFLAGS       - linker and its flags
    INCPATH, LIBPATH  - additional paths to include and library files
                        (f.ex. INCPATH+=/usr/opt/include )
    PREFIX            - installation prefix, default is perl5 hierarchy
    X11BASE           - path where X11 includes and libraries are installed
    WITH_XFT          - compile with/without Xft (default 1), X only
    WITH_ICONV        - compile with/without iconv (default 1), X only
    WITH_GTK2         - compile with/without GTK2 (default 1), X only
    CYGWIN_X11        - X11 build in cygwin environment

HELP
	exit;
} else {
	$CWD = cwd;

	die "This script should be launched from Prima root directory\n"
		unless -r qd( "include/apricot.h") && -d "Prima";

	$| = 1;

	print "Setting up working environment.\n";
=pod
	%USER_VARS_LINKS = (
		INSTALLSCRIPT => {
				src => 'PREFIX',
				format => '%s/bin',
		},
		INSTALLSITEARCH => {
				src => 'PREFIX',
				format => '%s',
		},
	);
=cut
	process_commandline;
	setup_variables;
	setup_X11;
	setup_perl;
	setup_defines;
	setup_xft;
	
	my $config_dir = "include/generic";
	my $config_h = "$config_dir/config.h";
	print "Creating $config_h\n";
	unless ( -d "$config_dir") {
		mkdir $config_dir, 0777;
	}
	open CONFIG, ">$config_h" or die "Creation of $config_h failed: $!";
	print CONFIG <<EOF;
#ifndef __GENERIC_CONFIG_H__
#define __GENERIC_CONFIG_H__
EOF
	foreach my $define ( sort keys %DEFINES) {
		print CONFIG "#undef $define\n";
		print CONFIG "#define $define $DEFINES{ $define}\n" if defined $DEFINES{ $define};
	}
	print CONFIG <<EOF;
#endif
EOF
	close CONFIG;
	
	setup_codecs;
	setup_misc; # contains code dependant on config.h

	push @allrealclean, $config_h;
	push @allinstall, $config_h => $INSTALL_LIB . qd( "/CORE/generic");
	
	# install pod files
	print "Enumerating POD files\n";
	push @allinstall, map { qd($_), $INSTALLSITEARCH} <pod/*.pod>;
	push @allman,     map { qd($_), manname($_, 1, 'pod')} <pod/*.pod>;
	finddepth( sub {
		my $f = "$File::Find::dir/$_";
		return unless -f $_;
		my $d = $File::Find::dir; 
		$d =~ s/^pod[\\\/]Prima//;
		if ( m/pod$/) {
			push @allinstall, qd($f), qd("$INSTALL_LIB$d");
			push @allman,     qd($f), manname($f, 3, 'pod');
		} elsif( m/gif$/) {
			push @allinstall, qd($f), qd("$INSTALL_LIB$d");
		}
	}, "pod/Prima");

	push @alldirs, qd( "include/generic"), qd( "auto/Prima");

	print "Creating Prima::Config.pm\n";
	create_config_pm;
	print "Creating img/codecs.c\n";
	create_codecs_c;
	#exit;

	my $make = <<HEADER;
# Makefile for Prima project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.
HEADER

	$make .= <<PREREQ if $binary_prereq;
#	
# PREREQ_PM => { Prima::codecs::$binary_prereq=>q[0] }
#
PREREQ

	$make .= <<RERUN if $binary_prereq and not $AUTOMATED_RUN;
prima_prereq:
\t\@echo Rebuilding Makefile...
\t\@$^X Makefile.PL --rm Makefile
\t\@$^X Makefile.PL $ARGV_STR AUTOMATED_RUN=1
\t\@$MAKE

RERUN

	$make .= "all: prima\n\n";

	$make .= join( "\n", @headers) . "\n";

	while ( <*.cls>) {
		$make .= clsmake( $_);
	}
	while ( <*.c>) {
		$make .= cmake( $_);
	}
	while ( <img/*.c>) {
		next if exists $PASSIVE_CODECS{$_};
		$make .= cmake( $_);
	}
	while ( <$PLATFORM/*.c>) {
		next if !$WITH_XFT && m/xft.c/;
		$make .= cmake( $_);
	}
	while ( <examples/*>) {
		if ( m/\.pl$/i) {
			push @allbins, $_, $INSTALL_EXAMPLES;
		} else {
			push @allinstall, $_, $INSTALL_EXAMPLES;
		}
	}
	while ( <utils/*.pl>) {
		push @allbins, $_, $INSTALLSCRIPT;
	}
	for ( @executables) {
		push @allbins, $_, $INSTALLSCRIPT;
		m/\/([^\/]+)$/;
		push @allman, qd($_), manname( $1, 1, '');
	}

	my $thunks_tinc = qd( "include/generic/thunks.tinc");
	push @allclean, $thunks_tinc;

	push @allrealclean, 'Makefile';

	print "Writing Makefile...";
	$make =~ tr[/][]s;
	open MAKE, ">Makefile" or die "Creation of Makefile failed: $!";
	print MAKE $make;

# dist-related veriables
	if ( $OS2) {
		print MAKE <<OS2D;
$extralibs:
\temximp -o $extralibs os2\\Prima.def
OS2D
	}

	my $libtarget = $win32_use_dlltool ? $PrimaLib : '';

	print MAKE <<EOF;
$thunks_tinc: Makefile @alltmldeps
\t$^X utils/tmlink.pl -Iinclude/generic -o$thunks_tinc @alltml

CP=$^X Makefile.PL --cp
CPBIN=$^X Makefile.PL --cpbin
MD=$^X Makefile.PL --md
RM=$^X Makefile.PL --rm
RMDIR=$^X Makefile.PL --rmdir
CHMOD=$^X -e $SHQUOTE chmod 0755, \@ARGV $SHQUOTE

dirs:
\t\@echo Creating directories...
\t\@\$(MD) @alldirs


clean:
\t\$(RM) @allclean $PrimaTarget

realclean: clean
\t\$(RM) @allrealclean

bindist: all
\t$^X Makefile.PL --dist bin $DISTNAME

zipdist:
\t$^X Makefile.PL --dist zip $DISTNAME

tardist:
\t$^X Makefile.PL --dist tar $DISTNAME

dist: tardist
\t\$(RM) $DISTNAME.tar.gz
\t\@gzip -9 $DISTNAME.tar

prima: dirs $PrimaTarget $libtarget

$PrimaTarget: @allojects $extralibs
\t${ \eval{ return join ' ', ld_command_line( $PrimaTarget, @allojects)}}

test: all
\t\@echo Testing Prima v$DEFINES{PRIMA_VERSION}.$DEFINES{PRIMA_SUBVERSION}
\t\@$^X -I. -w test.pl

ppd:
\t\@$^X -e "print qq{<SOFTPKG NAME=\\"Prima\\" VERSION=\\"$DEFINES{PRIMA_VERSION},$DEFINES{PRIMA_SUBVERSION},0,0\\">\\n}. qq{\\t<TITLE>Prima</TITLE>\\n}. qq{\\t<ABSTRACT>Perl GUI toolkit</ABSTRACT>\\n}. qq{\\t<AUTHOR>Dmitry Karasik</AUTHOR>\\n}. qq{\\t<IMPLEMENTATION>\\n}. qq{\\t\\t<OS NAME=\\"$^O\\" />\\n}. qq{\\t\\t<ARCHITECTURE NAME=\\"$Config{myarchname}\\" />\\n}. qq{\\t\\t<CODEBASE HREF=\\"http://www.prima.eu.org/download/Prima-$DEFINES{PRIMA_VERSION}.$DEFINES{PRIMA_SUBVERSION}.tar.gz\\" />\\n}. qq{\\t</IMPLEMENTATION>\\n}. qq{</SOFTPKG>\\n}" > Prima.ppd

Makefile: Makefile.PL @Makefile_deps
\t\@echo Rebuilding Makefile...
\t\@$^X Makefile.PL $ARGV_STR
\t\@$MAKE
\t\@echo You are safe to ignore the following error...
\t\@false

EOF

	print MAKE <<DLLTOOL if $win32_use_dlltool;
$PrimaLib: $PrimaTarget
\t\@dlltool -l $PrimaLib -d win32/Prima.def -D PRIMA.DLL $PrimaTarget

DLLTOOL

	my %cp;
	my %cpbin;
	my @chmodx; # cygwin's File::Copy sucks, doesn't save permissions
	$cp{$PrimaTarget} = $INSTALL_DL;
	$cp{$PrimaLib}    = $INSTALL_DL;
	$cp{"Prima.pm"}   = $INSTALLSITEARCH;
	$cp{'.packlist'}  = $INSTALL_DL;
	push @chmodx, "$INSTALL_DL/$OS2DLLF$DL_EXT";
	push @allman, 'Prima.pm', manname( 'Prima.pm', 3, '');

	my $cwd = cwd();
	finddepth( sub {
		return if $File::Find::dir =~ /CVS/ || m/CVS/;
		$cp{qd("$File::Find::dir/$_")} = qd("$INSTALLSITEARCH/$File::Find::dir") unless -d $_;
		if ( $install_manuals && m/\.pm$/) {
			if ( open F, $_ ) {
				my $fn = "$File::Find::dir/$_";
				while ( <F> ) {
					next unless /^\=head1/;
					push @allman, qd($fn), manname( $fn, 3, '');
					last;
				}
				close F;
			}
		}
	}, 'Prima');

	delete $cp{qd($_)} for @executables;

	while (@allinstall) {
		my ( $src, $dst) = ( shift(@allinstall), shift(@allinstall));
		next if $src =~ /CVS/;
		$cp{$src} = $dst;
	}
	while ( <include/*.h>) {
		my $dst = $_;
		$dst =~ s/^include/CORE/;
		$cp{$_} = dirname( qd( "$INSTALL_LIB/$dst"));
	}
	while ( <include/$PLATFORM/*.h>) {
		my $dst = $_;
		$dst =~ s/^include/CORE/;
		$cp{$_} = dirname( qd( "$INSTALL_LIB/$dst"));
	}
	my $i = 0;

	sub dump_cmd
	{
		my ( $line, $sub, $command) = ( 0, shift, shift);
		print MAKE "\t\@\$($command) \\\n" if scalar @_;
		for ( @_) {
			my $call = $sub->($_);
			if (( $line++ % 20) == 19) {
				print MAKE "\n\t\@\$($command) \\\n";
			} elsif ( $line <= scalar @_ && $line > 1) {
				print MAKE " \\\n";
			}
			print MAKE "\t$call";
		}
		print MAKE "\n";
	}

	print MAKE "\ninstall: all\n";
	dump_cmd( sub { "$_[0] $cp{$_[0]}"}, 'CP', sort keys %cp);
	while ( scalar @allbins) {
		my ( $src, $dst) = ( shift @allbins, shift @allbins);
		$cp{qd( basename( $src, '.pl') . $SCRIPT_EXT)} = $dst;
		$cpbin{$src} = qd( "$dst/" . basename( $src, '.pl')) . $SCRIPT_EXT;
	}
	dump_cmd( sub { "$_[0] $cpbin{$_[0]}"}, 'CPBIN', sort keys %cpbin);
	dump_cmd( sub { $_[0] }, 'CHMOD', @chmodx);
	
	print MAKE "\n\t\@echo Updating config...\n";
	print MAKE "\t$^X Makefile.PL --updateconfig $INSTALLSITEARCH/Prima/Config.pm";
	print MAKE " $INSTALLSITEARCH/Prima.pm" if $DL_LOAD_FLAGS;

	my @man_to_remove;
	if ( $install_manuals) {
		my $cmd = "\t\@pod2man --lax --section=\%d \%s " . ( length( $Config{gzip} ) ? 
			"| $Config{gzip} -c > \%s.gz\n" : 
			"> \%s\n" );
		print MAKE "\n\t\@echo Installing man pages...\n";
		print MAKE "\t\@\$(MD) $INSTALL_MAN1 $INSTALL_MAN3\n";
		while ( scalar @allman ) {
			my ( $src, $dest) = splice( @allman, 0, 2);
			$dest =~ s/::/./g if ($Win32 || $cygwin);
			push @man_to_remove, $dest;
			$dest =~ m/([^\/]+)\.(\d+)$/;
			print  MAKE "\t\@echo $dest" . (length( $Config{gzip} ) ? '.gz' : ''). "\n";
			printf MAKE $cmd, $2, $src, $dest;
		}
		if ( length $Config{gzip}) { s/$/.gz/ for @man_to_remove }
	}

	print MAKE "\n\ndeinstall:\n";
	my %dirs_to_rm;
	dump_cmd( sub { 
		$dirs_to_rm{$cp{$_[0]}} = 1;
		$_[0] =~ m/(\\|\/)?([^\\|\/]+)$/; 
		my $rm = "$cp{$_[0]}$dir_sep$2";
		$packlist->{$rm}++;
		return $rm;
	}, 'RM', keys %cp);
	dump_cmd( sub { $_ }, 'RM', @man_to_remove);
	delete $dirs_to_rm{$INSTALLSITEARCH};
	delete $dirs_to_rm{$INSTALLSCRIPT};
	dump_cmd( sub { $_[0] }, 'RMDIR', sort { length($b) <=> length($a) } keys %dirs_to_rm);
	print MAKE "\n";
	print MAKE join( "\n", @footers), "\n";

	close MAKE;
	$packlist-> write('.packlist');
	print "\nAll done.  Now you can run $MAKETYPE.\n";
}

