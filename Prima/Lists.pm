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
package Prima::Lists;

# contains:
#   AbstractListViewer
#   ListViewer
#   ListBox
#   ProtectedListBox
#   DirectoryListBox

use Carp;
use Prima::Const;
use Prima::Classes;
use Prima::ScrollBar;
use strict;
use Prima::StdBitmap;
use Prima::IntUtils;
use Cwd;

package ci;
use constant Grid      => 1 + MaxId;

package Prima::AbstractListViewer;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);

use Prima::Classes;

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   SelectItem => nt::Default,
   DrawItem   => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      autoHeight     => 1,
      borderWidth    => 2,
      extendedSelect => 0,
      focusedItem    => -1,
      gridColor      => cl::Black,
      hScroll        => 0,
      integralHeight => 0,
      itemHeight     => $def->{font}->{height},
      itemWidth      => $def->{width} - 2,
      multiSelect    => 0,
      multiColumn    => 0,
      offset         => 0,
      topItem        => 0,
      scaleChildren  => 0,
      selectable     => 1,
      selectedItems  => [],
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
   $p-> { multiSelect}    = 1 if exists $p-> { extendedSelect} && $p-> {extendedSelect};
   $p-> { autoHeight}     = 0 if exists $p-> { itemHeight} && !exists $p->{autoHeight};
}

sub init
{
   my $self = shift;
   for ( qw( lastItem topItem focusedItem))
      { $self->{$_} = -1; }
   for ( qw( scrollTransaction gridColor dx dy hScroll vScroll itemWidth offset multiColumn count autoHeight multiSelect extendedSelect borderWidth))
      { $self->{$_} = 0; }
   for ( qw( itemHeight integralHeight))
      { $self->{$_} = 1; }
   $self->{selectedItems} = {};
   my %profile = $self-> SUPER::init(@_);
   $self-> setup_indents;
   $self->{selectedItems} = {} unless $profile{multiSelect};
   for ( qw( gridColor hScroll vScroll offset multiColumn itemHeight autoHeight itemWidth multiSelect extendedSelect integralHeight focusedItem topItem selectedItems borderWidth))
      { $self->$_( $profile{ $_}); }
   $self-> reset;
   $self-> reset_scrolls;
   $self->{firstPaint} = 1;
   return %profile;
}


sub refresh
{
   my $self = $_[0];
   $self-> invalidate_rect( $self-> get_active_area);
   $self-> update_view;
   delete $self-> {singlePaint};
}

sub draw_items
{
   my ($self, $canvas) = (shift, shift);
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
   $self-> push_event;
   for ( @_) { $notifier->( @notifyParms, $canvas, @$_); }
   $self-> pop_event;
}

sub item2rect
{
   my ( $self, $item, @size) = @_;
   my @a = $self-> get_active_area( 0, @size);

   if ( $self->{multiColumn})
   {
      $item -= $self->{topItem};
      my ($j,$i,$ih,$iw) = (
         $self->{rows} ? int( $item / $self->{rows} - (( $item < 0) ? 1 : 0)) : -1,
         $item % ( $self->{rows} ? $self->{rows} : 1),
         $self->{itemHeight},
         $self->{itemWidth}
      );
      return $a[0] + $j * ( $iw + 1),
             $a[3] - $ih * ( $i + 1),
             $a[0] + $j * ( $iw + 1) + $iw,
             $a[3] - $ih * ( $i + 1) + $ih;
   } else {
      my ($i,$ih) = ( $item - $self->{topItem}, $self->{itemHeight});
      return $a[0], $a[3] - $ih * ( $i + 1), $a[2], $a[3] - $ih * $i;
   }
}

