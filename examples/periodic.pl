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

=pod 
=item NAME

A periodic table of elements

=item FEATURES

Demonstrates usage of grid widget

=cut

use strict;
use Prima qw(Application Grids);


my $w = Prima::MainWindow-> create(
    text => "Periodic table of elements",
    size => [ 50 * 14 + 25, 50 * 13 + 25],
);

my @layers = ( 2, 8, 8, 10, 8, 10, 8, 10, 8, 10, 0, 14, 14);
my %colors = (
   2 => {
      0 => cl::Blue,
      10 => cl::Red,
   },
   8 => {
      0 => cl::Black,
      1 => cl::Black,
      10 => cl::Red,
      default => 0x804000
   },
   10 => {
      0 => cl::Blue,
      1 => cl::Blue,
      default => cl::Black,
   },
   14 => {
      default => cl::Green,
   }
);
my %sides = (
  '9:0' => 4, '9:1' => 4, '9:2' => 4,
  '7:4' => 8, '8:4' => 8, '9:4' => 12,
  '7:6' => 8, '8:6' => 8, '9:6' => 12,
  '7:8' => 8, '8:8' => 8, '9:8' => 12,
  '10:3' => 8, '10:5' => 8, '10:7' => 8, '10:9' => 8,
  '0:10' => 8, '1:10' => 8, '2:10' => 8, '3:10' => 8, '4:10' => 8,
  '5:10' => 8, '6:10' => 8, '7:10' => 8, '8:10' => 8, '9:10' => 8,
);

my $g = $w-> insert( 'Prima::GridViewer' => 
   origin => [0,0],
   size   => [$w-> size],
   growMode => gm::Client,
   cells  => [
      ['H',  '',   '',   '',   '',   '',   '',   '',   '',   '', 'He','','',''],
      ['Li', 'Be', 'B',  'C',  'N',  'O',  'F',  '',   '',   '', 'Ne','','',''],
      ['Na', 'Mg', 'Al', 'Si', 'P',  'S',  'Cl', '',   '',   '', 'Ar','','',''],
      ['K',  'Ca', 'Sc', 'Ti', 'V',  'Cr', 'Mn', 'Fe', 'Co', 'Ni', '','','',''],
      ['Cu',  'Zn', 'Ga', 'Ge', 'As',  'Se', 'Br', '', '', '', 'Kr','','',''],
      ['Rb',  'Sr', 'Y', 'Zr', 'Nb',  'Mo', 'Tc', 'Ru', 'Rh', 'Pd', '','','',''],
      ['Ag',  'Cd', 'In', 'Sn', 'Sb',  'Te', 'J', '', '', '', 'Xe','','',''],
      ['Cs',  'Ba', 'La', 'Hf', 'Ta',  'W', 'Re', 'Os', 'Ir', 'Pt', '','','',''],
      ['Au',  'Hg', 'Tl', 'Pb', 'Bi',  'Po', 'At', '', '', '', 'Rn','','',''],
      ['Fr',  'Ra', 'Ac', 'Rf',  'Db', 'Sg', 'Bh', 'Hs', 'Mt', 'Ds','','','',''],
      ['',  '', '', '', '',  '', '', '', '', '', '','','',''],
      ['Ce',  'Pr', 'Nd', 'Pm', 'Sm', 'Eu',  'Gd', 'Tb', 'Dy', 'Ho', 'Er', 'Tm', 'Yb','Lu'],
      ['Th',  'Pa', 'U', 'Np', 'Pu', 'Am', 'Cm', 'Bk', 'Cf', 'Es', 'Fm', 'Md', 'No','Lr'],
   ],
   drawHGrid => 0,
   drawVGrid => 0,
   constantCellWidth => 50,
   constantCellHeight => 50,
   multiSelect => 0,
   onDrawCell => sub {
      my ( $self, $canvas, 
           $column, $row, $indent, 
           $sx1, $sy1, $sx2, $sy2, 
           $cx1, $cy1, $cx2, $cy2, 
           $selected, $focused) = @_;
      $canvas-> clear($sx1, $sy1, $sx2, $sy2);
      my $item = $self-> {cells}-> [$row]-> [$column];
      my $color = $colors{$layers[$row]};
      if ( length $item) {
         return unless $color;
         return unless defined 
            ($color = ( exists $color->{$column}) ? $color->{$column} : $color->{default});
         $canvas-> color( cl::Black);
         $canvas-> rectangle( $cx1-1, $cy1-1, $cx2, $cy2);
         if ( $focused) {
            $canvas-> color( $self-> hiliteBackColor);
            $canvas-> bar( $cx1, $cy1, $cx2-1, $cy2-1);
            $canvas-> color( $self-> hiliteColor);
         } else {
            $canvas-> color( $color);
         }
         $canvas-> text_out( $item, $cx1 + 10, $cy1 + 10);
         $canvas-> rect_focus( $sx1, $sy1, $sx2-1, $sy2-1) if $focused;
      } elsif ( exists $sides{"$column:$row"}) {
         my $side = $sides{"$column:$row"};
         $canvas-> color( cl::Black);
         $canvas-> line( $cx1-1,$cy1-1,$cx2,$cy1-1 ) if $side & 1;
         $canvas-> line( $cx1-1,$cy1-1,$cx1-1,$cy2 ) if $side & 2;
         $canvas-> line( $cx2,$cy1-1,$cx2,$cy2 ) if $side & 4;
         $canvas-> line( $cx1-1,$cy2,$cx2,$cy2 ) if $side & 8;
      }
   },
);

run Prima;
