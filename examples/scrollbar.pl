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

examples/scrollbar.pl - A Prima toolkit example

=head1 FEATURES

Tests correct representation of a color created by a combination
of R,G and B components. The Prima::Widget::sizeMin and Prima::Widget::sizeMax
implementation is tested - the Area widget changes its height on a mouse click,
up to the left button and down to the right button. 
Note how Area widget maintains its maximum size when the window gets maximized.

=cut

use strict;
use warnings;
use Prima qw(ScrollBar);
use Prima::Application name => 'scrollbar';

package MyWindow;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);

sub updateArea
{
	$_[0]-> Area-> backColor(
		$_[0]-> Blue-> value|($_[0]-> Green-> value<<8)|($_[0]-> Red-> value<<16)
	);
}

sub Timer1_Tick
{
	$_[ 0]-> Red  -> backColor(( $_[ 0]-> Red  -> backColor == cl::Red  ) ? cl::LightRed   : cl::Red  );
	$_[ 0]-> Green-> backColor(( $_[ 0]-> Green-> backColor == cl::Green) ? cl::LightGreen : cl::Green);
	$_[ 0]-> Blue -> backColor(( $_[ 0]-> Blue -> backColor == cl::Blue ) ? cl::LightBlue  : cl::Blue );
}

package UserInit;

my $w = MyWindow-> create(
	text => "Scrollbar & timer example",
	left    => 100,
	bottom  => 100,
	width   => 300,
	height  => 300,
	borderStyle => bs::Sizeable,
);

$w-> insert(
	"ScrollBar",
	 name     => "Red",
	 origin   => [ 2, 20],
	 vertical => 0,
	 max      => 255,
	 width    => $w-> width - 2,
	 onChange => sub { $w-> updateArea },
);
$w-> insert(
	"ScrollBar",
	 name     => "Green",
	 origin   => [ 2, 40],
	 vertical => 0,
	 max      => 255,
	 width    => $w-> width - 2,
	 onChange => sub { $w-> updateArea },
);
$w-> insert(
	"ScrollBar",
	 name     => "Blue",
	 origin   => [ 2, 60],
	 vertical => 0,
	 max      => 255,
	 width    => $w-> width - 2,
	 onChange => sub { $w-> updateArea },
);

$w-> insert(
	"ScrollBar",
	 name     => "Bluex",
	 origin   => [ 2, 80],
	 vertical => 1,
	 max      => 255,
	 height    => $w-> height - 80,
);


$w-> insert(
	"Widget",
	name      => "Area",
	rect      => [ 20, 100, 280, 280],
	backColor => cl::Black,
	growMode  => gm::GrowHiX | gm::GrowHiY,
	sizeMin   => [120, 120],
	sizeMax   => [420, 420],
	onPaint   => sub {
		my ($x,$y)=$_[0]-> size;
		$_[0]-> color($_[0]-> backColor);
		$_[0]-> bar(0,0,$x,$y);
		$_[0]-> color(cl::Set);
		$_[0]-> rop(rop::XorPut);
		$_[0]-> line(0,0,$x,$y);
		$_[0]-> line(0,$y,$x,0);
	},
	onMouseDown =>sub{
		$_[0]-> height($_[0]-> height+(($_[1]==1)?1:-1));
	},
);
my $t = $w-> insert( Timer=>
	timeout=> 2000,
	name => 'Timer1',
	delegations => ['Tick'],
);
$t-> start;

run Prima;
