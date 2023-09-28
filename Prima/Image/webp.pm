package Prima::Image::webp;

use strict;
use warnings;
use Prima qw(Image::Animate VB::VBLoader);

sub animation_to_frames
{
	my $a = Prima::Image::Animate::WebP->new(images => \@_);
	my @ret;
	for ( @_ ) {
		$a->next;
		my $n = (ref eq 'Prima::Icon') ? $a->icon : $a->image;
		$n->{extras} = $_->{extras} if exists $_->{extras};
		push @ret, $n;
	}
	return @ret;
}

sub cc($)  { join '', map { ucfirst } split '_', shift }
sub fields { qw( compression quality method filter_strength) }

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	my $e = $image-> {extras} //= {};
	$self-> bring(cc $_)-> text($e->{$_} // $codec->{saveInput}->{$_}) for fields;
}

sub OK_Click
{
	my $self = $_[0]-> owner;
	$self-> {image}-> {extras} ||= {};
	my $e = $self-> {image}-> {extras};
	$e->{$_} = $self->bring(cc $_)->text for fields;
	delete $self-> {image};
}

sub Cancel_Click
{
	my $self = $_[0]-> owner;
	delete $self-> {image};
}

sub save_dialog
{
	my $codec = $_[1];
	my $dialog = Prima::VBLoad( 'Prima::Image::webp.fm',
		Form1 => {
			visible => 0,
			onChange  => \&on_change,
		},
		OK      => { onClick => \&OK_Click },
		Cancel  => { onClick => \&Cancel_Click },
	);
	Prima::message($@) unless $dialog;
	return $dialog;
}

1;
