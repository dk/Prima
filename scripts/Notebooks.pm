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
package Notebook;

package TabSet;
use vars qw(@ISA);
@ISA = qw(Widget MouseScroller);

use strict;
use Const;
use Classes;

{
my %RNT = (
   %{Widget->notification_types()},
   DrawTab     => nt::Action,
   MeasureTab  => nt::Action,
);

sub notification_types { return \%RNT; }
}

use constant DefGapX   => 10;
use constant DefGapY   => 5;
use constant DefLeftX  => 5;
use constant DefArrowX => 25;

my @warpColors = (
   0x50d8f8, 0x80d8a8, 0x8090f8, 0xd0b4a8, 0xf8fca8,
   0xa890a8, 0xf89050, 0xf8d850, 0xf8b4a8, 0xf8d8a8,
);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my $font = $_[ 0]-> get_default_font;
   my %prf = (
      colored          => 1,
      firstTab         => 0,
      focusedTab       => 0,
      height           => $font-> { height} > 14 ? $font-> { height} * 2 : 28,
      ownerBackColor   => 1,
      selectable       => 1,
      selectingButtons => 0,
      tabStop          => 1,
      topMost          => 1,
      tabIndex         => 0,
      tabs             => [],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub init
{
   my $self = shift;
   $self->{tabIndex} = -1;
   for ( qw( colored firstTab focusedTab topMost lastTab arrows)) { $self->{$_} = 0; }
   $self->{tabs}     = [];
   $self->{widths}   = [];
   my %profile = $self-> SUPER::init(@_);
   $self-> {__DYNAS__}->{onMeasureTab} = $profile{onMeasureTab};
   $self-> {__DYNAS__}->{onDrawTab}    = $profile{onDrawTab};
   for ( qw( colored topMost tabs focusedTab firstTab tabIndex)) { $self->$_( $profile{ $_}); }
   $self-> recalc_widths;
   $self-> reset;
   return %profile;
}


sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onMeasureTab} = $profile{onMeasureTab},
      delete $profile{onMeasureTab} if exists $profile{onMeasureTab};
   $self->{__DYNAS__}->{onDrawTab} = $profile{onDrawTab},
      delete $profile{onDrawTab} if exists $profile{onDrawTab};
   $self-> SUPER::set( %profile);
}

sub reset
{
   my $self = $_[0];
   my @size = $self-> size;
   my $w = DefLeftX * 2 + DefGapX;
   for ( @{$self->{widths}}) { $w += $_ + DefGapX; }
   $self->{arrows} = (( $w > $size[0]) and ( scalar( @{$self->{widths}}) > 1));
   if ( $self->{arrows}) {
      my $ft = $self->{firstTab};
      $w  = DefLeftX * 2 + DefGapX;
      $w += DefArrowX if $ft > 0;
      my $w2 = $w;
      my $la = $ft > 0;
      my $i;
      my $ra = 0;
      my $ww = $self->{widths};
      for ( $i = $ft; $i < scalar @{$ww}; $i++) {
         $w += DefGapX + $$ww[$i];
         if ( $w + DefGapX + DefLeftX >= $size[0]) {
            $ra = 1;
            $i-- if $i > $ft && $w - $$ww[$i] >= $size[0] - DefLeftX - DefArrowX - DefGapX;
            last;
         }
      }
      $i = scalar @{$self->{widths}} - 1 if $i >= scalar @{$self->{widths}};
      $self->{lastTab} = $i;
      $self->{arrows} = ( $la ? 1 : 0) | ( $ra ? 2 : 0);
   } else {
      $self->{lastTab} = scalar @{$self->{widths}} - 1;
   }
}

sub recalc_widths
{
   my $self = $_[0];
   my @w = ();
   my $i;
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureTab));
   $self-> push_event;
   for ( $i = 0; $i < scalar @{$self->{tabs}}; $i++)
   {
      my $iw = 0;
      $notifier->( @notifyParms, $i, \$iw);
      push ( @w, $iw);
   }
   $self-> pop_event;
   $self-> {widths}    = [@w];
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction};
   $self-> clear_event;
   my ( $a, $ww, $ft, $lt) = (
      $self->{arrows}, $self->{widths}, $self->{firstTab}, $self->{lastTab}
   );
   if (( $a & 1) and ( $x < DefLeftX + DefGapX * 2 + DefArrowX)) {
      $self-> firstTab( $self-> firstTab - 1);
      $self-> capture(1);
      $self->{mouseTransaction} = -1;
      $self-> scroll_timer_start;
      $self-> scroll_timer_semaphore(0);
      return;
   }
   my @size = $self-> size;
   if (( $a & 2) and ( $x >= $size[0] - DefLeftX - DefGapX * 2 - DefArrowX)) {
      $self-> firstTab( $self-> firstTab + 1);
      $self-> capture(1);
      $self->{mouseTransaction} = 1;
      $self-> scroll_timer_start;
      $self-> scroll_timer_semaphore(0);
      return;
   }
   my $w = DefLeftX;
   $w += DefGapX + DefArrowX if $a & 1;
   my $i;
   my $found = undef;
   for ( $i = $ft; $i <= $lt; $i++) {
      $found = $i, last if $x < $w + $$ww[$i] + DefGapX;
      $w += $$ww[$i] + DefGapX;
   }
   return unless defined $found;
   if ( $found == $self-> {tabIndex}) {
      $self-> focusedTab( $found);
      $self-> focus;
   } else {
      $self-> tabIndex( $found);
   }
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   $self-> capture(0);
   $self-> scroll_timer_stop;
   $self->{mouseTransaction} = undef;
}


sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   return unless $self->scroll_timer_semaphore;
   $self->scroll_timer_semaphore(0);
   my $ft = $self-> firstTab;
   $self-> firstTab( $ft + $self->{mouseTransaction});
   $self-> notify(q(MouseUp),1,0,0,0) if $ft == $self-> firstTab;
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   if ( $key == kb::Left || $key == kb::Right) {
      $self-> focusedTab( $self-> focusedTab + (( $key == kb::Left) ? -1 : 1));
      $self-> clear_event;
      return;
   }
   if ( $key == kb::PgUp || $key == kb::PgDn) {
      $self-> tabIndex( $self-> tabIndex + (( $key == kb::PgUp) ? -1 : 1));
      $self-> clear_event;
      return;
   }
   if ( $key == kb::Home || $key == kb::End) {
      $self-> tabIndex(( $key == kb::Home) ? 0 : scalar @{$self->{tabs}});
      $self-> clear_event;
      return;
   }
   if ( $key == kb::Space || $key == kb::Enter) {
      $self-> tabIndex( $self-> focusedTab);
      $self-> clear_event;
      return;
   }
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> dark3DColor, $self-> light3DColor);
   my @size = $canvas-> size;

   $canvas-> color( $clr[1]);
   $canvas-> bar( 0, 0, @size);
   my ( $ft, $lt, $a, $ti, $ww, $tm) =
   ( $self->{firstTab}, $self->{lastTab}, $self->{arrows}, $self->{tabIndex},
     $self->{widths}, $self->{topMost});
   my $i;
   my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawTab));
   $self-> push_event;
   my $atX = DefLeftX;
   $atX += DefArrowX + DefGapX if $a & 1;
   my $atXti = undef;
   for ( $i = $ft; $i <= $lt; $i++) {
      $atX += $$ww[$i] + DefGapX;
   }
   my @colorSet = ( @clr, @c3d);

   $canvas-> clipRect( 0, 0, $size[0] - DefArrowX - DefGapX - DefLeftX, $size[1]) if $a & 2;
   for ( $i = $lt; $i >= $ft; $i--) {
      $atX -= $$ww[$i] + DefGapX;
      $atXti = $atX, next if $i == $ti;
      my @poly = (
         $atX, DefGapY, $atX + DefGapX, $size[1] - DefGapY - 1,
         $atX + DefGapX + $$ww[$i], $size[1] - DefGapY - 1,
         $atX + DefGapX * 2 + $$ww[$i], DefGapY
      );
      @poly[1,3,5,7] = @poly[3,1,7,5] unless $tm;
      $notifier->( @notifyParms, $canvas, $i, \@colorSet, \@poly);
   }

   my $swapDraw = ( $ti == $lt) && ( $a & 2);

   goto PaintSelTabBefore if $swapDraw;
PaintEarsThen:
   $canvas-> clipRect( 0, 0, @size) if $a & 2;
   if ( $a & 1) {
      my $x = DefLeftX;
      my @poly = (
         $x, DefGapY, $x + DefGapX, $size[1] - DefGapY - 1,
         $x + DefGapX + DefArrowX,   $size[1] - DefGapY - 1,
         $x + DefGapX * 2 + DefArrowX, DefGapY
      );
      @poly[1,3,5,7] = @poly[3,1,7,5] unless $tm;
      $notifier->( @notifyParms, $canvas, -1, \@colorSet, \@poly);
   }
   if ( $a & 2) {
      my $x = $size[0] - DefLeftX - DefArrowX - DefGapX * 2;
      my @poly = (
         $x, DefGapY, $x + DefGapX, $size[1] - DefGapY - 1,
         $x + DefGapX + DefArrowX, $size[1] - DefGapY - 1,
         $x + DefGapX * 2 + DefArrowX, DefGapY
      );
      @poly[1,3,5,7] = @poly[3,1,7,5] unless $tm;
      $notifier->( @notifyParms, $canvas, -2, \@colorSet, \@poly);
   }

   $canvas-> color( $c3d[0]);
   my @ld = $tm ? ( 0, DefGapY) : ( $size[1] - 0, $size[1] - DefGapY - 1);
   $canvas-> line( $size[0] - 1, $ld[0], $size[0] - 1, $ld[1]);
   $canvas-> color( $c3d[1]);
   $canvas-> line( 0, $ld[1], $size[0] - 1, $ld[1]);
   $canvas-> line( 0, $ld[0], 0, $ld[1]);
   $canvas-> color( $clr[0]);
   goto EndOfSwappedPaint if $swapDraw;

