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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  $Id$

use strict;
use Config;
use Prima::Config;
use File::Find;
use File::Basename;
use File::Path;
use File::Copy;
use DynaLoader;
use Cwd;
use Exporter;
use vars qw(@ISA @EXPORT);
@ISA = qw(Exporter);
@EXPORT = qw( 
	init qd dl_name quoted_split setvar env_true addlib dump_command
	canon_name find_file find_cdeps cc_command_line ld_command_line generate_def
	find_version manname
);

use vars qw( %make_trans @ovvars $dir_sep $path_sep);

use vars @ovvars = qw(
	@INCPATH
	@LIBPATH
	@LIBS
	$PREFIX
	$INSTALL_BIN
	$INSTALL_LIB
	$INSTALL_DL
	$INSTALL_EXAMPLES
	$INSTALL_MAN3
	$INSTALL_MAN1
	$DEBUG
);

use vars qw(
	$Win32
	$cygwin
	$OS2
	$Unix
	%DEFINES
	%USER_VARS
	%USER_VARS_ADDONS
	%USER_VARS_LINKS
	%USER_DEFINES
	%USER_DEFINES_ADDONS
	%overrideable
	$ARGV_STR
	@allclean
	@allrealclean
	@allobjects
	%alldeps
	%mapheader
	@allinstall
	@allbins
	@alldirs
	@allsubtargets
	@alllibtargets
	@subtargetsdirs
	@allman
	$install_manuals
);


$OS2   = $Prima::Config::Config{platform} eq 'os2';
$Win32 = $Prima::Config::Config{platform} eq 'win32';
$Unix  = $Prima::Config::Config{platform} eq 'unix';
$cygwin = $Win32 && $^O =~ /cygwin/;

