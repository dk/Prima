#!env perl
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

package MY;
use strict;
use warnings;
use lib '.';
use Cwd;
use Config;
use DynaLoader;
use Prima::Gencls;
use ExtUtils::MakeMaker;
use File::Find;
use File::Path;
use File::Basename;
use File::Copy;

use vars qw(
	$ARGV_STR
	$COUTOFLAG
	$COUTEXEFLAG
	$CLIBPATHFLAG
	$CLINKPREFIX
	$LDLIBFLAG
	$LDOUTFLAG
	$TMPDIR
	$NULLDEV
	$SCRIPT_EXT
	$LD_LIB_EXT
	$LIB_EXT
	$LIB_PREFIX
	@LIBS
	@INCPATH
	@LIBPATH
	%DEFINES
	%PREREQ
	%PASSIVE_CODECS
	@ACTIVE_CODECS
	$DL_LOAD_FLAGS
	$DISTNAME
	$CWD
	$SHQUOTE
	$DEFFILE
	$OPTIMIZE
	%ALL_PM_INSTALL
	%ALL_MAN_INSTALL
);

use vars qw(
	%cmd_options
	%passthru_options
	$Win32
	$Win64
	$unix
	$cygwin
	$mingw
	$platform
	$flavor
	$path_sep
	$dir_sep
	%alldeps
	@pm_files
	@prima_files
	@pod_files
	@c_files
	@h_files
	@o_files
	@exe_files
	@cls_files
	@target_clean
	%cache_find_files
	$binary_prereq
	$compiler_type
	$compiler_version
	@Prima_exports
	$win32_use_dlltool
	$cygwin_fake_Slib
);

my $END;
END {
	print $END if defined $END;
};

my ($offset_makefile_log, $captured_makefile_log);
my $see_makefile_log = "(see also makefile.log for details)";

sub see_makefile_log
{
	my $ret = "$see_makefile_log\n";
	$ret .= "\n--------------------------------------------\n$captured_makefile_log\n"
		if defined $captured_makefile_log;
	return $ret;
}

sub usage
{
	print <<HELP;

$0 - configuration script for Prima

syntax:
    perl Makefile.PL [options] 

options:
    X11BASE           - path where X11 includes and libraries are installed
    WITH_XFT          - compile with/without Xft (default 1), X only
    WITH_ICONV        - compile with/without iconv (default 1), X only
    WITH_GTK2         - compile with/without GTK2 (default 1), X only
    CYGWIN_WINAPI     - Winapi build in cygwin environment
    DEBUG             - build with debug information
    EXTRA_LDFLAGS      - add extra ld line (i.e. -lgcc )
    EXTRA_CCFLAGS      - add extra cc line (i.e. -I/usr/local/include )

HELP
	exit;
}

usage if grep /^(-h|--help)$/, @ARGV;
if ( defined($ARGV[0]) and $ARGV[0] =~ /^--(.*)$/) {
	my $sub = MY-> can("command_$1");
	die "invalid command $ARGV[0]\n" unless $sub;
	shift @ARGV;
	$sub->(@ARGV);
	exit;
}

setup_argv();
setup_variables();
setup_environment();
setup_compiler();
setup_exports();
setup_defines();
setup_dl_loadflags();
setup_X11() if $unix;
setup_xlibs() if $unix;
setup_codecs();
create_codecs_c();
create_config_h();
create_config_pm();
setup_files();

sub setup_argv
{
	$ARGV_STR = join(' ', @ARGV);

	%cmd_options = (
		X11BASE       => undef,
		WITH_XFT      => 1,
		WITH_ICONV    => 1,
		WITH_GTK2     => 1,
		CYGWIN_WINAPI => 0,
		DEBUG         => 0,
		EXTRA_CCFLAGS => '',
		EXTRA_LDFLAGS => '',
		
		# service vars
		AUTOMATED_RUN => 0,
		DL_LOAD_FLAGS => undef,
	);

	%passthru_options = (
		DEFINE        => '',
		LIBS          => '',
		INC           => '',
		LD            => $Config{ld},
		LDDLFLAGS     => $Config{lddlflags},
	);
	
	@ARGV = grep {
		my ( $k, $v) = split( '=', $_, 2 );
		exists($cmd_options{$k}) ? ($cmd_options{$k} = $v, 0) : 1;
	} @ARGV;

	for ( @ARGV ) {
		my ( $k, $v) = split( '=', $_, 2 );
		next unless exists $passthru_options{$k};
		$passthru_options{$k} = $v;
	}
}

sub setup_variables
{
	$DL_LOAD_FLAGS = 0;
	if ( $^O =~ /mswin32/i) {
		$Win32 = 1;
		$platform = 'win32';
	} elsif ( $^O =~ /cygwin/i) {
		if ( $cmd_options{CYGWIN_WINAPI}) {
			$Win32 = 1;
			$platform = 'win32';
		} else {
			$unix = 1;
			$platform = 'unix';
		}
		$cygwin = 1;
	} else {
		$platform = 'unix';
		$unix = 1;
		$DL_LOAD_FLAGS = -1; # check later
	}

	$DL_LOAD_FLAGS = $cmd_options{DL_LOAD_FLAGS} if defined $cmd_options{DL_LOAD_FLAGS};

	$dir_sep = (( ( $path_sep = $Config{ path_sep}) eq ':') || ( defined $Config{ emxpath})) ? '/' : '\\';
}

sub extmap
{
	my $newext = shift;
	return map { my $x = $_; $x =~ s/\.\w+$/$newext/; $x } @_;
}

sub ffind
{
	my ( $mask, $fdir ) = @_;
	$fdir = '.' unless defined $fdir;
	my @ret;
	File::Find::finddepth( sub {
		return unless -f and m/$mask/;
		my $dir = $File::Find::dir;
		$dir =~ s/^\.[\\\/]?//;
		return if $dir =~ /^blib/;
		my $f = length($dir) ? "$dir/$_" : $_;
		push @ret, $f;
	}, $fdir);
	return @ret;
}