PaintSelTabBefore:
   if ( defined $atXti) {
      my @poly = (
         $atXti, DefGapY, $atXti + DefGapX, $size[1] - DefGapY - 1,
         $atXti + DefGapX + $$ww[$ti], $size[1] - DefGapY - 1,
         $atXti + DefGapX * 2 + $$ww[$ti], DefGapY
      );
      @poly[1,3,5,7] = @poly[3,1,7,5] unless $tm;
      my @poly2 = $tm ? (
         $atXti, DefGapY, $atXti + DefGapX * 2 + $$ww[$ti], DefGapY,
         $atXti + DefGapX * 2 + $$ww[$ti] - 4, 0, $atXti + 4, 0
      ) : (
         $atXti, $size[1] - 1 - DefGapY, $atXti + DefGapX * 2 + $$ww[$ti], $size[1] - 1 - DefGapY,
         $atXti + DefGapX * 2 + $$ww[$ti] - 4, $size[1]-1, $atXti + 4, $size[1]-1
      );
      $canvas-> clipRect( 0, 0, $size[0] - DefArrowX - DefGapX - DefLeftX, $size[1]) if $a & 2;
      $notifier->( @notifyParms, $canvas, $ti, \@colorSet, \@poly, $swapDraw ? undef : \@poly2);
   }
   goto PaintEarsThen if $swapDraw;

EndOfSwappedPaint:
   $self-> pop_event;
}

sub on_size
{
   my ( $self, $ox, $oy, $x, $y) = @_;
   if ( $x > $ox && (( $self->{arrows} & 2) == 0)) {
       my $w  = DefLeftX * 2 + DefGapX;
       my $ww = $self->{widths};
       $w += DefArrowX + DefGapX if $self->{arrows} & 1;
       my $i;
       my $set = 0;
       for ( $i = scalar @{$ww} - 1; $i >= 0; $i--) {
          $w += $$ww[$i] + DefGapX;
          $set = 1, $self-> firstTab( $i + 1), last if $w >= $x;
       }
       $self-> firstTab(0) unless $set;
   }
   $self-> reset;
}
sub on_fontchanged { $_[0]-> reset; $_[0]-> recalc_widths; }
sub on_enter       { $_[0]-> repaint; }
sub on_leave       { $_[0]-> repaint; }

sub on_measuretab
{
   my ( $self, $index, $sref) = @_;
   $$sref = $self-> get_text_width( $self->{tabs}->[$index]) + DefGapX * 4;
}

#
#  @poly:
#     [2,3]        [4,5]
#       o..........o
#      .            .
#[0,1].   TAB_TEXT   . [6,7]
#    o................o
#
#  @poly2:
# [0,1]               [2,3]
#    o................o
#[6,7]o..............o[4,5]
#

sub on_drawtab
{
   my ( $self, $canvas, $i, $clr, $poly, $poly2) = @_;
   $canvas-> color(( $self->{colored} && ( $i >= 0)) ?
      ( $warpColors[ $i % scalar @warpColors]) : $$clr[1]);
   $canvas-> fillpoly( $poly);
   $canvas-> fillpoly( $poly2) if $poly2;
   $canvas-> color( $$clr[3]);
   $canvas-> polyline( [@{$poly}[0..($self->{topMost}?5:3)]]);
   $canvas-> color( $$clr[2]);
   $canvas-> polyline( [@{$poly}[($self->{topMost}?4:2)..7]]);
   $canvas-> line( $$poly[4]+1, $$poly[5], $$poly[6]+1, $$poly[7]);
   $canvas-> color( $$clr[0]);
   if ( $i >= 0) {
      my  @tx = (
         $$poly[0] + ( $$poly[6] - $$poly[0] - $self->{widths}->[$i]) / 2 + DefGapX * 2,
         $$poly[1] + ( $$poly[3] - $$poly[1] - $canvas-> font-> height) / 2
      );
      $canvas-> text_out( $self->{tabs}->[$i], @tx);
      $canvas-> rect_focus( $tx[0] - 1, $tx[1] - 1,
         $tx[0] + $self->{widths}->[$i] - DefGapX * 4 + 1, $tx[1] + $canvas-> font-> height + 1)
            if ( $i == $self->{focusedTab}) && $self-> focused;
   } elsif ( $i == -1) {
      $canvas-> fillpoly([
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.6,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.2,
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.6,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.6,
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.4,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.4,
      ]);
   } elsif ( $i == -2) {
      $canvas-> fillpoly([
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.4,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.2,
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.4,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.6,
         $$poly[0] + ( $$poly[6] - $$poly[0]) * 0.6,
         $$poly[1] + ( $$poly[3] - $$poly[1]) * 0.4,
      ]);
   }
}

