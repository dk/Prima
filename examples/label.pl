=pod

=head1 NAME

examples/label.pl - Prima label widget

=head1 FEATURES

Demonstrates the basic usage of a Prima toolkit
and L<Prima::Label> class capabilites, in particular
text wrapping.

=cut

use strict;
use warnings;
use Prima;
use Prima::Const;
use Prima::Buttons;
use Prima::Label;
use Prima::Application;

my $w = Prima::MainWindow-> create(
	size => [ 430, 200],
	text => "Static texts",
	designScale => [7, 16],
);

my $b1 = $w->insert( Button => left => 20 => bottom => 0);

$w->insert( Label =>
# font => { height => 24},
	origin => [ 20, 50],
	text => "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et ".
"dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo ".
"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. ".
"Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
	focusLink => $b1,
	wordWrap => 1,
	height => 80,
	width => 212,
	growMode => gm::Client,
	showPartial => 0,
	textJustify => 1,
);

my $b2 = $w->insert( Button =>
	left => 320,
	bottom => 0,
	growMode => gm::GrowLoX,
);

$w->insert(
	Label      => origin   => [ 320, 50],
	text       => 'Disab~led',
	focusLink  => $b2,
	autoHeight => 1,
	enabled    => 0,
	growMode   => gm::GrowLoX,
	textJustify => 1,
);


run Prima;

