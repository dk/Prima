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

examples/palette.pl - A palette functionality test

=head1 FEATURES

Test the Prima palette functionality on paletted
displays. True-color displays are of no interest here.

In theory, if a Prima::Widget::palette() is initialized,
the widget is expected to produce as much solid colors
from this palette as possible.

Note the $useImages that can be set to 1 to test
multiple images representation on a single widget.

=cut

use Prima qw(Application);

my $useImages = 0;

my $j = $::application;

my $i;
my @spal = ();
for ( $i = 0; $i < 33; $i++) {
	push( @spal, $i * 8, 0, 0);
};


my $aimg = Prima::Image-> create();
my $bimg = Prima::Image-> create();
my $cimg = Prima::Image-> create();

if ( $useImages) {
$aimg-> load('winnt256.bmp');
$bimg-> load('c:/home/tobez/aaa.tif');
$cimg-> load('baba.bmp'); $cimg-> type( im::bpp8);
}

@spal = (@{$aimg-> palette}, @{$bimg-> palette}, @{$cimg-> palette}) if $useImages;

my $w = Prima::MainWindow-> create(
	size    => [ 100, 33 * 8],
	palette => [ @spal],
	buffered => 1,
	onPaint => sub {
		my $self = $_[0];
		my $i;
		$self-> on_paint( $_[1]);
		if ( $useImages) {
			$self-> put_image( 0, 0, $bimg);
			$self-> put_image( 100, 100, $aimg);
			$self-> put_image( 400, 300, $cimg);
		} else {
			for ( $i = 0; $i < 33; $i++) {
				my $x = $i * 8;
				$x = $x > 255 ? 255 : $x;
				$self-> color( $self-> get_nearest_color( $x));
				$self-> bar( 0, $i * 8, 3000, $i * 8 + 8);
			}
			my @pal = @{$self-> get_physical_palette};
			for ( $i = 0; $i < scalar @pal / 3; $i++) {
				my $col = ( ($pal[$i*3]) | ($pal[$i*3+1] << 8) | ($pal[$i*3+2] << 16));
				$self-> color( $col);
				$self-> bar( 0, $i * 8, 3000, $i * 8 + 8);
			}
		}
	},
);

run Prima;

