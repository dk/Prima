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

# contains:
#   OutlineViewer
#   StringOutline
#   Outline
#   DirectoryOutline

use strict;
use Prima::Classes;
use Prima::IntUtils;
use Prima::StdBitmap;

package Prima::OutlineViewer;
use vars qw(@ISA @images @imageSize);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);

# node record:
#  user fields:
#  0 : item text of ID
#  1 : node subreference ( undef if none)
#  2 : expanded flag
#  private fields
#  3 : item width

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   SelectItem  => nt::Default,
   DrawItem    => nt::Action,
   Stringify   => nt::Action,
   MeasureItem => nt::Action,
   Expand      => nt::Action,
   DragItem    => nt::Default,
);


sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      autoHeight     => 1,
      borderWidth    => 2,
      dragable       => 1,
      hScroll        => 0,
      focusedItem    => -1,
      indent         => 12,
      itemHeight     => $def->{font}->{height},
      items          => [],
      topItem        => 0,
      offset         => 0,
      scaleChildren  => 0,
      selectable     => 1,
      showItemHint   => 1,
      vScroll        => 1,
      widgetClass    => wc::ListBox,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   $p-> { autoHeight}     = 0 if exists $p-> { itemHeight} && !exists $p->{autoHeight};
}

sub init
{
   my $self = shift;
   unless ( @images) {
      my $i = 0;
      for ( sbmp::OutlineCollaps, sbmp::OutlineExpand) {
         $images[ $i++] = Prima::StdBitmap::image($_);
      }
      if ( $images[0]) {
         @imageSize = $images[0]-> size;
      } else {
         @imageSize = (0,0);
      }
   }
   for ( qw( topItem focusedItem))
      { $self->{$_} = -1; }
   for ( qw( scrollTransaction dx dy hScroll vScroll offset count autoHeight borderWidth
      rows maxWidth hintActive showItemHint dragable))
      { $self->{$_} = 0; }
   for ( qw( itemHeight integralHeight indent))
      { $self->{$_} = 1; }
   $self->{items}      = [];
   my %profile = $self-> SUPER::init(@_);
   $self-> {indents} = [0,0,0,0];
   for ( qw( hScroll vScroll offset itemHeight autoHeight borderWidth indent
      items focusedItem topItem showItemHint dragable))
      { $self->$_( $profile{ $_}); }
   $self-> reset;
   $self-> reset_scrolls;
   return %profile;
}

sub iterate
{
   my ( $self, $sub, $full) = @_;
   my $position = 0;
   my $traverse;
   $traverse = sub {
      my ( $current, $level, $lastChild) = @_;
      return $current if $sub->( $current, $position, $level, $lastChild);
      $position++;
      $level++;
      if ( $current->[1] && ( $full || $current->[2])) {
         my $c = scalar @{$current->[1]};
         for ( @{$current->[1]}) {
            my $ret = $traverse->( $_, $level, --$c ? 0 : 1);
            return $ret if $ret;
         }
      }
   };
   my $c = scalar @{$self->{items}};
   for ( @{$self->{items}}) {
      my $ret = $traverse->( $_, 0, --$c ? 0 : 1);
      return $ret if $ret;
   }
}

sub adjust
{
   my ( $self, $index, $action) = @_;
   return unless defined $index;
   my ($node, $lev) = $self-> get_item( $index);
   return unless $node;
   return unless $node->[1];
   return if $node->[2] == $action;

   $self-> notify(q(Expand), $node, $action);

   $node->[2] = $action;
   my $c = $self->{count};
   my $f = $self->{focusedItem};
   $self-> reset_tree;

#  my ( $bw, $dx, $dy, $w, $h, $ih) = ( $self->{borderWidth}, $self->{dx},
#     $self->{dy}, $self-> size, $self->{itemHeight});
   my ( $ih, @a) = ( $self->{itemHeight}, $self-> get_active_area);
   $self-> scroll( 0, ( $c - $self->{count}) * $ih,
              #    clipRect => [ $bw, $bw + $dy, $w - $bw - $dx,
              #                  $h - $bw - $ih * ( $index - $self->{topItem} + 1) ]);
                   clipRect => [ @a[0..2], $a[3] - $ih * ( $index - $self->{topItem} + 1)]);
   $self-> invalidate_rect(
#     $bw, $h - ( $index - $self->{topItem} + 1) * $ih - 2,
#     $w - $dx - $bw, $h - ( $index - $self->{topItem}) * $ih
      $a[0], $a[3] - ( $index - $self->{topItem} + 1) * $ih,
      $a[2], $a[3] - ( $index - $self->{topItem}) * $ih
   );
   if ( $c > $self->{count} && $f > $index) {
      if ( $f <= $index + $c - $self->{count}) {
         $self-> focusedItem( $index);
      } else {
         $self-> focusedItem( $f + $self->{count} - $c);
      }
   } elsif ( $c < $self->{count} && $f > $index) {
      $self-> focusedItem( $f + $self->{count} - $c);
   }
   my ($ix,$l) = $self-> get_item( $self-> focusedItem);

   $self-> update_tree;
   $self-> reset_scrolls;
   $self-> offset( $self-> {offset} + $self-> {indent})
      if $action && $c != $self->{count};
}

sub expand_all
{
   my ( $self, $node) = @_;
   $node = [ 0, $self->{items}, 1] unless $node;
   $self->{expandAll}++;
   if ( $node->[1]) {
      #  - light version of adjust
      unless ( $node->[2]) {
          $node->[2] = 1;
          $self-> notify(q(Expand), $node, 1);
      }
      $self-> expand_all( $_) for @{$node->[1]};
   };
   return if --$self->{expandAll};
   delete $self->{expandAll};
   $self-> reset_tree;
   $self-> update_tree;
   $self-> repaint;
   $self-> reset_scrolls;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;
   my @clr    = $self-> enabled ?
    ( $self-> color, $self-> backColor) :
    ( $self-> disabledColor, $self-> disabledBackColor);
#  my ( $bw, $ih, $iw, $dx, $dy, $indent, $foc) = (
#     $self->{ borderWidth}, $self->{ itemHeight}, $self->{ maxWidth},
#     $self->{dx}, $self->{dy}, $self->{indent}, $self-> {focusedItem});
   my ( $bw, $ih, $iw, $indent, $foc, @a) = (
      $self->{ borderWidth}, $self->{ itemHeight}, $self->{ maxWidth},
      $self->{indent}, $self-> {focusedItem}, $self-> get_active_area( 1, @size));
   my $i;
   my $j;
#  my $locWidth = $size[0] - $dx - $bw * 2;
   my $locWidth = $a[2] - $a[0] + 1;
   my @clipRect = $canvas-> clipRect;
#  if ( $clipRect[0] > $bw && $clipRect[1] > $bw + $dy && $clipRect[2] < $size[0] - $bw - $dx && $clipRect[3] < $size[1] - $bw)
   if ( $clipRect[0] > $a[0] && $clipRect[1] > $a[1] && $clipRect[2] < $a[2] && $clipRect[3] < $a[3])
   {
      #$canvas-> color( $clr[ 1]);
      #$canvas-> clipRect( $bw, $bw + $dy, $size[0] - $bw - $dx - 1, $size[1] - $bw - 1);
      $canvas-> clipRect( @a);
      $canvas-> color( $clr[1]);
      $canvas-> bar( 0, 0, @size);
   } else {
      $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor, $clr[1]);
      $canvas-> clipRect( @a);
      #$canvas-> clipRect( $bw, $bw + $dy, $size[0] - $bw - $dx - 1, $size[1] - $bw - 1);
   }
   my ( $topItem, $rows) = ( $self->{topItem}, $self->{rows});
   my $lastItem  = $topItem + $rows + 1;
   my $timin = $topItem;
