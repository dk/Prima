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
#   Panel
#   Scroller

use Prima::Const;
use Prima::Classes;
use Prima::ScrollBar;
use strict;


package Prima::Panel;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
      ownerBackColor => 1,
      raise          => 1,
      borderWidth    => 1,
      image          => undef,
      imageFile      => undef,
      zoom           => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   if ( defined $p->{imageFile} && !defined $p->{image})
   {
      $p->{image} = Prima::Image-> create;
      delete $p->{image} unless $p->{image}-> load($p->{imageFile});
   }
}

sub init
{
   my $self = shift;
   for ( qw( image ImageFile))
      { $self->{$_} = undef; }
   for ( qw( zoom raise borderWidth iw ih)) { $self->{$_} = 1; }
   my %profile = $self-> SUPER::init(@_);
   $self-> { imageFile}  = $profile{ imageFile};
   for ( qw( image zoom borderWidth raise)) {
      $self->$_($profile{$_});
   }
   return %profile;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;

   my $clr    = $self-> backColor;
   my $bw     = $self-> {borderWidth};
   my @c3d    = ( $self-> light3DColor, $self-> dark3DColor);
   @c3d = reverse @c3d unless $self->{raise};
   my $cap = $self-> text;
   unless ( defined $self->{image}) {
      $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, @c3d, $clr);
      $canvas-> text_out( $cap,
         ( $size[0] - $canvas-> get_text_width( $cap)) / 2,
         ( $size[1] - $canvas-> font-> height) / 2,
      ) if defined $cap;
      return;
   }
   $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, @c3d) if $bw > 0;
   $canvas-> text_out ( $cap,
      ( $size[0] - $canvas-> get_text_width( $cap)) / 2,
      ( $size[1] - $canvas-> font-> height) / 2,
   ) if $cap;
   my ( $x, $y) = ( $bw, $bw);
   my ( $dx, $dy ) = ( $self-> {iw}, $self->{ih});
   if ( $bw > 0) {
      $canvas-> clipRect( $bw, $bw, $size[0] - $bw - 1, $size[1] - $bw - 1);
      $size[0] -= $bw;
      $size[1] -= $bw;
   }
   while ( $x < $size[0]) {
      $y = $bw;
      while ( $y < $size[1]) {
         $canvas-> stretch_image( $x, $y, $dx, $dy, $self->{image});
         $y += $dy;
      }
      $x += $dx;
   }
}

sub set_border_width
{
   my ( $self, $bw) = @_;
   $bw = 0 if $bw < 0;
   $bw = int( $bw);
   return if $bw == $self->{borderWidth};
   $self->{borderWidth} = $bw;
   $self-> repaint;
}

sub set_text
{
   $_[0]-> SUPER::set_text( $_[1]);
   $_[0]-> repaint;
}


sub set_raise
{
   my ( $self, $r) = @_;
   return if $r == $self->{raise};
   $self->{raise} = $r;
   $self-> repaint;
}

sub set_image_file
{
   my ($self,$file,$img) = @_;
   $img = Prima::Image-> create;
   return unless $img-> load($file);
   $self->{imageFile} = $file;
   $self->image($img);
}

sub set_image
{
   my ( $self, $img) = @_;
   $self->{image} = $img;
   return unless defined $img;
   ( $self->{iw}, $self->{ih}) = ($img-> width * $self->{zoom}, $img-> height * $self->{zoom});
   $self-> {image} = undef if $self->{ih} == 0 || $self->{iw} == 0;
   $self-> repaint;
}


sub set_zoom
{
   my ( $self, $zoom) = @_;
   $zoom = int( $zoom);
   $zoom = 1 if $zoom < 1;
   $zoom = 10 if $zoom > 10;
   return if $zoom == $self->{raise};
   $self->{zoom} = $zoom;
   return unless defined $self->{image};
   my $img = $self->{img};
   ( $self->{iw}, $self->{ih}) = ( $img-> width * $self->{zoom}, $img-> height * $self->{zoom});
   $self-> repaint;
}

sub image        {($#_)?$_[0]->set_image($_[1]):return $_[0]->{image} }
sub imageFile    {($#_)?$_[0]->set_image_file($_[1]):return $_[0]->{imageFile}}
sub zoom         {($#_)?$_[0]->set_zoom($_[1]):return $_[0]->{zoom}}
sub borderWidth  {($#_)?$_[0]->set_border_width($_[1]):return $_[0]->{borderWidth}}
sub raise        {($#_)?$_[0]->set_raise($_[1]):return $_[0]->{raise}}


package Prima::Scroller;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

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
      my $hsb = Prima::ScrollBar-> create(
         name     => "HScroll",
         owner    => $self-> owner,
         left     => $self-> left,
         bottom   => $self-> bottom,
         width    => $self-> width - $std[ 1],
         vertical => 0,
         onChange => sub { $_[0]->owner->bring( $name)->on_scroll},
      );
      my $vsb = Prima::ScrollBar-> create(
         name   => "VScroll",
         vertical => 1,
         owner  => $self-> owner,
         left   => $self-> right  - $std[ 0],
         bottom => $self-> bottom + $std[ 1],
         height => $self-> height - $std[ 1],
         onChange => sub { $_[0]->owner->bring( $name)->on_scroll},
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