sub setup_files
{
	@prima_files   = ('Prima.pm', ffind( qr/./, 'Prima' ));
	@pod_files  = ffind( qr/./, 'pod' );
	@pm_files  = ffind( qr/\.pm$/ );
	@c_files    = ( 
		<*.c>,
		(grep { not $PASSIVE_CODECS{$_} } <img/*.c>),
		(grep { !m/xft.c/ || $cmd_options{WITH_XFT} } <$platform/*.c>),
	);
	@cls_files  = ( <*.cls> );
	@h_files    = ( 
		<include/*.h>,
		<include/generic/*.h>,
		<include/$platform/*.h>,
		map { "include/generic/$_" } extmap( '.h', @cls_files),
	);
	@o_files  = extmap( $Config{_o}, @c_files );

	@exe_files = (
		<utils/*.pl>,
		<Prima/VB/*.pl>
	);

	@target_clean = (
		"include/generic/*",
		"*$Config{_o}",
		"img/*$Config{_o}",
		"$platform/*$Config{_o}",
	);

	# I still like to have my scripts as .pl, but installed without
	# extension. Hack, hack, hack. See also corresponding part in the postamble
	if ($unix or $cygwin) {
		@exe_files = extmap('', @exe_files);
		push @target_clean, @exe_files;
	}

	%ALL_PM_INSTALL = (
		# PM
		( map { $_ => '$(INST_LIBDIR)/'. $_ } @prima_files ),
		# INC
		( map {
			my $k = $_;
			$k =~ s/^include\///;
			( $_ => '$(INST_LIBDIR)/Prima/CORE/' . $k )
		} @h_files ),
		# POD
		( map { 
			my $k = $_;
			$k =~ s/^pod\///;
			$_ => '$(INST_LIBDIR)/' . $k 
		} @pod_files ),
		# examples
		( map { $_ => '$(INST_LIBDIR)/Prima/' . $_ } <examples/*> ),
	);
	%ALL_MAN_INSTALL = (
		map {
			my $target = $_;
			$target =~ s/\//::/g;
			$target =~ s/\.\w+$//;
			$_ => '$(INST_MAN3DIR)/'. $target . '.$(MAN3EXT)'
		}
		(@pm_files, grep { /pod$/ } @pod_files)
	);
}

sub setup_environment
{
	if ( $Config{ccname} =~ /\bgcc/ ) {
		$compiler_type = 'gcc';
	} else {
		$compiler_type = $Config{ccname};
	}

	if ( $Win32 and not $cygwin ) {
		$SCRIPT_EXT = '.bat';
		$SHQUOTE    = '"';
		if ($compiler_type eq 'gcc') {
			$win32_use_dlltool = $Config{dlltool} || 'dlltool';
			$mingw = 1;
		}
	} else {
		$SCRIPT_EXT = '';
		$SHQUOTE    = "'";
	}

	$OPTIMIZE = $Config{optimize};

	if ( $compiler_type eq 'cl') {
		$COUTOFLAG    = '-Fo';
		$COUTEXEFLAG  = '-Fe';
		$CLIBPATHFLAG = '/LIBPATH:';
		$CLINKPREFIX  = '/link';
		$CLINKPREFIX .= " $1" if $Config{libs} =~ /(bufferoverflowU.lib)/i;
		$LDLIBFLAG    = '';
		$LDOUTFLAG    = '/OUT:';
		$LD_LIB_EXT   = '.lib';
		$OPTIMIZE     = '-Zi' if $cmd_options{DEBUG};
		# link flag is /DEBUG, but we don't care because ActiveState has it on by default anyway
	}
	else {
		$COUTOFLAG    = '-o ';
		$COUTEXEFLAG  = '-o ';
		$CLIBPATHFLAG = '-L';
		$CLINKPREFIX  = '';
		$LDLIBFLAG    = '-l';
		$LDOUTFLAG    = '-o ';
		$LD_LIB_EXT   = '';
		$OPTIMIZE     = '-g' if $cmd_options{DEBUG};
	}
	
	$DEFFILE    = 'Prima.def';
	$LIB_EXT    = ($cygwin ? '.dll' : '') . $Config{ _a};
	$LIB_PREFIX = ($cygwin || $mingw) ? 'lib' : '';
	
	open F, 'Prima.pm' or die "Cannot open Prima.pm:$!\n";
	my ($ver1, $ver2);
	while (<F>) {
		next unless m/\$VERSION[^\.\d]*(\d+)\.([_\d]+)/;
		$ver1 = $1, $ver2 = $2, last;
	}
	close F;
	die "Cannot find VERSION string in Prima.pm\n" unless defined $ver1;
	print "Version: $ver1.$ver2\n";

	my $PATCHLEVEL = defined($Config{PATCHLEVEL}) ? $Config{PATCHLEVEL} : $Config{PERL_PATCHLEVEL};
	my $SUBVERSION = defined($Config{SUBVERSION}) ? $Config{SUBVERSION} : $Config{PERL_SUBVERSION};
	my $REVISION   = defined($Config{REVISION})   ? $Config{REVISION}   : $Config{PERL_REVISION};

	%DEFINES = (
		PRIMA_VERSION    => $ver1,
		PRIMA_SUBVERSION => $ver2,
		PERL_PATCHLEVEL  => $PATCHLEVEL,
		PERL_SUBVERSION  => $SUBVERSION,
		PRIMA_CORE       => 1,
		PERL_POLLUTE     => 1,
		PRIMA_DEBUG      => 0,
	);
	
	$TMPDIR  = $ENV{ TMPDIR} || $ENV{ TEMPDIR} || ( $Win32 ? ( $ENV{ TEMP} || "$ENV{SystemDrive}\\TEMP") : "/tmp");
	$NULLDEV = 'makefile.log';

	print "Flavor: ";
	$flavor = $^O;
	if ( $Win32 and not $cygwin ) {
		if ( $Config{myuname} =~ /strawberry\-?perl/i) {
			$flavor = 'strawberry';
		} elsif ( `perl -V` =~ /activeperl/i) {
			$flavor = 'activestate';
		}
	}
	$flavor =~ s/\s/_/g;
	print "$flavor\n";

	$flavor = 'sb32' if $flavor eq 'strawberry';
	$flavor = 'as32' if $flavor eq 'activestate';
	$flavor .= '.' . ( $cmd_options{CYGWIN_WINAPI} ? 'win32' : 'x11' ) if $flavor eq 'cygwin';
	
	$DISTNAME = "Prima-$ver1.$ver2-$flavor-$REVISION.$PATCHLEVEL.$SUBVERSION";
	print "Build: $DISTNAME\n";
   
	if ( $cygwin and $] > 5.010 ) {
		# 5.10 was shipped with a hack in ExtUtils/Liblist/Kid.pm where having auto/Prima.dll was ok
		# 5.14 is not patched that way, do it ourselves
		$cygwin_fake_Slib = 'SlibPrima' . ( $Config{lib_ext} || '.a' );
	} else {
		$cygwin_fake_Slib = '';
	}
}

sub qtilde
{
	my $path = shift;
	return $path unless $path =~ s/^~//;
	die "** path '~$path' begins with '~' but no HOME is set\n" unless exists $ENV{HOME};
	return $ENV{HOME} . $path;
}

sub qd
{
	my ( $path_str) = @_;
	$path_str =~ s[/][$dir_sep]g;
	return $path_str;
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

sub cc_command_line
{
	my ( $srcf, $objf, $exef, $compile_only, $dl) = @_;
	my $ccflags = $Config{ccflags};
	$ccflags =~ s/\b\-W(all|error|\d)w*//i;
	my $cc = "$Config{cc} $ccflags";
	$cc .= " $cmd_options{EXTRA_CCFLAGS}"  if length $cmd_options{EXTRA_CCFLAGS};
	$cc .= " $passthru_options{DEFINE}" if length $passthru_options{DEFINE};
	$cc .= " $Config{cccdlflags}" if $dl;
	$cc .= " $Config{ccdlflags}" if $dl && !$compile_only;
	$cc .= " -c " if $compile_only;
	$cc .= ' ' . join(' ', map { "-I$_" } @INCPATH);
	$cc .= " $passthru_options{INC}" if length $passthru_options{INC};
	$cc .= $compile_only ? " $COUTOFLAG$objf" : " $COUTEXEFLAG$exef";
	$cc .= " $COUTOFLAG$objf" if $compiler_type eq 'cl' && !$compile_only;
	$cc .= ' ' . join(' ', map { "$CLIBPATHFLAG$_"} @LIBPATH) unless $compile_only || ( $compiler_type eq 'cl');
	$cc .= " $srcf";
	return $cc if $compile_only;
	$cc .= " $CLINKPREFIX";
	$cc .= ' ' . join(' ', map { "\"$CLIBPATHFLAG\\\"$_\\\"\"" } @LIBPATH) if $compiler_type eq 'cl';
	$cc .= ' ' . join(' ', map { "$LDLIBFLAG$_$LD_LIB_EXT"} @LIBS);
	$cc .= " $passthru_options{LIBS}" if length $passthru_options{LIBS};
	$cc .= " $cmd_options{EXTRA_LDFLAGS}" if length $cmd_options{EXTRA_LDFLAGS};
	return $cc;
}

sub ld_command_line
{
	my ( $dstf) = shift;
	my $ld = "$passthru_options{LD} $passthru_options{LDDLFLAGS}";
	$ld .= " $cmd_options{EXTRA_LDFLAGS}" if length $cmd_options{EXTRA_LDFLAGS};
	$ld .= " $LDOUTFLAG$dstf @_";
	$ld .= ' ' . join(' ', map { "$LDLIBFLAG$_$LD_LIB_EXT"} @LIBS);
	return $ld;
}

sub null_output
{
	open OLDSTDOUT, ">&STDOUT" or die "STDOUT dup failed: $!";
	open OLDSTDERR, ">&STDERR" or die "STDERR dup failed: $!";
#	$NULLDEV = ( $Win32) ? "CON" : "/dev/tty";
#	$NULLDEV = ( $Win32) ? "NUL" : "/dev/null";
	if ( $^O !~ /linux/) {
		close STDOUT;
		close STDERR;
	}
	open STDOUT, ">>$NULLDEV" or die "STDOUT redirect failed: $!";
	open STDERR, ">&STDOUT" or die "STDERR redirect failed: $!";

	if ( -f $NULLDEV ) {
		$offset_makefile_log = -s $NULLDEV;
		undef $captured_makefile_log;
	}
}

sub restore_output
{
	if ( $^O !~ /linux/) {
		close STDOUT;
		close STDERR;
	}
	open STDOUT, ">&OLDSTDOUT" or die "STDOUT restoration failed: $!";
	open STDERR, ">&OLDSTDERR" or die "STDERR restoration failed: $!";
	close OLDSTDOUT;
	close OLDSTDERR;

	if ( -f $NULLDEV ) {
		if ( open MAKEFILELOG, '<', $NULLDEV) {
			binmode MAKEFILELOG;
			seek MAKEFILELOG, $offset_makefile_log, 0;
			local $/;
			$captured_makefile_log = <MAKEFILELOG>;
			close MAKEFILELOG;
		}
	}
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

sub compile
{
	my ( $text, $compile_only, @extra) = @_;
	my $tmpsrc = qd( tempfile( "$TMPDIR/pmts%04d.c"));
	my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$Config{_o}"));
	my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$Config{_exe}"));
	my @tmpextras = ( $tmpsrc, $tmpsrc);
	$tmpextras[0] =~ s/\.[^\.+]$/.ilk/;
	$tmpextras[1] =~ s/\.[^\.+]$/.pdb/;
	unlink @tmpextras; # leftovers are toxic to msvc

	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
	print TMPSRC $text;
	close TMPSRC;

	null_output;
	my $cc = cc_command_line( $tmpsrc, $tmpo, $tmpexe, $compile_only || 0);
	$cc .= ' ' . join(' ', @extra) if @extra;
	print STDERR "$cc\n";
	my $rc = system($cc);
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
	my $tmpo = qd( tempfile( "$TMPDIR/pmts%04d$Config{_o}"));
	my $tmpexe = qd( tempfile( "$TMPDIR/pmts%04d$Config{_exe}"));
	my @tmpextras = ( $tmpsrc, $tmpsrc);
	$tmpextras[0] =~ s/\.[^\.+]$/.ilk/;
	$tmpextras[1] =~ s/\.[^\.+]$/.pdb/;
	unlink @tmpextras; # leftovers are toxic to msvc

	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
	print TMPSRC $text;
	close TMPSRC;

	null_output;
	my $cc = cc_command_line( $tmpsrc, $tmpo, $tmpexe, 0);
	print STDERR "$cc\n";
	my $rc = system($cc);
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
	die "have_func() without any header is deprecated " unless @headers;
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
	$tmpo =~ s/\.c$/\.$Config{_o}/;
	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
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
	my $ccrc = system( "$Config{cc} -c $tmpsrc");
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

sub setup_compiler
{
	print "Compiler: $compiler_type\n";

	print "Checking if can compile... ";
	compile('int a;', 1) or die "no " . see_makefile_log;
	print "yes\n";

	print "Checking if can link... ";
	compile( <<EOF, 0) or die "no " . see_makefile_log;
int
main()
{
   return 0;
}
EOF
	print "yes\n";

	if ( $compiler_type eq 'cl' ) {
		print "Checking MSVC version... ";
		$compiler_version = compile_and_run(<<'MSCVER');
#include <stdio.h>
int main() { 
	printf("%d\n", _MSC_VER); 
	return 0;
}
MSCVER
		print "$compiler_version\n";

		# kill annoying warnings
		if ( $compiler_version < 1400) {
			$OPTIMIZE .= " -D_CRT_SECURE_NO_DEPRECATE";
		}
		if ( $compiler_version >= 1600 ) {
			$OPTIMIZE .= " -D_CRT_SECURE_NO_WARNINGS";
			$OPTIMIZE .= " /wd4244"; #  '=' : conversion from 'Bool' to 'char', possible loss of data
			$OPTIMIZE .= " /wd4267"; #  '=' : conversion from 'size_t' to 'int', possible loss of data
			$OPTIMIZE .= " /wd4018"; #  '<' : signed/unsigned mismatch";
		}
	}
	
	if ($Win32) {
		print "Checking windows subsystem...";
		$Win64 = have_define('_WIN64');
		print $Win64 ? " 64" : " 32";
		print " bits\n";
		$DISTNAME =~ s/(mswin|sb|as)32/${1}64/i if $Win64;
	}

	@INCPATH = (
		'include',
		'include/generic',
	);

	@LIBS = qw(gdi32 mpr winspool comdlg32) if $Win32; # add more when appropriate
	push @LIBPATH, '/usr/lib/w32api' if $cygwin;
	if ($^O eq 'solaris') {
		push @LIBPATH, '/opt/csw/lib';
		$OPTIMIZE .= ' -Wall' if $compiler_type eq 'gcc';
	}
}

sub setup_defines
{
	have_header( "io.h");
	have_header( "unistd.h");
	have_header( "strings.h");

	my @int_types = qw(int8_t int16_t int32_t int64_t);
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

	if ( $unix) {
		have_header( "sys/ipc.h", "sys/types.h");
		have_header( "sys/shm.h", "sys/types.h");
		have_header( "X11/extensions/shape.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
		have_header( "X11/extensions/XShm.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
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
}

sub setup_dl_loadflags
{
	return if $DL_LOAD_FLAGS >= 0;

	print "Determining dl_load_flags... ";
	
	local @INCPATH = ( qtilde($Config{archlib}) . qd( "/CORE"));

	my $c1  = qd( tempfile( "$TMPDIR/pmts%04d.c"));
	$c1 =~ m/pmts([^\.]*).c$/;
	my ( $n1, $n2) = ( $1, sprintf("%04d", 1 + $1));

	my $o1  = qd( "$TMPDIR/pmts$n1$Config{_o}");
	my $o2  = qd( "$TMPDIR/pmts$n2$Config{_o}");
	my $dl1 = qd( "$TMPDIR/pmts$n1.$Config{dlext}");
	my $dl2 = qd( "$TMPDIR/pmts$n2.$Config{dlext}");
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

	my $cc1 = cc_command_line( $c1, $o1, $dl1, 1, 1);
	my $cc2 = cc_command_line( $c2, $o2, $dl2, 1, 1);
	my $ld1 = ld_command_line( $dl1, $o1);
	my $ld2 = ld_command_line( $dl2, $o2);
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
	print STDERR "$cc1\n";
	goto FAIL if system($cc1);
	print STDERR "$ld1\n";
	goto FAIL if system($ld1);
	print STDERR "$cc2\n";
	goto FAIL if system($cc2);
	print STDERR "$ld2\n";
	goto FAIL if system($ld2);
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

sub setup_X11
{
	# find X11 include files
	print "Checking for X11 headers...";
	push @INCPATH, "$cmd_options{X11BASE}/include"
		if defined($cmd_options{X11BASE}) and -d "$cmd_options{X11BASE}/include";
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
		return unless $unix;
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
		(defined($cmd_options{X11BASE}) ? "$cmd_options{X11BASE}/lib" : ()), 
		"/usr/X11R6/lib", 
		"/usr/X11/lib",
		"/usr/X/lib",
		"/usr/openwin/lib",
		"/opt/X11/lib",
        "/usr/local/lib"
	);
	# using /usr/X11R6/lib64 ?
	unshift @libpath, map { s/lib$/lib64/; $_ } grep { /lib$/ } @libpath
		if $Config{intsize} == 8;

	print "Checking for library X11... ";
	my $libpath = find_lib( @libpath);
	unless ( defined $libpath) {
		print "no\n";
		unless ( $unix) {
			pop @INCPATH if @INCPATH and $INCPATH[-1] eq $incpath;
			return;
		}
		warn 
			"Prima needs X11 libraries for compilation! ".
			"Set X11BASE='/path/to/X' ".
			"if you have a non-standard path to X. $see_makefile_log\n";
		exit(0);
	}

	print "yes";
	if ( -d $libpath) {
		print ", in $libpath";
		push @LIBPATH, $libpath;
	}
	print "\n";

	if ( $unix) {
		push @LIBS, 'X11';
		if (defined find_lib( "Xext", '', '')) {
			print "Xext library found.\n";
			push @LIBS, "Xext";
		}
	}
}

sub setup_xlibs
{
	if ( $cmd_options{WITH_XFT}) {
		my $HAVE_XFT = 0;
		my $NEED_XFT = 4;
		my @pre_xft_libs = @LIBS;

		my @ft_incpaths = ( "",
			( defined($cmd_options{X11BASE}) ? qd("$cmd_options{X11BASE}/include/freetype2") : ()),
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
		my $have_ft2build_h = defined find_header( qd("ft2build.h"), @ft_incpaths );

		print "Checking for presence of freetype/freetype.h... ";
		my $incpath;
		if ($have_ft2build_h) {
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
				$DEFINES{HAVE_FREETYPE_FREETYPE_H} = 1;
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

		$cmd_options{WITH_XFT} = 0 unless $HAVE_XFT == $NEED_XFT;
		@LIBS = @pre_xft_libs unless $cmd_options{WITH_XFT};
	}

	$cmd_options{WITH_ICONV} = 0 unless $cmd_options{WITH_XFT}; # iconv is used for xft only
	if ( $cmd_options{WITH_ICONV} && have_header( "iconv.h")) {
		print "Checking for presence of libiconv... ";
		if ( defined find_lib( 'iconv', '', '')) {
			push @LIBS, 'iconv';
			print "yes\n";
		} else {
			my $ok = compile( "#include <iconv.h>\nint main() { iconv_close(0); return 0; }\n");
			if ( $ok ) {
				print "no, but works as part of libc\n";
			} else {
				$DEFINES{HAVE_ICONV_H} = undef;
				$cmd_options{WITH_ICONV} = 0;
				print "no\n";
			}
		}
	} else {
		$cmd_options{WITH_ICONV} = 0;
	}
	
	if ( $cmd_options{WITH_GTK2}) {
		print "Checking for presence of gtk2... ";
		my $pkg_config = `which pkg-config`;
		chomp $pkg_config;
		unless ( -f $pkg_config) {
			$cmd_options{WITH_GTK2} = 0;
			print "no pkg-config, no gtk2\n";
		}

		if ( $cmd_options{WITH_GTK2}) {
			my $vers = `pkg-config --modversion gtk+-2.0`;
			chomp $vers;
			if ( $vers =~ /^[\d\.]+$/) {
				$DEFINES{GTK_VERSION} = "\"$vers\"";
				print "yes, $vers\n";
			} else {
				$cmd_options{WITH_GTK2} = 0;
				print "no\n";
			}
		}

		if ( $cmd_options{WITH_GTK2}) {
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
				$cmd_options{WITH_GTK2} = 0;
				@LIBS     = @savelib;
				@INCPATH  = @saveinc;
				print "no\n";
			}
		}
	}

	if ( have_header( "X11/extensions/Xrandr.h")) {
		print "Checking for presence of libXrandr... ";
		if ( defined find_lib( 'Xrandr', '', '')) {
			push @LIBS, 'Xrandr';
			print "yes\n";
		} else {
			$DEFINES{HAVE_X11_EXTENSIONS_XRANDR_H} = undef;
			print "no\n";
		}
	}
	
	print "Using Xft library\n" if $cmd_options{WITH_XFT};
	print "Using iconv library\n" if $cmd_options{WITH_ICONV};
	print "Using gtk2 library\n" if $cmd_options{WITH_GTK2};
	print "Using Xrandr library\n" if $DEFINES{HAVE_X11_EXTENSIONS_XRANDR_H};
		
}

sub generate_win32_def
{
	open PRIMADEF, ">$DEFFILE" or die "Cannot create $DEFFILE: $!";
	print PRIMADEF <<EOF;
LIBRARY Prima
EXPORTS
EOF
	if ( $compiler_type eq 'bcc32') {
		print PRIMADEF map { "\t_$_\n\t$_=_$_\n"} @Prima_exports;
	}
	else {
		print PRIMADEF map { "\t$_\n\t_$_ = $_\n"} @Prima_exports;
	}
	close PRIMADEF;
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

sub setup_exports
{
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
	generate_win32_def() if $Win32;
}

sub setup_codecs
{
	# see if Prima::codecs:: is installed
	my ( $prereq, $have_binary_prereq);
	$prereq = 'win32' if $Win32 and not $cygwin;
	$prereq = 'win64' if $Win64 and not $cygwin;
	if ( $prereq) {
		print "Checking for Prima::codecs::$prereq... ";
		eval "use Prima::codecs::$prereq;";
		unless ( $@) {
			print "yes\n";
			$have_binary_prereq++;
			my $f = $INC{"Prima/codecs/$prereq.pm"};
			$f =~ s/.pm$//;
			push @LIBPATH, qd("$f/lib");
			push @INCPATH, qd("$f/include");

		} else {
			print "no\n";
		}
	}

	# finding image codecs
	my %libs = map { $_ => 1 } @LIBS;
	my @codecs;
	my @builtin_codecs;
	while ( <img/codec_*.c>) {
		if ( m/codec_(bmp)/) {
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
		} elsif ( $codec eq 'X11') {
			$DEFINES{EMULATE_X11_CODEC} = 1;
			push( @ACTIVE_CODECS, $codec);
			print "no, using built-in\n";
		} else {
			$PASSIVE_CODECS{$fn} = 1;
			push @warn_codecs, $codec;
			print "no\n";
		}
	}

	unless ( @ACTIVE_CODECS) {
		$binary_prereq = $prereq;
		$PREREQ{"Prima::codecs::$prereq"} = 0;
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
}

sub create_codecs_c
{
	print "Creating img/codecs.c\n";
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

sub create_config_h
{
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
}

sub _quote
{
	my $name = shift;
	$name =~ s/'/\\'/g;
	return \ "'$name'";
}

sub _quotepath { _quote(qd(@_)) }

sub create_config_pm
{
	my $cwd = cwd;
	my $qcwd = qd($cwd);
	my $ifs = $dir_sep;

	# includes
	my @ip = map { qd($_) } @INCPATH;
	$ip[0] = "$cwd${ifs}include";
	$ip[1] = "$cwd${ifs}include${ifs}generic";
	my $ipp = join(',', map {"\'$_\'"} @ip);
	my $inc    = join(' ', map { "-I$_" } @ip);
	$ip[0] = '$(lib)' . qd("/Prima/CORE");
	$ip[1] = '$(lib)' . qd("/Prima/CORE/generic");
	my $ippi = join(',', map {"\'$_\'"} @ip);
	my $inci = join(' ', map { "-I$_" } @ip);

	# libs
	my @libpath = @LIBPATH;
	my @libs    = @LIBS;
	unless ( $unix or $compiler_type eq 'gcc') {
		push @libpath, "$cwd/auto/Prima";
		push @libs, "Prima$LD_LIB_EXT";
	}
	my $libpath = qd(join( ',', map {"'$_'"} @libpath));
	unless ( $unix or $compiler_type eq 'gcc') {
		$libpath[-1] = '$(lib)/auto/Prima';
	}
	my $libpathi = qd(join( ',', map {"'$_'"} @libpath));
	my $ldlibs  = qd(join( ',', map {"'$_'"} @libs));

	my ($libs, $libsi) = ('','');
	if ( $cygwin) {
		$libs  = "-L$cwd/blib/arch/auto/Prima -lPrima";
		$libsi = '-L$(lib)/auto/Prima -lPrima';
	} elsif ( $Win32) {
		$libs  = qd("$cwd/blib/arch/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT");
		$libsi = '$(lib)' . qd("/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT");
	}

	open F, "> Prima/Config.pm" or die "cannot open Prima/Config.pm:$!\n";
	print F <<CONFIG;
# This file was automatically generated.
# Do not edit, you'll loose your changes anyway.
package Prima::Config;
use vars qw(%Config %Config_inst);

%Config_inst = (
	incpaths              => [ $ippi ],
	gencls                => '\$(bin)${ifs}gencls$SCRIPT_EXT',
	tmlink                => '\$(bin)${ifs}tmlink$SCRIPT_EXT',
	libname               => '\$(lib)${ifs}auto${ifs}Prima${ifs}${LIB_PREFIX}Prima$LIB_EXT',
	dlname                => '\$(lib)${ifs}auto${ifs}Prima${ifs}Prima.$Config{dlext}',
	ldpaths               => [$libpathi],

	inc                   => '$inci',
	libs                  => '$libsi',
);

%Config = (
	ifs                   => '\\$ifs',
	quote                 => '\\$SHQUOTE',
	platform              => '$platform',
	incpaths              => [ $ipp ],
	gencls                => ${_quotepath("$cwd/blib/script/gencls$SCRIPT_EXT")},
	tmlink                => ${_quotepath("$cwd/blib/script/tmlink$SCRIPT_EXT")},
	scriptext             => ${_quote($SCRIPT_EXT)},
	genclsoptions         => '--tml --h --inc',
	cobjflag              => ${_quote($COUTOFLAG)},
	coutexecflag          => ${_quote($COUTEXEFLAG)},
	clinkprefix           => ${_quote($CLINKPREFIX)},
	clibpathflag          => ${_quote($CLIBPATHFLAG)},
	cdefs                 => [],
	libext                => ${_quote($LIB_EXT)},
	libprefix             => ${_quote($LIB_PREFIX)},
	libname               => ${_quotepath("$cwd/blib/arch/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT")},
	dlname                => ${_quotepath("$cwd/blib/arch/auto/Prima/Prima.$Config{dlext}")},
	ldoutflag             => ${_quote($LDOUTFLAG)},
	ldlibflag             => ${_quote($LDLIBFLAG)},
	ldlibpathflag         => ${_quote($CLIBPATHFLAG)},
	ldpaths               => [$libpath],
	ldlibs                => [$ldlibs],
	ldlibext              => ${_quote($LD_LIB_EXT)},
	inline                => ${_quote($DEFINES{__INLINE__})},
	dl_load_flags         => $DL_LOAD_FLAGS,

	inc                   => '$inc',
	define                => '',
	libs                  => '$libs',
);

1;
CONFIG
	close F;
}

# executed from inside makefiles

sub command_postinstall
{
	my %opt = map { split /=/, $_, 2 } @_;

	if ( $opt{slib} ) {
		my $f = "$opt{dest}/auto/Prima/$opt{slib}";
		open F, ">", $f or warn "** warning: Cannot write to a fake lib '$f': Prima extensions won't build\n";
		close F;
	}
	
	my $fn_cfg = "$opt{dest}/Prima/Config.pm";
	print "Updating config $fn_cfg\n";
	open F, $fn_cfg or die "cannot open $fn_cfg:$!\n";
	open FF, "> $fn_cfg.tmp" or die "cannot open $fn_cfg.tmp:$!\n";
	my ( $c_state, $ci_state) = (0,0);
	my (%ci, %vars);

	%vars = %opt;
	if ( $^O =~ /mswin32/i) {
		s/\//\\/g for values %vars;
	}

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
			} elsif ( m/^\s*(\S+)\s*/ ) {
				my $k = $1;
				s/\$\((\w+)\)/$vars{$1}/g;
				$ci{$k} = $_;
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
}

sub command_dl
{
	$DL_LOAD_FLAGS = shift;
	my $f = "blib/lib/Prima.pm";

	open F, $f or die "cannot open $f:$!\n";
	local $/;
	my $ct = <F>;
	close F;
	
	$ct =~ m/dl_load_flags\s*\{\s*0x0(\d)/;
	return if $1 eq $DL_LOAD_FLAGS;

	print "Setting dl_load_flags=$DL_LOAD_FLAGS in $f\n";
	$ct =~ s/(dl_load_flags\s*\{\s*)0x00/${1}0x0$DL_LOAD_FLAGS/;
	# open_rw(\*F, $f);
	open F, "> $f.tmp" or die "Cannot open $f.tmp:$!";
	print F $ct;
	close F;
	unlink $f;
	rename "$f.tmp", $f;
}

sub command_bindist
{
	$CWD = cwd();
	$DISTNAME = shift;

	sub clean_dist
	{
		my @dirs;
		return unless -d $DISTNAME;
		print "Cleaning...\n";
		finddepth( sub {
			my $f = "$File::Find::dir/$_";
			-d($f) ? push(@dirs, $f) : unlink($f);
		}, "$CWD/$DISTNAME");
		rmdir $_ for sort {length($b) <=> length($a)} @dirs;
		rmdir $DISTNAME;
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
			($_ eq 'Makefile' && $File::Find::dir eq $CWD) || 
			$_ eq 'makefile.log' || 
			m/^\./;
		return if /\.(pdb|ncb|opt|dsp|dsw)$/i; # MSVC
		my $f = "$File::Find::dir/$_";
		return if $f =~ /include.generic|\.git|blib|dll.base|dll.exp|Prima.bs|Prima.def/;
		return if $f =~ /\.(c|cls|h)$/i;
		return if $f =~ /$CWD.(img|include|win32|unix|Makefile.PL|Makefile.old)/i;
		if ( -d $f) {
			$f =~ s/^$CWD/$DISTNAME/;
			push @dirs, $f;
		} else {
			return if $f =~ m/$Config{_o}$/;
			push @files, $f;
		}
	}, $CWD);

	print "Creating directories...\n";
	push @dirs, "$DISTNAME/auto/Prima";
	for ( @dirs) {
		next if -d $_;
		cleanup( "Can't mkdir $_") unless mkpath $_;
	}

	print "Copying files...\n";
	for ( @files) {
		my $f = $_;
		$f =~ s/^$CWD/$DISTNAME/;
		cleanup("Error copying $_ to $_") unless copy $_, $f;
	}
	for (<blib/arch/auto/Prima/*>) {
		next if m/(\.exists|Prima\.bs|Prima\.pdb)$/;
		my $f = $_;
		$f =~ s[^blib/arch][$DISTNAME];
		cleanup("Error copying $_ to $_") unless copy $_, $f;
	}
	if ($^O eq 'cygwin') {
		system "strip $DISTNAME/auto/Prima/Prima.$Config{dlext}" ; # it's 27 MB!
		if ( $] > 5.010 ) {
			$cygwin_fake_Slib = 'SlibPrima' . ( $Config{lib_ext} || '.a' );
			open F, '>', "$DISTNAME/auto/Prima/$cygwin_fake_Slib";
			close F;
		}
	}

	my $zipname = "$DISTNAME.zip";
	unlink $zipname;
	unlink "$DISTNAME/$zipname";
	system "zip -r $zipname $DISTNAME";

	clean_dist;
}

sub command_cpbin
{
	my ($from, $to) = @_;
	local $/;
	open FROM, '<', $from or die "Cannot open $from:$!\n";
	open TO, '>',   $to or die "Cannot open $to:$!\n";
	print TO "#!$Config{perlpath} -w\n";
	print TO <FROM>;
	close TO;
	close FROM;
	chmod 0755, $to;
}

# EU::MM overridden stuff

sub c_o
{
	my $t = shift-> SUPER::c_o(@_);
	unless ( $t =~ /.c\$\(OBJ_EXT\):\n\t.*\$\*\$\(OBJ_EXT\)/ ) {
		$t =~ s/(\.c\$\(OBJ_EXT\):\n\t.*)/$1 $COUTOFLAG\$*\$(OBJ_EXT)/;
	}
	return $t;
}

sub special_targets
{
	my $self = shift;
	my $t = $self->SUPER::special_targets(@_);
	$t .= <<RERUN if $binary_prereq and not $cmd_options{AUTOMATED_RUN};
all ::
\t\@echo Rebuilding Makefile...
\t\@\$(RM_F) Makefile
\t\@$^X Makefile.PL $ARGV_STR AUTOMATED_RUN=1
\t\@$Config{make}

RERUN
	return $t;
}

sub postamble
{
	my $self = shift;
	my $t = $self->SUPER::postamble(@_);

	my @alltml;
	my @alltmldeps;
	
    print "Finding .cls dependencies...\n";
	for my $clsfile ( @cls_files) {
		my ( $base ) = $clsfile =~ m/^(.*?).cls$/;
		my $ancestors = join(' ', map { "include/generic/$_.h $_.cls" } gencls( $clsfile, depend => 1));
	  	$t .= <<H;

include/generic/$base.h: \$(FIRST_MAKEFILE) $base.cls $ancestors
\t$^X -I. utils/gencls.pl --inc --h --tml $clsfile include/generic

H
		push @alltml, "include/generic/$base.tml";
		push @alltmldeps, "include/generic/$base.h";
	}

	$t .= <<H;
include/generic/thunks.tinc: \$(FIRST_MAKEFILE) @alltmldeps
\t$^X utils/tmlink.pl -Iinclude/generic -oinclude/generic/thunks.tinc @alltml

H

    my %dirs;
	for my $cfile ( @c_files ) {
        my ( $dir ) = ( $cfile =~ /^(\w+)\// );
        $dir = 'root directory' unless $dir;
	    print "Finding .c dependencies in $dir...\n" unless $dirs{$dir}++;

		my ( $base ) = $cfile =~ m/^(.*?).c$/;
		my @deps = find_cdeps( $cfile );
	  	$t .= <<H

$base$Config{_o}: \$(FIRST_MAKEFILE) $cfile @deps

H
	}

	my ( $pm_deinstall, $pm_deinstall_dir, %pm_deinstall_dir) = ('');
	my @rm;
	for ( values %ALL_PM_INSTALL ) {
		my $f = $_;
		$f =~ s/INST_LIBDIR/DESTINSTALLSITEARCH/;
		push @rm, $f;
		$f =~ s/[-\w\.]*$//;
		$pm_deinstall_dir{$f} = 1;
	}
	$pm_deinstall_dir{'$(DESTINSTALLSITEARCH)/Prima/sys'}  = 1;
	$pm_deinstall_dir{'$(DESTINSTALLSITEARCH)/auto/Prima'} = 1;
	for ( values %ALL_MAN_INSTALL ) {
		my $f = $_;
		$f =~ s/INST_MAN3DIR/DESTINSTALLSITEMAN3DIR/;
		push @rm, $f;
	}
	push @rm, 
		'$(DESTINSTALLSITEMAN3DIR)/gencls.$(MAN3EXT)',
		'$(DESTINSTALLSITEMAN1DIR)/VB.$(MAN1EXT)',
		'$(DESTINSTALLSITEMAN1DIR)/cfgmaint.$(MAN1EXT)',
		'$(DESTINSTALLSITEARCH)/auto/Prima/' . $LIB_PREFIX . '$(BASEEXT)$(LIB_EXT)',
		'$(DESTINSTALLSITEARCH)/auto/Prima/$(DLBASE).$(DLEXT)',
		'$(DESTINSTALLSITEARCH)/auto/Prima/$(BASEEXT).bs',
		'$(DESTINSTALLSITEARCH)/auto/Prima/Prima.exp',
		'$(DESTINSTALLSITEARCH)/auto/Prima/Prima.pdb',
		;
	while (@rm) {
		my @part = splice(@rm, 0, 20);
		$pm_deinstall .= "\t\$(ABSPERL) -e ${SHQUOTE}unlink \@ARGV${SHQUOTE} @part\n";
	}
	chomp $pm_deinstall;
	$pm_deinstall_dir = 
		"\t\$(ABSPERL) -e ${SHQUOTE}rmdir for \@ARGV${SHQUOTE} ".
		join( ' ', sort { length $b <=> length $a } keys %pm_deinstall_dir)
		;
	$t .= <<H;

bindist: all
\t$^X $0 --bindist $DISTNAME

devclean:
\t\$(RM_F) *$Config{_o} img/*$Config{_o} $platform/*$Config{_o}

deinstall:
$pm_deinstall
$pm_deinstall_dir 

H

	if ($unix or $cygwin) {
		$t .= <<H for @exe_files; # .pl -> . 
$_: $_.pl
\t$^X $0 --cpbin $_.pl $_

H
	}

	return $t;
}

sub dynamic_lib
{
	my $self = shift;
	my $t = $self->SUPER::dynamic_lib(@_);
	if ( $win32_use_dlltool ) {
		my $lib  = qd("blib/arch/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT");
		my $line = "\t$win32_use_dlltool -l $lib -d Prima.def -D PRIMA.$Config{dlext} \$\@\n";
		$END .= <<BAD_MAKEFILE unless $t =~ s/(^\$\(INST_DYNAMIC\)\:.*?\n(?:\t.*?\n)*)/$1$line/m;

** Warning: expected format of Makefile generated by ExtUtils::MakeMaker is
changed, so making of library Prima.a will not be performed correctly.  
Prima will run OK, but modules dependent on it may not build.
Please notify the author.

BAD_MAKEFILE

	}
	return $t;
}

# generate Prima.def ourselves - too many symbols, EU::MM dies with "command line too long"
sub dlsyms { '' }

sub install
{
	my $self = shift;
	my $t = $self->SUPER::install(@_);
	my $n = $t =~ s[
		(pure_\w+_install.*?)                       # 1
		(INST_ARCHLIB\)\s+)\$\(DEST(\w+)\)(.*?)     # 2,3,4
		(INST_BIN\)\s+)\$\(DEST(\w+)\)(.*?)         # 5,6,7
		(.*?)                                       # 8
		\n\n
	][
		"$1".
		"$2\$(DEST$3)$4".
		"$5\$(DEST$6)$7$8".
		"\n\t\$(NOECHO) \$(ABSPERL) $0 --postinstall ".
			"dest=\$(DEST$3) lib=\$($3) bin=\$($6) ".
			"slib=$cygwin_fake_Slib\n\n"
	]xgse;

	$END .= <<BAD_MAKEFILE if $n != 3;

** Warning: expected format of Makefile generated by ExtUtils::MakeMaker
is changed, so post-installation steps may not be performed correcty.
Prima will run OK, but modules dependent on it may not build.
Please notify the author.

BAD_MAKEFILE

	return $t;
}

sub linkext { shift->SUPER::linkext .  "\t\$(NOECHO) \$(ABSPERL) $0 --dl $DL_LOAD_FLAGS\n\n" }

WriteMakefile(
	NAME              => 'Prima',
	VERSION_FROM      => 'Prima.pm',
	ABSTRACT_FROM     => 'Prima.pm',
	AUTHOR            => 'Dmitry Karasik <dmitry@karasik.eu.org>',
	PM                => \%ALL_PM_INSTALL,
	OPTIMIZE          => $OPTIMIZE,
	PREREQ_PM         => \%PREREQ,
	OBJECT            => "@o_files",
	INC               => 
		join(' ', map { "-I$_" } @INCPATH ).
		' ' . $cmd_options{EXTRA_CCFLAGS},
	LIBS              => [ 
		$cmd_options{EXTRA_LDFLAGS} . ' ' .
		':nosearch ' .
		join(' ', map { "-L$_" } @LIBPATH) . ' ' .
		join(' ', map { "-l$_" } @LIBS),
	],
	LICENSE           => 'FREEBSD',
	EXE_FILES         => \@exe_files,
	PL_FILES          => {},
	MAN3PODS          => \%ALL_MAN_INSTALL,
	META_MERGE        => {
		resources => {
			homepage   => 'http://www.prima.eu.org/',
			repository => 'http://github.com/dk/Prima',
		},
		no_index  => {
			directory  => [qw(include img test unix win32)],
			file       => [qw(Makefile.PL test.pl ms_install.pl)],
		},
	},
	clean             => { FILES => "@target_clean" },
);
