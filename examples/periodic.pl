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

my @layers = ( 2, 8, 8, 10, 9, 10, 9, 10, 9, 10, 0, 14, 14);
my %colors = (
   2 => {
      0 => cl::Blue,
      10 => cl::Red,
   },
   8 => {
      0 => cl::Blue,
      1 => cl::Blue,
      10 => cl::Red,
      default => 0x804000
   },
   9 => {
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

package Periodic;
use vars qw(@ISA);
@ISA = qw(Prima::GridViewer);

my %elem_info = (
	H	=> { atomic_number => 1, },
	He	=> { atomic_number => 2, },

	Li	=> { atomic_number => 3, },
	Be	=> { atomic_number => 4, },
	B	=> { atomic_number => 5, },
	C	=> { atomic_number => 6, },
	N	=> { atomic_number => 7, },
	O	=> { atomic_number => 8, },
	F	=> { atomic_number => 9, },
	Ne	=> { atomic_number => 10, },

	Na	=> { atomic_number => 11, },
	Mg	=> { atomic_number => 12, },
	Al	=> { atomic_number => 13, },
	Si	=> { atomic_number => 14, },
	P	=> { atomic_number => 15, },
	S	=> { atomic_number => 16, },
	Cl	=> { atomic_number => 17, },
	Ar	=> { atomic_number => 18, },
	K	=> { atomic_number => 19, },
	Ca	=> { atomic_number => 20, },
	Sc	=> { atomic_number => 21, },
	Ti	=> { atomic_number => 22, },
	V	=> { atomic_number => 23, },
	Cr	=> { atomic_number => 24, },
	Mn	=> { atomic_number => 25, },
	Fe	=> { atomic_number => 26, },
	Co	=> { atomic_number => 27, },
	Ni	=> { atomic_number => 28, },
	Cu	=> { atomic_number => 29, },
	Zn	=> { atomic_number => 30, },
	Ga	=> { atomic_number => 31, },
	Ge	=> { atomic_number => 32, },
	As	=> { atomic_number => 33, },
	Se	=> { atomic_number => 34, },
	Br	=> { atomic_number => 35, },
	Kr	=> { atomic_number => 36, },

	Rb	=> { atomic_number => 37, },
	Sr	=> { atomic_number => 38, },
	Y	=> { atomic_number => 39, },
	Zr	=> { atomic_number => 40, },
	Nb	=> { atomic_number => 41, },
	Mo	=> { atomic_number => 42, },
	Tc	=> { atomic_number => 43, },
	Ru	=> { atomic_number => 44, },
	Rh	=> { atomic_number => 45, },
	Pd	=> { atomic_number => 46, },
	Ag	=> { atomic_number => 47, },
	Cd	=> { atomic_number => 48, },
	In	=> { atomic_number => 49, },
	Sn	=> { atomic_number => 50, },
	Sb	=> { atomic_number => 51, },
	Te	=> { atomic_number => 52, },
	I	=> { atomic_number => 53, },
	Xe	=> { atomic_number => 54, },

	Cs	=> { atomic_number => 55, },
	Ba	=> { atomic_number => 56, },
	La	=> { atomic_number => 57, },

	Hf	=> { atomic_number => 72, },
	Ta	=> { atomic_number => 73, },
	W	=> { atomic_number => 74, },
	Re	=> { atomic_number => 75, },
	Os	=> { atomic_number => 76, },
	Ir	=> { atomic_number => 77, },
	Pt	=> { atomic_number => 78, },
	Au	=> { atomic_number => 79, },
	Hg	=> { atomic_number => 80, },
	Tl	=> { atomic_number => 81, },
	Pb	=> { atomic_number => 82, },
	Bi	=> { atomic_number => 83, },
	Po	=> { atomic_number => 84, },
	At	=> { atomic_number => 85, },
	Rn	=> { atomic_number => 86, },

	Fr	=> { atomic_number => 87, },
	Ra	=> { atomic_number => 88, },
	Ac	=> { atomic_number => 89, },

	Rf	=> { atomic_number => 104, },
	Db	=> { atomic_number => 105, },
	Sg	=> { atomic_number => 106, },
	Bh	=> { atomic_number => 107, },
	Hs	=> { atomic_number => 108, },
	Mt	=> { atomic_number => 109, },
	Ds	=> { atomic_number => 110, },

	Ce	=> { atomic_number => 58, },
	Pr	=> { atomic_number => 59, },
	Nd	=> { atomic_number => 60, },
	Pm	=> { atomic_number => 61, },
	Sm	=> { atomic_number => 62, },
	Eu	=> { atomic_number => 63, },
	Gd	=> { atomic_number => 64, },
	Tb	=> { atomic_number => 65, },
	Dy	=> { atomic_number => 66, },
	Ho	=> { atomic_number => 67, },
	Er	=> { atomic_number => 68, },
	Tm	=> { atomic_number => 69, },
	Yb	=> { atomic_number => 70, },
	Lu	=> { atomic_number => 71, },

	Th	=> { atomic_number => 90, },
	Pa	=> { atomic_number => 91, },
	U	=> { atomic_number => 92, },
	Np	=> { atomic_number => 93, },
	Pu	=> { atomic_number => 94, },
	Am	=> { atomic_number => 95, },
	Cm	=> { atomic_number => 96, },
	Bk	=> { atomic_number => 97, },
	Cf	=> { atomic_number => 98, },
	Es	=> { atomic_number => 99, },
	Fm	=> { atomic_number => 100, },
	Md	=> { atomic_number => 101, },
	No	=> { atomic_number => 102, },
	Lr	=> { atomic_number => 103, },
);

sub focusedCell
{
   return $_[0]-> SUPER::focusedCell unless $#_;
   my ($self,$x,$y) = @_;
   ($x, $y) = @$x if !defined $y && ref($x) eq 'ARRAY';
   return unless $y >= 0 && $x >= 0 && $self-> {cells}-> [$y] && 
      $self-> {cells}-> [$y]-> [$x] && length $self-> {cells}-> [$y]-> [$x];
   $self-> SUPER::focusedCell( $x, $y);
}

my $g = $w-> insert( Periodic => 
   origin => [0,0],
   size   => [$w-> size],
   growMode => gm::Client,
   cells  => [
      ['H',  ('')x9,                    'He',   ('')x3],
      [qw(Li Be B  C  N  O  F),  ('')x3,'Ne',   ('')x3],
      [qw(Na Mg Al Si P  S  Cl), ('')x3,'Ar',   ('')x3],
      [qw(K  Ca Sc Ti V  Cr Mn Fe Co Ni),       ('')x4],
      [qw(Cu Zn Ga Ge As Se Br), ('')x3,'Kr',   ('')x3],
      [qw(Rb Sr Y  Zr Nb Mo Tc Ru Rh Pd),       ('')x4],
      [qw(Ag Cd In Sn Sb Te I),  ('')x3,'Xe',   ('')x3],
      [qw(Cs Ba La Hf Ta W  Re Os Ir Pt),       ('')x4],
      [qw(Au Hg Tl Pb Bi Po At), ('')x3,'Rn',   ('')x3],
      [qw(Fr Ra Ac Rf Db Sg Bh Hs Mt Ds),       ('')x4],
      [('') x 14],
      [qw(Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu)],
      [qw(Th Pa U  Np Pu Am Cm Bk Cf Es Fm Md No Lr)],
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
		 my $f = $canvas->font;
		 $canvas->font(size => $f->size - 4);
         $canvas->text_out( $elem_info{$item}->{atomic_number}||"", $cx1 + 35, $cy1 + 35);
		 $canvas->font($f);
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
   onClick => sub {
	   my ($self) = @_;
	   my @foc = $self->focusedCell;
	   my $text = $self->{cells}->[$foc[1]]->[$foc[0]];
	   return unless $text;
	   if ($text eq "La") {
		   $self->focusedCell(0, 11);
	   } elsif ($text eq "Ac") {
		   $self->focusedCell(0, 12);
	   }
   },
);

run Prima;