#  $timin    += int(( $size[1] - $bw - 1 - $clipRect[3]) / $ih) if $clipRect[3] < $size[1] - $bw - 1;
   $timin    += int(( $a[3] - $clipRect[3]) / $ih) if $clipRect[3] < $a[3];
#  if ( $clipRect[1] >= $bw + $dy) {
   if ( $clipRect[1] >= $a[1]) {
#     my $y = $size[1] - $clipRect[1] - $bw;
      my $y = $a[3] - $clipRect[1] + 1;
      $lastItem = $topItem + int($y / $ih) + 1;
   }
   $lastItem     = $self->{count} - 1 if $lastItem > $self->{count} - 1;
#  my $firstY    = $size[1] - $bw + $ih * $topItem;
#  my $lineY     = $size[1] - $bw - $ih * ( 1 + $timin - $topItem);
   my $firstY    = $a[3] + 1 + $ih * $topItem;
   my $lineY     = $a[3] + 1 - $ih * ( 1 + $timin - $topItem);
   my $dyim      = int(( $ih - $imageSize[1]) / 2) + 1;
   my $dxim      = int( $imageSize[0] / 2);


# drawing lines
   my @lines;
   my @marks;
   my @texts;

   my $deltax = - $self->{offset} + ($indent/2) + $a[0];
   $canvas-> set(
      fillPattern => fp::SimpleDots,
      color       => cl::White,
      backColor   => cl::Black,
      # rop         => rop::XorPut,
   );


   my ($array, $idx, $lim, $level) = ([['root'],$self->{items}], 0, scalar @{$self->{items}}, 0);
   my @stack;
   my $position = 0;

# preparing stack
   $i = int( $timin / 8) * 8 - 1;
   if ( $i > 0) {
      $position = $i;
      $j = int( $timin / 8) - 1;
      $i = $self->{stackFrames}->[$j];
      if ( $i) {
         my $k;
         for ( $k = 0; $k < scalar @{$i} - 1; $k++) {
            $idx   = $i->[$k] + 1;
            $lim   = scalar @{$array->[1]};
            push( @stack, [ $array, $idx, $lim]);
            $array = $array->[1]->[$idx - 1];
         }
         $idx   = $$i[$k];
         $lim   = scalar @{$array->[1]};
         $level = scalar @$i - 1;
         $i = $self->{lineDefs}->[$j];
         $lines[$k] = $$i[$k] while $k--;
      }
   }

# following loop is recursive call turned inside-out -
# so we can manipulate with stack

   if ( $position <= $lastItem) {
   while (1) {
      my $node      = $array->[1]->[$idx++];
      my $lastChild = $idx == $lim;

      # outlining part
      my $l = int(( $level + 0.5) * $indent) + $deltax;
      if ( $lastChild) {
         if ( defined $lines[ $level]) {
            $canvas-> bar(
               $l, $firstY - $ih * $lines[ $level],
               $l, $firstY - $ih * ( $position + 0.5))
            if $position >= $timin;
            $lines[ $level] = undef;
         } elsif ( $position > 0) {
         # first and last
            $canvas-> bar(
               $l, $firstY - $ih * ( $position - 0.5),
               $l, $firstY - $ih * ( $position + 0.5))
         }
      } elsif ( !defined $lines[$level]) {
         $lines[$level] = $position ? $position - 0.5 : 0.5;
      }
      if ( $position >= $timin) {
         $canvas-> bar( $l + 1, $lineY + $ih/2, $l + $indent - 1, $lineY + $ih/2);
         if ( defined $node->[1]) {
            my $i = $images[($node->[2] == 0) ? 1 : 0];
            push( @marks, [$l - $dxim, $lineY + $dyim, $i]) if $i;
         };
         push ( @texts, [ $node, $l + $indent * 1.5, $lineY,
            $l + $indent * 1.5 + $node->[3] - 1, $lineY + $ih - 1,
            $position, ( $foc == $position) ? 1 : 0]);
         $lineY -= $ih;
      }
      last if $position >= $lastItem;

      # recursive part
      $position++;

      if ( $node->[1] && $node->[2] && scalar @{$node->[1]}) {
         $level++;
         push ( @stack, [ $array, $idx, $lim]);
         $idx   = 0;
         $array = $node;
         $lim   = scalar @{$node->[1]};
         next;
      }
      while ( $lastChild) {
         last unless $level--;
         ( $array, $idx, $lim) = @{pop @stack};
         $lastChild = $idx == $lim;
      }
   }}

#  $self-> iterate( sub {
#     my ( $current, $position, $level, $lastChild) = @_;
#     my $l = int(( $level + 0.5) * $indent) + $deltax;
#     if ( $lastChild) {
#        if ( defined $lines[ $level]) {
#           $canvas-> bar(
#              $l, $firstY - $ih * $lines[ $level],
#              $l, $firstY - $ih * ( $position + 0.5))
#           if $position >= $timin;
#           $lines[ $level] = undef;
#        } elsif ( $position > 0) {
#        # first and last
#           $canvas-> bar(
#              $l, $firstY - $ih * ( $position - 0.5),
#              $l, $firstY - $ih * ( $position + 0.5))
#        }
#     } elsif ( !defined $lines[$level]) {
#        $lines[$level] = $position ? $position - 0.5 : 0.5;
#     }
#     if ( $position >= $timin) {
#        $canvas-> bar( $l + 1, $lineY + $ih/2, $l + $indent - 1, $lineY + $ih/2);
#        if ( defined $current->[1]) {
#           my $i = $images[($current->[2] == 0) ? 1 : 0];
#           push( @marks, [$l - $dxim, $lineY + $dyim, $i]) if $i;
#        };
#        push ( @texts, [ $current, $l + $indent * 1.5, $lineY,
#           $l + $indent * 1.5 + $node->[3] - 1, $lineY + $ih - 1,
#           $position, ( $foc == $position) ? 1 : 0]);
#        $lineY -= $ih;
#     }
#     return $position >= $lastItem;
#  });

# drawing line ends
   $i = 0;
   for ( @lines) {
      $i++;
      next unless defined $_;
      my $l = ( $i - 0.5) * $indent + $deltax;;
      $canvas-> bar( $l, $firstY - $ih * $_, $l, 0);
   }
   $canvas-> set(
      fillPattern => fp::Solid,
      color       => $clr[0],
      backColor   => $clr[1],
      # rop         => rop::CopyPut,
   );

#
   $canvas-> put_image( @$_) for @marks;
   $self-> draw_items( $canvas, \@texts);
}

sub on_size
{
   my $self = $_[0];
   $self-> reset;
   $self-> reset_scrolls;
}

sub on_fontchanged
{
   my $self = $_[0];
   $self-> itemHeight( $self-> font-> height), $self->{autoHeight} = 1 if $self-> { autoHeight};
   $self-> calibrate;
}