sub on_paint
{
   my ($self,$canvas)   = @_;
   my @size   = $canvas-> size;
   unless ( $self-> enabled) {
      $self-> color( $self-> disabledColor);
      $self-> backColor( $self-> disabledBackColor);
   }
   my ( $bw, $ih, $iw, @a) = (
     $self-> {borderWidth}, $self->{ itemHeight}, $self->{ itemWidth},
     $self-> get_active_area( 1, @size));
   my $i;
   my $j;
   my $locWidth = $a[2] - $a[0] + 1;
   my @invalidRect = $canvas-> clipRect;
   if ( exists $self->{firstPaint})
   {
      delete $self->{singlePaint};
      delete $self->{firstPaint};
   }
   unless ( exists $self->{singlePaint})
   {
      $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor);
   }
   if ( defined $self->{uncover})
   {
      if ( $self->{multiColumn})
      {
         my $xstart = $a[0] + $self->{activeColumns} * ( $iw + 1) - 2;
         $canvas-> clear( $xstart - $iw + 1, $self->{yedge} + $a[1],
            (( $xstart > $a[2]) ? $a[2] : $xstart),
            $self->{yedge} + $self->{uncover} - 1) if $xstart > $a[0];
      } else {
         $canvas-> clear( @a[0..2], $self->{uncover} - 1);
      }
   }
   if ( $self->{multiColumn})
   {
       my $xstart = $a[0] + $self->{activeColumns} * ( $iw + 1);
       if ( $self->{activeColumns} < $self->{columns})
       {
          for ( $i = $self->{activeColumns}; $i < $self->{columns}; $i++)
          {
             $canvas-> clear(
                $xstart, $self->{yedge} + $a[1],
                ( $xstart + $iw - 1 > $a[2]) ? $a[2] : $xstart + $iw - 1,
                $a[3],
             );
             $xstart += $iw + 1;
          }
       }
       $canvas-> clear( @a[0..2], $a[1] + $self->{yedge} - 1)
          if $self->{yedge};
       my $c = $canvas-> color;
       $canvas-> color( $self-> {gridColor});
       for ( $i = 1; $i < $self->{columns}; $i++)
       {
          $canvas-> line( $a[0] + $i * ( $iw + 1) - 1, $a[1],
                          $a[0] + $i * ( $iw + 1) - 1, $a[3]);
       }
       $canvas-> color( $c);
   }
   my $focusedState = $self-> focused ? ( exists $self->{unfocState} ? 0 : 1) : 0;
   $self-> {unfocVeil} = ( $focusedState && $self-> {focusedItem} < 0 && $locWidth > 0) ? 1 : 0;
   my $foci = $self-> {focusedItem};
   if ( $self->{count} > 0 && $locWidth > 0)
   {
      $canvas-> clipRect( @a);
      my $singlePaint = exists $self->{singlePaint};
      my @paintArray;
      my $rows = $self->{rows} ? $self->{rows} : 1;
      if ( $singlePaint)
      {
         for ( keys %{$self->{singlePaint}} )
         {
            my $item = $_;
            next if $item > $self->{lastItem} || $item < 0;
            my @itemRect = $self-> item2rect( $item, @size);
            next if $itemRect[3] < $invalidRect[1] ||
                    $itemRect[1] > $invalidRect[3] ||
                    $itemRect[2] < $invalidRect[0] ||
                    $itemRect[0] > $invalidRect[2];
            my $sel = $self->{multiSelect} ?
               exists $self->{selectedItems}->{$item} :
               (( $foci == $item) ? 1 : 0);
            my $foc = ( $foci == $item) ? $focusedState : 0;
            $foc = 1 if $item == 0 && $self-> {unfocVeil};
            push( @paintArray, [
              $item,
              $itemRect[0] - $self->{offset}, $itemRect[1],
              $itemRect[2]-1, $itemRect[3]-1,
              $sel, $foc,
              int(( $item - $self->{topItem}) / $rows),
            ]);
         }
      } else {
         my $item = $self->{topItem};
         if ( $self->{multiColumn})
         {
            my $rows = $self->{rows} ? $self->{rows} : 1;
            MAIN:for ( $j = 0; $j < $self->{activeColumns}; $j++)
            {
               for ( $i = 0; $i < $rows; $i++)
               {
                  last MAIN if $item > $self->{lastItem};
                  my @itemRect = (
                      $a[0] + $j * ( $iw + 1),
                      $a[3] - $ih * ( $i + 1) + 1,
                      $a[0] + $j * ( $iw + 1) + $iw,
                      $a[3] - $ih * ( $i + 1) + $ih + 1
                  );
                  $item++, next if $itemRect[3] < $invalidRect[1] ||
                                   $itemRect[1] > $invalidRect[3] ||
                                   $itemRect[2] < $invalidRect[0] ||
                                   $itemRect[0] > $invalidRect[2];
                  my $sel = $self->{multiSelect} ?
                  exists $self->{selectedItems}->{$item} :
                  (( $self->{focusedItem} == $item) ? 1 : 0);
                  my $foc = ( $foci == $item) ? $focusedState : 0;
                  $foc = 1 if $item == 0 && $self-> {unfocVeil};
                  push( @paintArray, [
                    $item,                                              # item number
                    $itemRect[0], $itemRect[1],
                    $itemRect[2]-1, $itemRect[3]-1,
                    $sel, $foc, # selected and focused state
                    $j                                                   # column
                  ]);
                  $item++;
               }
            }
         } else {
            for ( $i = 0; $i < $self->{rows} + $self-> {tailVisible}; $i++)
            {
               last if $item > $self->{lastItem};
               my @itemRect = (
                  $a[0], $a[3] - $ih * ( $i + 1) + 1,
                  $a[2], $a[3] - $ih * $i
               );
               $item++, next if ( $itemRect[3] < $invalidRect[1] || $itemRect[1] > $invalidRect[3]);
               my $sel = $self->{multiSelect} ?
                 exists $self->{selectedItems}->{$item} :
                 (( $foci == $item) ? 1 : 0);
               my $foc = ( $foci == $item) ? $focusedState : 0;
               $foc = 1 if $item == 0 && $self-> {unfocVeil};
               push( @paintArray, [
                  $item,                                               # item number
                  $itemRect[0] - $self->{offset}, $itemRect[1],        # logic rect
                  $itemRect[2], $itemRect[3],                          #
                  $sel, $foc, # selected and focused state
                  0 #column
               ]);
               $item++;
            }
         }
      }
      $self-> draw_items( $canvas, @paintArray);
   }
   delete $self->{singlePaint};
}

