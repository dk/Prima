=pod

=head1 NAME

examples/helloworld.pl - A "hello world" program

=head1 FEATURES

A very basic Prima toolkit usage is demonstrated

=cut

use strict;
use warnings;
use Prima;
use Prima::Application name => 'Generic';

my $w = Prima::MainWindow->new(
	sizeMin => [200, 200],
	text    => "Hello, world!",
	font    => { size => 30 },
	onPaint => sub {
		my ( $self, $canvas) = @_;
		$canvas-> clear;
		$canvas->matrix->translate(100, 100)->rotate(10)->apply($canvas);
		$canvas-> text_shape_out( $self-> text, 0, 0);
	},
);

$w-> insert( Timer =>
	timeout => 2000,
	onTick  => sub { $w-> width( $w-> width - 50) },
)-> start;

run Prima;
