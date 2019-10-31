package Prima::Image::png;

use strict;
use warnings;
use Prima qw(Image::Animate);

sub animation_to_frames
{
	my $a = Prima::Image::Animate::APNG->new(images => \@_);
	my @ret;
	for ( @_ ) {
		$a->next;
		my $n = (ref eq 'Prima::Icon') ? $a->icon : $a->image;
		$n->{extras} = $_->{extras} if exists $_->{extras};
		push @ret, $n;
	}
	return @ret;
}

1;