sub is_default_selection
{
   return $_[0]-> {unfocVeil};
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }
sub on_enter   { $_[0]-> repaint; }

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   $mod &= ( km::Shift|km::Ctrl|km::Alt);
   $self->notify(q(MouseUp),0,0,0) if defined $self->{mouseTransaction};
   if ( $mod & km::Ctrl && $self->{multiSelect})
   {
      my $c = chr ( $code & 0xFF);
      if ( $c eq '/' || $c eq '\\')
      {
         $self-> selectedItems(( $c eq '/') ? [0..$self->{count}-1] : []);
         $self-> clear_event;
         return;
      }
   }
   return if ( $code & 0xFF) && ( $key == kb::NoKey);

   if ( scalar grep { $key == $_ } (kb::Left,kb::Right,kb::Up,kb::Down,kb::Home,kb::End,kb::PgUp,kb::PgDn))
   {
      my $newItem = $self->{focusedItem};
      my $doSelect = 0;
      if ( $mod == 0 || ( $mod & km::Shift && $self->{ extendedSelect}))
      {
         my $pgStep  = $self->{rows} - 1;
         $pgStep = 1 if $pgStep <= 0;
         my $cols = $self->{multiColumn} ? $self->{columns} - $self->{xTailVisible} : 1;
         my $mc = $self->{multiColumn};
         if ( $key == kb::Up)   { $newItem--; };
         if ( $key == kb::Down) { $newItem++; };
         if ( $key == kb::Left) { $newItem -= $self->{rows} if $self->{multiColumn}};
         if ( $key == kb::Right){ $newItem += $self->{rows} if $self->{multiColumn} };
         if ( $key == kb::Home) { $newItem = $self->{topItem} };
         if ( $key == kb::End)  { $newItem = $mc ?
           $self->{topItem} + $self->{rows} * $cols - 1
           : $self->{topItem} + $pgStep; };
         if ( $key == kb::PgDn) { $newItem += $mc ? $self->{rows} * $cols : $pgStep };
         if ( $key == kb::PgUp) { $newItem -= $mc ? $self->{rows} * $cols : $pgStep};
         $doSelect = $mod & km::Shift;
      }
      if ( $mod & km::Ctrl ||
         (( $mod & ( km::Shift|km::Ctrl)==(km::Shift|km::Ctrl)) && $self->{ extendedSelect}))
      {
         if ( $key == kb::PgUp || $key == kb::Home) { $newItem = 0};
         if ( $key == kb::PgDn || $key == kb::End)  { $newItem = $self->{count} - 1};
         $doSelect = $mod & km::Shift;
      }
      if ( $doSelect )
      {
         my ( $a, $b) = ( defined $self->{anchor} ? $self->{anchor} : $self->{focusedItem}, $newItem);
         ( $a, $b) = ( $b, $a) if $a > $b;
         $self-> selectedItems([$a..$b]);
         $self->{anchor} = $self->{focusedItem} unless defined $self->{anchor};
      } else {
         $self-> selectedItems([$self-> focusedItem]) if exists $self->{anchor};
         delete $self->{anchor};
      }
      $self-> offset( $self->{offset} + 5 * (( $key == kb::Left) ? -1 : 1))
         if !$self->{multiColumn} && ($key == kb::Left || $key == kb::Right);
      $self-> focusedItem( $newItem >= 0 ? $newItem : 0);
      $self-> clear_event;
      return;
   } else {
      delete $self->{anchor};
   }

   if ( $mod == 0 && ( $key == kb::Space || $key == kb::Enter))
   {
      $self-> toggle_item( $self->{focusedItem}) if $key == kb::Space &&
          $self->{multiSelect} && !$self->{extendedSelect};
      $self-> clear_event;
      $self-> notify(q(Click)) if $key == kb::Enter && ($self->focusedItem >= 0);
      return;
   }
}

sub on_leave
{
   my $self = $_[0];
   if ( $self->{mouseTransaction})
   {
      $self-> capture(0) if $self->{mouseTransaction};
      $self->{mouseTransaction} = undef;
   }
   $self-> repaint;
}


sub point2item
{
   my ( $self, $x, $y) = @_;
   my ( $ih, $iw, @a) = ( $self-> {itemHeight}, $self->{itemWidth}, $self-> get_active_area);

   if ( $self->{multiColumn})
   {
      my ( $r, $t, $l, $c) = ( $self->{rows}, $self->{topItem}, $self->{lastItem}, $self->{columns});
      $c-- if $self->{multiColumn} && $self->{xTailVisible};
      $x -= $a[0];                          # a[2]???
      $y -= $a[1] + $self->{yedge};
      $x /= $iw + 1;
      $y /= $ih;
      $y = $r - $y;
      $x = int( $x - (( $x < 0) ? 1 : 0));
      $y = int( $y - (( $y < 0) ? 1 : 0));
      return $t - $r                if $y < 0   && $x < 1;
      return $t + $r * $x,  -1      if $y < 0   && $x >= 0 && $x < $c;
      return $t + $r * $c           if $y < 0   && $x >= $c;
      return $l + $y + 1 - (($c && $self->{xTailVisible} && ( $l < $self->{count}-1))?$r:0), $self->{activeColumns} <= $self->{columns} - $self->{xTailVisible} ? 0 :$r
         if $x > $c && $y >= 0 && $y < $r;
      return $t + $y - $r           if $x < 0   && $y >= 0 && $y < $r;
      return $l + $r                if $x >= $c - 1 && $y >= $r;
      return $t + $r * ($x + 1)-1,1 if $y >= $r && $x >= 0 && $x < $c;
      return $t + $r - 1            if $x < 0   && $y >= $r;
      return $x * $r + $y + $t;
   } else {
      return $self->{topItem} - 1 if $y >= $a[3];
      return $self->{lastItem} + !$self->{tailVisible} if $y <= $a[1];
      my $h = $a[3];

      my $i = $self->{topItem};
      while ( $y > 0)
      {
         return $i if $y <= $h && $y > $h - $ih;
         $h -= $ih;
         $i++;
      }
   }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   my $bw = $self-> { borderWidth};
   $self-> clear_event;
   return if $btn != mb::Left;
   my @a = $self-> get_active_area;
   return if defined $self->{mouseTransaction} ||
      $y < $a[1] || $y >= $a[3] ||
      $x < $a[0] || $x >= $a[2];

   my $item = $self-> point2item( $x, $y);
   if (( $self->{multiSelect} && !$self->{extendedSelect}) ||
       ( $self->{extendedSelect} && ( $mod & (km::Ctrl|km::Shift))))
   {
      $self-> toggle_item( $item);
      return if $self->{extendedSelect};
   } else {
      $self-> {anchor} = $item if $self->{extendedSelect};
   }
   $self-> {mouseTransaction} = 1;
   $self-> focusedItem( $item >= 0 ? $item : 0);
   $self-> selectedItems([$self->{focusedItem}]) if $self->{extendedSelect};
   $self-> capture(1);
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   $self-> clear_event;
   return if $btn != mb::Left || !$dbl;
   $self-> notify(q(Click)) if $self-> focusedItem >= 0;
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless defined $self->{mouseTransaction};
   my $bw = $self-> { borderWidth};
   my ($item, $aux) = $self-> point2item( $x, $y);
   my @a = $self-> get_active_area;
   if ( $y >= $a[3] || $y < $a[1] || $x >= $a[2] || $x < $a[0])
   {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self->scroll_timer_semaphore;
      $self->scroll_timer_semaphore(0);
   } else {
      $self-> scroll_timer_stop;
   }

   if ( $aux)
   {
      my $top = $self-> {topItem};
      $self-> {unfocState} = 1;
      $self-> {singlePaint}->{$self->{focusedItem}} = 1;
      $self-> refresh;
      $self-> topItem( $self-> {topItem} + $aux);
      delete $self-> {unfocState};
      $item += (( $top != $self-> {topItem}) ? $aux : 0);
   }

   if ( $self-> {extendedSelect} && exists $self->{anchor})
   {
       my ( $a, $b, $c) = ( $self->{anchor}, $item, $self->{focusedItem});
       my $globSelect = 0;
       if (( $b <= $a && $c > $a) || ( $b >= $a && $c < $a)) { $globSelect = 1
       } elsif ( $b > $a) {
          if ( $c < $b) { $self-> add_selection([$c + 1..$b], 1) }
          elsif ( $c > $b) { $self-> add_selection([$b + 1..$c], 0) }
          else { $globSelect = 1 }
       } elsif ( $b < $a) {
          if ( $c < $b) { $self-> add_selection([$c..$b], 0) }
          elsif ( $c > $b) { $self-> add_selection([$b..$c], 1) }
          else { $globSelect = 1 }
       } else { $globSelect = 1 }
       if ( $globSelect )
       {
          ( $a, $b) = ( $b, $a) if $a > $b;
          $self-> selectedItems([$a..$b]);
       }
   }
   $self-> focusedItem( $item >= 0 ? $item : 0);
   $self-> offset( $self->{offset} + 5 * (( $x < $a[0]) ? -1 : 1)) if $x >= $a[2] || $x < $a[0];
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless defined $self->{mouseTransaction};
   delete $self->{mouseTransaction};
   delete $self->{mouseHorizontal};
   delete $self->{anchor};
   $self-> capture(0);
   $self-> clear_event;
}

