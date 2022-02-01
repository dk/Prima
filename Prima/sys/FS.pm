package Prima::sys::FS;

use strict;
use warnings;
require Exporter;
use Symbol ();
use Scalar::Util qw(readonly);
use Encode;
use Fcntl qw(O_RDONLY O_WRONLY O_RDWR O_CREAT O_TRUNC O_APPEND);
use Prima;
use Prima::Utils qw(
	chdir chmod closedir getcwd link mkdir open_dir open_file
	read_dir rename rmdir unlink utime
	getenv setenv stat access getdir
	seekdir telldir rewinddir
);

use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT_OK = qw(
	chdir chmod getcwd link mkdir open opendir readdir closedir 
	rename rmdir unlink utime
	getenv setenv abs_path stat lstat access getdir
	seekdir telldir rewinddir glob
	_r _w _x _o _R _W _X _O _e _z _s _f _d _l _p _S _b _c _t _u _g _k _M _A _C
);
@EXPORT = @EXPORT_OK;

sub open(*;$*)
{
	my ( $handle, @p ) = @_;
	goto NATIVE unless @p;
	$p[0] =~ m/^([\<\>\|\-\+\=\&]*])(.*)/ if 1 == @p;
	my ( $mode, $what, @rest) = @p;
	goto NATIVE if !defined($what) || ref($what);
	goto NATIVE if $what =~ /[\-\|\=\&]/;

	my $flags;
	my @layers;

	if ( $mode =~ /^([^:\s]+)(.+)$/ ) {
		$mode = $1;
		my $binmode = $2;
		$binmode =~ s/^\s+//;
		$binmode =~ s/\s+$//;
		@layers = grep { length } split /[:\s]/, $binmode if length $binmode;
	}

	if ( $mode eq '>') {
		$flags = O_CREAT | O_WRONLY | O_TRUNC;
	} elsif ( $mode eq '>>') {
		$flags = O_CREAT | O_APPEND;
	} elsif ( $mode eq '<' ) {
		$flags = O_RDONLY;
	} elsif ( $mode eq '>+' ) {
		$flags = O_CREAT | O_RDWR;
	} elsif ( $mode eq '>>+' ) {
		$flags = O_CREAT | O_RDWR | O_APPEND;
	} elsif ( $mode eq '<+' ) {
		$flags = O_CREAT | O_RDWR;
	} elsif ( $mode eq '+>' ) {
		$flags = O_CREAT | O_RDWR | O_TRUNC;
	} elsif ( $mode eq '+>>' ) {
		$flags = O_CREAT | O_RDWR | O_APPEND | O_TRUNC;
	} elsif ( $mode eq '+<' ) {
		$flags = O_CREAT | O_RDWR | O_TRUNC;
	} else {
		goto NATIVE;
	}

	my $fd = open_file( $what, $flags );
	return if $fd < 0;

        $_[0] = Symbol::geniosym unless defined $_[0];
        $handle = Symbol::qualify_to_ref($_[0], scalar caller);

	my $ok = open $handle, "$mode&=", $fd;
	return unless $ok;
	binmode($handle, ":$_") for @layers;
	return $ok;

NATIVE:
	goto &CORE::open;
}

sub opendir(*$)
{
	if ( readonly($_[0])) {
		warn "Prima::sys::FS::opendir: cannot be use on filehandles, variables only\n";
		return;
	}
	$_[0] = open_dir( $_[1] );
	return defined $_[0];
}

sub readdir($)
{
	my $dh = shift;

	if ( wantarray ) {
		my @ret;
		while ( defined( my $f = read_dir($dh)) ) {
			push @ret, $f;
		}
		return @ret;
	} else {
		return read_dir($dh);
	}
}

