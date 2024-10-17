=pod

=head1 NAME

examples/frames.pl - dynamic subwindow panels

=head1 FEATURES

Demonstrates C<Prima::FrameSet> API

=cut

use strict;
use warnings;
use Prima qw(Buttons FrameSet Application);

my $w = Prima::MainWindow-> new(
	text   => "Frames example",
	size => [ map { $_ - 128} $::application-> size],
);

my $frame = $w-> insert(
	FrameSet =>
	size => [$w-> size],
	origin => [0, 0],
	frameSizes => [qw(211 20% 123 10% * 45% *)],
	frameProfiles => [ 0,0, { minFrameWidth => 123, maxFrameWidth => 123 }],
);

for (my $i = 0; $i < 6; $i++) {
	next if $i == 4;
	$frame-> insert_to_frame(
		$i,
		Button =>
		origin => [10, 10],
		text => '~Ok',
		onClick => sub {
			$_[0]-> text($_[0]-> owner-> name);
		},
	);
}

my $subframe = $frame-> insert_to_frame(
	4,
	FrameSet =>
	vertical => 1,
	size => [$frame-> frames-> [4]-> size],
	origin => [0, 0],
	opaqueResize => 0,
	frameSizes => [qw(33% 33% *)],
);

run Prima;

