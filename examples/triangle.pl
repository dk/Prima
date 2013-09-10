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

Escher's Impossible Triangle

=head1 FEATURES

Demonstrates the basic usage of the Prima toolkit.

=cut

use strict;
use warnings;
use Prima qw(Buttons);
use constant PI => 4.0 * atan2 1, 1;
use constant D2R => PI / 180;
use constant Cos_120 => cos(D2R*(-120));
use constant Sin_120 => sin(D2R*(-120));

use Prima::Application name => "Escher's Impossible Triangle";

sub recalc_coordinates
{
	my $me = $_[0];
	$me-> {angle} = 0 unless exists $me-> {angle};
	delete $me-> {line1};
	delete $me-> {line2};
	delete $me-> {line3};
	my ($w, $h) = $me-> size;
	my $sz = ($w > $h ? $h : $w) * 0.8 * 0.5;
	my ($x, $y) = (25*$sz/34,$sz);
	my ($fx, $fy) = ($x,$y);
	($w,$h) = ($w/2,$h/2);
	my $angle = $me-> {angle};
	for (1..3)
	{
		my $cos = cos(D2R*$angle);
		my $sin = sin(D2R*$angle);
		my @lines = ();
		($x,$y) = ($fx, $fy);
		push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
		$y = -$y;
		push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
		# rotate -120
		$x = Cos_120*$fx - Sin_120*$fy;
		$y = Sin_120*$fx + Cos_120*$fy;
		push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
		$y = -$y/2;
		push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
		($x, $y) = ($x-sqrt(3)*$y,0);
		push @lines, $cos*$x-$sin*$y+$w, $sin*$x+$cos*$y+$h;
		$me-> {'line' . $_} = [@lines];
		$angle += 120;
	}
}


my $w = Prima::MainWindow-> create(
#my $w = Window-> create(
	text   => $::application-> name,
	left   => 200,
	bottom => 300,
	width  => 400,
	height => 250,
	borderIcons => 0,
	windowState => ws::Maximized,
	borderStyle => bs::None,
	backColor => cl::Black,
	hiliteColor => cl::LightCyan,
	lineWidth => 2,
#   onSize    => \&recalc_coordinates,
	onCreate  => sub {
		$_[0]-> {r} = $_[0]-> {g} = $_[0]-> {b} = $_[0]-> {c} = 0;
		my $t = $_[0]-> insert( Timer => timeout => 10, name => 'Timer', onTick => sub {
			my $me = $_[0]-> owner;
			my $back = $me-> backColor;
#       		my $fore = $me-> hiliteColor;
			$me-> {c} = 255 if $me-> {c} >= 1785;
			$me-> {c}++;
			if  ( $me-> {c}   <=  255) { $me-> {r}++; $me-> {g}++; $me-> {b}++; }
			elsif ( $me-> {c} <=  510) { $me-> {g}--; $me-> {b}--; }
			elsif ( $me-> {c} <=  765) { $me-> {g}++; }
			elsif ( $me-> {c} <= 1020) { $me-> {r}--; }
			elsif ( $me-> {c} <= 1275) { $me-> {b}++; }
			elsif ( $me-> {c} <= 1530) { $me-> {g}--; }
			else  { $me-> {g}++; $me-> {r}++;}
			$me-> {c} = 255 if $me-> {c} >= 1785;
			$me-> {c}++;
			if  ( $me-> {c}   <=  255) { $me-> {r}++; $me-> {g}++; $me-> {b}++; }
			elsif ( $me-> {c} <=  510) { $me-> {g}--; $me-> {b}--; }
			elsif ( $me-> {c} <=  765) { $me-> {g}++; }
			elsif ( $me-> {c} <= 1020) { $me-> {r}--; }
			elsif ( $me-> {c} <= 1275) { $me-> {b}++; }
			elsif ( $me-> {c} <= 1530) { $me-> {g}--; }
			else  { $me-> {g}++; $me-> {r}++;}
			$me-> {c} = 255 if $me-> {c} >= 1785;
			$me-> {c}++;
			if  ( $me-> {c}   <=  255) { $me-> {r}++; $me-> {g}++; $me-> {b}++; }
			elsif ( $me-> {c} <=  510) { $me-> {g}--; $me-> {b}--; }
			elsif ( $me-> {c} <=  765) { $me-> {g}++; }
			elsif ( $me-> {c} <= 1020) { $me-> {r}--; }
			elsif ( $me-> {c} <= 1275) { $me-> {b}++; }
			elsif ( $me-> {c} <= 1530) { $me-> {g}--; }
			else  { $me-> {g}++; $me-> {r}++;}
			my $fore = $me-> {b} | ($me-> {g}<<8) | ($me-> {r}<<16);
			my $l1 = $me-> {line1};
			my $l2 = $me-> {line2};
			my $l3 = $me-> {line3};
#			($l1,$l2,$l3) = ();
			$me-> {angle} += 1;
			$me-> {angle} -= 360 if $me-> {angle} > 360;
			&recalc_coordinates($me);
			$me-> begin_paint;
			#$me-> color( $back);
			#$me-> polyline( $l1) if defined $l1;
			$me-> color( $fore);
			$me-> bar(0,0,30,30);
			$me-> polyline( $me-> {line1}) if exists $me-> {line1};
			$me-> color( $back);
			$me-> polyline( $l2) if defined $l2;
			$me-> color( $fore);
			$me-> polyline( $me-> {line2}) if exists $me-> {line2};
			#$me-> fillpoly( $me-> {line2}) if exists $me-> {line2};
			#$me-> color( $back);
			#$me-> polyline( $l3) if defined $l3;
			$me-> color( $fore);
			$me-> polyline( $me-> {line1}) if exists $me-> {line1};
			$me-> polyline( $me-> {line2}) if exists $me-> {line2};
			$me-> polyline( $me-> {line3}) if exists $me-> {line3};
			$me-> end_paint;
		});
		$t-> start;
		&recalc_coordinates($_[0]);
	},
	onPaint   => sub
	{
		my ($me,$canvas) = @_;
		$canvas-> color( $me-> backColor);
		$canvas-> bar( 0, 0, $canvas-> size);
	},
);

$w-> insert( Button =>
	text => "Install PRIMA now!",
	width => $w-> get_text_width( "Install PRIMA now!") * 1.2,
	# centered => 1,
	growMode => gm::Center,
	onClick => sub { $::application-> close},
);

run Prima;
