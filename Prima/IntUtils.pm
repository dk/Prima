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
package Prima::IntUtils;

use strict;
use Prima::Const;

package Prima::MouseScroller;

#    Used for system-defined scrolling delays for widgets that scrolls themselves
#  using MouseMove notification.
#
#  widget should inherit MouseScroller as
#   @ISA = qw(... MouseScroller)
#
#    typical mousemove code should be:
#
#   if ( mouse_pointer_inside_the_scrollable_area)
#   {
#      $self-> scroll_timer_stop;
#   } else {
#      $self-> scroll_timer_start unless $self->scroll_timer_active;
#      return unless $self-> scroll_timer_semaphore;
#      $self-> scroll_timer_semaphore( 0);
#   }
#
#     MouseScroller also use semaphore named "mouseTransaction", which should
#   be true when widget is in mouse capture state.

my $scrollTimer;

sub scroll_timer_active
{
   return 0 unless defined $scrollTimer;
   return $scrollTimer-> {active};
}

sub scroll_timer_semaphore
{
   return 0 unless defined $scrollTimer;
   $#_ ?
      $scrollTimer-> {semaphore} = $_[1] :
      return $scrollTimer-> {semaphore};
}

sub scroll_timer_stop
{
   return unless defined $scrollTimer;
   $scrollTimer-> stop;
   $scrollTimer-> {active} = 0;
   $scrollTimer-> timeout( $scrollTimer-> {firstRate});
   $scrollTimer-> {newRate} = $scrollTimer-> {nextRate};
}

sub scroll_timer_start
{
   my $self = $_[0];
   $self-> scroll_timer_stop;
   unless ( defined $scrollTimer) {
      my @rates = $::application-> get_scroll_rate;
      $scrollTimer = Prima::Timer-> create(
         owner      => $::application,
         timeout    => $rates[0],
         name       => q(ScrollTimer),
         onTick     => sub { $_[0]-> {delegator}-> ScrollTimer_Tick( @_)},
      );
      @{$scrollTimer}{qw(firstRate nextRate newRate)} = (@rates,$rates[1]);
   }
   $scrollTimer-> {delegator} = $self;
   $scrollTimer-> {semaphore} = 1;
   $scrollTimer-> {active} = 1;
   $scrollTimer-> start;
}

sub ScrollTimer_Tick
{
   my ( $self, $timer) = @_;
   if ( exists $scrollTimer->{newRate})
   {
      $timer-> timeout( $scrollTimer->{newRate});
      delete $scrollTimer->{newRate};
   }
   $scrollTimer-> {semaphore} = 1;
   $self-> notify(q(MouseMove), 0, $self-> pointerPos);
   $self-> scroll_timer_stop unless defined $self->{mouseTransaction};
}

package Prima::IntIndents;

sub indents
{
   return wantarray ? @{$_[0]->{indents}} : [@{$_[0]->{indents}}] unless $#_;
   my ( $self, @indents) = @_;
   @indents = @{$indents[0]} if ( scalar(@indents) == 1) && ( ref($indents[0]) eq 'ARRAY');
   for ( @indents) {
      $_ = 0 if $_ < 0;
   }
   $self-> {indents} = \@indents;
}

sub get_active_area
{
   my @r = ( scalar @_ > 2) ? @_[2,3] : $_[0]-> size;
   my $i = $_[0]-> {indents};
   if ( !defined($_[1]) || $_[1] == 0) {
      # returns inclusive - exclusive
      return $$i[0], $$i[1], $r[0] - $$i[2], $r[1] - $$i[3];
   } elsif ( $_[1] == 1) {
      # returns inclusive - inclusive
      return $$i[0], $$i[1], $r[0] - $$i[2] - 1, $r[1] - $$i[3] - 1;
   } else {
      # returns size
      return $r[0] - $$i[0] - $$i[2], $r[1] - $$i[1] - $$i[3];
   }
}

package Prima::GroupScroller;
use vars qw(@ISA);
@ISA = qw(Prima::IntIndents);

#  used for Groups that contains optional scroll bars and provides properties for
#  it's maintenance and notifications ( HScroll_Change and VScroll_Change).
#
#  GroupScroller provides properties hScroll and vScroll of boolean type, which
#  turns scroll bars on and off. It occupies sub namespace with:
#    set_h_scroll, set_v_scroll, insert_bone
#  and object hash with:
#    hScroll, vScroll, hScrollBar, vScrollBar, bone
#
#  GroupScroller uses optional variable named borderWidth ( the width of border that
#  runs around the group and scroll bars).
#
#  widget should inherit GroupScroller as
#   @ISA = qw(... GroupScroller) , provide hScroll and vScroll properies in
#   profile_default() and set up variables vScroll and hScroll to 0 at init().
#
#  if widget uses borderWidth property, it should call inherited set_border_width
#  sub, since it needs for maintenance of scroll bars position;
#  if widget plans to change hScroll and/or vScroll properies at runtime, it should
#  override set_h_scroll and set_v_scroll subs to reset it's own parameters due visible
#  area change.
#
#  widget should set up scrollbar's max/min/step/pageStep/whole/partial/value
#  properties manually.

use Prima::ScrollBar;

sub setup_indents
{
   $_[0]-> {indents} = [ 0,0,0,0];
}

