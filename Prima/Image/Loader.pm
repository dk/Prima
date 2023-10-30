package Prima::Image::Loader;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, $source, %opt ) = @_;

	$opt{className} = 'Prima::Icon' if delete $opt{icons};

	my $img = Prima::Image->new;
	my $ok = $img->load( $source,
		loadExtras => 1,
		wantFrames => 1,
		loadAll    => 1,
		%opt,
		session    => 1
	);
	unless ($ok) {
		$img->destroy;
		return (undef, $@);
	}
	if ( ($img->{extras}->{frames} // -1) < 0 ) {
		$img->destroy;
		return (undef, "cannot read number of frames");
	}

	return bless {
		source  => $source,
		image   => $img,
		extras  => $img->{extras},
		frames  => $img->{extras}->{frames},
		current => 0,
	}, $class;
}

sub rewind
{
	my ( $self, $index ) = @_;

	if (defined $self->{error} && $self->{error} ne "End of image list") {
		Carp::carp("cannot rewind after an error is encountered");
		return;
	}

	$index //= 0;
	$self->{rewind_request} = $index;
	$self->{current} = $index;
	delete $self->{error};
}

sub next
{
	my ( $self, %opt ) = @_;

	return (undef, $self->{error}) if defined $self->{error};

	my $new_frame;
	$new_frame = $opt{rewind} = delete $self->{rewind_request}
		if defined $self->{rewind_request};
	my @img = $self->{image}->load( undef, %opt, session => 1 );
	if ( $img[0] ) {
		my $e = $img[0]->{extras};
		%{ $self->{extras} } = ( %{ $self->{extras} }, %$e ) if $e;
		$self->{current} = $new_frame if defined $new_frame;
		$self->{current}++;
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
	my $current = $self->{current} // 0;
	return $current >= $self->{frames};
}

sub extras  { shift->{extras} }
sub frames  { shift->{frames} }
sub current { shift->{current} }
sub source  { shift->{source} }
sub DESTROY { $_[0]->{image}-> destroy if $_[0]->{image} }

1;

