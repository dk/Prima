use strict;
use Prima;
use Prima::Application name => 'Generic';

Prima::Window-> create(
    text => "Hello, world!",
    onDestroy => sub { $::application-> close},
    onPaint   => sub {
       my ( $self, $canvas) = @_;
       my $color = $self-> color;
       $canvas-> color( $self-> backColor);
       $canvas-> bar( 0, 0, $canvas-> size);
       $canvas-> color( $color);
       $canvas-> text_out( $self-> text, 10, 10);
    },
);

run Prima;
