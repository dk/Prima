package IntUtils;

use strict;
use Const;

package MouseScroller;

#    Used for system-defined scrolling delays fro widgets that scrolls itself
#  by MouseMove action.
#
#  widget should inherit MouseScroller as
#   @ISA = qw(... MouseScroller)
#
#    typical mousemove code should be:
#
#   if ( mouse_pointer_inside_the_scrollable_area)
#   {
#      $self-> stop_scroll_timer;
#   } else {
#      $self-> start_scroll_timer unless exists $self->{scrollTimer};
#      return unless $self->{scrollTimer}->{semaphore};
#      $self->{scrollTimer}->{semaphore} = 0;
#   }
#
#     MouseScroller also use semaphore named "mouseTransaction", which should
#   be true when widget is in mouse capture state.

sub stop_scroll_timer
{
   my $self = $_[0];
   return unless exists $self->{scrollTimer};
   $self->{scrollTimer}-> destroy;
   delete $self->{scrollTimer};
}

sub start_scroll_timer
{
   my $self = $_[0];
   $self-> stop_scroll_timer;
   my @rates = $::application-> get_scroll_rate;
   $self-> {scrollTimer} = Timer-> create(
      owner   => $self,
      timeout => $rates[0],
      name    => q(ScrollTimer),
   );
   $self->{scrollTimer}->{newRate} = $rates[1];
   $self->{scrollTimer}->{semaphore} = 1;
   $self->{scrollTimer}->start;
}

sub ScrollTimer_Tick
{
   my ( $self, $timer) = @_;
   if ( exists $self->{scrollTimer}->{newRate})
   {
      $timer-> timeout( $self->{scrollTimer}->{newRate});
      delete $self->{scrollTimer}->{newRate};
   }
   $timer->{semaphore} = 1;
   $self-> notify(q(MouseMove), 0, $self-> pointerPos);
   $self-> stop_scroll_timer unless defined $self->{mouseTransaction};
}

package GroupScroller;

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

use ScrollBar;

sub set_border_width
{
   my ( $self, $bw) = @_;
   $bw = 0 if $bw < 0;
   $bw = 1 if $bw > $self-> height / 2;
   $bw = 1 if $bw > $self-> width  / 2;
   return if $bw == $self-> {borderWidth};
   my $obw  = $self-> {borderWidth};
   my @size = $self-> size;
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
}


sub insert_bone
{
   my $self = $_[0];
   my $bw   = $self->{borderWidth};
   $self-> {bone}-> destroy if defined $self-> {bone};
   $self->{bone} = $self-> insert( q(Widget),
      name   => q(Bone),
      pointer=> cr::Arrow,
      origin => [ $bw + $self->{hScrollBar}-> width-1, $bw - 1],
      size   => [ $self->{vScrollBar}-> width-2, $self->{hScrollBar}-> height-1],
      ownerBackColor => 1,
      growMode  => gm::GrowLoX,
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
         name     => q(HScroll),
         vertical => 0,
         origin   => [ $bw-1, $bw-1],
         growMode => gm::GrowHiX,
         pointer  => cr::Arrow,
         width    => $self-> width - 2 * $bw + 2 - ( $self->{vScroll} ? $self->{vScrollBar}-> width - 2 : 0),
      );
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
         left     => $size[0] - $bw - $ScrollBar::stdMetrics[0] + 1,
         top      => $size[1] - $bw + 1,
         bottom   => $bw + ( $self->{hScroll} ? $self->{hScrollBar}-> height - 2 : 0),
         growMode => gm::GrowLoX | gm::GrowHiY,
         pointer  => cr::Arrow,
      );
      if ( $self->{hScroll})
      {
         $self-> {hScrollBar}->width(
            $self-> {hScrollBar}->width -
            $self-> {vScrollBar}->width + 2,
         );
         $self-> insert_bone;
      }
   } else {
      $self-> {vScrollBar}-> destroy;
      if ( $self->{hScroll})
      {
         $self-> {hScrollBar}-> width( $size[0] - 2 * $bw + 2);
         $self-> {bone}-> destroy;
         delete $self-> {bone};
      }
   }
}

sub hScroll         {($#_)?$_[0]->set_h_scroll       ($_[1]):return $_[0]->{hScroll}}
sub vScroll         {($#_)?$_[0]->set_v_scroll       ($_[1]):return $_[0]->{vScroll}}

1;
