=pod

=head1 NAME

examples/buttons2.pl - Prima button widgets

=head1 FEATURES

Demonstrates the variety of built-in buttons functionality.
Note the "Bits for toolbar" button, which copies and
pastes its image in the clipboard.

=cut

use strict;
use warnings;

use Prima qw(Buttons StdBitmap Application);

my $w = Prima::MainWindow-> new(
	text=> "Handmade buttons",
	size => [ 300, 200],
	centered => 1,
	popupItems => [['Hallo!' => '']],
	designScale => [6,16],
);

$w-> insert( CheckBox =>
	origin => [ 10, 150],
	selectable => 1,
	text => "~Check box",
	hint => 'Check box!',
	popupItems => [['Hallo2!' => '']],
);

$w-> insert( Radio =>
	origin => [ 190, 150],
	selectable => 1,
	text => "~Radio button",
	hint => 'Radio!',
);

my $i = Prima::StdBitmap::icon( sbmp::GlyphCancel);
my $j = Prima::StdBitmap::icon( sbmp::GlyphOK);

$w-> insert( Button =>
	origin => [ 10, 100],
	text => "~Default style",
	default => 1,
	hint => 'Nice?',
);

$w-> insert( Button =>
	origin  => [ 10, 55],
	text => "~BitBlt",
	image   => $i,
	glyphs  => 2,
	hint => 'Sly button',
);


$w-> insert( Button =>
	origin  => [ 10, 10],
	text => "Disab~led",
	image   => $i,
	glyphs  => 2,
	enabled => 0,
	hint => 'See me?',
);

$w-> insert( Button =>
	origin     => [ 130, 10],
	text    => "Bits for toolbar",
	image      => $i,
	glyphs     => 2,
	vertical   => 1,
	height     => 120,
	imageScale => 3,
	selectable => 0,
	flat       => 1,
	hint       => "Pressing this button copies its image\n into clipboard",
	onClick    => sub {
		my $self = $_[0];
		unless ( $self-> get_shift_state & km::Ctrl) {
			my $i = Prima::Image-> new(
				width  => $self-> width,
				height => $self-> height,
				font   => $self-> font,
				type   => im::Byte,
			);
			$i-> begin_paint;
			$self-> notify(q(Paint), $i);
			$i-> end_paint;
			$::application-> Clipboard-> image($i);
		} else {
			my $i = $::application-> Clipboard-> image;
			$self-> set(
				glyphs     => 1,
				imageScale => 1,
				image      => $i,
			) if $i;
		}
	},
);

$w-> insert( PopupButton =>
	origin     => [ 250, 10],
	image      => $j,
	glyphs     => 2,
	text    => 0,
	holdGlyph  => 1,
	popupItems => [
		['File' => sub {}],
		['Edit' => sub {}],
	],
);


run Prima;
