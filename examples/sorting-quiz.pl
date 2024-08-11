=head1 NAME

examples/sorting-quiz.pl - sorting list box items

=head1 AUTHOR

Reinier Maliepaard

=cut

use strict;
use warnings;

use Prima qw(Application Buttons Label Lists MsgBox);

my $mw = Prima::MainWindow-> new(
	text        => "Move listbox items by Ctrl + left mouse",
	size        => [530, 400],
        font        => { size => 12, },
	borderStyle => bs::Dialog,
	borderIcons => bi::All & ~bi::Maximize,
);

$mw-> insert( Label =>
	origin      => [15, 300],
	size        => [500, 70],
	text        => "Order the events by date\n\nDrag the items by CTRL + left mouse button",
	alignment   => ta::Center,
);

my @events = (
	'Signing of the Declaration of Independence',
	'Start of World War II',
	'Moon landing',
	'Fall of the Berlin Wall',
);

my @years = (
	1776,
	1939,
	1969,
	1989,
);

my $lb = $mw-> insert(ListBox =>
	origin      => [15,  150],
	size        => [500, 100],
	dragable    => 1,
	items       => [ @events[3,0,2,1] ],
);

$mw-> insert(Button =>
	origin      => [200, 50],
	size        => [150, 50],
	text        => "Evaluate",
	default     => 1,
	onClick     => sub {
		my $arr = $lb-> items;
		my $str = join("\n", map {
			'C<' .
			(( $arr->[$_] eq $events[$_] ) ?
        			"Green|Correct: $years[$_] - " :
        			"LightRed|False: "
			) .
			$arr->[$_] .
			'>';
		} 0..$#$arr );
		message( \$str, mb::Ok );
	}
);

run Prima;
