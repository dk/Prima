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
#  '11:4' => 8, '12:4' => 8, '13:4' => 12,
#  '10:5' => 12, '13:5' => 4, '13:6' => 4,
#  '11:7' => 8, '12:7' => 8, '13:7' => 8,
);

package Periodic;
use vars qw(@ISA);
@ISA = qw(Prima::GridViewer);

my %elem_info = (
	H	=> { atomic_number => 1, name => "Hydrogen", },
	He	=> { atomic_number => 2, name => "Helium", },

	Li	=> { atomic_number => 3, name => "Lithium", },
	Be	=> { atomic_number => 4, name => "Beryllium", },
	B	=> { atomic_number => 5, name => "Boron", },
	C	=> { atomic_number => 6, name => "Carbon", },
	N	=> { atomic_number => 7, name => "Nitrogen", },
	O	=> { atomic_number => 8, name => "Oxygen", },
	F	=> { atomic_number => 9, name => "Fluorine", },
	Ne	=> { atomic_number => 10, name => "Neon", },

	Na	=> { atomic_number => 11, name => "Sodium", },
	Mg	=> { atomic_number => 12, name => "Magnesium", },
	Al	=> { atomic_number => 13, name => "Aluminium", },
	Si	=> { atomic_number => 14, name => "Silicon", },
	P	=> { atomic_number => 15, name => "Phosphorus", },
	S	=> { atomic_number => 16, name => "Sulfur", },
	Cl	=> { atomic_number => 17, name => "Chlorine", },
	Ar	=> { atomic_number => 18, name => "Argon", },
	K	=> { atomic_number => 19, name => "Potassium", },
	Ca	=> { atomic_number => 20, name => "Calcium", },
	Sc	=> { atomic_number => 21, name => "Scandium", },
	Ti	=> { atomic_number => 22, name => "Titanium", },
	V	=> { atomic_number => 23, name => "Vanadium", },
	Cr	=> { atomic_number => 24, name => "Chromium", },
	Mn	=> { atomic_number => 25, name => "Manganese", },
	Fe	=> { atomic_number => 26, name => "Iron", },
	Co	=> { atomic_number => 27, name => "Cobalt", },
	Ni	=> { atomic_number => 28, name => "Nickel", },
	Cu	=> { atomic_number => 29, name => "Copper", },
	Zn	=> { atomic_number => 30, name => "Zinc", },
	Ga	=> { atomic_number => 31, name => "Gallium", },
	Ge	=> { atomic_number => 32, name => "Germanium", },
	As	=> { atomic_number => 33, name => "Arsenic", },
	Se	=> { atomic_number => 34, name => "Selenium", },
	Br	=> { atomic_number => 35, name => "Bromine", },
	Kr	=> { atomic_number => 36, name => "Krypton", },

	Rb	=> { atomic_number => 37, name => "Rubidium", },
	Sr	=> { atomic_number => 38, name => "Strontium", },
	Y	=> { atomic_number => 39, name => "Yttrium", },
	Zr	=> { atomic_number => 40, name => "Zirconium", },
	Nb	=> { atomic_number => 41, name => "Niobium", },
	Mo	=> { atomic_number => 42, name => "Molybdenum", },
	Tc	=> { atomic_number => 43, name => "Technetium", },
	Ru	=> { atomic_number => 44, name => "Ruthenium", },
	Rh	=> { atomic_number => 45, name => "Rhodium", },
	Pd	=> { atomic_number => 46, name => "Palladium", },
	Ag	=> { atomic_number => 47, name => "Silver", },
	Cd	=> { atomic_number => 48, name => "Cadmium", },
	In	=> { atomic_number => 49, name => "Indium", },
	Sn	=> { atomic_number => 50, name => "Tin", },
	Sb	=> { atomic_number => 51, name => "Antimony", },
	Te	=> { atomic_number => 52, name => "Tellurium", },
	I	=> { atomic_number => 53, name => "Iodine", },
	Xe	=> { atomic_number => 54, name => "Xenon", },

	Cs	=> { atomic_number => 55, name => "Caesium", },
	Ba	=> { atomic_number => 56, name => "Barium", },
	La	=> { atomic_number => 57, name => "Lanthanum", },

	Hf	=> { atomic_number => 72, name => "Hafnium", },
	Ta	=> { atomic_number => 73, name => "Tantalum", },
	W	=> { atomic_number => 74, name => "Tungsten", },
	Re	=> { atomic_number => 75, name => "Rhenium", },
	Os	=> { atomic_number => 76, name => "Osmium", },
	Ir	=> { atomic_number => 77, name => "Iridium", },
	Pt	=> { atomic_number => 78, name => "Platinum", },
	Au	=> { atomic_number => 79, name => "Gold", },
	Hg	=> { atomic_number => 80, name => "Mercury", },
	Tl	=> { atomic_number => 81, name => "Thallium", },
	Pb	=> { atomic_number => 82, name => "Lead", },
	Bi	=> { atomic_number => 83, name => "Bismuth", },
	Po	=> { atomic_number => 84, name => "Polonium", },
	At	=> { atomic_number => 85, name => "Astatine", },
	Rn	=> { atomic_number => 86, name => "Radon", },

	Fr	=> { atomic_number => 87, name => "Francium", },
	Ra	=> { atomic_number => 88, name => "Radium", },
	Ac	=> { atomic_number => 89, name => "Actinium", },

	Rf	=> { atomic_number => 104, name => "Rutherfordium", },
	Db	=> { atomic_number => 105, name => "Dubnium", },
	Sg	=> { atomic_number => 106, name => "Seaborgium", },
	Bh	=> { atomic_number => 107, name => "Bohrium", },
	Hs	=> { atomic_number => 108, name => "Hassium", },
	Mt	=> { atomic_number => 109, name => "Meitnerium", },
	Ds	=> { atomic_number => 110, name => "Darmstadtium", },

	Ce	=> { atomic_number => 58, name => "Cerium", },
	Pr	=> { atomic_number => 59, name => "Praseodymium", },
	Nd	=> { atomic_number => 60, name => "Neodymium", },
	Pm	=> { atomic_number => 61, name => "Promethium", },
	Sm	=> { atomic_number => 62, name => "Samarium", },
	Eu	=> { atomic_number => 63, name => "Europium", },
	Gd	=> { atomic_number => 64, name => "Gadolinium", },
	Tb	=> { atomic_number => 65, name => "Terbium", },
	Dy	=> { atomic_number => 66, name => "Dysprosium", },
	Ho	=> { atomic_number => 67, name => "Holmium", },
	Er	=> { atomic_number => 68, name => "Erbium", },
	Tm	=> { atomic_number => 69, name => "Thulium", },
	Yb	=> { atomic_number => 70, name => "Ytterbium", },
	Lu	=> { atomic_number => 71, name => "Lutetium", },

	Th	=> { atomic_number => 90, name => "Thorium", },
	Pa	=> { atomic_number => 91, name => "Protactinium", },
	U	=> { atomic_number => 92, name => "Uranium", },
	Np	=> { atomic_number => 93, name => "Neptunium", },
	Pu	=> { atomic_number => 94, name => "Plutonium", },
	Am	=> { atomic_number => 95, name => "Americium", },
	Cm	=> { atomic_number => 96, name => "Curium", },
	Bk	=> { atomic_number => 97, name => "Berkelium", },
	Cf	=> { atomic_number => 98, name => "Californium", },
	Es	=> { atomic_number => 99, name => "Einsteinium", },
	Fm	=> { atomic_number => 100, name => "Fermium", },
	Md	=> { atomic_number => 101, name => "Mendelevium", },
	No	=> { atomic_number => 102, name => "Nobelium",},
	Lr	=> { atomic_number => 103, name => "Lawrencium", },
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
