use strict;
use warnings;
use 5.012;

package MY::orderedhash;
require Tie::Hash;
our @ISA = qw(Tie::ExtraHash);
sub tie      { CORE::tie my %a, $_[0], $_[1]; \%a }
sub TIEHASH  { bless [{%{$_[1]}}], $_[0] }
sub FIRSTKEY { $_[0][1] = [ sort keys %{$_[0][0]} ]; shift @{ $_[0][1] } }
sub NEXTKEY  { shift @{ $_[0][1] } }

package MY;
use strict;
use warnings;
use lib '.';
use Cwd;
use Config;
use DynaLoader;
use Prima::sys::Gencls;
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
	$LDFLAGS
	$LDDLFLAGS
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
	$CC_OPENMP
	$LD_OPENMP
	%ALL_PM_INSTALL
	%ALL_MAN_INSTALL
	%MODVERSIONS
	$PKGCONFIG
);

use vars qw(
	%cmd_options
	%passthru_options
	$Win32
	$Win64
	$unix
	$cygwin
	$mingw
	$MacOSX
	$platform
	$flavor
	$path_sep
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
	$compiler_subtype
	$compiler_version
	@Prima_exports
	$win32_use_dlltool
	$cygwin_fake_Slib
	$force_cccdl
	$use_pkgconfig
	$macos_broken_setup
	$macos_x11_path
);

sub printlog($)
{
	print $_[0];
	print MAKELOG $_[0];
}

