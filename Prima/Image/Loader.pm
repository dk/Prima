package Prima::Image::Loader;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, $source, %opt ) = @_;

	my $img = Prima::Image->new;
	my $ok = $img->load( $source,
		loadExtras => 1,
		wantFrames => 1,
		loadAll    => 1,
		%opt,
		session => 1
	);
	unless ($ok) {
		$img->destroy;
		return (undef, $@);
	}

	return bless {
		image  => $img,
		extras => $img->{extras} // {},
	}, $class;
}

sub next
{
	my ( $self, %opt ) = @_;

	return (undef, $self->{error}) if defined $self->{error};

	my @img = $self->{image}->load( undef, %opt, session => 1 );
	if ( $img[0] ) {
		my $e = $img[0]->{extras};
		%{ $self->{extras} } = ( %{ $self->{extras} }, %$e ) if $e;
		$self->{frames} = $self->{extras}->{frames};
		$self->{loaded}++;
		return $img[0];
	} else {
		$self->{error} = @img ? $@ : "End of image list";
		$self->{image}->destroy;
		$self->{image} = undef;
		return (undef, $self->{error});
	}
}

sub eof
{
	my $self = shift;
	return 1 if defined $self->{error};
	my $frames = $self->{frames} // -1;
	return 0 if $frames < 0;
	my $loaded = $self->{loaded} // 0;
	return $loaded >= $frames;
}

sub loaded { shift->{loaded} }
sub extras { shift->{extras} }
sub frames { shift->{frames} }
sub DESTROY { $_[0]->{image}-> destroy if $_[0]->{image} }

1;