sub glob
{
	my $pat = shift;
	my @pats;
	while ( 1 ) {
		$pat =~ m/\G"((?:[^"]|\\")*)(?<!\\)"/gcs and push @pats, $1 and next;
		$pat =~ m/\G'((?:[^']|\\')*)(?<!\\)'/gcs and push @pats, $1 and next;
		$pat =~ m/\G((?:\S|\\\s)+)/gcs and push @pats, $1 and next;
		$pat =~ m/\G\s+/gcs and next;
		$pat =~ m/\G$/gcs and last;
	}
	my @matches = @pats;
	@pats = ();
	my $win32 = $^O =~ /win32/i;
	MATCH: while ( my $q = shift @matches ) {
		if ( $q =~ m/^(.*)\{([^}]*)\}(.*)$/ ) {
			my ( $pre, $subpat, $post ) = ( $1, $2, $3 );
			push @matches, map { "$pre$_$post" } split /,/, $subpat;
		} elsif ( $q =~ m/^(.*)\[([^\]]*)\](.*)$/ ) {
			my ( $pre, $subpat, $post ) = ( $1, $2, $3 );
			push @matches, map { "$pre$_$post" } split //, $subpat;
		} elsif ( $q =~ m/^~(\w*)(.*)/ ) {
			my @pwent;
			unless ( length $1 ) {
				push @matches, ($ENV{HOME} // ($win32 ? $ENV{USERPROFILE} : undef) // '/' ) . $2;
			} elsif (!$win32 && (@pwent = getpwnam($1)) && defined($pwent[7])) {
				push @matches, $pwent[7] .  $2;
			}
		} elsif ( $q =~ m/(?<!\\)\*|\?/ ) {
			my @paths = ('');
			my $expanded;
			for my $subpath ( split m{(/)}, $q ) {
				if ( !$expanded && $subpath =~ m/(?<!\\)\*|\?/ ) {
					$subpath =~ s/(?<!\\)\*/.*/g;
					$subpath =~ s/(?<!\\)\?/./g;
					$subpath = qr/$subpath/;
					next MATCH unless Prima::sys::FS::opendir( my $dh, length($paths[0]) ? $paths[0] : '.' );
					my $opath = pop @paths;
					for my $e ( Prima::sys::FS::readdir $dh ) {
						next unless $e =~ /^$subpath$/;
						push @paths, $opath . $e;
					}
					Prima::Utils::closedir $dh;
					$expanded++;
				} else {
					$_ .= $subpath for @paths;
				}
			}
			push @matches, @paths;
		} elsif (_e($q)) {
			push @pats, $q;
		}
	}

	return @pats;
}

sub lstat { Prima::Utils::stat($_[0], 1) }

sub __x(&$) {
	my @p = Prima::Utils::stat($_[1]);
	return undef unless scalar @p;
	$_[0]->(@p);
}

sub __f($$) {
	no strict 'refs';
	my @p = Prima::Utils::stat($_[1]);
	return undef unless scalar @p;
	return undef unless ${'Fcntl::'}{$_[0]};
	my $c = Fcntl->can($_[0])->();
	return (($c & $p[2]) == $c) ? 1 : 0;
}

sub _l ($) {
	no strict 'refs';
	my @p = Prima::Utils::stat($_[1], 1);
	return undef unless scalar @p;
	return undef unless ${'Fcntl::'}{S_IFLNK};
	my $c = Fcntl->can('S_IFLNK')->();
	return (($c & $p[2]) == $c) ? 1 : 0;
}