sub point2item
{
   my ( $self, $y, $h) = @_;
   my $i = $self->{indents};
   # my ( $bw, $dy) = ( $self->{borderWidth}, $self->{dy});
   $h = $self-> height unless defined $h;
   # return $self->{topItem} - 1 if $y >= $h - $bw;
   return $self->{topItem} - 1 if $y >= $h - $$i[3];
   # return $self->{topItem} + $self->{rows} if $y <= $bw + $dy;
   return $self->{topItem} + $self->{rows} if $y <= $$i[1];
   # $y = $h - $y - $bw;
   $y = $h - $y - $$i[3];
   return $self->{topItem} + int( $y / $self->{itemHeight});
}


sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   my $bw = $self-> { borderWidth};
   my @size = $self-> size;
   $self-> clear_event;
   # my ($dx,$dy,$o,$i) = ( $self->{dx}, $self->{dy}, $self->{offset}, $self->{indent});
   my ($o,$i,@a) = ( $self->{offset}, $self->{indent}, $self-> get_active_area(0, @size));
   return if $btn != mb::Left;
   return if defined $self->{mouseTransaction} ||
   #  $y < $bw + $dy || $y >= $size[1] - $bw ||
   #  $x < $bw || $x >= $size[0] - $bw - $dx;
      $y < $a[1] || $y >= $a[3] || $x < $a[0] || $x >= $a[2];

   my $item   = $self-> point2item( $y, $size[1]);
   my ( $rec, $lev) = $self-> get_item( $item);
#  if ( $rec && ( $x > -$o + $lev * $i + $i/2 + $a[0]) &&
#     ( $x < -$o + $lev * $i + ($i*3)/2 + $a[0])) {
   if ( $rec &&
         ( $x >= ( 1 + $lev) * $i + $a[0] - $o - $imageSize[0] / 2) &&
         ( $x <  ( 1 + $lev) * $i + $a[0] - $o + $imageSize[0] / 2)
      ) {
      $self-> adjust( $item, $rec->[2] ? 0 : 1) if $rec->[1];
      return;
   }

   $self-> {mouseTransaction} = (( $mod & km::Ctrl) && $self->{dragable}) ? 2 : 1;
   $self-> focusedItem( $item >= 0 ? $item : 0);
   $self-> {mouseTransaction} = 1 if $self-> focusedItem < 0;
   if ( $self-> {mouseTransaction} == 2) {
      $self-> {dragItem} = $self-> focusedItem;
      $self-> {mousePtr} = $self-> pointer;
      $self-> pointer( cr::Move);
   }
   $self-> capture(1);
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   $self-> clear_event;
   return if $btn != mb::Left || !$dbl;
   my $bw = $self-> { borderWidth};
   my @size = $self-> size;
   my $item   = $self-> point2item( $y, $size[1]);
   #my ($dx,$dy,$o,$i) = ( $self->{dx}, $self->{dy}, $self->{offset}, $self->{indent});
   my ($o,$i) = ( $self->{offset}, $self->{indent});
   my ( $rec, $lev) = $self-> get_item( $item);
#  if ( $rec && ( $x > -$o + $lev * $i + $i/2 + $self->{indents}->[0]) &&
#     ( $x < -$o + $lev * $i + ($i*3)/2 + $self->{indents}->[0])) {
   if ( $rec &&
         ( $x >= ( 1 + $lev) * $i + $self->{indents}->[0] - $o - $imageSize[0] / 2) &&
         ( $x <  ( 1 + $lev) * $i + $self->{indents}->[0] - $o + $imageSize[0] / 2)
      ) {
      $self-> adjust( $item, $rec->[2] ? 0 : 1) if $rec->[1];
      return;
   }
   $self-> notify( q(Click)) if $self->{count};
}

sub makehint
{
   my ( $self, $show, $itemid) = @_;
   return if !$show && !$self->{hintActive};
   if ( !$show) {
      $self->{hinter}-> hide;
      $self->{hintActive} = 0;
      return;
   }
   return if defined $self->{unsuccessfullId} && $self->{unsuccessfullId} == $itemid;

   return unless $self->{showItemHint};

   my ( $item, $lev) = $self-> get_item( $itemid);
   unless ( $item) {
      $self-> makehint(0);
      return;
   }

   # return if $show && $self->{hinter} && $self->{hinter}-> {id} == $itemid;

   my $w = $self-> get_item_width( $item);
#  my $x = $self-> width - $self-> {borderWidth} * 2 - $self->{dx};
#  my ($x,$y) = $self-> get_active_area( 2);
   my @a = $self-> get_active_area;
   my $ofs = ( $lev + 2.5) * $self->{indent} - $self->{offset} + $self-> {indents}->[0];

#  if ( $w + $ofs <= $x) {
   if ( $w + $ofs <= $a[2]) {
     $self-> makehint(0);
     return;
   }

   $self->{unsuccessfullId} = undef;

   unless ( $self->{hinter}) {
       $self->{hinter} = $self-> insert( Widget =>
           clipOwner      => 0,
           selectable     => 0,
           ownerColor     => 1,
           ownerBackColor => 1,
           ownerFont      => 1,
           visible        => 0,
           height         => $self->{itemHeight},
           name           => 'Hinter',
           delegations    => [qw(Paint MouseDown MouseLeave)],
       );
   }
   $self->{hintActive} = 1;
   $self->{hinter}-> {id} = $itemid;
   $self->{hinter}-> {node} = $item;
   my @org = $self-> client_to_screen(0,0);
   $self->{hinter}-> set(
      origin  => [ $org[0] + $ofs - 2,
#                $org[1] + $self-> height - $self->{borderWidth} -
                 $org[1] + $self-> height - $self->{indents}->[3] -
                 $self->{itemHeight} * ( $itemid - $self->{topItem} + 1),
                 ],
      width   => $w + 4,
      text    => $self-> get_item_text( $item),
      visible => 1,
   );
   $self->{hinter}-> repaint;
}

sub Hinter_Paint
{
   my ( $owner, $self, $canvas) = @_;
   my $c = $self-> color;
   $canvas-> color( $self-> backColor);
   my @sz = $canvas-> size;
   $canvas-> bar( 0, 0, @sz);
   $canvas-> color( $c);
   $canvas-> rectangle( 0, 0, $sz[0] - 1, $sz[1] - 1);
   my @rec = ([ $self->{node}, 2, 0,
       $sz[0] - 3, $sz[1] - 1, 0, 0
   ]);
   $owner-> draw_items( $canvas, \@rec);
}

sub Hinter_MouseDown
{
   my ( $owner, $self, $btn, $mod, $x, $y) = @_;
   $owner-> makehint(0);
   my @ofs = $owner-> screen_to_client( $self-> client_to_screen( $x, $y));
   $owner-> mouse_down( $btn, $mod, @ofs);
   $owner-> {unsuccessfullId} = $self->{id};
}

sub Hinter_MouseLeave
{
   $_[0]-> makehint(0);
}


sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   # my $bw = $self-> { borderWidth};
   # my ($dx,$dy)     = ( $self->{dx}, $self->{dy});
   my @size = $self-> size;
   my @a    = $self-> get_active_area( 0, @size);
   if ( !defined $self->{mouseTransaction} && $self->{showItemHint}) {
      my $item   = $self-> point2item( $y, $size[1]);
      my ( $rec, $lev) = $self-> get_item( $item);
      if ( !$rec || ( $x < -$self->{offset} + ($lev + 2) * $self->{indent} + $self->{indents}->[0])) {
         $self-> makehint( 0);
         return;
      }
#     if (( $y >= $size[1] - $bw) || ( $y <= $bw + $dy + $self->{itemHeight} / 2)) {
      if (( $y >= $a[3]) || ( $y <= $a[1] + $self->{itemHeight} / 2)) {
         $self-> makehint( 0);
         return;
      }
#     $y = $size[1] - $y - $bw;
      $y = $a[3] - $y;
      $self-> makehint( 1, $self->{topItem} + int( $y / $self->{itemHeight}));
      return;
   }
   my $item = $self-> point2item( $y, $size[1]);
#  if ( $y >= $size[1] - $bw || $y < $bw + $dx || $x >= $size[0] - $bw - $dx || $x < $bw)
   if ( $y >= $a[3] || $y < $a[1] || $x >= $a[2] || $x < $a[0])
   {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self->scroll_timer_semaphore;
      $self->scroll_timer_semaphore(0);
   } else {
      $self-> scroll_timer_stop;
   }
   $self-> focusedItem( $item >= 0 ? $item : 0);
#  $self-> offset( $self->{offset} + 5 * (( $x < $bw) ? -1 : 1)) if $x >= $size[0] - $bw - $dx || $x < $bw;
   $self-> offset( $self->{offset} + 5 * (( $x < $a[0]) ? -1 : 1)) if $x >= $a[2] || $x < $a[0];
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless defined $self->{mouseTransaction};
   my @dragnotify;
   if ( $self->{mouseTransaction} == 2) {
      $self-> pointer( $self-> {mousePtr});
      my $fci = $self-> focusedItem;
      @dragnotify = ($self-> {dragItem}, $fci) unless $fci == $self-> {dragItem};
   }
   delete $self->{mouseTransaction};
   delete $self->{mouseHorizontal};
   $self-> capture(0);
   $self-> clear_event;
   $self-> notify(q(DragItem), @dragnotify) if @dragnotify;
}

sub on_mousewheel
{
   my ( $self, $mod, $x, $y, $z) = @_;
   $z = int( $z/120);
   $z *= $self-> {rows} if $mod & km::Ctrl;
   my $newTop = $self-> topItem - $z;
   my $maxTop = $self-> {count} - $self-> {rows};
   $self-> topItem( $newTop > $maxTop ? $maxTop : $newTop);
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }
sub on_leave
{
   my $self = $_[0];
   if ( $self->{mouseTransaction})  {
      $self-> capture(0) if $self->{mouseTransaction};
      $self->{mouseTransaction} = undef;
   }
}


sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   return if $mod & km::DeadKey;
   $mod &= ( km::Shift|km::Ctrl|km::Alt);
   $self->notify(q(MouseUp),0,0,0) if defined $self->{mouseTransaction};

   return unless $self->{count};
   if (( $key == kb::NoKey) && ( $code & 0xFF)) {
      if ( chr( $code) eq '+') {
         $self-> adjust( $self->{focusedItem}, 1);
         $self-> clear_event;
         return;
      } elsif ( chr( $code) eq '-') {
         my ( $item, $lev) = $self-> get_item( $self->{focusedItem});
         if ( $item->[1] && $item->[2]) {
            $self-> adjust( $self->{focusedItem}, 0);
            $self-> clear_event;
            return;
         } elsif ( $lev > 0) {
            my $i = $self->{focusedItem};
            my ( $par, $parlev) = ( $item, $lev);
            ( $par, $parlev) = $self-> get_item( --$i) while $parlev != $lev - 1;
            $self-> adjust( $i, 0);
            $self-> clear_event;
            return;
         }
      }

      if ( !($mod & ~km::Shift))  {
         my $i;
         my ( $c, $hit, $items) = ( lc chr ( $code & 0xFF), undef, $self->{items});
         for ( $i = $self->{focusedItem} + 1; $i < $self->{count}; $i++)
         {
            my $fc = substr( $self-> get_index_text($i), 0, 1);
            next unless defined $fc;
            $hit = $i, last if lc $fc eq $c;
         }
         unless ( defined $hit) {
            for ( $i = 0; $i < $self->{focusedItem}; $i++)  {
               my $fc = substr( $self-> get_index_text($i), 0, 1);
               next unless defined $fc;
               $hit = $i, last if lc $fc eq $c;
            }
         }
         if ( defined $hit)  {
            $self-> focusedItem( $hit);
            $self-> clear_event;
            return;
         }
      }
      return;
   }

   return if $mod != 0;

   if ( scalar grep { $key == $_ } (kb::Left,kb::Right,kb::Up,kb::Down,kb::Home,kb::End,kb::PgUp,kb::PgDn))
   {
      my $newItem = $self->{focusedItem};
      my $pgStep  = $self->{rows} - 1;
      $pgStep = 1 if $pgStep <= 0;
      if ( $key == kb::Up)   { $newItem--; };
      if ( $key == kb::Down) { $newItem++; };
      if ( $key == kb::Home) { $newItem = $self->{topItem} };
      if ( $key == kb::End)  { $newItem = $self->{topItem} + $pgStep; };
      if ( $key == kb::PgDn) { $newItem += $pgStep };
      if ( $key == kb::PgUp) { $newItem -= $pgStep};
      $self-> offset( $self->{offset} + $self->{indent} * (( $key == kb::Left) ? -1 : 1))
         if $key == kb::Left || $key == kb::Right;
      $self-> focusedItem( $newItem >= 0 ? $newItem : 0);
      $self-> clear_event;
      return;
   }

   if ( $key == kb::Enter)  {
      $self-> adjust( $self->{focusedItem}, 1);
      $self-> clear_event;
      return;
   }
}


sub reset
{
   my $self = $_[0];
#  my @size = $self-> size;
   my @size = $self-> get_active_area( 2);
   $self-> makehint(0);
   my $ih   = $self-> {itemHeight};
#  my $bw   = $self-> {borderWidth};
#  $self->{dy} = ( $self->{hScroll} ? $self->{hScrollBar}-> height-1 : 0);
#  $self->{dx} = ( $self->{vScroll} ? $self->{vScrollBar}-> width-1  : 0);
#  $size[1] -= $bw * 2 + $self->{dy};
#  $size[0] -= $bw * 2 + $self->{dx};
   $self->{rows}  = int( $size[1] / $ih);
   $self->{rows}  = 0 if $self->{rows} < 0;
   $self->{yedge} = ( $size[1] - $self->{rows} * $ih) ? 1 : 0;
}

sub reset_scrolls
{
   my $self = $_[0];
   $self-> makehint(0);
   if ( $self-> {scrollTransaction} != 1 && $self->{vScroll})
   {
      $self-> {vScrollBar}-> set(
         max      => $self-> {count} - $self->{rows},
         pageStep => $self-> {rows},
         whole    => $self-> {count},
         partial  => $self-> {rows},
         value    => $self-> {topItem},
      );
   }
   if ( $self->{scrollTransaction} != 2 && $self->{hScroll})  {
   #  my $w  = $self-> width - $self->{borderWidth} * 2 - $self->{dx};
      my @sz = $self-> get_active_area( 2);
      my $iw = $self->{maxWidth};
      $self-> {hScrollBar}-> set(
   #     max      => $iw - $w,
         max      => $iw - $sz[0],
         whole    => $iw,
         value    => $self-> {offset},
   #     partial  => $w,
         partial  => $sz[0],
         pageStep => $iw / 5,
      );
   }
}

