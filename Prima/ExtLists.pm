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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
#  $Id$

# contains:
#   CheckList

use strict;
use Prima::Const;
use Prima::Classes;
use Prima::Lists;
use Prima::StdBitmap;

package Prima::CheckList;
use vars qw(@ISA);
@ISA = qw(Prima::ListBox);

my @images = (
   Prima::StdBitmap::image(sbmp::CheckBoxUnchecked),
   Prima::StdBitmap::image(sbmp::CheckBoxChecked),
);

my @imgSize = (0,0);
@imgSize = $images[0]-> size;

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      vector => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub init
{
   my $self = shift;
   $self->{vector} = 0;
   my %profile = $self-> SUPER::init(@_);
   $self->{fontHeight} = $self-> font-> height;
   $self-> recalc_icons;
   $self-> vector( $profile{vector});
   return %profile;
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self-> get_text_width( $self->{items}->[$index]) + $imgSize[0] + 4;
}

sub recalc_icons
{
   return unless $imgSize[0];
   my $self = $_[0];
   my $hei = $self-> font-> height + 2;
   $hei = $imgSize[1] + 2 if $hei < $imgSize[1] + 2;
   $self-> itemHeight( $hei);
}

sub on_fontchanged
{
   my $self = shift;
   $self-> recalc_icons;
   $self->{fontHeight} = $self-> font-> height;
   $self-> SUPER::on_fontchanged(@_);
}


sub draw_items
{
   shift-> std_draw_text_items( @_);
}

sub draw_text_items
{
   my ( $self, $canvas, $first, $last, $x, $y, $textShift, $clipRect) = @_;
   my $i;
   for ( $i = $first; $i <= $last; $i++)
   {
       next if $self-> {widths}->[$i] + $self->{offset} + $x + 1 < $clipRect-> [0];
       $canvas-> text_out( $self->{items}->[$i], $x + 2 + $imgSize[0],
          $y + $textShift - ($i-$first+1) * $self->{itemHeight} + 1);
       $canvas-> put_image( $x + 1, 
          $y + int(( $self->{itemHeight} - $imgSize[1]) / 2) - 
             ($i-$first+1) * $self->{itemHeight} + 1,
          $images[ vec($self->{vector}, $i, 1)],
       );
   }
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   $self-> SUPER::on_mouseclick( $btn, $mod, $x, $y);
   return if $btn != mb::Left;
   my $foc = $self-> focusedItem;
   return if $foc < 0;
   my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
   vec( $self-> {vector}, $foc, 1) = $f;
   $self-> notify(q(Change), $foc, $f);
   $self-> invalidate_rect( $self-> item2rect( $foc));
}

sub on_click
{
   my $self = $_[0];
   my $foc = $self-> focusedItem;
   return if $foc < 0;
   my $f = vec( $self-> {vector}, $foc, 1) ? 0 : 1;
   vec( $self-> {vector}, $foc, 1) = $f;
   $self-> notify(q(Change), $foc, $f);
   $self-> invalidate_rect( $self-> item2rect( $foc));
}


sub vector
{
   my $self = $_[0];
   return $self->{vector} unless $#_;
   $self-> {vector} = $_[1];
   $self-> repaint;
}

sub on_change
{
#  my ( $self, $index, $state) = @_;
}

1;