sub _r ($) { access($_[0], 4, 1) >= 0 }
sub _w ($) { access($_[0], 2, 1) >= 0 }
sub _x ($) { access($_[0], 1, 1) >= 0 }
sub _o ($) { __x sub { $> == $_[4] }, $_[0] }
sub _R ($) { access($_[0], 4, 0) >= 0 }
sub _W ($) { access($_[0], 2, 0) >= 0 }
sub _X ($) { access($_[0], 1, 0) >= 0 }
sub _O ($) { __x sub { $< == $_[4] }, $_[0] }
sub _e ($) { __x sub { 1 }, $_[0] }
sub _z ($) { __x sub { 0  == $_[7] }, $_[0] }
sub _s ($) { __x sub { $_[7] }, $_[0] }
sub _f ($) { __f S_IFREG  => $_[0] }
sub _d ($) { __f S_IFDIR  => $_[0] }
sub _p ($) { __f S_IFFIFO => $_[0] }
sub _S ($) { __f S_IFSOCK => $_[0] }
sub _b ($) { __f S_IFBLK  => $_[0] }
sub _c ($) { __f S_IFCHR  => $_[0] }
sub _t ($) { -t $_[0] }
sub _u ($) { __f S_ISUID  => $_[0] }
sub _g ($) { __f S_ISGID  => $_[0] }
sub _k ($) { __f S_ISVTX  => $_[0] }
sub _A ($) { __x sub { ( time - $_[8]  ) / 86400 }, $_[0] }
sub _M ($) { __x sub { ( time - $_[9]  ) / 86400 }, $_[0] }
sub _C ($) { __x sub { ( time - $_[10] ) / 86400 }, $_[0] }

# adapted from Cwd.pm
sub abs_path
{
	unless ( $^O =~ /win32|cygwin/i ) {
		require Cwd;
		my $p = $_[0];
		my $was_utf8 = Encode::is_utf8($p);
		$p = Cwd::abs_path($p);
		$p = Encode::decode('utf-8', $p) if $was_utf8;
		return $p;
	}

	my $cwd = Prima::Utils::getcwd();
	defined $cwd or return undef;

	my $path = @_ ? shift : '.';
	unless (_e $path) {
		require Errno;
		$! = Errno::ENOENT();
		return undef;
	}

	unless (_d $path) {
		# Make sure we can be invoked on plain files, not just directories.
		require File::Spec;
		my ($vol, $dir, $file) = File::Spec->splitpath($path);
		return File::Spec->catfile($cwd, $path) unless length $dir;

		return $dir eq File::Spec->rootdir
			? File::Spec->catpath($vol, $dir, $file)
			: abs_path(File::Spec->catpath($vol, $dir, '')) . '/' . $file;
	}

	return undef unless Prima::Utils::chdir($path);
	my $realpath = Prima::Utils::getcwd();
	if (! ((_d $cwd) && (Prima::Utils::chdir($cwd)))) {
		_croak("Cannot chdir back to $cwd: $!");
	}

	return $realpath;
}

1;

=pod

=head1 NAME

Prima::sys::FS - unicode-aware core file functions

=head1 DESCRIPTION

Since perl win32 unicode support for files is unexistent, Prima has its own
parallel set of functions mimicking native functions, ie open, chdir etc. This
means that files with names that cannot be converted to ANSI (ie
user-preferred) codepage are not visible in perl, but the functions below
mitigate that problem.

This module exports the unicode-aware functions from C<Prima::Utils> to override
the core functions. Read more in L<Prima::Utils/"Unicode-aware filesystem functions">.

=head2 SYNOPSIS

  use Prima::sys::FS;

  my $fn = "\x{dead}\x{beef};
  if ( _f $fn ) {
     open F, ">", $fn or die $!;
     close F;
  }
  print "ls: ", getdir, "\n";
  print "pwd: ", getcwd, "\n";

=head1 API

The module exports by default three groups of functions:

These are described in L<Prima::Utils/API>:

  chdir chmod getcwd link mkdir open rename rmdir unlink utime
  getenv setenv stat access getdir
  opendir closedir rewinddir seekdir readdir telldir

The underscore-prefixed functions are same as the ones in L<perlfunc/-X> (all are present except -T and -B ).

  _r _w _x _o _R _W _X _O _e _z _s _f _d _l _p _S _b _c _t _u _g _k _M _A _C

The functions that are implemented in the module itself:

=over

=item abs_path

Same as C<Cwd::abs_path>.

=item glob PATTERN

More or less same as C<CORE::glob> or C<File::Glob::glob>.

=item lstat PATH

Same as C<CORE::lstat>

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Utils>, L<Win32::Unicode>.

