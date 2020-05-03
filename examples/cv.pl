=pod

=head1 NAME

examples/cv.pl - Standard color dialog

=head1 FEATURES

Demonstrates usage of a standard color dialog.
Note the left-button drag effect from the color wheel with
compbinations of Shift,Alt,and Control.

=cut

use strict;
use warnings;
use Prima 'Dialog::ColorDialog', 'Application';

my $p = Prima::Dialog::ColorDialog-> create(
	value => 0x3030F0,
	visible => 1,
	quality => 1,
);

my $banner = $p-> {wheel}-> insert( Label =>
	text => <<MSG,
Drag colors from the color wheel by left mouse button together with combinations of Alt, Shift, and Control
MSG
	autoHeight => 1,
	wordWrap   => 1,
	transparent => 1,
	alignment => ta::Center,
	left  => $p-> {wheel}-> width * 0.125,
	top => 0,
	width => $p-> {wheel}-> width * 0.75,
);

$p-> insert( Timer =>
	timeout => 100,
	onTick  => sub {
		if ( $banner-> bottom > $p->{wheel}-> height) {
			$_[0]-> destroy;
		} else {
			$banner-> bottom( $banner-> bottom + 2);
		}
	},
)-> start;

$p-> execute;