sub init
{
	my @argv = @_;
	if ( $#argv >= 0 && $argv[ 0] =~ /^\-\-cp(bin)?$/) {
		my $isbin = defined $1;
		shift @argv;
		die qq(Even number of parameters expected) unless ( $#argv % 2);
		while ( scalar @argv) {
			my ( $src, $dst) = ( shift @argv, shift @argv);
			print qq(Installing $src -> $dst\n);
			next unless -f $src;
			if ( $isbin) {
				my $dstdir = dirname( $dst);
				mkpath $dstdir if $dstdir && ! -d $dstdir;
				open SRCPL, "<$src" or die "Cannot open $src: $!";
				open DSTPL, ">$dst" or die "Cannot create $dst: $!";
				chmod 0555, $dst unless $Win32 || $OS2;
				if ( $Win32) {
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
					my $perlpath = $Config{ perlpath};
					$perlpath =~ s/(perl)(\.exe)?$/$1__$2/i if $perlpath =~ /perl(\.exe)?$/i;
					print DSTPL <<EOF;
extproc $perlpath -wS
EOF
				}
				else {
					print DSTPL <<EOF;
#! $Config{ perlpath} -w
EOF
				}
				my $filestart = 1;
				while ( <SRCPL>) {
					next if $filestart && /^\#\!/;
					$filestart = 0;
					print DSTPL;
				}
				if ( $Win32) {
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
				if ( $OS2) {
					$src =~ /(?:^|[\\\/])([^\\\/]*)$/;
					unlink $dst.'\\'.$1 if defined $1;
				}
				copy( $src, $dst) or die qq(Copy failed: $!);
			}
		}
		return 0;
	}
	elsif ( $#argv >= 0 && $argv[ 0] eq '--md') {
		shift @argv;
		mkpath \@argv;
		return 0;
	}
	elsif ( $#argv >= 0 && $argv[ 0] eq '--rm') {
		shift @argv;
		unlink @argv;
		return 0;
	} elsif ( $#argv >= 2 && $argv[ 0] eq '--dist') {
		my $type = lc $argv[1];
		my $cwd = cwd();
		my $distname = $argv[2];

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
			return if $_ eq '.' || $_ eq 'Makefile' || $_ eq 'makefile.log';
			return if /\.(pdb|ncb|opt|dsp|dsw)$/i; # M$VC
			my $f = "$File::Find::dir/$_";
			return if $f =~ /include.generic|CVS/;
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
			cleanup("") unless copy $_, $f;
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
		return 0;
	}
	

	%USER_VARS_LINKS = (
		INSTALL_BIN => {
			src => 'PREFIX',
			format => '%s/bin',
		},
	);

	%overrideable = map { /^(.)(.*)$/; $2 => $1} @ovvars; # Script variables which may be overridden.
	$dir_sep   = $Prima::Config::Config{ifs};
	$path_sep  = $Config{path_sep};
	%DEFINES  = map { my @z = split('=', $_, 2); scalar(@z)?(@z):($_ => 1)} @{$Prima::Config::Config{cdefs}};
	$mapheader{'apricot.h'} = undef;
	$ARGV_STR = join( " ", map { "\"$_\""} @argv);
	foreach my $arg ( @argv) {
		if ( $arg =~ /^\s*(\w+)\s*(\+?)\=(.*)$/) {
			my ( $varname, $setmode, $varval) = ( $1, $2 || '', $3);
			die "Unknown variable $varname" unless $overrideable{ $varname};
			if ( $overrideable{ $varname} eq '$') {
				if ( $setmode eq '+') {
					push @{ $USER_VARS_ADDONS{ $varname}}, $varval;
				}
				else {
					$USER_VARS{ $varname} = $varval;
				}
				die $@ if $@;
			}
			elsif ( $overrideable{ $varname} eq '@') {
				my @values = split /$path_sep/, $varval;
				if ( $setmode eq '+') {
					push @{ $USER_VARS_ADDONS{ $varname}}, @values;
				}
				else {
					$USER_VARS{ $varname} = @values;
				}
				die $@ if $@;
			}
		}
		elsif ( $arg =~ /^-(D|U)(\w+)(?:(\+?)=(.*))?$/) {
			my ( $defmode, $defname, $setmode, $value) = ( $1, $2, $3 || '', $4 || '');
			if ( $defmode eq 'U') {
				$USER_DEFINES{ $defname} = undef; # Still exists in the hash...
			}
			else {
				if ( $setmode eq '+') {
					push @{ $USER_DEFINES_ADDONS{ $defname}}, $value;
				}
				else {
					$USER_DEFINES{ $defname} = $value;
				}
			}
		}
		else {
			die "Unknown command line argument or wrong syntax: '$arg'";
		}
	}

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
	setvar('INCPATH', @{$Prima::Config::Config{incpaths}});
	setvar('LIBPATH', @{$Prima::Config::Config{ldpaths}});
	setvar('LIBS',    @{$Prima::Config::Config{ldlibs}});
	setvar( 'DEBUG', 0);
	setvar( 'PREFIX', $Config{ installsitearch});
	setvar( 'INSTALL_BIN', $Config{ installbin});
	$install_manuals = (!$Win32 && !$OS2);
	if ( exists $USER_VARS{PREFIX}) {
		setvar( 'INSTALL_MAN1', $PREFIX . qd( "/man/man1"));
		setvar( 'INSTALL_MAN3', $PREFIX . qd( "/man/man3"));
	} else {
		setvar( 'INSTALL_MAN1', $Config{installman1dir});
		setvar( 'INSTALL_MAN3', $Config{installman3dir});
	}

	return 1;
}


sub manname
{
	my ( $name, $section, $prefix) = @_;
	$prefix = quotemeta( $prefix );
	my $ds = quotemeta($dir_sep);
	$name =~ s/^$prefix(?:[\\\/$ds])//;
	$name =~ s/[\\\/$ds]/::/g;
	$name =~ s/\.[^\.]*$/\.$section/;
	my $x = (( $section == 3 ) ? $INSTALL_MAN3 : $INSTALL_MAN1) . $dir_sep . $name;
}

sub qd
{
	my ( $path_str) = @_;
	$path_str =~ s[/][$dir_sep]g;
	return $path_str;
}

sub dl_name
{
	my @n = split(/\:\:/, $_[0]);
	$n[-1] = &DynaLoader::mod2fname(\@n) if $OS2 && defined &DynaLoader::mod2fname;
	return $n[-1] . $Prima::Config::Config{dlext};
}


sub quoted_split
{
	my @neww = ();
	my $text = shift;
	while ($text =~ m<
		("[^\"\\]*(?:\\.[^\"\\]*)*")\s*
	| (\S+)\s*
	| \s+
	>gx) {
		push @neww, $+ if defined $+;
	}
	return @neww;
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
			&& exists $USER_VARS{ $USER_VARS_LINKS{ $varname}-> { src}}) {
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
			eval "\@$varname = \$USER_VARS{ \$varname}";
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

	return () if exists $deps-> { $cfile};
	$deps-> { $cfile} = [];
	return @{ $alldeps{ $cfile}} if exists $alldeps{ $cfile};
	$alldeps{ $cfile} = [];
	return () unless -f $cfile;

	local *CF;
	open CF, "<$cfile" or die "Cannot open $cfile: $!";
	while ( <CF>) {
		chomp;
		next unless /^\s*\#\s*include\s+"([^\"]+)"/;
		my $incfile = $1;
		my $found = 0;
		if ( exists $mapheader{$incfile}) {
			$incfile = $mapheader{$incfile};
			next unless defined $incfile; # usually apricot.h
			$found = 1;
		} else {
			( $incfile, $found) = find_file( $incfile, ".");
		}
		die "Can't find $incfile\n" unless $found;
		$incfile = canon_name( $incfile);
		unless ( exists $included-> { $incfile}) {
			push @{ $alldeps{ $cfile}}, $incfile;
			push @{ $deps-> { $cfile}}, $incfile;
			$included-> { $incfile} = 1;
		}
		my @subdeps = find_cdeps( $incfile, $deps, $included);
		push @{ $deps-> { $cfile}}, @subdeps;
		push @{ $alldeps{ $cfile}}, @subdeps;
	}
	close CF;
	return @{ $deps-> { $cfile}};
}

sub cc_command_line
{
	my ( $srcf, $objf) = @_;
	my $ret = $Prima::Config::Config{cc} . ' ';
	$ret .= ' ' . $Prima::Config::Config{cflags};
	$ret .= ' ' . $Prima::Config::Config{cdebugflags} if $DEBUG;
	$ret .= ' ' . join( ' ' , map { $Prima::Config::Config{cdefflag}.$_.'='.$DEFINES{$_}} keys %DEFINES);
	$ret .= ' ' . join( ' ' , map { $Prima::Config::Config{cincflag}.$_} @INCPATH);
	$ret .= ' ' . $srcf;
	$ret .= ' ' . $Prima::Config::Config{cobjflag} . $objf;
	return $ret;
}

sub quote
{
	my @q = @_;
	if ( $Win32 || $OS2) {
		return map { 
			if ( m/[\s\"]/) {
				s/\"/\"\"/gs;
				$_ = "\"$_\"";
			}
			$_;
		} @q;
	} else {
		return map { s/([\'\\])/\\$1/gs; $_  } @q;
	}
}

sub ld_command_line
{
	my ( $dstf, $deffile) = (shift,shift);
	my $ret = $Prima::Config::Config{ld} . ' ';
	$ret .= ' ' . $Prima::Config::Config{ldflags};
	$ret .= ' ' . $Prima::Config::Config{lddebugflags} if $DEBUG;
	if ( $Prima::Config::Config{compiler} eq 'bcc32') {
		$ret .= '-L"' . join( ';', @LIBPATH) . '" c0d32.obj ';
		$ret .= join( ' ', @_);
	}  else {
		$ret .= ' ' . join( ' ' , 
			quote( map { $Prima::Config::Config{ldlibpathflag}.$_} @LIBPATH));
		$ret .= ' ' . $Prima::Config::Config{ldoutflag} . $dstf;
		$ret .= ' ' . join( ' ', @_);
		$ret .= ' ' . join( ' ' , map { $Prima::Config::Config{ldlibflag}.$_} @LIBS);
	}
	if ( $Win32) {
		if ( $Prima::Config::Config{compiler} eq 'bcc32') {
			$ret .= ',' . $dstf . ', ,' . join(' ', @LIBS) . ','.$deffile.',';
		} else {
			$ret .= ' ' . $Prima::Config::Config{lddefflag} . $deffile;
		}
	} elsif ( $OS2) {
		$ret .= ' ' . $deffile;
	}
	return $ret;
}

sub generate_def
{
	my ( $file, $libid, @exports) = @_;
	return if !$Win32 && !$OS2;
	open PRIMADEF, ">$file" or die "Cannot create $file: $!";
	if ( $Win32) {
		$libid = [split(/::/, $libid)]-> [-1];
		print PRIMADEF <<EOF;
LIBRARY $libid
EXPORTS
EOF
		if ( $Prima::Config::Config{compiler} eq 'bcc32') {
			print PRIMADEF map { "\t_$_\n\t$_=_$_\n"} @exports;
		}
		else {
			print PRIMADEF map { "\t$_\n\t_$_ = $_\n"} @exports;
		}
	} else {
		$libid = &DynaLoader::mod2fname([split(/::/, $libid)]) if defined &DynaLoader::mod2fname;
		print PRIMADEF <<EOF;
LIBRARY $libid INITINSTANCE TERMINSTANCE
CODE LOADONCALL
DATA LOADONCALL NONSHARED MULTIPLE
EXPORTS
EOF
		print PRIMADEF map { "\t$_\n"} @exports;
	}
	close PRIMADEF;
}

sub addlib
{
	push @LIBS, $_[0] . $Prima::Config::Config{ldlibext};
}

sub dump_command
{
	my ( $line, $sub, $command, $ret) = ( 0, shift, shift, '');
	$ret .= "\t\@\$($command) \\\n" if scalar @_;
	for ( @_) {
		my $call = $sub-> ($_);
		if (( $line++ % 20) == 19) {
			$ret .= "\n\t\@\$($command) \\\n";
		} elsif ( $line <= scalar @_ && $line > 1) {
			$ret .=  " \\\n";
		}
		$ret .=  "\t$call";
	}
	$ret .=  "\n";
}

sub find_version
{
	my $name = $_[0];
	open F, $name or die "Cannot open $name:$!\n";
	my ($ver1, $ver2);
	while (<F>) {
		next unless m/\$VERSION[^\.\d]*(\d+)\.(\d+)/;
		$ver1 = $1, $ver2 = $2, last;
	}
	close F;
	die "Cannot find VERSION string in $name\n" unless defined $ver1;
	return ( $ver1, $ver2);
}

1;

__DATA__

=pod

=head1 NAME

Prima::Make - module for automated Makefile creating 

=head1 DESCRIPTION

Prima does not use standard C<Make::MakeMaker> module for
creating its Makefile, nor it encourages to use it for the build of
Prima-dependent C modules, although this is possible. For such
a module it is typical to use C<Prima::Make> inside its C<Makefile.PL>,
and use the methods that provide compilation and linking commands.

The module implements its own basic file operations, required for 
a typical installer: file copying, removing, creation of distributives etc.
See L<init> for details.

=head1 SYNOPSIS

	use Prima::Make;

	# check command line 
	exit unless init( @ARGV ); 

	# object file extension
	my $o  = $Prima::Config::Config{objext};

	# store .pm files under /File directory
	setvar( 'INSTALL_LIB', $PREFIX . qd( "/File"));

	# compile command
	my $cc = cc_command_line( "a.c", "a.$o");

	# so / dll name
	my $dl = dl_name( "a" );

	# link command
	my $ld = ld_command_line( "a.c", "a.def", "a.$o");

	open F, "> Makefile";
	print F <<MAKE;
   a.$o: a.c
   \t$cc

   $dl: a.$o
   \t$ld

   clean:
   \t$^X Makefile.PL --rm a.$o $ld

   install:
   \t$^X Makefile.PL --cp $ld $INSTALL_DL a.pm $INSTALL_LIB
   \t$^X Makefile.PL --cpbin a.pl $INSTALL_BIN
      
   MAKE
   close F;


=head2 API

=head2 Methods

=over

=item addlib LIBRARY

Adds LIBRARY to list of libraries, stored in C<@LIBS>.

=item canon_name FILENAME

Converts FILENAME to a canonized form, by eliminating C<..>
up-references in the path. 

=item cc_command_line C_FILE, O_FILE

Returns suggested command for compilation of a C_FILE into output object O_FILE.

=item dl_name NAME

Converts NAME without an extension into shared library / dll file name
by adding the OS-dependant extension. Under OS2 NAME can be modified to overcome
a problem that the system does not handle dll files with name longer than 8 letters.
This is the reason why under OS/2 Prima dll file is named PrimaDI.dll .

=item dump_command SUB, COMMAND, @PARAMS

Creates a line for executing COMMAND on set of PARAMS.
This method is used to overcome a problem when PARAMS array
is too long to be fit into the command line. The method
calls SUB for each of PARAMS array scalars, and breaks the
command line at each 20 parameters. SUB is an anonymous subroutine,
as an only parameter receiving PARAMS array member and returns
its value adapted for COMMAND.

=item env_true ENV_STRING

Returns a boolean value, reflecting whether the evironment variable is a true value,
( e.g. C<yes>, C<1>, C<on>, C<true> ) or not.

=item find_version MODULE

Reads MODULE file and extracts its version without actual MODULE load via C<use>.
If C<$VERSION> string is found there, returns two integers - major and minor file
version. Otherwise throws an exception.

=item generate_def DEF_FILE, LIBNAME, @EXPORTS

Writes DEF_FILE used in linking of LIBNAME ( library or dll file ).
DEF_FILE is used on Win32 and OS/2 to explicitly declare the
list of exported names. These names are passed in @EXPORTS array.

Does nothing under unix environment.

=item init @ARGV

Checks the command line by parsing C<@ARGV> array ( not C<@main::ARGV> ).
First, the checks are done if the module was invoked as an utility;
such a call is constructed with a command, started with a double hyphen,
for example, C<--cp a.pm /perl/site/lib>. The full list of commands
and actions follows:

=over

=item --cp SOURCE, DEST, [ SOURCE, DEST, ... ]

Copies SOURCE file to DEST. Can accept more than one pair.

=item --cpbin SOURCE, DEST, [ SOURCE, DEST, ... ]

Copies SOURCE file to DEST and modifies DEST so that it
presents an executable script; during the operation,
DEST can change both its extension and content. 
The method can accept more than one pair.

=item --dist TYPE, DISTFILE

Creates a distributive archive of the given TYPE,
which is either C<bin>, C<zip>, or C<tar>. These correspond to
binary archive, zip source archive, and tar source archive.
In case of the binary archive, DISTFILE is combined with OS type string.

=item --md PATH, [ PATH, ... ]

Creates one or more PATH directories by calling C<File::Path::mkpath>, which
can create a directory with more than one level at a time.

=item --rm FILE, [ FILE, ... ]

Unlinks one or more FILEs.

=back

If none of the options found, it is assumed that module is
called for generation of C<Makefile>. The command line
is parsed to see if it contains overridden variables. 
This allows the user to add, for example, paths to include and library files,
select installation directories, compiling and debugging switches etc.

The list of the user-overrideable variables is described in L<Variables> below.

=item find_cdeps C_FILE, DEPENDENCIES, INCLUDES

Scans C_FILE for the eventual dependencies of local include files.
DEPENDENCIES and INCLUDES are hash references, used for
global dependency accounting. DEPENDENCIES keys are C files,
and its values are arrays of include files. INCLUDES keys are include
files, and values are always 1. All dependencies are also
cached internally, so successive calls to same C or include file do 
not result in file scan.

The method does not handle conditional defines.

=item find_file FILENAME, PATH

Recursively searches for FILENAME in PATH; returns two scalars -
the full name and success flag.

=item ld_command_line DL_FILE, DEF_FILE, @O_FILES

Returns suggested command for linking of set of object files O_FILES,
with an eventual DEF_FILE into DL_FILE shared library / dll file.
The name of DL_FILE is returned by C<dl_name>.

=item qd PATH

Returns PATH with standard directory separator C</> mapped to 
the OS-default one.

=item quoted_split STRING

Splits STRING by separate spaces, preserving the double-quoted text.
Returns array of split text chunks.

=item setvar VARIABLE, @DEFAULT

Sets the internal VARIABLE. If the command line contains user-preferred
settings, these are used. Otherwise, DEFAULT parameters are used.
The operations allowed are assignment ( C<DEBUG=1> ) 
and concatenation ( C<INCPATH+=/usr/local/include>).

=back

=head2 Variables

The variables influence many aspects of Makefile generation.
The user can tune the build process by passing variables in
the command line, which are parsed in C<init>.

=over

=item $DEBUG

Boolean; if 1, the commands for preserving debugging symbols are 
emitted by C<cc_command_line> and C<ld_command_line>.

=item @INCPATH

Set of include files paths.

=item $INSTALL_BIN

Installation directory for executable scripts.

=item $INSTALL_DL

Installation directory for shared objects / dll files.

=item $INSTALL_EXAMPLES

Installation directory for example executable scripts.

=item $INSTALL_LIB

Installation directory for perl modules and include files.

=item @LIBPATH

Set of library files paths.

=item @LIBS

Set of library files used for linking.

=item $PREFIX

An installation prefix path, under which the installed files
will be copied. 

=item $Unix, $Win32, $OS2, $cygwin

Boolean flags for fast check if the current OS type is
unix, win32, or os2, or cygwin. For extended info use C<$^O> variable.

=back

=head1 BUGS

The module is very experimental. Many features
simply were not realized, for example, documentation in pod and man files.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<gencls>, L<IPA> ( http://www.prima.eu.org/IPA ).

=cut
