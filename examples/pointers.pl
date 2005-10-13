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
=item NAME

Prima mouse pointer example

=item FEATURES

Demonstrates the usage of Prima mouse pointer functionality.
Note the custom pointer creation and its dynamic change ( the "User" button ).

=cut

use strict;
use Prima qw( StdBitmap Buttons Application);

package UserInit;

my $ph = Prima::Application-> get_system_value(sv::YPointer);
my $w = Prima::MainWindow-> create(
	size    => [ 350, 20 + ($ph+8)*11],
	left    => 200,
	text    => 'Pointers',
);

my %grep_out = (
	'BEGIN' => 1,
	'END' => 1,
	'AUTOLOAD' => 1,
	'constant' => 1
);

my @a = sort { &{$cr::{$a}}() <=> &{$cr::{$b}}()} grep { !exists $grep_out{$_}} keys %cr::;
shift @a;
$::application-> pointerVisible(0);
my $i = 1;

for my $c ( @a[0..$#a-1])
{
	my $p = eval("cr::$c"); 
	$w-> pointer( $p);
	my $b = $w-> insert( Button =>
		flat => 1,
		left    => 10 + (($i-1) % 2)*170,
		width   => 160,
		height  => $ph + 4,
		bottom  => $w-> height - int(($i+1)/2) * ($ph+8) - 10,
		pointer => $p,
		name    => $c,
		image   => $w-> pointerIcon,
		onClick => sub { $::application-> pointer( $_[0]-> pointer); },
	);
	$i++;
};

my $ptr = Prima::StdBitmap::icon( sbmp::DriveCDROM);

my @mapset = map {
	my ($x,$a) = $ptr-> split;
	my $j = Prima::Icon-> create;
	$x-> begin_paint;
	$x-> text_out( $_, 3, 3); 
	$x-> end_paint;
	$a-> begin_paint;
	$a-> text_out( $_, 3, 3); 
	$a-> end_paint;
	$j-> combine( $x, $a);
	$j;
} 1..4;   
my $mapsetID = 0;

my $b = $w-> insert( SpeedButton =>
	left    => 10 + (($i-1) % 2)*170,
	width   => 160,
	height  => $ph+4,
	bottom  => $w-> height - int(($i+1)/2) * ($ph+8) - 10,
	pointer => $ptr,
	image   => $ptr,
	text => $a[-1],
	flat => 1,
	onClick => sub {
		$::application-> pointer( $_[0]-> pointer);
	},
);

$b-> insert( Timer => 
	timeout => 1250, 
	onTick  => sub {
		$b-> pointerIcon( $mapset[$mapsetID]);
		$b-> image( $mapset[$mapsetID]);
		$mapsetID++;
		$mapsetID = 0 if $mapsetID >= @mapset;
	},   
)-> start;

$w-> pointer( cr::Default);
$::application-> pointerVisible(1);

run Prima;
