#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#

=pod

=head1 NAME

examples/buttons2.pl - Prima button widgets

=head1 FEATURES

Demonstrates the variety of built-in buttons functionality.
Note the "Bits for toolbar" button, which copies and
pastes its image into the clipboard.

=cut

use strict;
use warnings;

use Prima qw(Buttons StdBitmap), Application => { name => 'Buttons sample' };

my $w = Prima::MainWindow-> create(
text=> "Handmade buttons",
size => [ 300, 200],
centered => 1,
popupItems => [['Hallo!' => '']],
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
			my $i = Prima::Image-> create(
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

$w-> insert( SpeedButton =>
	origin     => [ 250, 10],
	image      => $j,
	glyphs     => 2,
	text    => 0,
	checkable  => 1,
	holdGlyph  => 1,
	hint       => 'C\'mon, press me!',
);


run Prima;