my $END;
END {
	printlog $END if defined $END;
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
    WITH_FRIBIDI      - compile with/without fribidi (default 1)
    WITH_FONTCONFIG   - compile with/without fontconfig (default 1), X only
    WITH_FREETYPE     - compile with/without freetype (default 1), X only
    WITH_HARFBUZZ     - compile with/without harfbuzz (default 1), X only
    WITH_ICONV        - compile with/without iconv (default 1), X only
    WITH_GTK3         - compile with/without GTK3 (default 1), X only
    WITH_GTK2         - compile with/without GTK2 (default 0), X only
    WITH_OPENMP       - compile with/without openmp (default 1)
    WITH_COCOA        - compile with/without Cocoa (default 1), MacOSX only
    WITH_LIBTHAI      - compile with/without libthai (default 1)
    DEBUG             - build with debug information
    VERBOSE           - don't redirect commands to makefile.log
    EXTRA_LDFLAGS     - add extra ld line (i.e. -lgcc )
    EXTRA_CCFLAGS     - add extra cc line (i.e. -I/usr/local/include )

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

unlink "showlog";
rename "makefile.log", "makefile.log.old";
open MAKELOG, ">", "makepl.log";

setup_argv();
setup_variables();
setup_environment();
setup_compiler();
setup_exports();
setup_defines();
setup_dl_loadflags();
setup_pkgconfig();
setup_fribidi() if $cmd_options{WITH_FRIBIDI};
setup_libthai() if $cmd_options{WITH_LIBTHAI};
setup_X11() if $unix;
setup_xlibs() if $unix;
setup_codecs();
setup_macosx() if $cmd_options{WITH_COCOA} || $macos_x11_path;
finish_setup();
create_codecs_c();
create_config_h();
create_config_pm();
create_par_list();
setup_files();

sub setup_argv
{
	$ARGV_STR = join(' ', @ARGV);

	%cmd_options = (
		X11BASE         => $ENV{PRIMA_X11BASE}, # Default value of undef is ok
		WITH_XFT        => $ENV{PRIMA_WITH_XFT}         // ($^O !~ /mswin32/i),
		WITH_HARFBUZZ   => $ENV{PRIMA_WITH_HARFBUZZ}    // ($^O !~ /mswin32/i),
		WITH_FONTCONFIG => $ENV{PRIMA_WITH_FONTCONFIG}  // ($^O !~ /mswin32/i),
		WITH_FREETYPE   => $ENV{PRIMA_WITH_FREETYPE}    // ($^O !~ /mswin32/i),
		WITH_FRIBIDI    => $ENV{PRIMA_WITH_FRIBIDI}     // ($^O !~ /mswin32/i),
		WITH_ICONV      => $ENV{PRIMA_WITH_ICONV}       // ($^O !~ /mswin32/i),
		WITH_GTK2       => $ENV{PRIMA_WITH_GTK2}        // 0,
		WITH_GTK3       => $ENV{PRIMA_WITH_GTK3}        // ($^O !~ /mswin32/i),
		WITH_OPENMP     => $ENV{PRIMA_WITH_OPENMP}      // 1,
		WITH_COCOA      => $ENV{PRIMA_WITH_COCOA}       // ($^O eq 'darwin'),
		WITH_LIBTHAI    => $ENV{PRIMA_WITH_LIBTHAI}     // 1,
		DEBUG           => $ENV{PRIMA_DEBUG}            // 0,
		EXTRA_CCFLAGS   => $ENV{PRIMA_EXTRA_CCFLAGS}    // '',
		EXTRA_LDFLAGS   => $ENV{PRIMA_EXTRA_LDFLAGS}    // '',
		VERBOSE         => $ENV{PRIMA_VERBOSE}          // 0,

		# service vars
		AUTOMATED_RUN   => $ENV{PRIMA_AUTOMATED_RUN}    // 0,
		DL_LOAD_FLAGS   => $ENV{PRIMA_DL_LOAD_FLAGS}, # Default value of undef is ok
	);

	%passthru_options = (
		DEFINE          => $ENV{PRIMA_DEFINE}           // '',
		LIBS            => $ENV{PRIMA_LIBS}             // '',
		INC             => $ENV{PRIMA_INC}              // '',
		LD              => $ENV{PRIMA_LD}               // $Config{ld},
		LDDLFLAGS       => $ENV{PRIMA_LDDLFLAGS}        // $Config{lddlflags},
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
		$unix = 1;
		$platform = 'unix';
		$cygwin = 1;
	} else {
		$platform = 'unix';
		$unix = 1;
		$DL_LOAD_FLAGS = -1; # check later
		$MacOSX = ($^O eq 'darwin') ? 1 : 0;
	}

	$DL_LOAD_FLAGS = $cmd_options{DL_LOAD_FLAGS} if defined $cmd_options{DL_LOAD_FLAGS};
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
		return if /\.swp$/;
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
		<api/*.c>,
		<class/*.c>,
		<class/*/*.c>,
		(grep { not $PASSIVE_CODECS{$_} } <img/*.c>),
		(grep { !m/xft.c/ || $cmd_options{WITH_XFT} } <$platform/*.c>),
	);
	@cls_files  = ( <class/*.cls> );
	@h_files    = (
		<include/*.h>,
		<include/generic/*.h>,
		<include/$platform/*.h>,
		map { s[class/][]; "include/generic/$_" } extmap( '.h', @cls_files),
	);
	@o_files  = extmap( $Config{_o}, @c_files );

	@exe_files = (
		<utils/*.pl>,
		<Prima/VB/*.pl>
	);

	@target_clean = (
		"include/generic/*",
		"class/*$Config{_o}",
		"class/*/*$Config{_o}",
		"img/*$Config{_o}",
		"api/*$Config{_o}",
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

sub gcc { $compiler_type eq 'gcc' }

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
		if (gcc) {
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
		$CC_OPENMP    = '/openmp';
		$LD_OPENMP    = '';
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
		$CC_OPENMP    = '-fopenmp';
		$LD_OPENMP    = '-fopenmp';
		$OPTIMIZE    .= ' -Wall' if gcc;
		if ($cmd_options{DEBUG}) {
			$passthru_options{LDDLFLAGS} .= ' -g';
			$passthru_options{LDDLFLAGS} =~ s/(^| )\-s\b//;
		}
	}

	$DEFFILE    = 'Prima.def';
	$LIB_EXT    = ($cygwin ? '.dll' : '') . $Config{ _a};
	$LIB_PREFIX = ($cygwin || $mingw) ? 'lib' : '';
	$LDFLAGS    = '';

	open F, 'Prima.pm' or die "Cannot open Prima.pm:$!\n";
	my ($ver1, $ver2);
	while (<F>) {
		next unless m/\$VERSION[^\.\d]*(\d+)\.([_\d]+)/;
		$ver1 = $1, $ver2 = $2, last;
	}
	close F;
	die "Cannot find VERSION string in Prima.pm\n" unless defined $ver1;
	printlog "Version: $ver1.$ver2\n";

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
	);

	if ( $platform eq 'win32') {
		$DEFINES{PRIMA_PLATFORM_WINDOWS} = 1;
	} else {
		$DEFINES{PRIMA_PLATFORM_X11} = 1;
	}

	$TMPDIR  = $ENV{ TMPDIR} || $ENV{ TEMPDIR} || ( $Win32 ? ( $ENV{ TEMP} || "$ENV{SystemDrive}\\TEMP") : "/tmp");
	$NULLDEV = 'makefile.log';

	printlog "Flavor: ";
	$flavor = $^O;
	if ( $Win32 and not $cygwin ) {
		if ( $Config{myuname} =~ /strawberry\-?perl/i) {
			$flavor = 'strawberry';
		} elsif ( `perl -V` =~ /activeperl/i) {
			$flavor = 'activestate';
		}
		$flavor .= ( $Config{ptrsize} == 8 ) ? '64' : '32';
	} elsif ( $cygwin ) {
		$flavor .= ( $Config{ptrsize} == 8 ) ? '64' : '32';
		$flavor .= '.x11';
	} else {
		my $arch = `uname -m`;
		chomp $arch;
		$flavor .= ".$arch";
	}
	$flavor =~ s/\s/_/g;
	printlog "$flavor\n";

	$flavor =~ s/strawberry/sb/;
	$flavor =~ s/activestate/as/;

	$DISTNAME = "Prima-$ver1.$ver2-$flavor-$REVISION.$PATCHLEVEL.$SUBVERSION";
	printlog "Build: $DISTNAME\n";

	$cygwin_fake_Slib = 'SlibPrima' . ( $Config{lib_ext} || '.a' );
	$PKGCONFIG = 'pkg-config';
}

sub qtilde
{
	my $path = shift;
	return $path unless $path =~ s/^~//;
	die "** path '~$path' begins with '~' but no HOME is set\n" unless exists $ENV{HOME};
	return $ENV{HOME} . $path;
}

sub _find_file
{
	my ( $fname, $dir) = @_;
	my ( $pathname, $found);
	$pathname = "$dir/$fname";
	return $pathname if $pathname !~ /\.\./ && -e $pathname;
	opendir D, $dir or die "Cannot open dir $dir: $!";
	my @entries =
		map { "$dir/$_"}
		grep { !/^blib/ }
		grep { /^[^.]/ && -d "$dir/$_"}
		readdir D;
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
	return $cache_find_files{$fname} if exists $cache_find_files{$fname};
	return $cache_find_files{$fname} = _find_file( $fname, '.');
}

sub canon_name
{
	my ( $fname) = @_;
	my $qdirsep = quotemeta('/');
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
		$incfile = defined($i) ? $i : "include/generic/$incfile";
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
	$cc .= " $Config{cccdlflags}" if $dl || $force_cccdl;
	$cc .= " $Config{ccdlflags}" if $dl && !$compile_only;
	$cc .= " -c " if $compile_only;
	$cc .= ' ' . join(' ', map { "-I$_" } @INCPATH);
	$cc .= " $passthru_options{INC}" if length $passthru_options{INC};
	$cc .= $compile_only ? " $COUTOFLAG$objf" : " $COUTEXEFLAG$exef";
	$cc .= " $COUTOFLAG$objf" if $compiler_type eq 'cl' && !$compile_only;
	$cc .= ' ' . join(' ', map { "$CLIBPATHFLAG$_"} @LIBPATH) unless $compile_only || ( $compiler_type eq 'cl');
	$cc .= " $srcf";
	return $cc if $compile_only;

	$cc .= " $passthru_options{LDDLFLAGS}" if gcc;
	$cc .= " $CLINKPREFIX";
	$cc .= " $LDFLAGS";
	$cc .= ' ' . join(' ', map { "\"$CLIBPATHFLAG\\\"$_\\\"\"" } @LIBPATH) if $compiler_type eq 'cl';
	$cc .= ' ' . join(' ', map { "$LDLIBFLAG$_$LD_LIB_EXT"} @LIBS);
	$cc .= " $passthru_options{LIBS}" if length $passthru_options{LIBS};
	$cc .= " $cmd_options{EXTRA_LDFLAGS}" if length $cmd_options{EXTRA_LDFLAGS};
	$cc =~ s/\-bundle// if $MacOSX;

	return $cc;
}

sub ld_command_line
{
	my ( $dstf) = shift;
	my $ld = "$passthru_options{LD} $passthru_options{LDDLFLAGS}";
	$ld .= " $LDFLAGS";
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
			print "\n", $captured_makefile_log if $cmd_options{VERBOSE};
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
	my $tmpsrc = tempfile( "$TMPDIR/pmts%04d.c");
	my $tmpo = tempfile( "$TMPDIR/pmts%04d$Config{_o}");
	my $tmpexe = tempfile( "$TMPDIR/pmts%04d$Config{_exe}");
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
	my $tmpsrc = tempfile( "$TMPDIR/pmts%04d.c");
	my $tmpo = tempfile( "$TMPDIR/pmts%04d$Config{_o}");
	my $tmpexe = tempfile( "$TMPDIR/pmts%04d$Config{_exe}");
	my @tmpextras = ( $tmpsrc, $tmpsrc);
	$tmpextras[0] =~ s/\.[^\.+]$/.ilk/;
	$tmpextras[1] =~ s/\.[^\.+]$/.pdb/;
	unlink @tmpextras; # leftovers are toxic to msvc

	open TMPSRC, ">$tmpsrc" or die "Creation of temporary file $tmpsrc failed";
	print TMPSRC $text;
	close TMPSRC;

	null_output;
	my $cc = cc_command_line( $tmpsrc, $tmpo, $tmpexe, 0);
	$cc =~ s/\s\-+(s(hared)?|mdll)\b//g if gcc;
	print STDERR "$cc\n";
	my $rc = system($cc);
	print "$tmpexe\n";
	restore_output;
	unlink $tmpsrc;
	unlink $tmpo if -w $tmpo;
	my $ret = '';
	if ( -x $tmpexe ) {
		$ret = `$tmpexe`;
		chomp $ret;
	}
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
	printlog "Checking for presence of $header... ";
	my $present = compile( <<EOF, 1);
@pre_headers
#include <$header>
EOF
	$DEFINES{ $defname} = undef;
	$DEFINES{ $defname} = 1 if $present;
	printlog ($present ? "yes" : "no");
	printlog "\n";
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
	printlog "Checking for function $funcname... ";
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
		printlog "yes\n";
	} else {
		$DEFINES{ $defname} = undef;
		printlog "no\n";
	}
	return $rc;
}

sub have_define
{
	my ( $defname) = @_;
	my $cwd = cwd;
	chdir $TMPDIR;
	my $tmpsrc = tempfile( "pmts%04d.c");
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
	printlog "Checking for presence of type $type... ";
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
		printlog "yes\n";
	}
	else {
		$DEFINES{ $defname} = undef;
		printlog "no\n";
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
	printlog "Checking for inline... ";
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
			printlog "$i\n";
			return;
		}
	}
	printlog "none found\n";
	$DEFINES{__INLINE__} = '';
}

sub find_glibc
{
	printlog "Checking for glibc... ";
	my $rc = compile_and_run( <<'GLIBC');
#include <stdio.h>
int
main()
{
#ifdef __GLIBC__
	printf("1\n");
#else
	printf("0\n");
#endif
	return 0;
}
GLIBC
	printlog ($rc ? "yes\n" : "no\n");
	$DEFINES{HAS_GLIBC} = 1 if $rc;
}

