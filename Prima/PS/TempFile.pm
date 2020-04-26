package Prima::PS::TempFile;

use strict;
use warnings;

sub new
{
	my $tmpdir = $ENV{TMPDIR} // $ENV{TEMPDIR} // (($^O =~ /win/i) ? ($ENV{TEMP} // "$ENV{SystemDrive}\\TEMP") : "/tmp");
	my $id = unpack('H*', pack('f', rand(10 ** rand(37))));

	my %self;
	$self{filename} = "$tmpdir/p-$id-$$.ps";
	if ( open( my $f, '+>', $self{filename})) {
		$self{fh} = $f;
	} else {
		warn "cannot create temp file $self{filename}: $!\n";
		return undef;
	}
	unlink $self{filename};

	return bless \%self, shift;
}

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
	$blocksize //= 10240;

	seek( $self->{fh}, 0, 0);
	while (1) {
		my $buf = '';
		my $n = sysread( $self->{fh}, $buf, $blocksize);
		if ( !defined $n) {
			warn "cannot read back from temp file $self->{filename}: $!\n";
			return 0;
		}
		last unless $n;
		return 0 unless $cb->($buf);
	}
	return 1;
}

1;
