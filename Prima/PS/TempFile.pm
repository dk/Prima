package Prima::PS::TempFile;

use strict;
use warnings;

eval "use Compress::Raw::Zlib;";
my $use_zlib = !$@;

sub new_filename
{
	my $tmpdir = $ENV{TMPDIR} // $ENV{TEMPDIR} // (($^O =~ /win/i) ? ($ENV{TEMP} // "$ENV{SystemDrive}\\TEMP") : "/tmp");
	my $id = unpack('H*', pack('f', rand(10 ** rand(37))));
	return "$tmpdir/p-$id-$$.ps";
}

sub new
{
	my ( $class, %opt ) = @_;

	my %self = ( filename => new_filename );
	if ( open( my $f, '+>', $self{filename})) {
		$self{fh} = $f;
		binmode $self{fh};
	} else {
		warn "cannot create temp file $self{filename}: $!\n" if $opt{warn} // 1;
		return undef;
	}
	if ($opt{unlink} // 1) {
		my $ok = unlink $self{filename};
		unless ($ok) {
			# win32 doesn't let it?
			$self{force_unlink} = 1;
		}
	}
	$self{size} = 0;

	$self{compress} = Compress::Raw::Zlib::Deflate->new if $opt{compress} && $use_zlib;

	return bless \%self, $class;
}

sub DESTROY
{
	if ($_[0]->{force_unlink}) {
		close $_[0]->{fh};
		unlink $_[0]->{filename};
	}
}

sub remove { unlink shift->{filename} }

sub is_deflated { $_[0]->{compress} ? 1 : 0 }

sub size { $_[0]-> {size} }

sub write
{
	my $self = shift;

	if ( $self-> {compress} ) {
		my $output = '';
		$self->{compress}->deflate($_[0], $output);
		local $self->{compress} = undef;
		return $self-> write($output);
	}

	my $n = syswrite $self->{fh}, $_[0];
	unless (defined $n) {
		warn "cannot write to temp file $self->{filename}: $!\n";
		return 0;
	}
	$self->{size} += length $_[0];
	return 1;
}

sub reset
{
	my ($self) = @_;
	if ( $self-> {compress} ) {
		my $output = '';
		$self->{compress}->flush($output);
		$self->{compress} = undef;
		$self-> write($output);
	}
	seek( $self->{fh}, 0, 0);
}

sub tell
{
	my $self = shift;
	return CORE::tell( $self->{fh} );
}

sub seek
{
	my ($self, $pos) = @_;
	return CORE::seek( $self->{fh}, $pos, 0 );
}

sub read
{
	my ( $self, $bytes ) = @_;
	my $buf = '';
	my $n = sysread( $self->{fh}, $buf, $bytes);
	return undef if !defined $n;
	return $buf;
}

sub read_str
{
	my $self = shift;
	my $buf = '';
	my $n = sysread( $self->{fh}, $buf, 2);
	return undef if !defined $n;
	my $bytes = unpack('n', $buf);
	$buf = '';
	$n = sysread( $self->{fh}, $buf, $bytes);
	return undef if !defined $n;
	return $buf;
}

sub evacuate
{
	my ($self, $cb, $blocksize) = @_;

	$self->reset;
	$blocksize //= 16384;
	my $ok = 0;

	while (1) {
		my $buf = '';
		my $n = sysread( $self->{fh}, $buf, $blocksize);
		if ( !defined $n) {
			warn "cannot read back from temp file $self->{filename}: $!\n";
			goto EXIT;
		}
		last unless $n;
		goto EXIT unless $cb->($buf);
	}

	$ok = 1;
EXIT:
	close $self->{fh};
	return $ok;
}

1;

=pod

=head1 NAME

Prima::PS::TempFile - store parts of PS output in files

=head1 DESCRIPTION

Temp files are allocated, then are written to, accumulating PS code.
Then the code is read back and is sent to main PS file.

=cut
