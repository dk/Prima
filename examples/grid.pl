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
#  $Id$
#
#
#  Grid example
#
=pod 

=head1 NAME

examples/grid.pl - Prima grid widget example

=head1 FEATURES

Demonstrates the usage of grid widgets - Prima::AbstractGrid and
its text-oriented descendant, Prima::Grid. To switch between
these, toggle $abstract_grid flag.

=cut


use strict;
use Prima;
use Prima::Application;
use Prima::Grids;


my $g;
my $w = Prima::MainWindow-> create(
	text => 'Grid example',
	packPropagate => 0,
	menuItems => [
		['~Options' => [
			['*dhg', 'Draw HGrid'=> sub { $g-> drawHGrid( $_[0]-> menu-> toggle( $_[1])) }],
			['*dvg', 'Draw VGrid'=> sub { $g-> drawVGrid( $_[0]-> menu-> toggle( $_[1])) }],
			['mse', 'Multi select'=> sub { $g-> multiSelect( $_[0]-> menu-> toggle( $_[1])) }],
			['*ind', 'Use indents' => sub { $g-> cellIndents(($_[0]-> menu-> toggle( $_[1])) x 4); }],
			['ccw', 'Constant cell width' => sub { $g-> constantCellWidth($_[0]-> menu-> toggle( $_[1]) ? 100 : undef); }],
			['cch', 'Constant cell height' => sub { $g-> constantCellHeight($_[0]-> menu-> toggle( $_[1]) ? 100 : undef); }],
		]
	]],
);  

# change this to 0 and Prima::Grid will be created instead
my $abstract_grid = 0;

my @user_breadths=({},{});

$g = $w-> insert( 
( $abstract_grid ? 'Prima::AbstractGrid' : 'Prima::Grid' ),
( $abstract_grid ? () : ('cells', [ map { my $a = $_; [ map {"$a.$_"} 0 .. 300]} 0 .. 300])),
	onMeasure => sub {
		my ( $self, $axis, $index, $ref) = @_;
		if ( defined $user_breadths[$axis]-> {$index} ) {
			$$ref = $user_breadths[$axis]-> {$index};
		} else {
			$$ref = ( 11 - ( $index % 11)) * 15;
			$$ref = 15 if $$ref < 15;
		}
	},
	onSetExtent => sub {
		my ( $self, $axis, $index, $breadth) = @_;
		$user_breadths[$axis]-> {$index} = $breadth;
	},
	onDrawCell => sub {
		my ( $self, $canvas, 
				$col, $row, $type,
				$x1, $y1, $x2, $y2,
				$X1, $Y1, $X2, $Y2,
				$sel, $foc) = @_;
		
		$canvas-> backColor( $sel ? $self-> hiliteBackColor :
						( $type ? $self-> indentCellBackColor : cl::Back));
		$canvas-> clear( $x1, $y1, $x2, $y2);
		$canvas-> color( $sel ? $self-> hiliteColor :
						( $type ? $self-> indentCellColor : cl::Fore));
		$canvas-> text_out( "$col.$row", $X2-50, $Y1+3);
		$canvas-> rect_focus( $x1, $y1, $x2, $y2 ) if $foc;
	},
	onGetRanges => sub {
		my ( $self, $axis, $index, $min, $max) = @_;
		$$min = 50;
	},
	onStringify => sub {
		my ( $self, $col, $row, $ref) = @_;
		$$ref = "$col.$row";
	},
	allowChangeCellWidth => 1,
	allowChangeCellHeight => 1,
	clipCells => 2,
	pack => { expand => 1, fill => 'both' },
);
if ( $abstract_grid) {
	$g-> columns(10000);
	$g-> rows(10000);
}
$g-> cellIndents(1,1,1,1);

#$g-> insert_row( 0, 0..300 ); 
#$g-> insert_column( 100, 0..300 ); 
#$g-> delete_columns( 100, 1);
#$g-> add_column( 100, [0..300] ); 

run Prima;