sub reset_tree
{
   my ( $self, $i) = ( $_[0], 0);
   $self-> makehint(0);
   $self-> {stackFrames} = [];
   $self-> {lineDefs}    = [];
   my @stack;
   my @lines;
   my $traverse;
   $traverse = sub {
      my ( $node, $level, $lastChild) = @_;
      $lines[ $level] = $lastChild ? undef : ( $i ? $i - 0.5 : 0.5);
      if (( $i % 8) == 7) {
         push( @{$self->{stackFrames}}, [@stack[0..$level]]);
         push( @{$self->{lineDefs}},    [@lines[0..$level]]);
      }
      $i++;
      $level++;
      if ( $node->[1] && $node->[2]) {
         $stack[$level] = 0;
         my $c = @{$node->[1]};
         for ( @{$node->[1]}) {
            $traverse->( $_, $level, --$c ? 0 : 1);
            $stack[$level]++;
         }
      }
   };

   $stack[0] = 0;
   my $c = @{$self->{items}};
   for (@{$self->{items}}) {
      $traverse->( $_, 0, --$c ? 0 : 1);
      $stack[0]++;
   }

   $self-> {count} = $i;
   delete $self-> {itemCache};

   my $fullc = $self->{fullCalibrate};
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
   my $maxWidth = 0;
   my $indent = $self->{indent};
   $self-> push_event;
   $self-> begin_paint_info;
   $self-> iterate( sub {
      my ( $current, $position, $level) = @_;
      my $iw = $fullc ? undef : $current->[3];
      unless ( defined $iw) {
         $notifier->( @notifyParms, $current, \$iw);
         $current->[3] = $iw;
      }
      my $iwc = $iw + ( 2.5 + $level) * $indent;
      $maxWidth = $iwc if $maxWidth < $iwc;
      return 0;
   });
   $self-> end_paint_info;
   $self-> pop_event;
   $self-> {maxWidth} = $maxWidth;
}

sub calibrate
{
   my $self = $_[0];
   $self->{fullCalibrate} = 1;
   $self-> reset_tree;
   delete $self->{fullC};
   $self-> update_tree;
}

sub update_tree
{
   my $self = $_[0];
   $self-> topItem( $self-> {topItem});
   $self-> offset( $self-> {offset});
}


sub draw_items
{
   my ($self, $canvas, $paintStruc) = @_;
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
   $self-> push_event;
   for ( @$paintStruc) { $notifier->( @notifyParms, $canvas, @$_); }
   $self-> pop_event;
}

sub set_auto_height
{
   my ( $self, $auto) = @_;
   $self-> itemHeight( $self-> font-> height) if $auto;
   $self->{autoHeight} = $auto;
}

