=pod

=head1 NAME

examples/generic.pl - A "hello world" program

=head1 FEATURES

A very basic Prima toolkit usage is demonstrated

=cut

use strict;
use warnings;
use Prima;
use Prima::Application name => 'Generic';

my $w = Prima::MainWindow->new(
	text => "Hello, world!",
	onClose => sub {
		$::application-> destroy;
	},
	onPaint   => sub {
		my ( $self, $canvas) = @_;
		my $color = $self-> color;
		$canvas-> color( $self-> backColor);
		$canvas-> bar( 0, 0, $canvas-> size);
		$canvas-> color( $color);
		$canvas-> text_shape_out( $self-> text, 10, 10);
	},
);

$w-> insert( Timer =>
timeout => 2000,
onTick => sub {
	$w-> width( $w-> width - 50);
},
) -> start;

run Prima;