sub get_item_width
{
   return $_[0]->{widths}->[$_[1]];
}

sub tab2firstTab
{
   my ( $self, $ti) = @_;
   if (( $ti >= $self->{lastTab}) and ( $self->{arrows} & 2) and ( $ti != $self->{firstTab})) {
      my $w = DefLeftX;
      $w += DefArrowX + DefGapX if $self->{arrows} & 1;
      my $i;
      my $W = $self-> width;
      my $ww = $self->{widths};
      my $moreThanOne = ( $ti - $self->{firstTab}) > 0;
      for ( $i = $self->{firstTab}; $i <= $ti; $i++) {
         $w += $$ww[$i] + DefGapX;
      }
      my $lim = $W - DefLeftX - DefArrowX - DefGapX * 2;
      $lim -= DefGapX * 2 if $moreThanOne;
      if ( $w >= $lim) {
         my $leftw = DefLeftX * 2 + DefGapX + DefArrowX;
         $leftw += DefArrowX + DefGapX if $self->{arrows} & 1;
         $leftw = $W - $leftw;
         $leftw -= $$ww[$ti] if $moreThanOne;
         $w = 0;
         for ( $i = $ti; $i >= 0; $i--) {
            $w += $$ww[$i] + DefGapX;
            last if $w > $leftw;
         }
         return $i + 1;
      }
   } elsif ( $ti < $self->{firstTab}) {
      return $ti;
   }
   return undef;
}

sub set_tab_index
{
   my ( $self, $ti) = @_;
   $ti = 0 if $ti < 0;
   my $mx = scalar @{$self->{tabs}} - 1;
   $ti = $mx if $ti > $mx;
   return if $ti == $self->{tabIndex};
   $self-> {tabIndex} = $ti;
   $self-> {focusedTab} = $ti;
   my $newFirstTab = $self-> tab2firstTab( $ti);
   defined $newFirstTab ?
      $self-> firstTab( $newFirstTab) :
      $self-> repaint;
   $self-> notify(q(Change));
}

sub set_first_tab
{
   my ( $self, $ft) = @_;
   $ft = 0 if $ft < 0;
   unless ( $self->{arrows}) {
      $ft = 0;
   } else {
      my $w = DefLeftX * 2 + DefGapX * 2;
      $w += DefArrowX if $ft > 0;
      my $haveRight = 0;
      my $i;
      my @size = $self-> size;
      for ( $i = $ft; $i < scalar @{$self->{widths}}; $i++) {
         $w += DefGapX + $self->{widths}->[$i];
         $haveRight = 1, last if $w >= $size[0];
      }
      unless ( $haveRight) {
         $w += DefGapX;
         for ( $i = $ft - 1; $i >= 0; $i--) {
            $w += DefGapX + $self->{widths}->[$i];
            if ( $w >= $size[0]) {
               $i++;
               $ft = $i if $ft > $i;
               last;
            }
         }
      }
   }
   return if $self->{firstTab} == $ft;
   $self-> {firstTab} = $ft;
   $self-> reset;
   $self-> repaint;
}

sub set_focused_tab
{
   my ( $self, $ft) = @_;
   $ft = 0 if $ft < 0;
   my $mx = scalar @{$self->{tabs}} - 1;
   $ft = $mx if $ft > $mx;
   $self->{focusedTab} = $ft;

   my $newFirstTab = $self-> tab2firstTab( $ft);
   defined $newFirstTab ?
      $self-> firstTab( $newFirstTab) :
      ( $self-> focused ? $self-> repaint : 0);
}

