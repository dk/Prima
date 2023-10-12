=pod

=head1 NAME

examples/extlist.pl - check-list widget

=head1 FEATURES

Demonstrates basic usage of the CheckList class.

=cut

use strict;
use warnings;
use Prima;
use Prima::ExtLists;
use Prima::Application;

my $w = Prima::MainWindow-> create(
	size => [ 200, 200],
	designScale => [7,16],
);
my $v = '';
vec($v, 0, 8) = 0x77;
$w-> insert( 'Prima::CheckList' =>
	pack     => { fill => 'both', expand => 1},
	items    => [qw( 'SpaceBar' toggles selection 'Enter' toggles checkbox )],
	multiColumn => 1,
	vertical => 0,
	multiSelect => 1,
	vector   => $v,
	extendedSelect => 0,
);

run Prima;
