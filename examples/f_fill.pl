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
#  $Id$
#
=pod 

=head1 NAME

examples/f_fill.pl - A gradient fill example

=head1 FEATURES

Demonstrates the usage of graphic context regions.
Note that the $i region is not created, but is assigned
on every onPaint. Tests whether image object is able
to hold a cached region copy.

=cut

use strict;
use Prima qw(Application);

my $i = Prima::Image-> create(
	preserveType => 1,
	type => im::BW,
	font => { size => 100, style => fs::Bold|fs::Italic },
);
$i-> begin_paint_info;
my $textx = $i-> get_text_width( "PRIMA");
my $texty = $i-> font-> height;
$i-> end_paint_info;
$i-> size( $textx + 20, $texty + 20);

my @is = $i-> size;
$i-> begin_paint;
$i-> color( cl::Black);
$i-> bar(0,0,@is);
$i-> color( cl::White);
$i-> text_out( "PRIMA", 0,0);
$i-> end_paint;


sub gradient_circle
{
	my ( $canvas, $x, $y, $diameter, $request ) = @_;
	my $gradient = $request->{gradient} //= $canvas-> gradient_realize3d( $diameter, $request );
	for ( my $i = 0; $i < @$gradient; $i+=2) {
		$canvas->color( $gradient->[$i]);
		$canvas->fill_ellipse( $x, $y, $diameter, $diameter );
		$diameter -= $gradient->[$i+1];
	}
}

my $w = Prima::MainWindow-> create(
	size   => [ @is],
	centered => 1,
	buffered => 1,
	palette => [ map { ($_) x 3 } 0..255 ],
	onPaint => sub {
	my ( $self, $canvas) = @_;
		$canvas->clear;
		gradient_circle($canvas, $is[0]/2 ,$is[1]/2, $is[0], { palette => [ cl::White, cl::Black ] });
		$canvas-> region( $i);
		gradient_circle($canvas, $is[0]/2 ,$is[1]/2, $is[0], { palette => [ cl::Black, cl::White ] });
	},
);

run Prima;