sub setup_compiler
{
	printlog "Compiler: $compiler_type\n";
	if ( $compiler_type eq 'gcc' ) {
		my $r = `$Config{cc} -v 2>&1`;
		if ( $r =~ /^(.*)\s+version\s+([\d\.]+)/) {
			$compiler_subtype = $1;
			$compiler_version = $2;
			if ( $compiler_subtype =~ /clang/ ) {
				$cmd_options{EXTRA_CCFLAGS} .= " -Wno-compound-token-split-by-macro";
			}
		}
	}

	printlog "Checking if can compile... ";
	compile('int a;', 1) or die "no " . see_makefile_log;
	printlog "yes\n";

	my $text = <<EOF;
int
main()
{
   return 0;
}
EOF

	printlog "Checking if can link... ";
	unless ( compile($text, 0)) {
		printlog "no, let's try with '$Config{cccdlflags}'... ";
		$force_cccdl = 1;
		compile( $text, 0) or die "no " . see_makefile_log;
	}
	printlog "yes\n";

	if ( $cmd_options{WITH_OPENMP} ) {
		printlog "Checking if can compile with OpenMP... ";
	my $text = <<'EOF';
#include <omp.h>
#include <stdio.h>
int
main()
{
   int i, x = omp_get_thread_num();
#pragma omp parallel for
   for ( i = 0; i < x; i++) x++;
   printf("1\n");
   return 0;
}
EOF
		my $savecc = $cmd_options{EXTRA_CCFLAGS};
		my $saveld = $passthru_options{LDDLFLAGS};
		$cmd_options{EXTRA_CCFLAGS} .= " $CC_OPENMP";
		$passthru_options{LDDLFLAGS} .= " $LD_OPENMP";
		my @savelibs = @LIBS;
#		push @LIBS, 'gomp' if $compiler_type eq 'gcc';
		push @LIBS, 'vcompd' if $compiler_type eq 'cl';
		if ( compile_and_run($text)) {
		YES_OPENMP:
			$DEFINES{HAVE_OPENMP} = 1;
			printlog "yes\n";
		} else {
			if ( $captured_makefile_log =~ /recompile with (\-fpic)/i) {
				printlog "no, trying with $1 ...";
				$cmd_options{EXTRA_CCFLAGS} .= " $1";
				$CC_OPENMP .= " $1";
				$LD_OPENMP .= " $1";
				goto YES_OPENMP if compile($text);
			}

			@LIBS = @savelibs;
			$cmd_options{WITH_OPENMP} = 0;
			$cmd_options{EXTRA_CCFLAGS} = $savecc;
			$passthru_options{LDDLFLAGS} = $saveld;
			printlog "no\n";
		}
	}

	if ( $compiler_type eq 'cl' ) {
		printlog "Checking MSVC version... ";
		$compiler_version = compile_and_run(<<'MSCVER');
#include <stdio.h>
int main() {
	printf("%d\n", _MSC_VER);
	return 0;
}
MSCVER
		printlog "$compiler_version\n";

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
		printlog "Checking windows subsystem...";
		$Win64 = have_define('_WIN64');
		printlog ($Win64 ? " 64" : " 32");
		printlog " bits\n";
		$DISTNAME =~ s/(mswin|sb|as)32/${1}64/i if $Win64;
	}

	@INCPATH = (
		'include',
		'include/generic',
	);

	push @LIBS, qw(usp10 gdi32 gdiplus mpr winspool comdlg32 msimg32 ole32 uuid) if $Win32; # add more when appropriate
	push @LIBPATH, '/usr/lib/w32api' if $cygwin;
	if ($^O eq 'solaris') {
		push @LIBPATH, '/opt/csw/lib';
	}

	if ( $MacOSX ) {
		push @INCPATH, '/opt/homebrew/include';
		push @LIBPATH, '/opt/homebrew/lib', '/opt/homebrew/opt/libiconv/lib';
	}

	if ( $Win32 ) {
		printlog "Checking if can compile with GDI+... ";
		my $gdip = compile(<<'MSCVER');
#include <windows.h>
#ifdef _MSC_VER
#undef __inline__
#define __inline__ inline
#undef __extension__
#define __extension__
#endif
#include <gdiplus/gdiplus.h>
int main() {
	ULONG_PTR t;
	GdiplusStartupInput id = { 1, NULL, 0, 0 };
	GdiplusStartup(&t, &id, NULL);
	return 0;
}
MSCVER
		unless ( length $gdip ) {
			printlog "no\n";
			print <<GDIP;

** Error: Cannot complile with GDI+. If your compiler is MSVC, then it can only
compile GDI+ headers in C++ mode. Grab a copy of the headers adapted for C at

   http://prima.eu.org/download/gdiplus.zip,

and unpack it in the include catalog so that the headers are in 
./include/gdiplus/*.h

GDIP
			exit 255;
		}
		printlog "yes\n";
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
	my @size_types = qw(ssize_t);
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
	have_types_in( "sys/types.h", @size_types)
		|| have_types_in( "io.h", @size_types)
		|| have_types_in( "unistd.h", @size_types);

	if ( $unix) {
		have_header( "sys/ipc.h", "sys/types.h");
		have_header( "sys/shm.h", "sys/types.h");
	}

	if ( !have_funcs_in( 'strings.h', 'strcasecmp')) {
	   have_funcs_in( 'string.h', 'stricmp');
	}
	if ( !have_funcs_in( 'strings.h', 'strncasecmp')) {
	   have_funcs_in( 'string.h', 'strnicmp');
	}
	have_funcs_in( 'strings.h', 'strcasestr');
	have_funcs_in( 'stdio.h', 'snprintf');
	have_funcs_in( 'stdio.h', '_snprintf');
	have_funcs_in( 'stdlib.h', 'reallocf');
	have_funcs_in( 'strings.h', 'bzero');
	have_funcs_in( 'string.h', 'memmem');
	have_funcs_in( 'string.h', 'strlcpy');
	have_funcs_in( 'unistd.h', 'eaccess');
	if ( $Win32) {
		have_type( "BOOLEAN", "windows.h");
	}
	find_inline();
	find_glibc;
}

sub setup_dl_loadflags
{
	return if $DL_LOAD_FLAGS >= 0;

	printlog "Determining dl_load_flags... ";

	local @INCPATH = ( qtilde($Config{archlib}) . "/CORE");

	my $c1  = tempfile( "$TMPDIR/pmts%04d.c");
	$c1 =~ m/pmts([^\.]*).c$/;
	my ( $n1, $n2) = ( $1, sprintf("%04d", 1 + $1));

	my $o1  = "$TMPDIR/pmts$n1$Config{_o}";
	my $o2  = "$TMPDIR/pmts$n2$Config{_o}";
	my $dl1 = "$TMPDIR/pmts$n1.$Config{dlext}";
	my $dl2 = "$TMPDIR/pmts$n2.$Config{dlext}";
	my @ex = map { "$TMPDIR/pmts$_"} map { ("$n1$_", "$n2$_") } ('.ilk', '.pdb');

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

	my $c2  = tempfile( "$TMPDIR/pmts%04d.c");
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
		printlog <<DLERR;
unknown
** warning: set DL_LOAD_FLAGS=1 if your system requires RTLD_GLOBAL
DLERR
		$DL_LOAD_FLAGS = 0;
	} else {
		printlog sprintf("0x%02x\n", $DL_LOAD_FLAGS);
	}
}

sub setup_pkgconfig
{
	printlog "Checking for pkg-config... ";
	my $v = `pkg-config --version` // '';
	if ( $Win32 && $? ) {
		$v = `pkg-config.bat --version`;
		$PKGCONFIG = 'pkg-config.bat';
	}
	chomp $v;
	unless ($v =~ /^[\d\.]+$/) {
		printlog "no\n";
		return;
	}

	# is your cygwin's pkg-config shading over old strawberry?
	if ( $Win32 && $flavor =~ /^sb/ && $DEFINES{PERL_PATCHLEVEL} < 20) {
		$v = `$PKGCONFIG --real-version 2>&1`;
		if ( $? ) {
			printlog "no, it's cygwin's\n";
			$END .= <<MIXED;

** Warning: it seems that your setup has old perl and cygwin mixed.

This is a bad idea, and no guarantees are made that Prima will build.
Please take cygwin away from your PATH or upgrade your perl to 5.20 at least.
MIXED
			return;
		}
	}

	# it is mac's broken XQuartz setup?
	my $xquartz_pkgconfig_path = '/opt/X11/lib/pkgconfig';
	if ( $MacOSX && !exists $ENV{PKG_CONFIG_PATH} && -d $xquartz_pkgconfig_path) {
		printlog "trying PKG_CONFIG_PATH=$xquartz_pkgconfig_path... ";
		my $add_path;
		null_output;
		if ( system "$PKGCONFIG --exists x11" ) {
			local $ENV{PKG_CONFIG_PATH} = $xquartz_pkgconfig_path;
			$add_path = 1 unless system "$PKGCONFIG --exists x11";
		}
		restore_output;
		if ($add_path) {
			$ENV{PKG_CONFIG_PATH} = "/usr/local/lib/pkgconfig:$xquartz_pkgconfig_path";
			$macos_broken_setup = 1;
		}
	}

	$use_pkgconfig = 1;
	printlog "yes\n";
}

sub pkgconfig
{
	my $package = shift;

	printlog "Checking for $package using pkg-config...";
	null_output;

	if ( system "$PKGCONFIG --exists $package" ) {
		restore_output;
		printlog "no\n";
		return 0;
	}
	restore_output;

	my $modversion  = `$PKGCONFIG --modversion $package`;
	chomp $modversion;
	$MODVERSIONS{$package} = $modversion;
	printlog "$modversion\n";

	my %h;

	%h = map { $_ => 1 } @INCPATH;
	my $inc  = `$PKGCONFIG --cflags-only-I $package`;
	if ( $? != 0 ) { # package has error?
		printlog "misconfigured\n";
		return 0;
	}
	chomp $inc;
	$inc =~ s/\-I//g;
	for ( split " ", $inc ) {
		push @INCPATH, $_ unless $h{$_};
	}

	%h = map { $_ => 1 } @LIBS;
	my $libs    = `$PKGCONFIG --libs-only-l $package`;
	if ( $? != 0 ) {
		   printlog "misconfigured\n";
		   return 0;
	}
	chomp $libs;
	$libs =~ s/\-l//g;
	for ( split " ", $libs ) {
		push @LIBS, $_ unless $h{$_};
	}

	%h = map { $_ => 1 } @LIBPATH;
	my $libpath    = `$PKGCONFIG --libs-only-L $package`;
	if ( $? != 0 ) {
		   printlog "misconfigured\n";
		   return 0;
	}
	chomp $libpath;
	$libpath =~ s/\-[LR]//g;
	for ( split " ", $libpath ) {
		push @LIBPATH, $_ unless $h{$_};
	}

	return 1;
}

sub setup_X11
{
	if ($use_pkgconfig) {
		my $n_libpath = @LIBPATH;
		unless (pkgconfig('x11')) {
			warn
				"Prima needs package with x11 development file for compilation! ".
				"Please install it with your system installer tool.\n";
			exit 255;
		}
		pkgconfig('xext');
		if ( $macos_broken_setup ) {
			# stuff /opt/X11 or whatever comes with XQuartz at the end of the list
			# to use libs that come with homebrew instead. If we won't do that,
			# strange coredumps ensue
			$macos_x11_path = [ @LIBPATH[$n_libpath .. $#LIBPATH] ]
				if $n_libpath < @LIBPATH;
		}
		return;
	}

	# find X11 include files
	printlog "Checking for X11 headers...";
	push @INCPATH, "$cmd_options{X11BASE}/include"
		if defined($cmd_options{X11BASE}) and -d "$cmd_options{X11BASE}/include";
	for ( 'local/', 'freeware/', 'gnu/', 'opt/') {
		push @INCPATH, "/usr/${_}include" if -d "/usr/${_}include";
	}
	my $incpath = find_header( "X11/Xlib.h",
		"/usr/X11R6/include",
		"/usr/X11/include",
		"/usr/X/include",
		"/usr/openwin/include",
		"/opt/X11/include"
	);

	unless ( defined $incpath) {
		printlog "no\n";
		warn
			"Prima needs X11 headers for compilation! ".
			"Set X11BASE='/path/to/X' or INCPATH+='/path/to/X/include' ".
			"if you have a non-standard path to X. $see_makefile_log\n";
		exit 255;
	}

	printlog "yes";
	if ( -d $incpath) {
		printlog ", in $incpath";
		push @INCPATH, $incpath;
	}
	printlog "\n";

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

	printlog "Checking for library X11... ";
	my $libpath = find_lib( @libpath);
	unless ( defined $libpath) {
		printlog "no\n";
	NO_X:
		warn
			"Prima needs X11 libraries for compilation! ".
			"Set X11BASE='/path/to/X' ".
			"if you have a non-standard path to X. $see_makefile_log\n";
		exit 255;
	}

	printlog "yes";
	if ( -d $libpath) {
		printlog ", in $libpath";
		push @LIBPATH, $libpath;
	}
	printlog "\n";

	push @LIBS, 'X11';
	if (defined find_lib( "Xext", '', '')) {
		printlog "Xext library found.\n";
		push @LIBS, "Xext";
	}
	setup_x11_extension('Xext');
}

sub setup_fontconfig
{
	$cmd_options{WITH_FONTCONFIG} = 0;

	my $ok;
	if ( $use_pkgconfig ) {
		$ok = pkgconfig('fontconfig');
	} elsif ( have_header( "fontconfig/fontconfig.h")) {
		printlog "Checking for presence of fontconfig... ";
		if ( defined find_lib( 'fontconfig', '', '')) {
			push @LIBS, 'fontconfig';
			$ok = 1;
			printlog "yes\n";
		} else {
			printlog "no\n";
		}
	}

	if ( $ok ) {
		$DEFINES{WITH_FONTCONFIG} = 1;
		$cmd_options{WITH_FONTCONFIG} = 1;
	}
}

sub setup_freetype
{
	$cmd_options{WITH_FREETYPE} = 0;

	my $ok;
	if ( $use_pkgconfig ) {
		$ok = pkgconfig('freetype2');
		goto LEAVE;
	}

	my @ft_incpaths = ( "",
		( defined($cmd_options{X11BASE}) ? "$cmd_options{X11BASE}/include/freetype2" : ()),
		"/usr/include/freetype2",
		"/usr/X11R6/include/freetype2",
		"/usr/X11/include/freetype2",
		"/usr/X/include/freetype2",
		"/usr/openwin/include/freetype2",
		"/opt/X11/include/freetype2",
		"/usr/local/include/freetype2",
		"/usr/gnu/include/freetype2",
		"/usr/freeware/include/freetype2",
		"/usr/opt/include/freetype2"
	);
	my $have_ft2build_h = defined find_header( "ft2build.h", @ft_incpaths );

	printlog "Checking for presence of freetype/freetype.h... ";
	my $incpath;
	if ($have_ft2build_h) {
		$incpath = find_header(
			"freetype/freetype.h", {
				Code => <<EOF,
#include "ft2build.h"
#include FT_FREETYPE_H
EOF
			},
			@ft_incpaths,
		);
	} else {
		$incpath = find_header( "freetype/freetype.h", @ft_incpaths);
	}

	if (defined $incpath) {
		printlog "yes";
		printlog ", in $incpath" if $incpath;
		printlog "\n";
		push @INCPATH, $incpath if $incpath;
		printlog "Checking for presence of freetype... ";
		if ( defined find_lib( 'freetype', '', '')) {
			push @LIBS, 'freetype';
			printlog "yes\n";
		} else {
			printlog "no\n";
		}
	} else {
		printlog "no\n";
	}

LEAVE:
	if ( $ok ) {
		$DEFINES{WITH_FREETYPE} = 1;
		$cmd_options{WITH_FREETYPE} = 1;
	}
}

sub setup_xft
{
	if ( !$cmd_options{WITH_FONTCONFIG} || !$cmd_options{WITH_FREETYPE} || !$DEFINES{HAVE_X11_EXTENSIONS_XRENDER_H} ) {
		printlog "Xft deselected as it requires fontconfig, freetype, and xrender\n";
		$cmd_options{WITH_XFT} = 0;
		return;
	}

	my $HAVE_XFT = 0;
	my @pre_xft_libs    = @LIBS;
	my @pre_xft_libpath = @LIBPATH;
	my @pre_xft_incpath = @INCPATH;

	if ( $use_pkgconfig ) {
		$HAVE_XFT = pkgconfig('xft');
		my @post_xft_libpath = @LIBPATH;
		if (!$HAVE_XFT) {
			$cmd_options{WITH_XFT} = 0;
			@LIBS    = @pre_xft_libs;
			@LIBPATH = @pre_xft_libpath;
			@INCPATH = @pre_xft_incpath;
		} else {
			$DEFINES{HAVE_X11_XFT_XFT_H} = 1;
			if (
				$macos_broken_setup &&
				$macos_x11_path &&
				$cmd_options{WITH_HARFBUZZ} &&
				@pre_xft_libpath == @post_xft_libpath
			) {
				$cmd_options{WITH_HARFBUZZ} = 0;
				$END .= <<BROKEN_PATH_SETUP;

** Warning: harfbuzz have been disabled.

This happened because broken setup was detected where both harfbuzz and libXft
depend on freetype2, but one on /opt/X11 and another in /usr/local.
This _WILL_ result in coredumps.

To resolve the issue install libXft that uses the recent freetype2, with

    brew install dk/x11/libxft

BROKEN_PATH_SETUP
			}
		}
		return;
	}


	if ( have_header( "X11/Xft/Xft.h", "X11/X.h", "X11/Xlib.h",
			"X11/extensions/Xext.h", "X11/extensions/Xrender.h")) {
		printlog "Checking for presence of libXft... ";
		if ( defined find_lib( 'Xft', '', '')) {
			printlog "yes\n";
			push @LIBS, 'Xft';
			$HAVE_XFT++;
		} else {
			printlog "no\n";
			$DEFINES{HAVE_X11_XFT_XFT_H} = undef;
		}
	}

	$cmd_options{WITH_XFT} = 0 unless $HAVE_XFT;
	@LIBS = @pre_xft_libs unless $cmd_options{WITH_XFT};
}

sub setup_iconv
{
	if ( have_header( "iconv.h")) {
		printlog "Checking for presence of libiconv... ";
		if ( defined find_lib( 'iconv', '', '')) {
			push @LIBS, 'iconv';
			printlog "yes\n";
		} else {
			my $ok = compile( "#include <iconv.h>\nint main() { iconv_close(0); return 0; }\n", 1, $Config{cccdlflags});
			if ( $ok ) {
				printlog "no, but works as part of libc\n";
			} else {
				$DEFINES{HAVE_ICONV_H} = undef;
				$cmd_options{WITH_ICONV} = 0;
				printlog "no\n";
			}
		}
	} else {
		$cmd_options{WITH_ICONV} = 0;
	}
}

sub setup_fribidi
{
	$cmd_options{WITH_FRIBIDI} = 0;
	unless ( $use_pkgconfig ) {
		printlog "Checking for presence of fribidi... no, can only check with pkg-config installed\n";
		goto WARN;
	}
	goto WARN unless pkgconfig('fribidi');
	$DEFINES{WITH_FRIBIDI} = 1;
	$cmd_options{WITH_FRIBIDI} = 1;
	return;

WARN:
	$END .= <<BIDI;

** Warning: fribidi is needed for bidirectional text.
Prima compiled without fribidi will not be able
to handle right-to-left text, f.ex. Arabic or Hebrew.
Consider fixing your installation.

BIDI
	$END .= "Get fribidi binary from http://prima.eu.org/download/libfribidi-1.0.10-win64.zip\n"
		if $Win32 and not $cygwin;
}

sub setup_harfbuzz
{
	$cmd_options{WITH_HARFBUZZ} = 0;

	if ( !$cmd_options{WITH_XFT}) {
		printlog "harfbuzz deselected as it requires Xft\n";
		goto WAIL;
	}
	unless ( $use_pkgconfig ) {
		printlog "Checking for presence of harfbuzz... no, can only check with pkg-config installed\n";
		goto WAIL;
	}
	unless (pkgconfig('harfbuzz')) {
		goto WAIL;
	}

	$DEFINES{WITH_HARFBUZZ} = 1;
	$cmd_options{WITH_HARFBUZZ} = 1;
	return;

WAIL:
	$END .= <<NO_HARFBUZZ;

** Warning: Prima compiled without harfbuzz will not be able
to render proper ligatured text, f.ex. Arabic, Hindi, Tibetan etc etc.
Consider fixing your installation.

NO_HARFBUZZ
}

sub setup_gtk
{
	my $use_gtk3 = $cmd_options{WITH_GTK3};
	my $use_gtk2 = $cmd_options{WITH_GTK2};
	$cmd_options{WITH_GTK3} = 0;
	$cmd_options{WITH_GTK2} = 0;

	unless ( $use_pkgconfig ) {
		printlog "Checking for presence of gtk... no, can only check with pkg-config installed\n";
		return;
	}

	if ( $use_gtk3 && pkgconfig('gtk+-3.0')) {
		my $version = `$PKGCONFIG --modversion gtk+-3.0`;
		printlog "Checking if can compile and link with gtk3... ";
		my $minversion = 9;
		unless ( $version =~ m/^3\.(\d+)/ && $1 > $minversion ) {
			printlog "no, need at least v3.$minversion.0\n";
			return;
		}
		$use_gtk2 = 0;
	}
	if ( $use_gtk2 && pkgconfig('gtk+-2.0')) {
		my $version = `$PKGCONFIG --modversion gtk+-2.0`;
		printlog "Checking if can compile and link with gtk2... ";
		my $minversion = 7;
		unless ( $version =~ m/^2\.(\d+)/ && $1 > $minversion ) {
			printlog "no, need at least v2.$minversion.0\n";
			return;
		}
		$use_gtk3 = 0;
	}
	# now, try to compile with GTK. I've got lots of CPAN build failures
	# because GTK wasn't willing to compile or god knows what.
	unless (compile( "#include <gtk/gtk.h>\nint main() { return 0; }\n")) {
		printlog "no\n";
		$END .= <<WARNING;

** Warning: not compiling with GTK2. Fonts and colors will look
differently from Gnome environment. You might want to install
'libgtk+-3.0-dev' or 'libgtk+-2.0-dev' package.

WARNING
		return;
	}
	if ( $MacOSX ) {
		printlog "without X11... ";
		$DEFINES{WITH_GTK_NONX11} = 1 unless compile( "#include <gdk/gdkx.h>\nint main() { return 0; }\n");
	}
	printlog "yes\n";
	$DEFINES{WITH_GTK} = $use_gtk3 ? 3 : 2;
	$cmd_options{$use_gtk3 ? 'WITH_GTK3' : 'WITH_GTK2'} = 1;
}

sub setup_x11_extension
{
	my $name = shift;

	my $define = 'HAVE_X11_EXTENSIONS_' . uc($name) . '_H';
	if ( $use_pkgconfig ) {
		$DEFINES{$define} = pkgconfig($name) ? 1 : undef;
		return;
	}

	my $libname = ucfirst $name;
	if ( have_header( "X11/extensions/$libname.h")) {
		printlog "Checking for presence of lib$libname... ";
		if ( defined find_lib( $libname, '', '')) {
			push @LIBS, $libname;
			printlog "yes\n";
		} else {
			$DEFINES{$define} = undef;
			printlog "no\n";
		}
	}
}

sub setup_xrandr
{
	setup_x11_extension('xrandr');
	return unless ( $MODVERSIONS{xrandr} // '') =~ m/^(\d+)\.(\d+)/;
	return unless $1 * 10 + $2 >= 15;
	$DEFINES{HAVE_XRANDR_1_5} = 1;
}

sub setup_xrender
{
	setup_x11_extension('xrender');
}

sub setup_xcomposite
{
	setup_x11_extension('xcomposite');
}

sub setup_xcursor
{
	if ( $use_pkgconfig ) {
		$DEFINES{HAVE_X11_XCURSOR_XCURSOR_H} = 1 if pkgconfig('xcursor');
	} elsif ( have_header( "X11/Xcursor/Xcursor.h", "X11/Xlib.h")) {
		printlog "Checking for presence of libXcursor... ";
		if ( defined find_lib( 'Xcursor', '', '')) {
			printlog "yes\n";
			push @LIBS, 'Xcursor';
		} else {
			printlog "no\n";
			$DEFINES{HAVE_X11_XCURSOR_XCURSOR_H} = undef;
		}
	}
}

sub setup_xlibs
{
	have_header( "X11/extensions/shape.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");
	have_header( "X11/extensions/XShm.h", "X11/X.h", "X11/Xlib.h", "X11/Xutil.h");

	setup_fontconfig() if $cmd_options{WITH_FONTCONFIG};
	setup_freetype() if $cmd_options{WITH_FREETYPE};
	setup_xrender();
	setup_xft() if $cmd_options{WITH_XFT};
	setup_harfbuzz() if $cmd_options{WITH_HARFBUZZ};
	$cmd_options{WITH_ICONV} = 0 unless $cmd_options{WITH_XFT}; # iconv is used for xft only
	setup_iconv() if $cmd_options{WITH_ICONV};
	setup_gtk() if $cmd_options{WITH_GTK2} || $cmd_options{WITH_GTK3};
	setup_xrandr();
	setup_xcomposite();
	setup_xcursor();
}

sub setup_macosx
{
	if ( $macos_x11_path ) {
		my %remove = map { $_ => 1 } @{ $macos_x11_path // [] };
		@LIBPATH = grep { !exists $remove{$_} } @LIBPATH;
		push @LIBPATH, @{ $macos_x11_path // [] };
	}
	
	return unless $cmd_options{WITH_COCOA};
	printlog "Can compile with Cocoa...";
	my $old_ldflags = $LDFLAGS;

	$LDFLAGS .= ' -framework Cocoa';
	my $ok = compile(<<TEXT);
#import <ApplicationServices/ApplicationServices.h>
int main() {
	printf("%x\\n", (void*)CGColorSpaceCreateDeviceRGB());
	return 0;
}
TEXT
	unless ($ok) {
		printlog "no\n";
		$LDFLAGS = $old_ldflags;
		return;
	}

	printlog "yes\n";
	$DEFINES{WITH_COCOA} = 1;
}

sub setup_libthai
{
	$cmd_options{WITH_LIBTHAI} = 0;

	unless ( $use_pkgconfig ) {
		printlog "Checking for presence of libthai... no, can only check with pkg-config installed\n";
		goto WARN;
	}

	my @lib = @LIBS;
	goto WARN unless pkgconfig('libthai');

	printlog "Checking if can compile and link with libthai... ";
	unless (compile( <<LIBTHAI)) {
#include <thai/thwchar.h>
#include <thai/thwbrk.h>
int main() {
	void * ixi = th_brk_wc_find_breaks;
	return 0;
}
LIBTHAI
		printlog "no\n";
		@LIBS = @lib;
		goto WARN;
	}
	printlog "yes\n";
	$cmd_options{WITH_LIBTHAI} = $DEFINES{WITH_LIBTHAI} = 1;
	return;

WARN:
	$END .= <<LIBTHAI;

** Warning: libthai is needed for proper breaking of thai text.
Consider fixing your installation.

LIBTHAI
	$END .= <<LIBTHAI if $Win32 and not $cygwin;
Get libthai binary from http://prima.eu.org/download/libthai-0.1.29-win64.zip

LIBTHAI
	$END .= <<LIBTHAI if $MacOSX;
Get it with `brew install dk/libthai/libthai`

LIBTHAI
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
ctx_remap_def cv_call_perl duplicate_string eval gimme_the_mate
gimme_the_vmt kind_of prima_kill_zombies notify_perl Object_create Object_destroy parse_hv
plist_create plist_destroy prima_mallocz pop_hv_for_REDEFINED protect_object
push_hv push_hv_for_REDEFINED query_method sv_call_perl sv_query_method
unprotect_object perl_error exception_remember exception_block exception_check_raise
exception_charged
);
	push @Prima_exports, grep { /^(apc|list|prima)/ } suck_symbols('include/apricot.h');
	push @Prima_exports, suck_symbols('include/img.h');
	push @Prima_exports, suck_symbols('include/img_conv.h');
	generate_win32_def() if $Win32;
}

sub setup_codecs
{
	# see if Prima::codecs:: is installed
	my ( $prereq, $have_binary_prereq);
	$prereq = 'win32' if $Win32 and not $cygwin;
	$prereq = 'win64' if $Win64 and not $cygwin;
	if ( $prereq) {
		printlog "Checking for Prima::codecs::$prereq... ";
		eval "use Prima::codecs::$prereq;";
		unless ( $@) {
			printlog "yes\n";
			$have_binary_prereq++;
			my $f = $INC{"Prima/codecs/$prereq.pm"};
			$f =~ s/.pm$//;
			push @LIBPATH, "$f/lib";
			push @INCPATH, "$f/include";

		} else {
			printlog "no\n";
		}
	}

	# finding image codecs
	my %libs = map { $_ => 1 } @LIBS;
	my @codecs;
	my @builtin_codecs;
	while ( <img/codec_*.c>) {
		if ( m/codec_(bmp|X11)/) {
			push @builtin_codecs, $1;
		} else {
			push @codecs, $_;
		}
	}

	my @codec_libpath = $Config{installsitearch};
	my @warn_codecs;
	my $webp_version;
	CODEC: for my $cx ( @codecs) {
		my @inc;
		my $foundlib;
		$cx =~ m/codec_(.*)\.c$/i;
		my ( $fn, $lib, $codec) = ( $cx, $1, $1);

		# First check if pkg-config can help us here. Not necessarily it can, not all
		# graphic libs have .pc files. But if it can, it helps greatly with dll hell.
		if ( $use_pkgconfig ) {
			my $found;
			if ( $codec eq 'webp') {
				if (pkgconfig('libwebp') && pkgconfig('libwebpdemux') && pkgconfig('libwebpmux')) {
					$found = 1;
					$webp_version = $1 * 0x10000 + $2 * 0x100 + $3
						if $MODVERSIONS{libwebp} =~ /^(\d+).(\d+).(\d+)/;
				}
			} elsif ( $codec eq 'heif') {
				my @savelib = @LIBS;
				if ( pkgconfig('libheif')) {
					if ( $MODVERSIONS{libheif} =~ /^(\d+).(\d+).\d+/) {
						my $heif_version = $1 * 100 + $2;
						if ( $heif_version < 112 ) {
							printlog "** Warning: libheif version is too low, at least 1.12.0 is needed, skipping\n";
							@ACTIVE_CODECS = grep { !/heif/ } @ACTIVE_CODECS;
							$PASSIVE_CODECS{'img/codec_heif.c'} = 1;
							@LIBS = @savelib;
							push @warn_codecs, 'heif';
							next CODEC;
						} else {
							$found = 1;
						}
					}
				}
			} elsif ( $codec eq 'jpeg') {
				$found = 1 if pkgconfig('libjpeg');
			} elsif ( $codec eq 'png') {
				$found = 1 if pkgconfig('libpng');
			} elsif ( $codec eq 'tiff') {
				$found = 1 if pkgconfig('libtiff-5') || pkgconfig('libtiff-4');
			} elsif ( $codec eq 'Xpm') {
				$found = 1 if pkgconfig('xpm');
			} # gif and jpeg -- I've never seen yet these libs shipped with .pc file, so don't check them so far

			if ( $found ) {
				push( @ACTIVE_CODECS, $codec);
				next;
			}
		}

		next unless open F, $fn;
		while(<F>) {
			push @inc, $_ if m/^\s*#include\s*\</;
		}
		close F;

		# do we have a versioned inc/lib from dependency hell?
		my $version = '';
		if (
			( $codec ne 'X11' ) &&
			( my @versioned = grep { /$codec\d+$/ } @INCPATH )
		) {
			$versioned[0] =~ /$codec(\d+)$/;
			$version = $1;
			$lib .= $1;
		}

	AGAIN:
		printlog "Checking for $codec$version library... ";
		if (
			$libs{$lib} ||
			defined ( $foundlib = find_lib( $lib, join('', @inc), '', @codec_libpath))
		) {
			if ( defined $foundlib and length $foundlib) {
				push @LIBPATH, $foundlib;
				@codec_libpath = ();
			}
			push( @ACTIVE_CODECS, $codec);

			printlog "yes";
			if ($codec eq 'webp') {
				my @addlib;
				for my $lib2 ('webpdemux', 'webpmux') {
					if ( defined find_lib($lib2,join('', @inc), '', @codec_libpath)) {
						push @addlib, $lib2 unless $libs{$lib2};
					} else {
						printlog ", but $lib2 not found, skipping\n";
						@addlib = ();
						pop @ACTIVE_CODECS;
						$PASSIVE_CODECS{$fn} = 1;
						pop @LIBPATH if defined $foundlib and length $foundlib;
						next CODEC;
					}
				}
				unshift( @LIBS, @addlib );
			}
			# In gcc, order of libs matters. libXpm requires libgdi32, and
			# has to be mentioned _after_ it to work.
			unshift( @LIBS, $lib) unless $libs{$lib};
			printlog ", in $foundlib" if defined($foundlib) and length($foundlib);
			printlog "\n";
		} elsif ( $codec eq 'X11') {
			$DEFINES{EMULATE_X11_CODEC} = 1;
			push( @ACTIVE_CODECS, $codec);
			printlog "no, using built-in\n";
		} elsif ( length $version) {
			$lib = $codec;
			$version = '';
			printlog "no\n";
			goto AGAIN;
		} else {
			$PASSIVE_CODECS{$fn} = 1;
			push @warn_codecs, $codec;
			printlog "no\n";
		}
	}

	# check supported versions
	if ( grep {/webp/} @ACTIVE_CODECS) {
		$webp_version //= compile_and_run(<<'VER');
#include <stdio.h>
#include <webp/decode.h>
int main() {
	printf("%d\n", WebPGetDecoderVersion());
	return 0;
}
VER
		if (( $webp_version // 0) < 1537 ) { # 0.6.1
			printlog "** Warning: webp version ".
				((defined($webp_version) ?
					"is too low ($webp_version), 0.6.1 at least is needed" :
					"cannot be determined")).
				", skipping\n";
			@ACTIVE_CODECS = grep { !/webp/ } @ACTIVE_CODECS;
			$PASSIVE_CODECS{'img/codec_webp.c'} = 1;
		}
	}

	if ( grep {/gif/} @ACTIVE_CODECS) {
		printlog "Checking for libgif version...";
		my $ver = compile_and_run(<<'VER') // '';
#include <stdio.h>
#include <gif_lib.h>
int main() {
	void *p;
#if defined(GIFLIB_MAJOR)
	printf("%d.%d\n", GIFLIB_MAJOR, GIFLIB_MINOR);
#else
	printf("%s\n", GIF_LIB_VERSION);
	p = MakeMapObject; /* observed some linking failures on the function in v4.1 */
#endif
	return 0;
}
VER
		if ( $ver =~ /.*(\d+)\.(\d+)/) {
			if ( $1 < 4 ) {
				$ver = "$1.$2";
				goto NO_GIF;
			} else {
				printlog "$1.$2\n";
			}
		} else {
		NO_GIF:
			printlog "no usable version found ($ver)\n";
			@ACTIVE_CODECS = grep { !/gif/ } @ACTIVE_CODECS;
			$PASSIVE_CODECS{'img/codec_gif.c'} = 1;
			push @warn_codecs, 'gif';
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
		$END .= <<LIBHEIF if $Win32 and not $cygwin and grep {/heif/} @warn_codecs;
Get libheif binary from http://prima.eu.org/download/libheif-1.17.6-win64.zip
LIBHEIF
		$END .= <<LIBWEBP if $Win32 and not $cygwin and grep {/webp/} @warn_codecs;
Get libwebp binary from http://prima.eu.org/download/libwebp-1.0.2-win64.zip
LIBWEBP
	}
	push @ACTIVE_CODECS, @builtin_codecs;
}

sub finish_setup
{
	print "Using system libraries:";
	for my $k ( sort keys %cmd_options ) {
		next unless $cmd_options{$k};
		next unless $k =~ m/^WITH_(.*)$/;
		print ' ', lc($1);
	}
	print "\n";
	print "Using image libraries: @ACTIVE_CODECS\n";
}

sub create_codecs_c
{
	printlog "Creating img/codecs.c\n";
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
	printlog "Creating $config_h\n";
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

sub _quotepath { _quote(@_) }

sub create_config_pm
{
	my $cwd = cwd;
	my $ifs = '/';

	# includes
	my @ip = @INCPATH;
	$ip[0] = "$cwd${ifs}include";
	$ip[1] = "$cwd${ifs}include${ifs}generic";
	my $ipp = join(',', map {"\'$_\'"} @ip);
	my $inc    = join(' ', map { "-I$_" } @ip);
	$ip[0] = '$(lib)' . "/Prima/CORE";
	$ip[1] = '$(lib)' . "/Prima/CORE/generic";
	my $ippi = join(',', map {"\'$_\'"} @ip);
	my $inci = join(' ', map { "-I$_" } @ip);

	# libs
	my @libpath = @LIBPATH;
	my @libs    = @LIBS;
	unless ( $unix or gcc) {
		push @libpath, "$cwd/auto/Prima";
		push @libs, "Prima$LD_LIB_EXT";
	}
	my $libpath = join( ',', map {"'$_'"} @libpath);
	unless ( $unix or gcc) {
		$libpath[-1] = '$(lib)/auto/Prima';
	}
	my $libpathi = join( ',', map {"'$_'"} @libpath);
	my $ldlibs  = join( ',', map {"'$_'"} @libs);

	my ($libs, $libsi) = ('','');
	if ( $cygwin) {
		$libs  = "-L$cwd/blib/arch/auto/Prima -lPrima";
		$libsi = '-L$(lib)/auto/Prima -lPrima';
	} elsif ( $Win32) {
		$libs  = "$cwd/blib/arch/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT";
		$libsi = '$(lib)' . "/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT";
	}

	my $cc_openmp = $cmd_options{WITH_OPENMP} ? $CC_OPENMP : '';

	open F, "> Prima/Config.pm" or die "cannot open Prima/Config.pm:$!\n";
	print F <<CONFIG;
# This file was automatically generated.
# Do not edit, you'll loose your changes anyway.
package Prima::Config;
use strict;
use warnings;
use vars qw(%Config %Config_inst);

%Config_inst = (
	incpaths              => [ $ippi ],
	gencls                => '\$(bin)${ifs}prima-gencls$SCRIPT_EXT',
	tmlink                => '\$(bin)${ifs}prima-tmlink$SCRIPT_EXT',
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
	gencls                => ${_quotepath("$cwd/blib/script/prima-gencls$SCRIPT_EXT")},
	tmlink                => ${_quotepath("$cwd/blib/script/prima-tmlink$SCRIPT_EXT")},
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
	optimize              => '$OPTIMIZE',
	openmp                => '$cc_openmp',

	inc                   => '$inc',
	define                => '',
	libs                  => '$libs',
);

1;
CONFIG
	close F;
}

sub create_par_list
{
	open F, ">", "utils/par.txt" or die "Cannot open utils/par.txt:$!";
	find( sub {
		return unless -f;
		return if /\.p[lm]$/i || /^(VB|prima-cfgmaint)$/;
		my $d = $File::Find::dir;
		return if $d =~ /examples/;
		print F "$d/$_\n";
	}, 'Prima');
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
use vars qw(\%Config);

my \$bin = q($opt{bin});

# Determine lib based on the location of this module
use File::Basename qw(dirname);
use File::Spec;
my \$lib = File::Spec->catfile(dirname(__FILE__), '..');

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
				s/\$\((\w+)\)/\$$1/g;
				s/'/"/g;
				s{\\}{\\\\}g;
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
	my %dirs;
	my @files;

	open MAN, "<", "MANIFEST" or die "Cannot open MANIFEST:$!\n";
	for my $path (<MAN>) {
		chomp $path;
		my ( $d, $f );
		if ( $path =~ m[^(.+)/([^\/]+)$]) {
			($d, $f) = ($1, $2);
		} else {
			($d, $f) = ('', $path);
		}
		next if $d =~ m[^(t|img|include|win32|unix|api|class)(/|$)];
		if ( length $d ) {
			$dirs{ "$DISTNAME/$d" }++;
			push @files, "$d/$f";
		} else {
			$dirs{ $DISTNAME }++;
			push @files, $f;
		}
	}

	print "Creating directories...\n";
	$dirs{ "$DISTNAME/auto/Prima" }++;
	for ( sort keys %dirs) {
		next if -d $_;
		cleanup( "Can't mkdir $_") unless mkpath $_;
	}

	print "Copying files...\n";
	for ( @files) {
		cleanup("Error copying $_ to $DISTNAME/$_") unless copy $_, "$DISTNAME/$_";
	}
	for (<blib/arch/auto/Prima/*>) {
		next if m/(\.exists|Prima\.bs|Prima\.pdb)$/;
		my $f = $_;
		$f =~ s[^blib/arch][$DISTNAME];
		cleanup("Error copying $_ to $_") unless copy $_, $f;
	}
	if ($^O eq 'cygwin') {
		system "strip $DISTNAME/auto/Prima/Prima.$Config{dlext}" ; # it's 27 MB!
		$cygwin_fake_Slib = 'SlibPrima' . ( $Config{lib_ext} || '.a' );
		open F, '>', "$DISTNAME/auto/Prima/$cygwin_fake_Slib";
		close F;
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
	my $re1 = '\.c\$\(OBJ_EXT\)\s*:\n\t.*';
	my $ending = '$*$(OBJ_EXT)';
	unless ( $t =~ /$re1\Q$ending\E/ ) {
		$t =~ s/($re1)/$1 $COUTOFLAG$ending/;
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

	my $showlog = '';

	# that's because CPAN doesn't save Makefile.PL output, and I want it for tests
	if ( grep { $ENV{$_} } qw(AUTOMATED_TESTING NONINTERACTIVE_TESTING PERL_BATCH)) {
		$showlog = 'showlog';
		$t .= <<SHOWMORE;

showlog:
\t\$(NOECHO) $^X -e ${SHQUOTE}open F,q(makepl.log);print <F>${SHQUOTE}
\t\$(NOECHO) \$(TOUCH) showlog

SHOWMORE
	}

    	printlog "Finding .cls dependencies...\n";

	for my $clsfile ( @cls_files) {
		my ( $base ) = $clsfile =~ m/^class\/(.*?).cls$/;
		my $ancestors = join(' ', map { "include/generic/$_.h class/$_.cls" } gencls( $clsfile, depend => 1, incpath => ['class']));
	  	$t .= <<H;

include/generic/$base.h include/generic/$base.inc include/generic/$base.tml: \$(FIRST_MAKEFILE) $showlog class/$base.cls $ancestors
\t$^X -I. utils/prima-gencls.pl --inc --h -Iclass --tml $clsfile include/generic

H
		push @alltml, "include/generic/$base.tml";
		push @alltmldeps, "include/generic/$base.h";
		$showlog = '';
	}

	$t .= <<H;
include/generic/thunks.tinc: \$(FIRST_MAKEFILE) @alltmldeps
\t$^X utils/prima-tmlink.pl -Iinclude/generic -oinclude/generic/thunks.tinc @alltml

H

	my %dirs;
	for my $cfile ( @c_files ) {
        	my ( $dir ) = ( $cfile =~ /^(\w+)\// );
        	$dir = 'root directory' unless $dir;
		printlog "Finding .c dependencies in $dir...\n" unless $dirs{$dir}++;

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
		'$(DESTINSTALLSITEMAN3DIR)/prima-gencls.$(MAN3EXT)',
		'$(DESTINSTALLSITEMAN1DIR)/VB.$(MAN1EXT)',
		'$(DESTINSTALLSITEMAN1DIR)/prima-cfgmaint.$(MAN1EXT)',
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

xtest ::
\t\$(MAKE) test TEST_FILES=t/*/*.xt

bindist: all
\t$^X $0 --bindist $DISTNAME

devclean:
\t\$(RM_F) api/*$Config{_o} class/*/*$Config{_o} class/*$Config{_o} img/*$Config{_o} $platform/*$Config{_o}

deinstall:
$pm_deinstall
$pm_deinstall_dir

include/img_conv.h: include/generic/Image.h

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
		my $lib  = "blib/arch/auto/Prima/${LIB_PREFIX}Prima$LIB_EXT";
		my $line = "\t$win32_use_dlltool -l $lib -d Prima.def -D PRIMA.$Config{dlext} \$\@\n";
		$END .= <<BAD_MAKEFILE unless $t =~ s/(^\$\(INST_DYNAMIC\)\s*\:.*?\n(?:\t.*?\n)*)/$1$line/m;

** Warning: expected format of Makefile generated by ExtUtils::MakeMaker is
changed, so making of library Prima.a will not be performed correctly.
Prima will run OK, but modules dependent on it may not build.
Please notify the author by sending a report with Makefile and
noting that ExtUtils\:\:MakeMaker version $ExtUtils::MakeMaker::VERSION was used.

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
	my $q = $SHQUOTE;
	my $n = $t =~ s[
		(pure_\w+_install.*?)                       # 1
		(INST_ARCHLIB\)"?\s+"?)\$\(DEST(\w+)\)(.*?)     # 2,3,4
		(INST_BIN\)"?\s+"?)\$\(DEST(\w+)\)(.*?)         # 5,6,7
		(.*?)                                       # 8
		\n\n
	][
		"$1".
		"$2\$(DEST$3)$4".
		"$5\$(DEST$6)$7$8".
		"\n\t\$(NOECHO) \$(ABSPERL) $0 --postinstall ".
			"${q}dest=\$(DEST$3)$q ${q}bin=\$($6)$q slib=$cygwin_fake_Slib\n\n"
	]xgse;

	$END .= <<BAD_MAKEFILE if $n != 3;

** Warning: expected format of Makefile generated by ExtUtils::MakeMaker
is changed, so post-installation steps may not be performed correctly.
Prima will run OK, but modules dependent on it may not build.
Please notify the author by sending a report with Makefile and
noting that ExtUtils\:\:MakeMaker version $ExtUtils::MakeMaker::VERSION was used.

BAD_MAKEFILE

	return $t;
}

sub linkext { shift->SUPER::linkext .  "\t\$(NOECHO) \$(ABSPERL) $0 --dl $DL_LOAD_FLAGS\n\n" }

WriteMakefile(
	NAME              => 'Prima',
	VERSION_FROM      => 'Prima.pm',
	ABSTRACT_FROM     => 'Prima.pm',
	AUTHOR            => 'Dmitry Karasik <dmitry@karasik.eu.org>',
	PM                => MY::orderedhash->tie(\%ALL_PM_INSTALL),
	OPTIMIZE          => $OPTIMIZE,
	LDDLFLAGS         => $passthru_options{LDDLFLAGS},
	PREREQ_PM         => \%PREREQ,
	OBJECT            => "@o_files",
	INC               =>
		join(' ', map { "-I$_" } @INCPATH ).
		' ' . $cmd_options{EXTRA_CCFLAGS},
	LIBS              => [
		$cmd_options{EXTRA_LDFLAGS} . ' ' .
		$LDFLAGS . ' ' .
		':nosearch ' .
		join(' ', map { "-L$_" } @LIBPATH) . ' ' .
		join(' ', map { "-l$_" } @LIBS),
	],
	LICENSE           => 'FREEBSD',
	EXE_FILES         => \@exe_files,
	PL_FILES          => {},
	MAN3PODS          => MY::orderedhash->tie(\%ALL_MAN_INSTALL),
	META_MERGE        => {
		resources => {
			homepage   => 'http://www.prima.eu.org/',
			repository => 'http://github.com/dk/Prima',
		},
		no_index  => {
			directory  => [qw(include t img unix win32)],
			file       => [qw(Makefile.PL ms_install.pl)],
		},
	},
        test => {TESTS => 't/*/*.t'},
	clean             => { FILES => "@target_clean" },
	MIN_PERL_VERSION  => 5.012,
);