sub on_mousewheel
{
   my ( $self, $mod, $x, $y, $z) = @_;
   $z = int( $z/120);
   $z *= $self-> {rows} if $mod & km::Ctrl;
   my $newTop = $self-> topItem - $z;
   my $cols = $self->{multiColumn} ? $self->{columns} - $self->{xTailVisible} : 1;
   my $maxTop = $self-> {count} - $self-> {rows} * $cols;
   $self-> topItem( $newTop > $maxTop ? $maxTop : $newTop);
}

sub on_size
{
   my $self = $_[0];
   $self-> offset( $self-> offset) if $self->{multiColumn};
   $self-> reset;
   $self-> reset_scrolls;
}

sub reset
{
   my $self = $_[0];
   my @a    = $self-> indents;
   my @size = $self-> get_active_area( 2);
   my $ih   = $self-> {itemHeight};
   my $iw   = $self-> {itemWidth};
   $self->{rows}  = int( $size[1]/$ih);
   $self->{rows}  = 0 if $self->{rows} < 0;
   $self->{yedge} = $size[1] - $self->{rows} * $ih;
   if ( $self->{multiColumn})
   {
      $self->{tailVisible} = 0;
      $self->{columns}     = 0;
      my $w = 0;
      $self->{lastItem}      = $self->{topItem};
      $self->{activeColumns} = 0;
      my $top = $self->{topItem};
      my $max = $self->{count} - 1;
      $self->{uncover} = undef;
      if ( $self->{rows} == 0)
      {
         $self->{activeColumns}++, $self->{columns}++, $w += $iw + 1 while ( $w < $size[0]);
      } else {
         while ( $w <= $size[0])
         {
             $self->{columns}++;
             if ( $top + $self->{rows} - 1 < $max)
             {
                $self->{activeColumns}++;
                $self->{lastItem} = $top + $self->{rows} - 1;
             } elsif ( $top + $self->{rows} - 1 == $max) {
                $self->{activeColumns}++;
                $self->{lastItem} = $max;
             } elsif ( $top <= $max) {
                $self->{lastItem} = $max;
                $self->{activeColumns}++;
                $self->{uncover} = $ih * ( $self->{rows} - $max + $top - 1) + $a[1];
             }
             $w   += $iw + 1;
             $top += $self->{rows};
         }
      }
      $self->{xTailVisible} = $size[0] + 1 < $self->{columns} * ( $iw + 1);
   } else {
      $self->{columns}     = 1;
      my ($x,$z) = ( $self->{count} - 1, $self->{topItem} + $self->{rows} - 1);
      $self->{lastItem} = $x > $z ? $z : $x;
      $self->{uncover} = ( $self->{count} == 0) ? $size[1] :
                         $size[1] - ( $self->{lastItem} - $self->{topItem} + 1) * $ih;
      $self->{uncover} += $a[1];
      $self->{tailVisible} = 0;
      my $integralHeight = ( $self->{integralHeight} && ( $self-> {rows} > 0)) ? 1 : 0;
      if ( $self->{count} > 0 && $self->{lastItem} < $self->{count}-1 && !$integralHeight && $self->{yedge} > 0)
      {
         $self->{tailVisible} = 1;
         $self->{lastItem}++;
         $self->{uncover} = undef;
      }
      $self->{uncover} = undef if $size[0] <= 0 || $size[1] <= 0;
   }
}

