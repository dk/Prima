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
#  Created by Dmitry Karasik <dk@plab.ku.dk>

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
   my ($self,$canvas) = (shift,shift);
   my @clrs = (
      $self-> color,
      $self-> backColor,
      $self-> colorIndex( ci::HiliteText),
      $self-> colorIndex( ci::Hilite)
   );
   my @clipRect = $canvas-> clipRect;
   my $i;
   my $drawVeilFoc = -1;
   my $atY    = ( $self-> {itemHeight} - $canvas-> font-> height) / 2;
   my $ih     = $self->{itemHeight};
   my $offset = $self->{offset};
   my $v      = $self->{vector};

   my @colContainer;
   for ( $i = 0; $i < $self->{columns}; $i++){ push ( @colContainer, [])};
   for ( $i = 0; $i < scalar @_; $i++) {
      push ( @{$colContainer[ $_[$i]->[7]]}, $_[$i]);
      $drawVeilFoc = $i if $_[$i]->[6];
   }
   my ( $lc, $lbc) = @clrs[0,1];
   for ( @colContainer)
   {
      my @normals;
      my @selected;
      my ( $lastNormal, $lastSelected) = (undef, undef);
      my $isSelected = 0;
      # sorting items in single column
      { $_ = [ sort { $$a[0]<=>$$b[0] } @$_]; }
      # calculating conjoint bars
      for ( $i = 0; $i < scalar @$_; $i++)
      {
         my ( $itemIndex, $x, $y, $x2, $y2, $selected, $focusedItem) = @{$$_[$i]};
         if ( $selected)
         {
            if ( defined $lastSelected && ( $y2 + 1 == $lastSelected) &&
               ( ${$selected[-1]}[3] - $lastSelected < 100))
            {
               ${$selected[-1]}[1] = $y;
               ${$selected[-1]}[5] = $$_[$i]->[0];
            } else {
               push ( @selected, [ $x, $y, $x2, $y2, $$_[$i]->[0], $$_[$i]->[0], 1]);
            }
            $lastSelected = $y;
            $isSelected = 1;
         } else {
            if ( defined $lastNormal && ( $y2 + 1 == $lastNormal) &&
               ( ${$normals[-1]}[3] - $lastNormal < 100))
            {
               ${$normals[-1]}[1] = $y;
               ${$normals[-1]}[5] = $$_[$i]->[0];
            } else {
               push ( @normals, [ $x, $y, $x2, $y2, $$_[$i]->[0], $$_[$i]->[0], 0]);
            }
            $lastNormal = $y;
         }
      }
      for ( @selected) { push ( @normals, $_); }
      # draw items
      my $yimg = int(( $ih - $imgSize[1]) / 2);

      for ( @normals)
      {
         my ( $x, $y, $x2, $y2, $first, $last, $selected) = @$_;
         my $c = $clrs[ $selected ? 3 : 1];
         if ( $c != $lbc) {
            $canvas-> backColor( $c);
            $lbc = $c;
         }
         $canvas-> clear( $x, $y, $x2, $y2);
         $c = $clrs[ $selected ? 2 : 0];
         if ( $c != $lc) {
            $canvas-> color( $c);
            $lc = $c;
         }
         for ( $i = $first; $i <= $last; $i++)
         {
             next if $self-> {widths}->[$i] + $offset + $x + 1 < $clipRect[0];
             $canvas-> text_out( $self->{items}->[$i], $x + 2 + $imgSize[0],
                $y2 + $atY - ($i-$first+1) * $ih + 1);
             $canvas-> put_image( $x + 1, $y2 + $yimg - ($i-$first+1) * $ih + 1,
                $images[ vec($v, $i, 1)],
             );
         }
      }
   }
   # draw veil
   if ( $drawVeilFoc >= 0 && $self->{multiSelect})
   {
      my ( $itemIndex, $x, $y, $x2, $y2) = @{$_[$drawVeilFoc]};
      $canvas-> rect_focus( $x + $self->{offset}, $y, $x2, $y2);
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

