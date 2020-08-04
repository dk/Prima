package Prima::PS::TempFile;

use strict;
use warnings;

sub new
{
	my ( $class, %opt ) = @_;
	my $tmpdir = $ENV{TMPDIR} // $ENV{TEMPDIR} // (($^O =~ /win/i) ? ($ENV{TEMP} // "$ENV{SystemDrive}\\TEMP") : "/tmp");
	my $id = unpack('H*', pack('f', rand(10 ** rand(37))));

	my %self;
	$self{filename} = "$tmpdir/p-$id-$$.ps";
	if ( open( my $f, '+>', $self{filename})) {
		$self{fh} = $f;
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

	return bless \%self, $class;
}

sub DESTROY
{
	unlink $_[0]->{filename} if $_[0]->{force_unlink};
}

sub remove { unlink shift->{filename} }

sub write
{
	my $self = shift;
	my $n = syswrite $self->{fh}, $_[0];
	unless (defined $n) {
		warn "cannot write to temp file $self->{filename}: $!\n";
		return 0;
	}
	return 1;
}

sub evacuate
{
	my ($self, $cb, $blocksize) = @_;
	$blocksize //= 16384;
	my $ok = 0;

	seek( $self->{fh}, 0, 0);
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