sub reset_scrolls
{
   my $self = $_[0];
   if ( $self-> {scrollTransaction} != 1 && $self->{vScroll})
   {
      $self-> {vScrollBar}-> set(
         max      => $self-> {count} -  $self->{rows} *
         ( $self-> {multiColumn} ?
           ( $self->{columns} - $self->{xTailVisible}) : 1),
         pageStep => $self-> {rows},
         whole    => $self-> {count},
         partial  => $self-> {columns} * $self->{rows},
         value    => $self-> {topItem},
      );
   }
   if ( $self->{scrollTransaction} != 2 && $self->{hScroll})
   {
      if ( $self->{multiColumn})
      {
         $self-> {hScrollBar}-> set(
            max      => $self-> {count} - $self->{rows} * ( $self->{columns} - $self->{xTailVisible}),
            step     => $self-> {rows},
            pageStep => $self-> {rows} * $self-> {columns},
            whole    => $self-> {count},
            partial  => $self-> {columns} * $self->{rows},
            value    => $self-> {topItem},
         );
      } else {
         my @sz = $self-> get_active_area( 2);
         my $iw = $self->{itemWidth};
         $self-> {hScrollBar}-> set(
            max      => $iw - $sz[0],
            whole    => $iw,
            value    => $self-> {offset},
            partial  => $sz[0],
            pageStep => $iw / 5,
         );
      }
   }
}

sub select_all {
   my $self = $_[0];
   $self-> selectedItems([0..$self->{count}-1]);
}

