use strict;
use Const;
use Classes;
use IntUtils;

package ScrollWidget;
use vars qw(@ISA);
@ISA = qw( Widget GroupScroller);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      borderWidth    => 0,
      hScroll        => 0,
      vScroll        => 0,
      deltaX         => 0,
      deltaY         => 0,
      limitX         => 0,
      limitY         => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub init
{
   my $self = shift;
   for ( qw( scrollTransaction hScroll vScroll limitX limitY deltaX deltaY borderWidth marginX marginY))
      { $self->{$_} = 0; }
   my %profile = $self-> SUPER::init(@_);
   for ( qw( hScroll vScroll borderWidth))
      { $self->$_( $profile{ $_}); }
   $self-> limits( $profile{limitX}, $profile{limitY});
   $self-> deltas( $profile{deltaX}, $profile{deltaY});
   $self-> reset_scrolls;
   return %profile;
}

sub reset_scrolls
{
   my $self = $_[0];
   my $bw = $self->{borderWidth};
   my ($x, $y) = $self-> size;
   my ($w, $h) = $self-> limits;
   $x -= $bw * 2;
   $y -= $bw * 2;
   $x -= $self-> {vScrollBar}-> width - 1 if $self-> {vScroll};
   $y -= $self-> {hScrollBar}-> height - 1 if $self-> {hScroll};

   if ( $self-> {hScroll}) {
      $self-> {hScrollBar}-> set(
        max     => $x < $w ? $w - $x : 0,
        whole   => $w,
        partial => $x < $w ? $x : $w,
      );
   }
   if ( $self-> {vScroll}) {
      $self-> {vScrollBar}-> set(
        max     => $y < $h ? $h - $y : 0,
        whole   => $h,
        partial => $y < $h ? $y : $h,
      );
   }
   $self-> set_deltas( $self->{deltaX}, $self->{deltaY});
}

sub set_limits
{
   my ( $self, $w, $h) = @_;
   $w = 0 if $w < 0;
   $h = 0 if $h < 0;
   $w = int( $w);
   $h = int( $h);
   return if $w == $self-> {limitX} and $h == $self->{limitY};
   $self-> {limitY} = $h;
   $self-> {limitX} = $w;
   $self-> reset_scrolls;
}

sub set_deltas
{
   my ( $self, $w, $h) = @_;
   my ($odx,$ody) = ($self->{deltaX},$self->{deltaY});
   $w = 0 if $w < 0;
   $h = 0 if $h < 0;
   $w = int( $w);
   $h = int( $h);
   my ($x, $y) = $self-> limits;
   my ( $ww, $hh) = $self-> size;
   my ( $www, $hhh) = ( $ww, $hh);
   my $bw = $self->{borderWidth};
   $ww -= $bw * 2;
   $hh -= $bw * 2;
   $ww -= $self-> {vScrollBar}-> width - 1 if $self-> {vScroll};
   $hh -= $self-> {hScrollBar}-> height - 1 if $self-> {hScroll};
   $x -= $ww;
   $y -= $hh;
   $x = 0 if $x < 0;
   $y = 0 if $y < 0;

   $w = $x if $w > $x;
   $h = $y if $h > $y;
   return if $w == $odx and $h == $ody;
   $self-> {deltaY} = $h;
   $self-> {deltaX} = $w;
   my @r = $self->get_active_area($www,$hhh);
   $self-> clipRect( @r);
   $self-> scroll( $odx - $w, $h - $ody);
   $self-> {scrollTransaction} = 1;
   $self-> {hScrollBar}-> value( $w) if $self->{hScroll};
   $self-> {vScrollBar}-> value( $h) if $self->{vScroll};
   $self-> {scrollTransaction} = undef;
}

sub set_v_scroll
{
   my ( $self, $val) = @_;
   $self-> SUPER::set_v_scroll( $val);
   $self->{marginX} = ( $self->{vScroll} ? $self->{vScrollBar}-> width-1  : 0);
}

sub set_h_scroll
{
   my ( $self, $val) = @_;
   $self-> SUPER::set_h_scroll( $val);
   $self->{marginY} = ( $self->{hScroll} ? $self->{hScrollBar}-> height-1 : 0);
}

sub get_active_area
{
   my $self = $_[0];
   my @sz = ($_[1],$_[2]);
   my ( $dx, $dy, $bw) = ($self->{marginX}, $self->{marginY}, $self->{borderWidth});
   @sz = $self-> size unless defined $sz[0] and defined $sz[1];
   return $bw, $dy + $bw, $sz[0] - $bw - $dx, $sz[1] - $bw;
}

sub on_size
{
   $_[0]-> reset_scrolls;
}

sub VScroll_Change
{
   $_[0]-> deltaY( $_[1]-> value) unless $_[0]-> {scrollTransaction};
}

sub HScroll_Change
{
   $_[0]-> deltaX( $_[1]-> value) unless $_[0]-> {scrollTransaction};
}


sub borderWidth   {($#_)?$_[0]->set_border_width   ($_[1]):return $_[0]->{borderWidth}    }
sub limitX        {($#_)?$_[0]->set_limits($_[1],$_[0]->{limitY}):return $_[0]->{'limitX'};  }
sub limitY        {($#_)?$_[0]->set_limits($_[0]->{'limitX'},$_[1]):return $_[0]->{'limitY'};  }
sub limits        {($#_)?$_[0]->set_limits         ($_[1], $_[2]):return ($_[0]->{'limitX'},$_[0]->{'limitY'});}
sub deltaX        {($#_)?$_[0]->set_deltas($_[1],$_[0]->{deltaY}):return $_[0]->{'deltaX'};  }
sub deltaY        {($#_)?$_[0]->set_deltas($_[0]->{'deltaX'},$_[1]):return $_[0]->{'deltaY'};  }
sub deltas        {($#_)?$_[0]->set_deltas         ($_[1], $_[2]):return ($_[0]->{'deltaX'},$_[0]->{'deltaY'}); }

1;