sub set_border_width
{
   my ( $self, $bw) = @_;
   $bw = 0 if $bw < 0;
   $bw = 1 if $bw > $self-> height / 2;
   $bw = 1 if $bw > $self-> width  / 2;
   return if $bw == $self-> {borderWidth};
   $self-> SUPER::set_border_width( $bw);
   $self-> reset;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub set_focused_item
{
   my ( $self, $foc) = @_;
   my $oldFoc = $self->{focusedItem};
   $foc = $self->{count} - 1 if $foc >= $self->{count};
   $foc = -1 if $foc < -1;
   return if $self->{focusedItem} == $foc;
   return if $foc < -1;
   $self-> {focusedItem} = $foc;
   $self-> notify(q(SelectItem), $foc) if $foc >= 0;
   my $topSet = undef;
   if ( $foc >= 0)
   {
      my $rows = $self->{rows} ? $self->{rows} : 1;
      if ( $foc < $self->{topItem}) {
         $topSet = $foc;
      } elsif ( $foc >= $self->{topItem} + $rows) {
         $topSet = $foc - $rows + 1;
      }
   }
   $self-> topItem( $topSet) if defined $topSet;
   ( $oldFoc, $foc) = ( $foc, $oldFoc) if $foc > $oldFoc;
#  my @sz = $self-> size;
   my @a  = $self-> get_active_area;
#  my $bw = $self->{borderWidth};
#  $sz[1] -= $bw;
   my $ih = $self->{itemHeight};
   my $lastItem = $self->{topItem} + $self->{rows};
   $foc = $oldFoc if $foc < 0 || $foc < $self->{topItem} || $foc > $self->{topItem} + $self->{rows};
   $oldFoc = $foc if $oldFoc < 0 || $oldFoc < $self->{topItem} || $oldFoc > $self->{topItem} + $self->{rows};
   $self-> invalidate_rect(
      $a[0], $a[3] - ( $oldFoc - $self->{topItem} + 1) * $ih,
      $a[2], $a[3] - ( $foc - $self->{topItem}) * $ih
#     $bw, $sz[1] - ( $oldFoc - $self->{topItem} + 1) * $ih,
#     $sz[0] - $self->{dx} - $bw, $sz[1] - ( $foc - $self->{topItem}) * $ih
   ) if $foc >= 0;
}

sub set_indent
{
   my ( $self, $i) = @_;
   return if $i == $self->{indent};
   $i = 1 if $i < 1;
   $self->{indent} = $i;
   $self-> calibrate;
   $self-> repaint;
}


sub set_item_height
{
   my ( $self, $ih) = @_;
   $ih = 1 if $ih < 1;
   $self-> autoHeight(0);
   return if $ih == $self->{itemHeight};
   $self->{itemHeight} = $ih;
   $self->reset;
   $self->reset_scrolls;
   $self->repaint;
   $self-> {hinter}-> height( $ih) if $self-> {hinter};
}

sub validate_items
{
   my ( $self, $items) = @_;
   my $traverse;
   $traverse = sub {
      my $current  = $_[0];
      my $spliceTo = 3;
      if ( ref $current->[1] eq 'ARRAY') {
         $traverse->( $_) for @{$current->[1]};
         $current->[2] = 0 unless defined $current->[2];
      } else {
         $spliceTo = 1;
      }
      splice( @$current, $spliceTo);
   };
   $traverse->( $items);
}

sub set_items
{
   my ( $self, $items) = @_;
   $items = [] unless defined $items;
   $self-> validate_items( [ 0, $items]);
   $self-> {items} = $items;
   $self-> reset_tree;
   $self-> update_tree;
   $self-> repaint;
   $self-> reset_scrolls;
}

sub insert_items
{
   my ( $self, $where, $at, @items) = @_;
   return unless scalar @items;
   my $forceReset = 0;
   $where = [0, $self->{items}], $forceReset = 1 unless $where;
   $self-> validate_items( $_) for @items;
   return unless $where->[1];
   my $ch = scalar @{$where->[1]};
   $at = 0 if $at < 0;
   $at = $ch if $at > $ch;
   my ( $x, $l) = $self-> get_index( $where);
   splice( @{$where->[1]}, $at, 0, @items);
   return if $x < 0 && !$forceReset;
   $self-> reset_tree;
   $self-> update_tree;
   $self-> repaint;
   $self-> reset_scrolls;
}

sub delete_items
{
   my ( $self, $where, $at, $amount) = @_;
   $where = [0, $self->{items}] unless $where;
   return unless $where->[1];
   my ( $x, $l) = $self-> get_index( $where);
   $at = 0 unless defined $at;
   $amount = scalar @{$where->[1]} unless defined $amount;
   splice( @{$where->[1]}, $at, $amount);
   return if $x < 0;
   my $f = $self->{focusedItem};
   $self-> focusedItem( -1) if $f >= $x && $f < $x + $amount;
   $self-> reset_tree;
   $self-> update_tree;
   $self-> repaint;
   $self-> reset_scrolls;
}

sub delete_item
{
   my ( $self, $item) = @_;
   return unless $item;
   my ( $x, $l) = $self-> get_index( $item);

   my ( $parent, $offset) = $self-> get_item_parent( $item);
   if ( defined $parent) {
      splice( @{$parent->[1]}, $offset, 1);
   } else {
      splice( @{$self->{items}}, $offset, 1) if defined $offset;
   }
   if ( $x >= 0) {
      $self-> reset_tree;
      $self-> update_tree;
      $self-> focusedItem( -1) if $x == $self->{focusedItem};
      $self-> repaint;
      $self-> reset_scrolls;
   }
}

sub get_item_parent
{
   my ( $self, $item) = @_;
   my $parent;
   my $offset;
   my $count = 0;
   return unless $item;
   for ( @{$self->{items}}) {
      return ( undef, $count) if $_ == $item;
      $count++;
   }
   $self-> iterate( sub {
      my $cur = $_[0];
      if ( $cur->[1] && $cur->[2]) {
         my $count = 0;
         for ( @{$cur->[1]}) {
            if ( $_ == $item) {
               $offset = $count;
               $parent = $cur;
               return 1;
            }
            $count++;
         }
         return 0;
      }
   });
   return ( $parent, $offset) if $parent;

   my $traverse;
   $traverse = sub {
      my $current  = $_[0];
      if ( defined $current->[1]) {
         my $count = 0;
         for ( @{$current->[1]}) {
            if ( $_ == $item) {
               $offset = $count;
               $parent = $current;
               return 1;
            }
            $traverse->( $_);
         }
         $count++;
      }
   };
   for ( @{$self->{items}}) {
      last if $traverse->( $_);
   }
   return $parent, $offset;
}

sub set_offset
{
   my ( $self, $offset) = @_;
#  my ( $iw, $bw, $w, $dx, $dy) = (
#     $self->{maxWidth}, $self->{borderWidth}, $self-> width,
#     $self->{dx}, $self->{dy});
   my ( $iw, @a) = ($self->{maxWidth}, $self-> get_active_area);

#  my $lc = $w - 2 * $bw - $dx;
   my $lc = $a[2] - $a[0];
   if ( $iw > $lc) {
      $offset = $iw - $lc if $offset > $iw - $lc;
      $offset = 0 if $offset < 0;
   } else {
      $offset = 0;
   }
   return if $self->{offset} == $offset;
   my $oldOfs = $self->{offset};
   $self-> {offset} = $offset;
#  $w -= 2 * $bw + $dx;
#  my $dt = $offset - $oldOfs;
   if ( $self->{hScroll} && $self->{scrollTransaction} != 2) {
      $self->{scrollTransaction} = 2;
      $self-> {hScrollBar}-> value( $offset);
      $self->{scrollTransaction} = 0;
   }
   $self-> makehint(0);
   # $self-> scroll( -$dt, 0,
   $self-> scroll( $oldOfs - $offset, 0,
   #               clipRect => [ $bw, $bw + $dy, $bw + $w, $self-> height - $bw ]);
                   clipRect => \@a);
}

sub set_top_item
{
   my ( $self, $topItem) = @_;
   $topItem = 0 if $topItem < 0;   # first validation
   $topItem = $self-> {count} - 1 if $topItem >= $self-> {count};
   $topItem = 0 if $topItem < 0;   # count = 0 case
   return if $topItem == $self->{topItem};
   my $oldTop = $self->{topItem};
   $self->{topItem} = $topItem;
#  my ($bw, $ih, $iw, $w, $h, $dx, $dy) = (
#     $self->{borderWidth}, $self->{itemHeight}, $self->{itemWidth},
#     $self->size, $self->{dx}, $self->{dy});
   my ($ih, @a) = (
      $self->{itemHeight}, $self-> get_active_area);
#  my $dt = $topItem - $oldTop;
   $self-> makehint(0);
   if ( $self->{scrollTransaction} != 1 && $self->{vScroll}) {
      $self->{scrollTransaction} = 1;
      $self-> {vScrollBar}-> value( $topItem);
      $self->{scrollTransaction} = 0;
   }

#  $self-> scroll( 0, $dt * $ih,
   $self-> scroll( 0, ($topItem - $oldTop) * $ih,
#                  clipRect => [ $bw, $bw + $dy, $w - $bw - $dx, $h - $bw ]);
                   clipRect => \@a);
}


sub VScroll_Change
{
   my ( $self, $scr) = @_;
   return if $self-> {scrollTransaction};
   $self-> {scrollTransaction} = 1;
   $self-> topItem( $scr-> value);
   $self-> {scrollTransaction} = 0;
}

sub HScroll_Change
{
   my ( $self, $scr) = @_;
   return if $self-> {scrollTransaction};
   $self-> {scrollTransaction} = 2;
   $self-> {multiColumn} ?
      $self-> topItem( $scr-> value) :
      $self-> offset( $scr-> value);
   $self-> {scrollTransaction} = 0;
}


sub set_h_scroll
{
   my ( $self, $hs) = @_;
   return if $hs == $self->{hScroll};
   $self-> SUPER::set_h_scroll( $hs);
   $self-> reset;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub set_v_scroll
{
   my ( $self, $vs) = @_;
   return if $vs == $self->{vScroll};
   $self-> SUPER::set_v_scroll( $vs);
   $self-> reset;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub showItemHint
{
   return $_[0]-> {showItemHint} unless $#_;
   my ( $self, $sh) = @_;
   return if $sh == $self->{showItemHint};
   $self->{showItemHint} = $sh;
   $self-> makehint(0) if !$sh && $self->{hintActive};
}

sub dragable
{
   return $_[0]-> {dragable} unless $#_;
   $_[0]->{dragable} = $_[1];
}


sub get_index
{
   my ( $self, $item) = @_;
   return -1, undef unless $item;
   my $lev;
   my $rec;
   my $res = $self-> iterate( sub {
      my ( $current, $position, $level, $lastChild) = @_;
      $lev = $level;
      $rec = $position;
      return $current == $item;
   });
   return $rec, $lev if $res;
   return -1, undef;
}


sub get_item
{
   my ( $self, $item) = @_;
   return if $item < 0 || $item >= $self-> {count};
   return @{$self->{itemCache}->{$item}} if exists $self->{itemCache}->{$item};
   my $lev;
   my $rec  = $self-> iterate( sub {
      my ( $current, $position, $level, $lastChild) = @_;
      $lev = $level;
      $self->{itemCache}->{$position} = [$current, $level];
      return $position == $item;
   });
   return $rec, $lev;
}

sub get_item_text
{
   my ( $self, $item) = @_;
   my $txt = '';
   $self-> notify(q(Stringify), $item, \$txt);
   return $txt;
}

sub get_item_width
{
   return $_[1]->[3];
}

sub get_index_text
{
   my ( $self, $index) = @_;
   my $txt = '';
   my ( $node, $lev) = $self->get_item( $index);
   $self-> notify(q(Stringify), $node, \$txt);
   return $txt;
}

sub get_index_width
{
   my ( $self, $index) = @_;
   my ( $node, $lev) = $self-> get_item( $index);
   return $node->[3];
}

sub on_drawitem
{
#  my ( $self, $canvas, $node, $left, $bottom, $right, $top, $position, $focused) = @_;
}

sub on_measureitem
{
#   my ( $self, $node, $result) = @_;
}

sub on_stringify
{
#   my ( $self, $node, $result) = @_;
}

sub on_selectitem
{
#   my ( $self, $index) = @_;
}

sub on_expand
{
#   my ( $self, $node, $action) = @_;
}

sub on_dragitem
{
    my ( $self, $from, $to) = @_;
    my ( $fx, $fl) = $self-> get_item( $from);
    my ( $tx, $tl) = $self-> get_item( $to);
    my ( $fpx, $fpo) = $self-> get_item_parent( $fx);
    return unless $fx && $tx;
    my $found_inv = 0;

    my $traverse;
    $traverse = sub {
       my $current = $_[0];
       $found_inv = 1, return if $current == $tx;
       if ( $current->[1] && $current->[2]) {
          my $c = scalar @{$current->[1]};
          for ( @{$current->[1]}) {
             my $ret = $traverse->( $_);
             return $ret if $ret;
          }
       }
    };
    $traverse->( $fx);
    return if $found_inv;


    if ( $fpx) {
      splice( @{$fpx->[1]}, $fpo, 1);
    } else {
       splice( @{$self->{items}}, $fpo, 1);
    }
    unless ( $tx-> [1]) {
       $tx->[1] = [$fx];
       $tx->[2] = 1;
    } else {
       splice( @{$tx->[1]}, 0, 0, $fx);
    }
    $self-> reset_tree;
    $self-> update_tree;
    $self-> repaint;
    $self-> clear_event;
}


sub autoHeight    {($#_)?$_[0]->set_auto_height    ($_[1]):return $_[0]->{autoHeight}     }
sub focusedItem   {($#_)?$_[0]->set_focused_item   ($_[1]):return $_[0]->{focusedItem}    }
sub indent        {($#_)?$_[0]->set_indent( $_[1])        :return $_[0]->{indent}         }
sub items         {($#_)?$_[0]->set_items( $_[1])         :return $_[0]->{items}          }
sub itemHeight    {($#_)?$_[0]->set_item_height    ($_[1]):return $_[0]->{itemHeight}     }
sub offset        {($#_)?$_[0]->set_offset         ($_[1]):return $_[0]->{offset}         }
sub topItem       {($#_)?$_[0]->set_top_item       ($_[1]):return $_[0]->{topItem}        }

package Prima::StringOutline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

sub draw_items
{
   my ($self, $canvas, $paintStruc) = @_;
   for ( @$paintStruc) {
      my ( $node, $left, $bottom, $right, $top, $position, $focused) = @$_;
      if ( $focused) {
         my $c = $canvas-> color;
         $canvas-> color( $self-> hiliteBackColor);
         $canvas-> bar( $left, $bottom, $right, $top);
         $canvas-> color( $self-> hiliteColor);
         $canvas-> text_out( $node->[0], $left, $bottom);
         $canvas-> color( $c);
      } else {
         $canvas-> text_out( $node->[0], $left, $bottom);
      }
   }
}

sub on_measureitem
{
   my ( $self, $node, $result) = @_;
   $$result = $self-> get_text_width( $node->[0]);
}

sub on_stringify
{
   my ( $self, $node, $result) = @_;
   $$result = $node->[0];
}

package Prima::Outline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

sub draw_items
{
   my ($self, $canvas, $paintStruc) = @_;
   for ( @$paintStruc) {
      my ( $node, $left, $bottom, $right, $top, $position, $focused) = @$_;
      if ( $focused) {
         my $c = $canvas-> color;
         $canvas-> color( $self-> hiliteBackColor);
         $canvas-> bar( $left, $bottom, $right, $top);
         $canvas-> color( $self-> hiliteColor);
         $canvas-> text_out( $node->[0]->[0], $left, $bottom);
         $canvas-> color( $c);
      } else {
         $canvas-> text_out( $node->[0]->[0], $left, $bottom);
      }
   }
}

sub on_measureitem
{
   my ( $self, $node, $result) = @_;
   $$result = $self-> get_text_width( $node->[0]->[0]);
}

sub on_stringify
{
   my ( $self, $node, $result) = @_;
   $$result = $node->[0]->[0];
}

package Prima::DirectoryOutline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

# node[0]:
#  0 : node text
#  1 : parent path, '' if none
#  2 : icon width
#  3 : drive icon, only for roots

use Cwd;

my $unix = Prima::Application-> get_system_info->{apc} == apc::Unix;
my @images;
my @drvImages;

{
   my $i = 0;
   my @idx = (  sbmp::SFolderOpened, sbmp::SFolderClosed);
   $images[ $i++] = Prima::StdBitmap::icon( $_) for @idx;
   unless ( $unix) {
      $i = 0;
      for ( sbmp::DriveFloppy, sbmp::DriveHDD,    sbmp::DriveNetwork,
            sbmp::DriveCDROM,  sbmp::DriveMemory, sbmp::DriveUnknown) {
         $drvImages[ $i++] = Prima::StdBitmap::icon($_);
      }
   }
}

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      path           => '',
      dragable       => 0,
      openedGlyphs   => 1,
      closedGlyphs   => 1,
      openedIcon     => undef,
      closedIcon     => undef,
   }
}

sub init
{
   my $self = shift;
   my %profile = @_;
   $profile{items} = [];
   %profile = $self-> SUPER::init( %profile);
   for ( qw( files filesStat items))             { $self->{$_} = []; }
   for ( qw( openedIcon closedIcon openedGlyphs closedGlyphs indent))
      { $self->{$_} = $profile{$_}}
   $self-> {openedIcon} = $images[0] unless $self-> {openedIcon};
   $self-> {closedIcon} = $images[1] unless $self-> {closedIcon};
   $self->{fontHeight} = $self-> font-> height;
   $self-> recalc_icons;

   my @tree;
   if ( $unix) {
      push ( @tree, [[ '/', ''], [], 0]);
   } else {
      my @drv = split( ' ', Prima::Utils::query_drives_map('A:'));
      for ( @drv) {
         my $type = Prima::Utils::query_drive_type($_);
         push ( @tree, [[ $_, ''], [], 0]);
      }
   }
   $self-> items( \@tree);
   $self-> {cPath} = $profile{path};
   return %profile;
}

sub on_create
{
   my $self = $_[0];
# path could invoke adjust(), thus calling notify(), which
# fails until init() ends.
   $self-> path( $self-> {cPath}) if length $self-> {cPath};
}

sub draw_items
{
   my ($self, $canvas, $paintStruc) = @_;
   for ( @$paintStruc) {
      my ( $node, $left, $bottom, $right, $top, $position, $focused) = @$_;
      my $c;
      my $dw = length $node->[0]->[1] ?
         $self->{iconSizes}->[0] :
         $node->[0]->[2];
      if ( $focused) {
         $c = $canvas-> color;
         $canvas-> color( $self-> hiliteBackColor);
         $canvas-> bar( $left - $self->{indent} / 4, $bottom, $right, $top);
         $canvas-> color( $self-> hiliteColor);
      }
      my $icon = (length( $node->[0]->[1]) || $unix) ?
         ( $node->[2] ? $self->{openedIcon} : $self->{closedIcon}) : $node->[0]->[3];
      $canvas-> put_image(
        $left - $self->{indent} / 4,
        int($bottom + ( $self->{itemHeight} - $self->{iconSizes}->[1]) / 2),
        $icon);
      $canvas-> text_out( $node->[0]->[0], $left + $dw,
        int($bottom + ( $self->{itemHeight} - $self->{fontHeight}) / 2));
      $canvas-> color( $c) if $focused;
   }
}

sub recalc_icons
{
   my $self = $_[0];
   my $hei = $self-> font-> height + 2;
   my ( $o, $c) = (
      $self->{openedIcon} ? $self->{openedIcon}-> height : 0,
      $self->{closedIcon} ? $self->{closedIcon}-> height : 0
   );
   my ( $ow, $cw) = (
      $self->{openedIcon} ? ($self->{openedIcon}-> width / $self->{openedGlyphs}): 0,
      $self->{closedIcon} ? ($self->{closedIcon}-> width / $self->{closedGlyphs}): 0
   );
   $hei = $o if $hei < $o;
   $hei = $c if $hei < $c;
   unless ( $unix) {
      for ( @drvImages) {
         next unless defined $_;
         my @s = $_->size;
         $hei = $s[1] + 2 if $hei < $s[1] + 2;
      }
   }
   $self-> itemHeight( $hei);
   my ( $mw, $mh) = ( $ow, $o);
   $mw = $cw if $mw < $cw;
   $mh = $c  if $mh < $c;
   $self-> {iconSizes} = [ $mw, $mh];
}

sub on_fontchanged
{
   my $self = shift;
   $self-> recalc_icons;
   $self->{fontHeight} = $self-> font-> height;
   $self-> SUPER::on_fontchanged(@_);
}

sub on_measureitem
{
   my ( $self, $node, $result) = @_;
   my $tw = $self-> get_text_width( $node->[0]->[0]) + $self->{indent} / 4;

   unless ( length $node->[0]->[1]) { #i.e. root
      if ( $unix) {
         $node->[0]->[2] = $self->{iconSizes}->[0];
      } else {
         my $dt = Prima::Utils::query_drive_type($node->[0]->[0]) - dt::Floppy;
         $node->[0]->[2] = $drvImages[$dt] ? $drvImages[$dt]-> width : 0;
         $node->[0]->[3] = $drvImages[$dt];
      }
      $tw += $node->[0]->[2];
   } else {
      $tw += $self->{iconSizes}->[0];
   }
   $$result = $tw;
}

sub on_stringify
{
   my ( $self, $node, $result) = @_;
   $$result = $node->[0]->[0];
}

sub get_directory_tree
{
   my ( $self, $path) = @_;
   my @fs = Prima::Utils::getdir( $path);
   return [] unless scalar @fs;
   my $oldPointer = $::application-> pointer;
   $::application-> pointer( cr::Wait);
   my $i;
   my @fs1;
   my @fs2;
   for ( $i = 0; $i < scalar @fs; $i += 2) {
      push( @fs1, $fs[ $i]);
      push( @fs2, $fs[ $i + 1]);
   }

   $self-> {files}     = \@fs1;
   $self-> {filesStat} = \@fs2;
   my @d   = sort grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
   my $ind = 0;
   my @lb;
   for (@d)  {
      @fs = Prima::Utils::getdir( "$path/$_");
      @fs1 = ();
      @fs2 = ();
      for ( $i = 0; $i < scalar @fs; $i += 2) {
         push( @fs1, $fs[ $i]);
         push( @fs2, $fs[ $i + 1]);
      }
      $self-> {files}     = \@fs1;
      $self-> {filesStat} = \@fs2;
      my @dd   = sort grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
      push @lb, [[ $_, "$path/"], scalar @dd ? [] : undef, 0];
   }
   $::application-> pointer( $oldPointer);
   return \@lb;
}

sub files {
   my ( $fn, $fs) = ( $_[0]->{files}, $_[0]-> {filesStat});
   return wantarray ? @$fn : $fn unless ($#_);
   my @f;
   for ( my $i = 0; $i < scalar @$fn; $i++)
   {
      push ( @f, $$fn[$i]) if $$fs[$i] eq $_[1];
   }
   return wantarray ? @f : \@f;
}

sub on_expand
{
   my ( $self, $node, $action) = @_;
   return unless $action;
   my $x = $self-> get_directory_tree( $node->[0]->[1].$node->[0]->[0]);
   $node->[1] = $x;
#  valid way to do the same -
#  $self-> delete_items( $node);
#  $self-> insert_items( $node, 0, @$x);
}

sub path
{
   my $self = $_[0];
   unless ( $#_) {
      my ( $n, $l) = $self-> get_item( $self-> focusedItem);
      return '' unless $n;
      return $n->[0]->[1].$n->[0]->[0];
   }
   my $p = $_[1];
   $p =~ s{^([^\\\/]*[\\\/][^\\\/]*)[\\\/]$}{$1};
   $p = eval { Cwd::abs_path($p) };
   $p = "." if $@;
   $p = "" unless -d $p;
   $p .= '/' unless $p =~ m![/\\]$!;
   $self-> {path} = $p;
   if ( $p eq '/') {
      $self-> focusedItem(0);
      return;
   }
   $p = lc $p unless $unix;
   my @ups = split /[\/\\]/, $p;
   my $root;
   if ( $unix) {
      $root = $self->{items}->[0]->[1];
   } else {
      my $lr = shift @ups;
      for ( @{$self->{items}}) {
         my $drive = lc $_->[0]->[0];
         $root = $_, last if $lr eq $drive;
      }
      return unless defined $root;
   }

   UPS: for ( @ups) {
      last UPS unless defined $root->[1];
      my $subdir = $_;
      unless ( $root->[2]) {
         my ( $idx, $lev) = $self-> get_index( $root);
         $self-> adjust( $idx, 1);
      }
      BRANCH: for ( @{$root->[1]}) {
         my $branch = lc $_->[0]->[0];
         next unless $branch eq $subdir;
         $root = $_;
         last BRANCH;
      }
   }
   my ( $idx, $lev) = $self-> get_index( $root);
   $self-> focusedItem( $idx);
   $self-> adjust( $idx, 1);
   $self-> topItem( $idx);
}

sub openedIcon
{
   return $_[0]->{openedIcon} unless $#_;
   $_[0]-> {openedIcon} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub closedIcon
{
   return $_[0]->{closedIcon} unless $#_;
   $_[0]->{closedIcon} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub openedGlyphs
{
   return $_[0]->{openedGlyphs} unless $#_;
   $_[1] = 1 if $_[1] < 1;
   $_[0]->{openedGlyphs} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

sub closedGlyphs
{
   return $_[0]->{closedGlyphs} unless $#_;
   $_[1] = 1 if $_[1] < 1;
   $_[0]->{closedGlyphs} = $_[1];
   $_[0]-> recalc_icons;
   $_[0]-> calibrate;
}

1;
