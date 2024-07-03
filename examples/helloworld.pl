=pod

=head1 NAME

examples/helloworld.pl - a "hello world" program

=head1 FEATURES

A very basic Prima demo

=cut

use strict;
use warnings;
use Prima qw(StdBitmap);
use Prima::Application name => 'Generic';

my $p = Prima::StdBitmap::image(0);

my $w = Prima::MainWindow->new(
	sizeMin => [200, 200],
	text    => "Hello, world!",
	font    => { size => 30 },
	onPaint => sub {
		my ( $self, $canvas) = @_;
		$canvas-> clear;
		$canvas->matrix->translate(100, 100)->rotate(10);
		$canvas->stretch_image(100, 100, 300, 300, $p);
		$canvas-> text_shape_out( $self-> text, 0, 0);
	},
);

$w-> insert( Timer =>
	timeout => 2000,
	onTick  => sub { $w-> width( $w-> width - 50) },
)-> start;

run Prima;
