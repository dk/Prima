package Widgets;

# contains:
#   Panel
#   Scroller

use Const;
use Classes;
use ScrollBar;
use strict;


# Panel is widget that can draw itself. Not too correct, but it's very
# productive for radio buttons etc.

package Panel;
use vars qw(@ISA);
@ISA = qw(Widget);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      indent         => 0,
      raise          => 0,
      ownerBackColor => 1,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {raise}  = $profile{ raise};
   $self-> {indent} = $profile{ indent};
   return %profile;
}

sub set_text
{
   $_[0]-> SUPER::set_text( $_[1]);
   $_[0]-> repaint;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my ($x, $y) = $canvas-> get_size;
   my $i = $self-> indent;
   my ($clFore,$clBack) = ($self-> color, $self-> backColor);
   $canvas-> color( $clBack);
   $canvas-> bar (0, 0, $x, $y);
   $canvas-> color( ( $self-> raise) ? cl::White : cl::DarkGray);
   $canvas-> line( $i, $i, $i, $y - $i - 1);
   $canvas-> line( $i, $y - $i - 1, $x - $i - 1, $y - $i - 1);
   $canvas-> color( ( $self-> raise) ? cl::DarkGray : cl::White);
   $canvas-> line( $i + 1, $i, $x - $i - 1, $i);
   $canvas-> line( $x - $i - 1, $i, $x - $i - 1, $y - $i - 1);
   $canvas-> color ( $clFore);
   my $cap = $self-> text;
   $canvas-> text_out ( $cap,
      ( $x - $canvas-> get_text_width( $cap)) / 2,
      ( $y - $canvas-> font-> height) / 2,
   ) if $cap;
}

sub indent {($#_)?($_[0]->{indent} = $_[1],$_[0]-> repaint):return $_[0]->{indent};}
sub raise  {($#_)?($_[0]->{raise } = $_[1],$_[0]-> repaint):return $_[0]->{raise };}

package Scroller;
use vars qw(@ISA);
@ISA = qw(Widget);

# Scroller is an abstract object, so it needs on_paint to be overriden.
# also area limits should be set through set_limits.

sub profile_default
{
   my %d = %{$_[ 0]-> SUPER::profile_default};
   return {
      %d,
      growMode           => gm::GrowHiX | gm::GrowHiY,
      deltaX             => 0,
      deltaY             => 0,
      limitX             => 0,
      limitY             => 0,
      standardScrollBars => 0,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   if ( $profile{ standardScrollBars})
   {
      my $name = $profile{ name};
      my @std = @ScrollBar::stdMetrics;
      my $hsb = ScrollBar-> create(
         name     => "HScroll",
         owner    => $self-> owner,
         left     => $self-> left,
         bottom   => $self-> bottom,
         width    => $self-> width - $std[ 1],
         vertical => 0,
         onChange => sub { $_[0]->owner->fetch( $name)->on_scroll},
      );
      my $vsb = ScrollBar-> create(
         name   => "VScroll",
         vertical => 1,
         owner  => $self-> owner,
         left   => $self-> right  - $std[ 0],
         bottom => $self-> bottom + $std[ 1],
         height => $self-> height - $std[ 1],
         onChange => sub { $_[0]->owner->fetch( $name)->on_scroll},
      );
      $self-> set(
        width  => $self-> width  - $std[ 0],
        bottom => $self-> bottom + $std[ 1],
        height => $self-> height - $std[ 1],
      );
      ( $self-> { hscroll}, $self-> { vscroll}) = ( $hsb, $vsb);
   } else {
      ( $self-> { hscroll}, $self-> { vscroll}) = ( undef, undef);
   }
   $self-> {deltaX }  = $profile{ deltaX };
   $self-> {deltaY }  = $profile{ deltaY };
   $self-> limits ( $profile{ limitX }, $profile{ limitY });
   return %profile;
}

sub set_limits
{
   $_[0]-> {limitY} = $_[1];
   $_[0]-> {limitX} = $_[2];
   $_[0]-> reset_scrolls;
}

sub set_delta
{
   $_[0]-> {deltaY} = $_[1];
   $_[0]-> {deltaX} = $_[2];
   $_[0]-> reset_scrolls;
}


sub reset_scrolls
{
   my $self = $_[0];
   my ($w, $h) = $self-> limits;
   my ($x, $y) = $self-> size;
   $self-> { deltaX} = $w - $x if ($self-> deltaX > $w - $x) && ( $self-> deltaX > 0);
   if ( $self-> hscroll) {
      $self-> hscroll-> set(
         max     => $x < $w ? $w - $x : 0,
         value   => $self-> deltaX,
         partial => $x < $w ? $x : $w,
         whole   => $w,
      );
      $self-> { deltaX} = $self-> hscroll-> value;
   }
   if ( $self-> vscroll) {
      $self-> vscroll-> set(
         max     => $y < $h ? $h - $y : 0,
         value   => $self-> deltaY,
         partial => $y < $h ? $y : $h,
         whole   => $h,
      );
      $self-> { deltaY} = $self-> vscroll-> value;
   }
}

sub on_size
{
  $_[0]->reset_scrolls;
}

sub on_scroll
{
   my $self = $_[0];
   my $odx = $self-> deltaX;
   my $ody = $self-> deltaY;
   $self-> {deltaX} = $self-> hscroll-> value if $self-> hscroll;
   $self-> {deltaY} = $self-> vscroll-> value if $self-> vscroll;
   $self-> scroll( $odx - $self-> deltaX, $self-> deltaY - $ody);
}


sub vscroll { return $_[0]-> { vscroll}};
sub hscroll { return $_[0]-> { hscroll}};
sub limitX {($#_)?$_[0]->set_limits($_[1],$_[0]->{limitY}):return $_[0]->{'limitX'};  }
sub limitY {($#_)?$_[0]->set_limits($_[0]->{'limitX'},$_[1]):return $_[0]->{'limitY'};  }
sub limits {($#_)?$_[0]->set_limits         ($_[1], $_[2]):return ($_[0]->{'limitY'},$_[0]->{'limitX'});}
sub deltaX {($#_)?$_[0]->set_delta ($_[1],$_[0]->{deltaY}):return $_[0]->{'deltaX'};  }
sub deltaY {($#_)?$_[0]->set_delta ($_[0]->{'deltaX'},$_[1]):return $_[0]->{'deltaY'};  }
sub delta  {($#_)?$_[0]->set_delta          ($_[1], $_[2]):return ($_[0]->{'deltaY'},$_[0]->{'deltaX'}); }


1;