sub set_tabs
{
   my $self = shift;
   my @tabs = ( scalar @_ == 1 && ref( $_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;
   $self-> {tabs} = \@tabs;
   $self-> recalc_widths;
   $self-> reset;
   $self-> lock;
   $self-> firstTab( $self-> firstTab);
   $self-> tabIndex( $self-> tabIndex);
   $self-> unlock;
   $self-> update_view;
}

sub set_top_most
{
   my ( $self, $tm) = @_;
   return if $tm == $self->{topMost};
   $self->{topMost} = $tm;
   $self-> repaint;
}

sub colored      {($#_)?($_[0]->{colored}=$_[1],$_[0]->repaint)         :return $_[0]->{colored}}
sub focusedTab   {($#_)?($_[0]->set_focused_tab(    $_[1]))             :return $_[0]->{focusedTab}}
sub firstTab     {($#_)?($_[0]->set_first_tab(    $_[1]))               :return $_[0]->{firstTab}}
sub tabIndex     {($#_)?($_[0]->set_tab_index(    $_[1]))               :return $_[0]->{tabIndex}}
sub topMost      {($#_)?($_[0]->set_top_most (    $_[1]))               :return $_[0]->{topMost}}
sub tabs         {($#_)?(shift->set_tabs     (    @_   ))               :return $_[0]->{tabs}}

package Notebook;
use vars qw(@ISA);
@ISA = qw(Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      pageCount        => 0,
      pageIndex        => 0,
      tabStop          => 0,
      ownerBackColor   => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   $self->{pageIndex} = -1;
   $self->{pageCount} = 0;
   my %profile = $self-> SUPER::init(@_);
   $self-> {pageCount} = $profile{pageCount};
   $self-> {pageCount} = 0 if $self-> {pageCount} < 0;
   my $j = $profile{pageCount};
   push (@{$self-> {widgets}},[]) while $j--;
   for ( qw( pageIndex)) { $self->$_( $profile{ $_}); }
   return %profile;
}

sub set_page_index
{
   my ( $self, $pi) = @_;
   $pi = 0 if $pi < 0;
   $pi = $self->{pageCount} - 1 if $pi > $self->{pageCount} - 1;
   my $sel = $self-> selected;
   return if $pi == $self->{pageIndex};
   $self-> lock;
   my $cp = $self->{widgets}->[$self->{pageIndex}];
   my $i;
   if ( defined $cp) {
      for ( $i = 0; $i < scalar @{$cp} / 4; $i++) {
         my $cx = $$cp[ $i * 4];
         $$cp[ $i * 4 + 1] = $cx-> enabled;
         $$cp[ $i * 4 + 2] = $cx-> visible;
         $$cp[ $i * 4 + 3] = $cx-> current;
         $cx-> visible(0);
         $cx-> enabled(0);
      }
   }
   $cp = $self->{widgets}->[$pi];
   my $hasSel = undef;
   if ( defined $cp) {
      for ( $i = 0; $i < scalar @{$cp} / 4; $i++) {
         my $cx = $$cp[ $i * 4];
         $cx-> enabled($$cp[ $i * 4 + 1]);
         $cx-> visible($$cp[ $i * 4 + 2]);
         $hasSel = $i if !defined $hasSel && $$cp[ $i * 4 + 3];
         $cx-> current($$cp[ $i * 4 + 3]);
      }
      $hasSel = 0 unless defined $hasSel;
      $$cp[ $hasSel * 4]-> select if $i and $sel;
   }
   $self-> unlock;
   $self-> update_view;
   $self->{pageIndex} = $pi;
}

sub insert_page
{
   my ( $self, $at) = @_;
   $at = -1 unless defined $at;
   $at = $self->{pageCount} if $at < 0 || $at > $self->{pageCount};
   splice( @{$self->{widgets}}, $at, 0, []);
   $self->{pageCount}++;
   $self-> pageIndex(0) if $self->{pageCount} == 1;
}

sub delete_page
{
   my ( $self, $at, $removeChildren) = @_;
   $removeChildren = 1 unless defined $removeChildren;
   $at = -1 unless defined $at;
   $at = $self->{pageCount} - 1 if $at < 0 || $at >= $self->{pageCount};
   my @r = splice( @{$self->{widgets}}, $at, 1);
   $self-> {pageCount}--;
   $self-> pageIndex( $self-> pageIndex);
   if ( $removeChildren) {
      my $i; my $s = $r[0];
      for ( $i = 0; $i < scalar @{$s} / 4; $i++) {
         $$s[$i*4]-> destroy;
      }
   }
}

sub attach_to_page
{
   my $self  = shift;
   my $page  = shift;
   $self-> insert_page if $self-> {pageCount} == 0;
   $page = $self-> {pageCount} - 1 if $page > $self-> {pageCount} - 1 || $page < 0;
   my $cp = $self->{widgets}->[$page];
   my $dt = $self-> delegateTo;
   for ( @_) {
      push( @{$cp}, $_);
      push( @{$cp}, $_->enabled);
      push( @{$cp}, $_->visible);
      push( @{$cp}, $_->current);
      if ( $page != $self->{pageIndex}) {
         $_-> visible(0);
         $_-> enabled(0);
      }
      $_-> delegateTo( $dt);
   }
}

sub insert
{
   my $self = shift;
   $self-> insert_to_page( $self->pageIndex, @_);
}

sub insert_to_page
{
   my $self  = shift;
   my $page  = shift;
   my $sel   = $self-> selected;
   $page = $self-> {pageCount} - 1 if $page > $self-> {pageCount} - 1 || $page < 0;
   $self-> lock;
   my @ctrls = $self-> SUPER::insert( @_);
   $self-> attach_to_page( $page, @ctrls);
   $ctrls[0]-> select if $sel && scalar @ctrls && $page == $self->{pageIndex};
   $self-> unlock;
   return wantarray ? @ctrls : $ctrls[0];
}

sub insert_transparent
{
   shift-> SUPER::insert( @_);
}

sub contains_widget
{
   my ( $self, $ctrl) = @_;
   my $i;
   my $j;
   my $cptr = $self->{widgets};
   for ( $i = 0; $i < $self->{pageCount}; $i++) {
      my $cp = $$cptr[$i];
      for ( $j = 0; $j < scalar @{$cp}/4; $j++) {
         return ( $i, $j) if $$cp[$j*4] == $ctrl;
      }
   }
   return;
}

sub widgets_from_page
{
   my ( $self, $page) = @_;
   return if $page < 0 or $page >= $self->{pageCount};
   my @a = @{$self->{widgets}->[$page]};
   my $i;
   my @r = ();
   for ( $i = 0; $i < scalar @a; $i+=4) {
      push( @r, $a[$i])
   };
   return @r;
}

sub detach
{
   my ( $self, $widget, $killFlag) = @_;
   my ( $page, $number) = $self-> contains_widget( $widget);
   splice( @{$self->{widgets}->[$page]}, $number * 4, 4) if defined $page;
   $self->SUPER::detach( $widget, $killFlag);
}

sub detach_from_page
{
   my ( $self, $ctrl)   = @_;
   my ( $page, $number) = $self-> contains_widget( $ctrl);
   return unless defined $page;
   splice( @{$self->{widgets}->[$page]}, $number * 4, 4);
}

sub delete_widget
{
   my ( $self, $ctrl)   = @_;
   my ( $page, $number) = $self-> contains_widget( $ctrl);
   return unless defined $page;
   $ctrl-> destroy;
}

sub move_widget
{
   my ( $self, $widget, $newPage) = @_;
   my ( $page, $number) = $self-> contains_widget( $widget);
   return unless defined $page;
   @{$self->{widgets}->[$newPage]} = splice( @{$self->{widgets}->[$page]}, $number * 4, 4);
   $self-> repaint if $self->{pageIndex} == $page || $self->{pageIndex} == $newPage;
}

sub set_page_count
{
   my ( $self, $pageCount) = @_;
   $pageCount = 0 if $pageCount < 0;
   return if $pageCount == $self->{pageCount};
   if ( $pageCount < $self->{pageCount}) {
      splice(@{$self-> {widgets}},$pageCount);
      $self->{pageCount} = $pageCount;
      $self->pageIndex($pageCount - 1) if $self->{pageIndex} < $pageCount - 1;
   } else {
      my $i = $pageCount - $self->{pageCount};
      push (@{$self-> {widgets}},[]) while $i--;
      $self->{pageCount} = $pageCount;
      $self->pageIndex(0) if $self->{pageIndex} < 0;
   }
}

sub pageIndex     {($#_)?($_[0]->set_page_index   ( $_[1]))    :return $_[0]->{pageIndex}}
sub pageCount     {($#_)?($_[0]->set_page_count   ( $_[1]))    :return $_[0]->{pageCount}}

package TabbedNotebook;
use vars qw(@ISA %notebookProps);
@ISA = qw(Widget);

use constant DefBorderX   => 11;
use constant DefBookmarkX => 32;

%notebookProps = (
   pageCount      => 1,
   attach_to_page => 1, insert_to_page   => 1, insert         => 1, insert_transparent => 1,
   delete_widget  => 1, detach_from_page => 1, move_widget    => 1, contains_widget    => 1,
);

for ( keys %notebookProps) {
   eval <<GENPROC;
   sub $_ { return shift-> {notebook}-> $_(\@_); }
GENPROC
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
      return {
      %{Notebook->profile_default},
      %{$_[ 0]-> SUPER::profile_default},
      ownerBackColor => 1,
      tabs           => [],
      tabIndex       => 0,
      tabCount       => 0,
   }
}

sub init
{
   my $self = shift;
   my %profile = @_;
   my $visible       = $profile{visible};
   my $scaleChildren = $profile{scaleChildren};
   $profile{visible}       = 0;
   $self->{tabs}     = [];
   %profile = $self-> SUPER::init(%profile);
   my @size = $self-> size;
   my $maxh = $self-> font-> height * 2;
   $self->{tabSet} = TabSet-> create(
      owner     => $self,
      name      => 'TabSet',
      left      => 0,
      width     => $size[0],
      top       => $size[1] - 1,
      growMode  => gm::Ceiling,
      height    => $maxh > 28 ? $maxh : 28,
      buffered  => 1,
      designScale => undef,
   );
   $self->{notebook} = Notebook-> create(
      owner      => $self,
      name       => 'Notebook',
      origin     => [ DefBorderX + 1, DefBorderX + 1],
      size       => [ $size[0] - DefBorderX * 2 - 5,
         $size[1] - DefBorderX * 2 - $self->{tabSet}-> height - DefBookmarkX - 4],
      growMode   => gm::Client,
      pageIndex  => $profile{pageIndex},
      scaleChildren => $scaleChildren,
      (map { $_  => $profile{$_}} keys %notebookProps),
      delegateTo => $self-> delegateTo,
      designScale => undef,
      pageCount  => scalar @{$profile{tabs}},
   );
   $self-> {notebook}-> designScale( $self-> designScale); # propagate designScale
   $self-> tabs( $profile{tabs});
   $self-> visible( $visible);
   return %profile;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> dark3DColor, $self-> light3DColor);
   my @size = $canvas-> size;
   $canvas-> color( $clr[1]);
   $canvas-> bar( 0, 0, @size);
   $size[1] -= $self->{tabSet}-> height;
   $canvas-> rect3d( 0, 0, $size[0] - 1, $size[1] - 1 + TabSet::DefGapY, 1, reverse @c3d);
   $canvas-> rect3d( DefBorderX, DefBorderX, $size[0] - 1 - DefBorderX,
      $size[1] - DefBorderX + TabSet::DefGapY, 1, @c3d);
   my $y = $size[1] - DefBorderX + TabSet::DefGapY;
   my $x  = $size[0] - DefBorderX - DefBookmarkX;

   return if $y < DefBorderX * 2 + DefBookmarkX;
   $canvas-> color( $c3d[0]);
   $canvas-> line( DefBorderX + 2,  $y - 2, $x - 2, $y - 2);
   $canvas-> line( $x + DefBookmarkX - 4, $y - DefBookmarkX + 1, $x + DefBookmarkX - 4, DefBorderX + 2);

   my $fh = 24;
   my $a  = 0;
   my ($pi, $mpi) = ( $self->{notebook}->pageIndex, $self->{notebook}->pageCount - 1);
   $a |= 1 if $pi > 0;
   $a |= 2 if $pi < $mpi;
   $canvas-> line( DefBorderX + 4, $y - $fh * 1.6, $x - 6, $y - $fh * 1.6);
   $canvas-> polyline([ $x - 2, $y - 2, $x - 2, $y - DefBookmarkX, $x + DefBookmarkX - 4, $y - DefBookmarkX]);
   $canvas-> line( $x - 1, $y - 3, $x + DefBookmarkX - 5, $y - DefBookmarkX + 1);
   $canvas-> line( $x - 1, $y - 4, $x + DefBookmarkX - 6, $y - DefBookmarkX + 1);
   $canvas-> line( $x - 0, $y - 2, $x + DefBookmarkX - 4, $y - DefBookmarkX + 2);
   $canvas-> line( $x + 5, $y - DefBookmarkX - 2, $x + DefBookmarkX - 5, $y - DefBookmarkX - 2);
   $canvas-> polyline([
       $x + 4, $y - DefBookmarkX + 6,
       $x + 10, $y - DefBookmarkX + 6,
       $x + 10, $y - DefBookmarkX + 8]) if $a & 1;
   my $dx = DefBookmarkX / 2;
   my ( $x1, $y1) = ( $x + $dx, $y - $dx);
   $canvas-> line( $x1 + 1, $y1 + 4, $x1 + 3, $y1 + 4) if $a & 2;
   $canvas-> line( $x1 + 5, $y1 + 6, $x1 + 5, $y1 + 8) if $a & 2;
   $canvas-> polyline([ $x1 + 3, $y1 + 2, $x1 + 5, $y1 + 2,
      $x1 + 5, $y1 + 4, $x1 + 7, $y1 + 4, $x1 + 7, $y1 + 6]) if $a & 2;
   $canvas-> color( $c3d[1]);
   $canvas-> line( $x - 1, $y - 7, $x + DefBookmarkX - 9, $y - DefBookmarkX + 1);
   $canvas-> line( DefBorderX + 4, $y - $fh * 1.6 - 1, $x - 6, $y - $fh * 1.6 - 1);
   $canvas-> polyline([ $x + 4, $y1 - 9, $x + 4, $y1 - 8, $x + 10, $y1 - 8]) if $a & 1;
   $canvas-> line( $x1 + 3, $y1 + 2, $x1 + 3, $y1 + 3) if $a & 2;
   $canvas-> line( $x1 + 6, $y1 + 6, $x1 + 7, $y1 + 6) if $a & 2;
   $canvas-> polyline([ $x1 + 1, $y1 + 4, $x1 + 1, $y1 + 6,
       $x1 + 3, $y1 + 6, $x1 + 3, $y1 + 8, $x1 + 5, $y1 + 8]) if $a & 2;
   $canvas-> color( cl::Black);
   $canvas-> line( $x - 1, $y - 2, $x + DefBookmarkX - 4, $y - DefBookmarkX + 1);
   $canvas-> line( $x + 5, $y - DefBookmarkX - 1, $x + DefBookmarkX - 5, $y - DefBookmarkX - 1);
   $canvas-> color( $clr[0]);
   my $t = $self->{tabs};
   if ( scalar @{$t}) {
      my $tx = $self->{tabSet}-> tabIndex;
      my $t1 = $$t[ $tx * 2];
      my $yh = $y - $fh * 0.8 - $self-> font-> height / 2;
      $canvas-> clipRect( DefBorderX + 1, $y - $fh * 1.6 + 1, $x - 4, $y - 3);
      $canvas-> text_out( $t1, DefBorderX + 4, $yh);
      if ( $$t[ $tx * 2 + 1] > 1) {
         $t1 = sprintf("Page %d of %d ", $self->pageIndex - $self->tab2page( $tx) + 1, $$t[ $tx * 2 + 1]);
         my $tl1 = $size[0] - DefBorderX - 3 - DefBookmarkX - $self-> get_text_width( $t1);
         $canvas-> text_out( $t1, $tl1, $yh) if $tl1 > 4 + DefBorderX + $fh * 3;
      }
   }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   $self-> clear_event;
   my @size = $self-> size;
   my $th = $self->{tabSet}-> height;
   $x -= $size[0] - DefBorderX - DefBookmarkX - 1;
   $y -= $size[1] - DefBorderX - $th - DefBookmarkX + 4;
   return if $x < 0 || $x > DefBookmarkX || $y < 0 || $y > DefBookmarkX;
   $self-> pageIndex( $self-> pageIndex + (( -$x + DefBookmarkX < $y) ? 1 : -1));
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}


sub page2tab
{
   my ( $self, $index) = @_;
   my $t = $self->{tabs};
   my $i = 0;
   my $j = 0;
   while( $i < $index) {
      $j++;
      $i += $$t[ $j*2 + 1];
   }
   return $j;
}

sub tab2page
{
   my ( $self, $index) = @_;
   my $t = $self->{tabs};
   my $i;
   my $j = 0;
   for ( $i = 0; $i < $index; $i++) { $j += $$t[ $i * 2 + 1]; }
   return $j;
}

sub TabSet_Change
{
   my ( $self, $tabset) = @_;
   return if $self->{changeLock};
   $self-> pageIndex( $self-> tab2page( $tabset-> tabIndex));
}


sub set_tabs
{
   my $self = shift;
   my @tabs = ( scalar @_ == 1 && ref( $_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;
   my @nTabs = ();
   my @loc   = ();
   my $prev  = undef;
   for ( @tabs) {
      if ( defined $prev && $_ eq $prev) {
         $loc[-1]++;
      } else {
         push( @loc,   $_);
         push( @loc,   1);
         push( @nTabs, $_);
      }
      $prev = $_;
   }
   my $pages = $self-> {notebook}-> pageCount;
   $self-> {tabs} = \@loc;
   $self-> {tabSet}-> tabs( \@nTabs);
   my $i;
   if ( $pages > scalar @tabs) {
      for ( $i = scalar @tabs; $i < $pages; $i++) {
         $self-> {notebook}-> delete_page( $i);
      }
   } elsif ( $pages < scalar @tabs) {
      for ( $i = $pages; $i < scalar @tabs; $i++) {
         $self-> {notebook}-> insert_page;
      }
   }
}

sub get_tabs
{
   my $self = $_[0];
   my $i;
   my $t = $self->{tabs};
   my @ret = ();
   for ( $i = 0; $i < scalar @{$t} / 2; $i++) {
      my $j;
      for ( $j = 0; $j < $$t[$i*2+1]; $j++) { push( @ret, $$t[$i*2]); }
   }
   return @ret;
}

sub set_page_index
{
   my ( $self, $pi) = @_;
   my ($pix, $mpi) = ( $self->{notebook}->pageIndex, $self->{notebook}->pageCount - 1);
   $self->{notebook}-> pageIndex( $pi);
   $self->{changeLock} = 1;
   $self->{tabSet}-> tabIndex( $self-> page2tab( $self->{notebook}-> pageIndex));
   delete $self->{changeLock};
   my @size = $self-> size;
   my $th = $self->{tabSet}-> height;
   my $a  = 0;
   $a |= 1 if $pix > 0;
   $a |= 2 if $pix < $mpi;
   my $newA = 0;
   $pix = $self->{notebook}-> pageIndex;
   $newA |= 1 if $pix > 0;
   $newA |= 2 if $pix < $mpi;
   $self->invalidate_rect(
      DefBorderX + 1, $size[1] - DefBorderX - $th - DefBookmarkX - 1,
      $size[0] - DefBorderX - (( $a == $newA) ? DefBookmarkX + 2 : 0),
      $size[1] - DefBorderX - $th + 3
   );
}

sub tabIndex     {($#_)?($_[0]->{tabSet}->tabIndex( $_[1]))   :return $_[0]->{tabSet}->tabIndex}
sub pageIndex    {($#_)?($_[0]->set_page_index   ( $_[1]))    :return $_[0]->{notebook}->pageIndex}
sub tabs         {($#_)?(shift->set_tabs     (    @_   ))     :return $_[0]->get_tabs}

1;
