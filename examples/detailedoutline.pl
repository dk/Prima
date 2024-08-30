=pod

=head1 NAME

examples/detailedoutline.pl - detailed outline

=head1 FEATURES

Demonstrates C<Prima::DetailedOutline> API.

=cut

use strict;
use warnings;
use Prima qw(Application DetailedOutline);

my $Main = Prima::MainWindow-> new(
	name   => 'Main',
	text   => 'DetailedOutline Test',
	origin => [50,50],
	size   => [500,500],
);

my $Outline = $Main-> insert('DetailedOutline',
	name   => 'Outline',
	origin => [5,5],
	size   => [490,490],
	columns => 2,
	headers => ['Column 1', 'Column 2'],
	autoRecalc => 1,
	growMode => gm::Client,
);

$Outline-> items([
	[ ['Item 1, Col 1', 'Item 1, Col 2'], [
		[ ['Item 1-1, Col 1', 'Item 1-1, Col 2'] ],
		[ ['Item 1-2, Col 1', 'Item 1-2, Col 2'], [
			[ ['Item 1-2-1, Col 1', 'Item 1-2-1, Col 2'] ],
		] ],
	] ],
	[ ['Item 2, Col 1', 'Item 2, Col 2'], [
		[ ['Item 2-1, Col 1', 'Item 2-1, Col 2'] ],
	] ],
]);

run Prima;