sub set_border_width
{
   my ( $self, $bw) = @_;
   my @size = $self-> size;
   $bw = 0 if $bw < 0;
   $bw = 1 if $bw > $size[1] / 2;
   $bw = 1 if $bw > $size[0] / 2;
   return if $bw == $self-> {borderWidth};
   my $obw  = $self-> {borderWidth};
   $self-> {borderWidth} = $bw;
   $self-> {hScrollBar}-> set(
      left   => $bw - 1,
      bottom => $bw - 1,
      width  => $size[0] - $bw * 2 + 2 - ( $self->{vScroll} ? $self->{vScrollBar}-> width - 2 : 0),
   ) if $self-> {hScroll};
   $self-> {vScrollBar}-> set(
      top    => $size[1] - $bw + 1,
      right  => $size[0] - $bw + 1,
      bottom => $bw + ( $self->{hScroll} ? $self->{hScrollBar}-> height - 2 : 0),
   ) if $self-> {vScroll};
   $self-> insert_bone if defined $self-> {bone};
   my $d = $bw - $obw;
   $self-> {indents}-> [$_] += $d for 0..3;
}


sub insert_bone
{
   my $self = $_[0];
   my $bw   = $self->{borderWidth};
   $self-> {bone}-> destroy if defined $self-> {bone};
   $self->{bone} = $self-> insert( q(Widget),
      name   => q(Bone),
      pointerType => cr::Arrow,
      origin => [ $bw + $self->{hScrollBar}-> width-1, $bw - 1],
      size   => [ $self->{vScrollBar}-> width-2, $self->{hScrollBar}-> height-1],
      growMode  => gm::GrowLoX,
      widgetClass => wc::ScrollBar,
      onPaint   => sub {
         my ( $self, $canvas, $owner, $w, $h) = ($_[0], $_[1], $_[0]-> owner, $_[0]-> size);
         $canvas-> color( $self-> backColor);
         $canvas-> bar( 0, 1, $w - 2, $h - 1);
         $canvas-> color( $owner-> light3DColor);
         $canvas-> line( 0, 0, $w - 1, 0);
         $canvas-> line( $w - 1, 0, $w - 1, $h - 1);
      },
   );
}

sub set_h_scroll
{
   my ( $self, $hs) = @_;
   return if $hs == $self->{hScroll};
   my $bw = $self-> {borderWidth} || 0;
   if ( $self->{hScroll} = $hs) {
      $self->{hScrollBar} = $self-> insert( q(ScrollBar),
         name        => q(HScroll),
         vertical    => 0,
         origin      => [ $bw-1, $bw-1],
         growMode    => gm::GrowHiX,
         pointerType => cr::Arrow,
         width       => $self-> width - 2 * $bw + 2 - ( $self->{vScroll} ? $self->{vScrollBar}-> width - 2 : 0),
         delegations => ['Change'],
      );
      $self-> {indents}->[1] += $self->{hScrollBar}-> height - 1;
      if ( $self->{vScroll})
      {
         my $h = $self-> {hScrollBar}-> height;
         $self-> {vScrollBar}-> set(
            bottom => $self-> {vScrollBar}-> bottom + $h - 2,
            top    => $self-> {vScrollBar}-> top,
         );
         $self-> insert_bone;
      }
   } else {
      $self->{indents}->[1] -= $self->{hScrollBar}-> height - 1;
      $self->{hScrollBar}-> destroy;
      if ( $self->{vScroll})
      {
         $self-> {vScrollBar}-> set(
            bottom => $bw,
            height => $self-> height - $bw * 2,
         );
         $self-> {bone}-> destroy;
         delete $self-> {bone};
      }
   }
}

sub set_v_scroll
{
   my ( $self, $vs) = @_;
   return if $vs == $self->{vScroll};
   my $bw = $self-> {borderWidth} || 0;
   my @size = $self-> size;
   if ( $self->{vScroll} = $vs) {
      $self->{vScrollBar} = $self-> insert( q(ScrollBar),
         name     => q(VScroll),
         vertical => 1,
         left     => $size[0] - $bw - $Prima::ScrollBar::stdMetrics[0] + 1,
         top      => $size[1] - $bw + 1,
         bottom   => $bw + ( $self->{hScroll} ? $self->{hScrollBar}-> height - 2 : 0),
         growMode => gm::GrowLoX | gm::GrowHiY,
         pointerType  => cr::Arrow,
         delegations  => ['Change'],
      );
      $self->{indents}->[2] += $self->{vScrollBar}-> width - 1;
      if ( $self->{hScroll})
      {
         $self-> {hScrollBar}->width(
            $self-> {hScrollBar}->width -
            $self-> {vScrollBar}->width + 2,
         );
         $self-> insert_bone;
      }
   } else {
      $self->{indents}->[2] -= $self->{vScrollBar}-> width - 1;
      $self-> {vScrollBar}-> destroy;
      if ( $self->{hScroll})
      {
         $self-> {hScrollBar}-> width( $size[0] - 2 * $bw + 2);
         $self-> {bone}-> destroy;
         delete $self-> {bone};
      }
   }
}

sub borderWidth     {($#_)?($_[0]->set_border_width( $_[1])):return $_[0]->{borderWidth}}
sub hScroll         {($#_)?$_[0]->set_h_scroll       ($_[1]):return $_[0]->{hScroll}}
sub vScroll         {($#_)?$_[0]->set_v_scroll       ($_[1]):return $_[0]->{vScroll}}



1;
