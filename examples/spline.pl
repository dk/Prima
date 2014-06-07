#
#  Copyright (c) 1997-2003 The Protein Laboratory, University of Copenhagen
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

examples/spline.pl - A spline demo

=head1 FEATURES

Demonstrates parabolic splines as graphic primitives.
The points arount the filled shapes are spline vertexes,
which are dragable by the mouse.

Using L<Prima::Drawable::CurvedText>.

=cut

use strict;
use warnings;
use Prima qw(Application Drawable::CurvedText);

my @bounds;
my @points = (
	10, 10, 
	130, 30, 
	20, 20, 
	20, 300, 
	50, 300, 
);
my @vectors = (
	2, 2,
	2, 1,
	-1, -1,
	-1, 3,
	-2, 2,
);
my @rgb = (
	0,
	0, 
	0,
);
my @rgb_vectors = (
	-1.04,
	1.79, 
	1.38,
);
my $capture;
my $aperture = 6;

my $w = Prima::MainWindow-> create(
	text => 'Spline demo',
	backColor => cl::Black,
	buffered => 1,
	onPaint => sub {
		my ( $self, $canvas) = @_;
		$canvas-> clear;

		my $spline = $canvas-> render_spline( [ @points, @points[0,1]]);
		$canvas-> fillpoly( $spline);
		if ( defined $capture) {
			$canvas-> fill_ellipse( 
				$points[$capture], $points[$capture+1], 
				$aperture, $aperture
			);
		}

		$canvas-> rop( rop::XorPut);
		$canvas-> curved_text_out("Hello, world!", $spline);

		my $i;
		for ( $i = 0; $i < @points; $i+=2) {
			$canvas-> ellipse( 
				$points[$i], $points[$i+1], 
				$aperture, $aperture
			);
		}
	},
	onSize => sub {
		my ( $self, $ox, $oy, $x, $y) = @_;
		@bounds = ( $x, $y);
	},
	onMouseDown => sub {
		my ( $self, $mod, $btn, $x, $y) = @_;
		my $i;
		$capture = undef;
		for ( $i = 0; $i < @points; $i+=2) {
			if ( $points[$i] > $x - $aperture && $points[$i] < $x + $aperture && 
				$points[$i+1] > $y - $aperture && $points[$i+1] < $y + $aperture) {
				$capture = $i;
				last;
			}
		}
	},
	onMouseUp => sub {
		undef $capture;
	},
	onMouseMove => sub {
		my ( $self, $btn, $x, $y) = @_;
		return unless defined $capture;
		$points[$capture] = $x if $x >= 0 && $x < $bounds[0];
		$points[$capture+1] = $y if $y >= 0 && $y < $bounds[1];
	},
);

$w-> insert( Timer =>
	timeout => 50,
	onTick => sub {
		my $i;
		my $idx = 0;
		for ( $i = 0; $i < @points; $i++) {
			next if defined $capture && ( $capture == $i || $capture == $i - 1);
			$points[$i] += $vectors[$i];
			if ( $points[$i] < 0) {
				$points[$i] *= -1;
				$vectors[$i] *= -1;
			} elsif ( $points[$i] >= $bounds[$idx]) {
				$points[$i] = $bounds[$idx] * 2 - $points[$i];
				$vectors[$i] *= -1;
			}
			$idx = ($idx ? 0 : 1);
		}

		for ( $i = 0; $i < 3; $i++) {
			$rgb[$i] += $rgb_vectors[$i];
			if ( $rgb[$i] < 0) {
				$rgb_vectors[$i] *= -1;
				$rgb[$i] *= -1;
			} elsif ( $rgb[$i] > 255) {
				$rgb[$i] = 511 - $rgb[$i];
				$rgb_vectors[$i] *= -1;
			}
		}
		$w-> color( int($rgb[2]) + int($rgb[1]) * 256 + int($rgb[0]) * 65536);
	},
)-> start;

run Prima;
