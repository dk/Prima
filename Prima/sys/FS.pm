package Prima::sys::FS;

use strict;
use warnings;
require Exporter;
use Symbol;
use Encode;
use Fcntl qw(O_RDONLY O_WRONLY O_RDWR O_CREAT O_TRUNC O_APPEND);
use Prima;
use Prima::Utils qw(
	chdir chmod getcwd link mkdir open_file rename rmdir unlink utime
	getenv setenv stat access getdir
);

use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT_OK = qw(
	chdir chmod getcwd link mkdir open rename rmdir unlink utime
	getenv setenv abs_path stat lstat access getdir
	_r _w _x _o _R _W _X _O _e _z _s _f _d _l _p _S _b _c _t _u _g _k _M _A _C
);
@EXPORT = @EXPORT_OK;

sub open(*;$*)
{
	my ( $handle, @p ) = @_;
	goto NATIVE unless @p;
	@p = m/^([\<\>\|\-\+\=\&]*])(.*)/ if 1 == @p;
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

	use Symbol ();
        $_[0] = Symbol::geniosym unless defined $_[0];
        $handle = Symbol::qualify_to_ref($_[0], scalar caller);

	my $ok = open $handle, "$mode&=", $fd;
	return unless $ok;
	binmode($handle, ":$_") for @layers;
	return $ok;

NATIVE:
	if ( 0 == @p ) {
		return CORE::open($handle);
	} elsif ( 1 == @p ) {
		return CORE::open($handle, $p[0]);
	} elsif ( 2 == @p ) {
		return CORE::open($handle, $p[0], $p[1]);
	} else {
		my ( $x, $y ) = (shift @p, shift @p);
		return CORE::open($handle, $x, $y, @p);
	}
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