sub deselect_all {
   my $self = $_[0];
   $self-> selectedItems([]);
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


sub set_count
{
   my ( $self, $count) = @_;
   $count = 0 if $count < 0;
   my $oldCount = $self->{count};
   $self-> { count} = $count;
   my $doFoc = undef;
   if ( $oldCount > $count)
   {
      for ( keys %{$self->{selectedItems}})
      {
         delete $self->{selectedItems}->{$_} if $_ >= $count;
      }
   }
   $self-> reset;
   $self-> reset_scrolls;
   $self-> focusedItem( -1) if $self->{focusedItem} >= $count;
   $self-> refresh;
}

sub set_extended_select
{
   my ( $self, $esel) = @_;
   if ( $self-> {extendedSelect} = $esel)
   {
      $self-> multiSelect( 1);
      $self-> selectedItems([$self-> {focusedItem}]);
   }
}

sub set_focused_item
{
   my ( $self, $foc) = @_;
   my $oldFoc = $self->{focusedItem};
   $foc = $self->{count} - 1 if $foc >= $self->{count};
   $foc = -1 if $foc < -1;
   return if $self->{focusedItem} == $foc;
   return if $foc < -1;
   $self->{focusedItem} = $foc;
   $self-> notify(q(SelectItem), [ $foc], 1) if $foc >= 0;
   $self-> selectedItems([$foc]) if $self->{extendedSelect} && ! exists $self->{anchor};
   my $topSet = undef;
   if ( $foc >= 0)
   {
      my $rows = $self->{rows} ? $self->{rows} : 1;
      my $mc   = $self->{multiColumn};
      my $cols = $mc ? $self->{columns} - $self->{xTailVisible} : 1;
      $cols++ unless $cols;
      if ( $foc < $self->{topItem}) {
         $topSet = $mc ? $foc - $foc % $rows : $foc;
      } elsif ( $foc >= $self->{topItem} + $rows * $cols) {
         $topSet = $mc ? $foc - $foc % $rows - $rows * ( $cols - 1) : $foc - $rows + 1;
      }
   }
   $self-> {singlePaint}->{$oldFoc} = 1;
   if ( defined $topSet)
   {
      $self-> {syncScroll} = 1 if abs( $foc - $oldFoc) == 1 && !$self->{multiColumn};
      $self-> {unfocState} = 1;
      $self-> refresh;
      delete $self-> {unfocState};
      $self-> topItem( $topSet);
      delete $self-> {syncScroll};
   }
   $self-> {singlePaint}->{$foc} = 1;
   delete $self-> {singlePaint} unless $self->{rows};
   delete $self-> {singlePaint} if $self-> {unfocVeil};
   $self-> refresh;
}

sub colorIndex
{
   my ( $self, $index, $color) = @_;
   return ( $index == ci::Grid) ?
      $self-> {gridColor} : $self-> SUPER::colorIndex( $index)
        if $#_ < 2;
   ( $index == ci::Grid) ?
      ( $self-> gridColor( $color), $self-> notify(q(ColorChanged), ci::Grid)) :
      ( $self-> SUPER::colorIndex( $index, $color));
}

sub set_grid_color
{
   my ( $self, $gc) = @_;
   return if $gc == $self->{gridColor};
   $self->{gridColor} = $gc;
   $self-> repaint;
}


sub set_integral_height
{
  my ( $self, $ih) = @_;
  return if $self->{multiColumn} || $self->{ integralHeight} == $ih;
  $self->{ integralHeight} = $ih;
  $self-> reset;
  $self-> reset_scrolls;
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
   $self-> reset_scrolls;
   $self->repaint;
}

sub set_item_width
{
   my ( $self, $iw) = @_;
   $iw = $self-> width - 1 if $iw <= 0;
   $iw = 1 if $iw < 1;
   return if $iw == $self->{itemWidth};
   $self->{itemWidth} = $iw;
   $self->reset;
   $self->reset_scrolls;
   $self->repaint;
}

sub set_multi_column
{
   my ( $self, $mc) = @_;
   return if $mc == $self->{multiColumn};
   $self-> offset(0) if $self->{multiColumn} = $mc;
   $self-> reset;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub set_multi_select
{
   my ( $self, $ms) = @_;
   return if $ms == $self->{multiSelect};
   unless ( $self-> {multiSelect} = $ms)
   {
      $self-> extendedSelect( 0);
      $self-> selectedItems([]);
      $self-> {singlePaint}->{$self-> focusedItem} = 1;
      $self-> repaint;
   } else {
      $self-> selectedItems([$self-> focusedItem]);
   }
}

sub set_offset
{
   my ( $self, $offset) = @_;
   $self->{offset} = 0, return if $self->{multiColumn};
   my ( $iw, @a) = ( $self->{itemWidth}, $self-> get_active_area);
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
   my $dt = $offset - $oldOfs;
   $self-> reset;
   if ( $self->{hScroll} && !$self->{multiColumn} && $self->{scrollTransaction} != 2) {
      $self->{scrollTransaction} = 2;
      $self-> {hScrollBar}-> value( $offset);
      $self->{scrollTransaction} = 0;
   }
   $self-> scroll( -$dt, 0,
                     clipRect => \@a);
   $self-> update_view;
   delete $self-> {singlePaint};
}

sub redraw_items
{
   my $self = shift;
   for ( @_) {
      $self->{singlePaint}->{$_} = 1;
   }
   $self-> refresh;
}

sub set_selected_items
{
   my ( $self, $items) = @_;
   return if !$self->{ multiSelect} && ( scalar @{$items} > 0);
   my $ptr = $::application-> pointer;
   $::application-> pointer( cr::Wait)
      if scalar @{$items} > 500;
   my $sc = $self->{count};
   my %newItems;
   for (@{$items}) { $newItems{$_}=1 if $_>=0 && $_<$sc; }
   my @stateChangers; # $#stateChangers = scalar @{$items};
   my $k;
   while (defined($k = each %{$self->{selectedItems}})) {
      next if exists $newItems{$k};
      push( @stateChangers, $k);
   };
   my @indeces;
   my $sel = $self->{selectedItems};
   $self->{selectedItems} = \%newItems;
   $self-> notify(q(SelectItem), [@stateChangers], 0) if scalar @stateChangers;
   while (defined($k = each %newItems)) {
      next if exists $sel->{$k};
      push( @stateChangers, $k);
      push( @indeces, $k);
   };
   $self-> notify(q(SelectItem), [@indeces], 1) if scalar @indeces;
   $::application-> pointer( $ptr);
   return unless scalar @stateChangers;
   for ( @stateChangers) { $self->{singlePaint}->{$_} = 1; };
   $self-> refresh;
}

sub get_selected_items
{
    return $_[0]->{multiSelect} ?
       [ sort { $a<=>$b } keys %{$_[0]->{selectedItems}}] :
       (
         ( $_[0]->{focusedItem} < 0) ? [] : [$_[0]->{focusedItem}]
       );
}

sub get_selected_count
{
   return scalar keys %{$_[0]->{selectedItems}};
}

sub is_selected
{
   return exists $_[0]->{selectedItems}->{$_[1]};
}

sub set_item_selected
{
   my ( $self, $index, $sel) = @_;
   return unless $self->{multiSelect};
   return if $index < 0 || $index >= $self->{count};
   return if $sel == exists $self->{selectedItems}->{$index};
   $sel ? $self->{selectedItems}->{$index} = 1 : delete $self->{selectedItems}->{$index};
   $self-> notify(q(SelectItem), [ $index], $sel);
   $self-> {singlePaint}->{$index} = 1;
   $self-> refresh;
}

sub select_item   {  $_[0]-> set_item_selected( $_[1], 1); }
sub unselect_item {  $_[0]-> set_item_selected( $_[1], 0); }
sub toggle_item   {  $_[0]-> set_item_selected( $_[1], !$_[0]-> is_selected( $_[1]))}

sub add_selection
{
   my ( $self, $items, $sel) = @_;
   return unless $self->{multiSelect};
   my @notifiers;
   my $count = $self->{count};
   for ( @{$items})
   {
      next if $_ < 0 || $_ >= $count;
      next if exists $self->{selectedItems}->{$_} == $sel;
      $self->{singlePaint}->{$_}  = 1;
      $sel ? $self->{selectedItems}->{$_} = 1 : delete $self->{selectedItems}->{$_};
      push ( @notifiers, $_);
   }
   return unless scalar @notifiers;
   $self-> notify(q(SelectItem), [ @notifiers], $sel) if scalar @notifiers;
   $self-> refresh;
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
   my ( $ih, $iw, @a) = ( $self->{itemHeight}, $self->{itemWidth}, $self-> get_active_area);
   my $dt = $topItem - $oldTop;
   $self-> reset;
   if ( $self->{scrollTransaction} != 1 && $self->{vScroll}) {
      $self->{scrollTransaction} = 1;
      $self-> {vScrollBar}-> value( $topItem);
      $self->{scrollTransaction} = 0;
   }

   if ( $self->{scrollTransaction} != 2 && $self->{hScroll} && $self->{multiColumn}) {
      $self->{scrollTransaction} = 2;
      $self-> {hScrollBar}-> value( $topItem);
      $self->{scrollTransaction} = 0;
   }

   if ( $self->{ multiColumn}) {
      if (( $self->{rows} != 0) && ( $dt % $self->{rows} == 0)) {
         $self-> scroll( -( $dt / $self->{rows}) * ($iw + 1), 0,
                         clipRect => \@a);
      } else {
         $a[1] += $self->{yedge};
         $self-> scroll( 0, $ih * $dt,
                         clipRect => \@a);

      }
   } else {
      $self-> scroll( 0, $dt * $ih,
                      clipRect => \@a);
   }
   $self-> update_view;
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


#sub on_drawitem
#{
#   my ($self, $canvas, $itemIndex, $x, $y, $x2, $y2, $selected, $focused) = @_;
#}

#sub on_selectitem
#{
#   my ($self, $itemIndex, $selectState) = @_;
#}

sub autoHeight    {($#_)?$_[0]->set_auto_height    ($_[1]):return $_[0]->{autoHeight}     }
sub count         {($#_)?$_[0]->set_count          ($_[1]):return $_[0]->{count}          }
sub extendedSelect{($#_)?$_[0]->set_extended_select($_[1]):return $_[0]->{extendedSelect} }
sub gridColor     {($#_)?$_[0]->set_grid_color     ($_[1]):return $_[0]->{gridColor}      }
sub focusedItem   {($#_)?$_[0]->set_focused_item   ($_[1]):return $_[0]->{focusedItem}    }
sub integralHeight{($#_)?$_[0]->set_integral_height($_[1]):return $_[0]->{integralHeight} }
sub itemHeight    {($#_)?$_[0]->set_item_height    ($_[1]):return $_[0]->{itemHeight}     }
sub itemWidth     {($#_)?$_[0]->set_item_width     ($_[1]):return $_[0]->{itemWidth}      }
sub multiSelect   {($#_)?$_[0]->set_multi_select   ($_[1]):return $_[0]->{multiSelect}    }
sub multiColumn   {($#_)?$_[0]->set_multi_column   ($_[1]):return $_[0]->{multiColumn}    }
sub offset        {($#_)?$_[0]->set_offset         ($_[1]):return $_[0]->{offset}         }
sub selectedCount {($#_)?$_[0]->raise_ro("selectedCount") :return $_[0]->get_selected_count;}
sub selectedItems {($#_)?shift->set_selected_items    (@_):return $_[0]->get_selected_items;}
sub topItem       {($#_)?$_[0]->set_top_item       ($_[1]):return $_[0]->{topItem}        }

package Prima::ListViewer;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractListViewer);

{
my %RNT = (
   %{Prima::AbstractListViewer->notification_types()},
   Stringify   => nt::Action,
   MeasureItem => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
       items         => [],
       autoWidth     => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   $self->{items}      = [];
   $self->{widths}     = [];
   $self->{maxWidth}   = 0;
   $self->{autoWidth}  = 0;
   my %profile = $self-> SUPER::init(@_);
   $self->autoWidth( $profile{autoWidth});
   $self->items    ( $profile{items});
   $self->focusedItem  ( $profile{focusedItem});
   return %profile;
}


sub calibrate
{
   my $self = $_[0];
   $self-> recalc_widths;
   $self-> itemWidth( $self-> {maxWidth}) if $self->{autoWidth};
   $self-> offset( $self-> offset);
}

sub get_item_text
{
   my ( $self, $index) = @_;
   my $txt = '';
   $self-> notify(q(Stringify), $index, \$txt);
   return $txt;
}

sub get_item_width
{
   return $_[0]->{widths}->[$_[1]];
}

sub on_stringify
{
   my ( $self, $index, $sref) = @_;
   $$sref = '';
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   $$sref = 0;
}

sub on_fontchanged
{
   my $self = $_[0];
   $self-> itemHeight( $self-> font-> height), $self->{autoHeight} = 1 if $self-> { autoHeight};
   $self-> calibrate;
}


sub recalc_widths
{
   my $self = $_[0];
   my @w;
   my $maxWidth = 0;
   my $i;
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
   $self-> push_event;
   $self-> begin_paint_info;
   for ( $i = 0; $i < scalar @{$self->{items}}; $i++)
   {
      my $iw = 0;
      $notifier->( @notifyParms, $i, \$iw);
      $maxWidth = $iw if $maxWidth < $iw;
      push ( @w, $iw);
   }
   $self-> end_paint_info;
   $self-> pop_event;
   $self->{widths}    = [@w];
   $self-> {maxWidth} = $maxWidth;
}

sub set_items
{
   my ( $self, $items) = @_;
   return unless ref $items eq q(ARRAY);
   my $oldCount = $self-> {count};
   $self->{items} = [@{$items}];
   $self-> recalc_widths;
   $self-> reset;
   scalar @$items == $oldCount ? $self->refresh : $self-> SUPER::count( scalar @$items);
   $self-> itemWidth( $self-> {maxWidth}) if $self->{autoWidth};
   $self-> offset( $self-> offset);
   $self-> selectedItems([]);
}

sub get_items
{
   my $self = shift;
   my @inds = (@_ == 1 and ref($_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;
   my ($c,$i) = ($self->{count}, $self->{items});
   for ( @inds) { $_ = ( $_ >= 0 && $_ < $c) ? $i->[$_] : undef; }
   return wantarray ? @inds : $inds[0];
}

sub insert_items
{
   my ( $self, $where) = ( shift, shift);
   $where = $self-> {count} if $where < 0;
   my ( $is, $iw, $mw) = ( $self-> {items}, $self-> {widths}, $self->{maxWidth});
   if (@_ == 1 and ref($_[0]) eq q(ARRAY)) {
      return unless scalar @{$_[0]};
      $self->{items} = [@{$_[0]}];
   } else {
      return unless scalar @_;
      $self->{items} = [@_];
   }
   $self->{widths} = [];
   my $num = scalar @{$self->{items}};
   $self-> recalc_widths;
   splice( @{$is}, $where, 0, @{$self->{items}});
   splice( @{$iw}, $where, 0, @{$self->{widths}});
   ( $self->{items}, $self->{widths}) = ( $is, $iw);
   $self-> itemWidth( $self->{maxWidth} = $mw)
     if $self->{autoWidth} && $self->{maxWidth} < $mw;
   $self-> SUPER::count( scalar @{$self->{items}});
   $self-> itemWidth( $self-> {maxWidth}) if $self->{autoWidth};
   $self-> focusedItem( $self->{focusedItem} + $num)
      if $self->{focusedItem} >= 0 && $self->{focusedItem} >= $where;
   $self-> offset( $self-> offset);

   my @shifters;
   for ( keys %{$self->{selectedItems}})
   {
       next if $_ < $where;
       push ( @shifters, $_);
   }
   for ( @shifters) { delete $self->{selectedItems}->{$_};     }
   for ( @shifters) { $self->{selectedItems}->{$_ + $num} = 1; }
   $self-> refresh if scalar @shifters;
}

sub add_items { shift-> insert_items( -1, @_); }

sub delete_items
{
   my $self = shift;
   my ( $is, $iw, $mw) = ( $self-> {items}, $self-> {widths}, $self->{maxWidth});
   my %indeces;
   if (@_ == 1 and ref($_[0]) eq q(ARRAY)) {
      return unless scalar @{$_[0]};
      %indeces = map{$_=>1}@{$_[0]};
   } else {
      return unless scalar @_;
      %indeces = map{$_=>1}@_;
   }
   my @removed;
   my $wantarray = wantarray;
   my @newItems;
   my @newWidths;
   my $i;
   my $num = scalar keys %indeces;
   my ( $items, $widths) = ( $self->{items}, $self-> {widths});
   $self-> focusedItem( -1) if exists $indeces{$self->{focusedItem}};
   for ( $i = 0; $i < scalar @{$self->{items}}; $i++)
   {
      unless ( exists $indeces{$i})
      {
         push ( @newItems,  $$items[$i]);
         push ( @newWidths, $$widths[$i]);
      } else {
         push ( @removed, $$items[$i]) if $wantarray;
      }
   }
   my $newFoc = $self->{focusedItem};
   for ( keys %indeces) { $newFoc-- if $newFoc >= 0 && $_ < $newFoc; }

   my @selected = sort {$a<=>$b} keys %{$self->{selectedItems}};
   $i = 0;
   my $dec = 0;
   my $d;
   for $d ( sort {$a<=>$b} keys %indeces)
   {
      while ($i < scalar(@selected) and $d > $selected[$i]) { $selected[$i] -= $dec; $i++; }
      last if $i >= scalar @selected;
      $selected[$i++] = -1 if $d == $selected[$i];
      $dec++;
   }
   while ($i < scalar(@selected)) { $selected[$i] -= $dec; $i++; }
   $self->{selectedItems} = {};
   for ( @selected) {$self->{selectedItems}->{$_} = 1;}
   delete $self->{selectedItems}->{-1};

   ( $self->{items}, $self-> {widths}) = ([@newItems], [@newWidths]);
   my $maxWidth = 0;
   for ( @newWidths) { $maxWidth = $_ if $maxWidth < $_; }
   $self-> lock;
   $self-> itemWidth( $self->{maxWidth} = $maxWidth)
     if $self->{autoWidth} && $self->{maxWidth} > $maxWidth;
   $self-> SUPER::count( scalar @{$self->{items}});
   $self-> focusedItem( $newFoc);
   $self-> unlock;
   return @removed if $wantarray;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   $self->notify(q(MouseUp),0,0,0) if defined $self->{mouseTransaction};
   if ( $code & 0xFF && $key == kb::NoKey && !($mod & ~km::Shift) && $self->{count})
   {
      my $i;
      my ( $c, $hit, $items) = ( lc chr ( $code & 0xFF), undef, $self->{items});
      for ( $i = $self->{focusedItem} + 1; $i < $self->{count}; $i++)
      {
         my $fc = substr( $self-> get_item_text($i), 0, 1);
         next unless defined $fc;
         $hit = $i, last if lc $fc eq $c;
      }
      unless ( defined $hit) {
         for ( $i = 0; $i < $self->{focusedItem}; $i++)
         {
            my $fc = substr( $self-> get_item_text($i), 0, 1);
            next unless defined $fc;
            $hit = $i, last if lc $fc eq $c;
         }
      }
      if ( defined $hit)
      {
         $self-> focusedItem( $hit);
         $self-> clear_event;
         return;
      }
   }
   $self-> SUPER::on_keydown( $code, $key, $mod);
}

sub autoWidth     {($#_)?$_[0]->{autoWidth} = $_[1]       :return $_[0]->{autoWidth}      }
sub count         {($#_)?$_[0]->raise_ro('count')         :return $_[0]->{count}          }
sub items         {($#_)?$_[0]->set_items( $_[1])         :return $_[0]->{items}          }

package Prima::ProtectedListBox;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);

BEGIN {
   for ( qw(font color backColor rop rop2 linePattern lineWidth lineEnd textOutBaseline fillPattern clipRect))
   {
      my $sb = $_;
      $sb =~ s/([A-Z]+)/"_\L$1"/eg;
      $sb = "set_$sb";
      eval <<PROC;
      sub $sb
      {
         my \$self = shift;
         \$self->SUPER::$sb(\@_);
         \$self->{protect}->{$_} = 1 if exists \$self->{protect};
      }
PROC
   }
}

sub draw_items
{
   my ( $self, $canvas, @items) = @_;
   return if $canvas != $self;   # this does not support 'uncertain' drawings due that
   my %protect;                  # it's impossible to override $canvas's methods dynamically
   for ( qw(font color backColor rop rop2 linePattern lineWidth lineEnd textOutBaseline fillPattern))
        { $protect{$_} = $canvas-> $_(); }
   my @clipRect = $canvas-> clipRect;
   $self->{protect} = {};

   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
   $self-> push_event;
   for ( @items)
   {
      $notifier->( @notifyParms, $canvas, @$_);

      $canvas-> clipRect( @clipRect), delete $self->{protect}->{clipRect}
         if exists $self->{protect}->{clipRect};
      for ( keys %{$self->{protect}}) { $self->$_($protect{$_}); }
      $self->{protect} = {};
   }
   $self-> pop_event;
   delete $self->{protect};
}


package Prima::ListBox;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);

sub get_item_text  { return $_[0]->{items}->[$_[1]]; }

sub on_stringify
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self->{items}->[$index];
}

sub on_measureitem
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self-> get_text_width( $self->{items}->[$index]);
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

   my @colContainer;
   for ( $i = 0; $i < $self->{columns}; $i++){ push ( @colContainer, [])};
   for ( $i = 0; $i < scalar @_; $i++) {
      push ( @{$colContainer[ $_[$i]->[7]]}, $_[$i]);
      $drawVeilFoc = $i if $_[$i]->[6];
   }
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
      my ( $lc, $lbc) = @clrs[0,1];
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
             $canvas-> text_out( $self->{items}->[$i], $x,
                $y2 + $atY - ($i-$first+1) * $ih + 1);
         }
      }
   }
   # draw veil
   if ( $drawVeilFoc >= 0)
   {
      my ( $itemIndex, $x, $y, $x2, $y2) = @{$_[$drawVeilFoc]};
      $canvas-> rect_focus( $x + $self->{offset}, $y, $x2, $y2);
   }
}

1;
