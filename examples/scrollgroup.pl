=pod

=head1 NAME

examples/scrollwidget.pl - scrolling dialog panel

=head1 FEATURES

Panel with widgets that is too big for the screen

=cut

use strict;
use warnings;
use Prima qw(Widget::ScrollWidget InputLine Application);

my $w = Prima::MainWindow->new( packPropagate => 0);

my $sc = $w->insert( ScrollGroup =>
	pack => { expand => 1, fill => 'both' },
);
$sc->insert( InputLine => pack => { side => 'bottom' }) for 1..20;
$w->packPropagate(1);
$w->height( $w->height / 2);


run Prima;
