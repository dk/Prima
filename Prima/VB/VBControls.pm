#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
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
package Prima::VB::VBControls;

use Prima::Classes;
use Prima::Lists;

package Divider;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      vertical       => 0,
      growMode       => gm::GrowHiX,
      pointerType    => cr::SizeNS,
      min            => 0,
      max            => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   my $vertical = exists $p-> {vertical} ? $p-> {vertical} : $default->{ vertical};
   if ( $vertical)
   {
      $p-> { growMode} = gm::GrowLoX | gm::GrowHiY if !exists $p-> { growMode};
      $p-> { pointerType } = cr::SizeWE if !exists $p-> { pointerType};
   }
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   for ( qw( vertical min max))
      { $self->{$_} = 0; }
   for ( qw( vertical min max))
      { $self->$_( $profile{ $_}); }
   return %profile;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $canvas-> size;
   $self-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,1,$self->light3DColor,$self->dark3DColor,$self->backColor);
   my $v = $self->{vertical};
   return if $sz[ $v ? 0 : 1] < 4;
   $self->color($self->light3DColor);
   my $d = int($sz[ $v ? 0 : 1] / 2);
   $v ?
     $self->line($d, 2, $d, $sz[1]-3) :
     $self->line(2, $d, $sz[0]-3, $d);
   $self->color($self->dark3DColor);
   $d++;
   $v ?
     $self->line($d, 2, $d, $sz[1]-3) :
     $self->line(2, $d, $sz[0]-3, $d);
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $btn == mb::Left && !$self->{drag}) {
      $self-> capture( 1, $self-> owner);
      $self->{drag} = 1;
      $self->{pos}  = $self->{vertical} ? $x : $y;
   }
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   if ( $btn == mb::Left && $self->{drag}) {
      $self-> capture(0);
      $self->{drag} = 0;
   }
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   if ( $self->{drag}) {
      if ( $self->{vertical}) {
         my $np = $self-> left - $self->{pos} + $x;
         my $w = $self-> owner-> width;
         my $ww = $self-> width;
         $np = $self->{min} if $np < $self->{min};
         $np = $w - $self->{max} - $ww if $np > $w - $self->{max} - $ww;
         $self-> left( $np);
         $self-> notify( q(Change));
      } else {
         my $np = $self-> bottom - $self->{pos} + $y;
         my $w  = $self-> owner-> height;
         my $ww = $self-> height;
         $np = $self->{min} if $np < $self->{min};
         $np = $w - $self->{max} - $ww if $np > $w - $self->{max} - $ww;
         $self-> bottom( $np);
         $self-> notify( q(Change));
      }
   }
}

sub vertical
{
   if ( $#_) {
      return if $_[1] == $_[0]->{vertical};
      $_[0]->{vertical} = $_[1];
      $_[0]->repaint;
   } else {
      return $_[0]->{vertical};
   }
}

sub min
{
   if ( $#_) {
      my $mp = $_[1];
      return if $_[0]->{min} == $mp;
      $_[0]->{min} = $mp;
   } else {
      return $_[0]->{min};
   }
}

sub max
{
   if ( $#_) {
      my $mp = $_[1];
      return if $_[0]->{max} == $mp;
      $_[0]->{max} = $mp;
   } else {
      return $_[0]->{max};
   }
}

package PropListViewer;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);


sub reset_items
{
   my ( $self, $id, $check, $ix) = @_;
   $self-> {id}    = $id;
   $self-> {items} = $id;
   $self-> {check} = $check;
   $self-> {fHeight} = $self-> font-> height;
   $self-> {index}  = $ix;
   $self-> set_count( scalar @{$self-> {id}});
   $self-> calibrate;
}

sub on_stringify
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self->{id}->[$index];
   $self-> clear_event;
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self->get_text_width($self-> {id}->[$index]) + 2;
   $self-> clear_event;
}

sub on_drawitem
{
   my ( $me, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focused) = @_;
   my $ena = $me->{check}->[$index];
   unless ( defined $me->{hBenchColor}) {
      $me->color( $me-> hiliteBackColor);
      my $i1 = $me-> color;
      $me->color( $me-> backColor);
      my $i2 = $me-> color;
      my ( $r1, $g1, $b1, $r2, $g2, $b2) = (
         ( $i1 >> 16) & 0xFF, ( $i1 >> 8) & 0xFF, $i1 & 0xFF,
         ( $i2 >> 16) & 0xFF, ( $i2 >> 8) & 0xFF, $i2 & 0xFF,
      );
      $r1 = int(( $r1 + $r2) / 2);
      $g1 = int(( $g1 + $g2) / 2);
      $b1 = int(( $b1 + $b2) / 2);
      $me->{hBenchColor} = $b1 | ( $g1 << 8) | ( $r1 << 16);
      $me->{hBenchColor} = $i1 if $me->{hBenchColor} == $i2;
   }
   my ( $c, $bc);
   if ( $hilite) {
      $bc = $canvas-> backColor;
      $canvas-> backColor( $ena ? $me-> hiliteBackColor : $me-> {hBenchColor});
   }
   if ( $hilite || !$ena) {
      $c = $canvas-> color;
      $canvas-> color( $ena ? $me-> hiliteColor : cl::Gray);
   }

   $canvas-> clear( $left, $bottom, $right, $top);
   my $text = $me->{id}->[$index];
   my $x = $left + 2;
   $canvas-> text_out( $text, $x, ($top + $bottom - $me->{fHeight}) / 2);
   $canvas-> backColor( $bc) if $hilite;
   $canvas-> color( $c) if $hilite || !$ena
}


sub on_click
{
   my $self = $_[0];
   my $index = $self-> focusedItem;
   return if $index < 0;
   my $id = $self-> {'id'}->[$index];
   $self->{check}->[$index] = !$self->{check}->[$index];
   $self-> redraw_items( $index);
}


1;
