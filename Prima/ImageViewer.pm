#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#  Created by:
#     Anton Berezin  <tobez@tobez.org>
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$
#
use strict;
use Prima::ScrollWidget;

package Prima::ImageViewer;
use vars qw(@ISA);
@ISA = qw( Prima::ScrollWidget);

sub profile_default
{
   my $def = $_[0]-> SUPER::profile_default;
   my %prf = (
      image        => undef,
      imageFile    => undef,
      zoom         => 1,
      alignment    => ta::Left,
      valignment   => ta::Bottom,
      quality      => 1,
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
   for ( qw( alignment quality valignment imageX imageY))
      { $self->{$_} = 0; }
   for ( qw( zoom integralScreen integralImage))
      { $self->{$_} = 1; }
   my %profile = $self-> SUPER::init(@_);
   $self-> { imageFile}     = $profile{ imageFile};
   for ( qw( image zoom alignment valignment quality)) {
      $self->$_($profile{$_});
   }
   return %profile;
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;
   my $bw     = $self-> {borderWidth};

   unless ( $self-> {image}) {
      $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor, $self-> backColor);
      return;
   }

   $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, $self-> dark3DColor, $self-> light3DColor) if $bw;
   my @r = $self-> get_active_area( 0, @size);
   $canvas-> clipRect( @r);
   $canvas-> transform( @r[0,1]);
   my $imY  = $self->{imageY};
   my $imX  = $self->{imageX};
   my $z = $self->{zoom};
   my $imYz = int($imY * $z);
   my $imXz = int($imX * $z);
   my $winY = $r[3] - $r[1];
   my $winX = $r[2] - $r[0];
   my $deltaY = ($imYz - $winY - $self->{deltaY} > 0) ? $imYz - $winY - $self->{deltaY}:0;
   my ($xa,$ya) = ($self->{alignment}, $self->{valignment});
   my ($iS, $iI) = ($self->{integralScreen}, $self->{integralImage});
   my ( $atx, $aty, $xDest, $yDest);

   if ( $imYz < $winY) {
      if ( $ya == ta::Top) {
         $aty = $winY - $imYz;
      } elsif ( $ya != ta::Bottom) {
         $aty = ($winY - $imYz)/2;
      } else {
         $aty = 0;
      }
      $canvas-> clear( 0, 0, $winX-1, $aty-1) if $aty > 0;
      $canvas-> clear( 0, $aty + $imYz, $winX-1, $winY-1) if $aty + $imYz < $winY;
      $yDest = 0;
   } else {
      $aty   = -($deltaY % $iS);
      $yDest = ($deltaY + $aty) / $iS * $iI;
      $imYz = int(($winY - $aty + $iS - 1) / $iS) * $iS;
      $imY = $imYz / $iS * $iI;
   }

   if ( $imXz < $winX) {
      if ( $xa == ta::Right) {
         $atx = $winX - $imXz;
      } elsif ( $xa != ta::Left) {
         $atx = ($winX - $imXz)/2;
      } else {
         $atx = 0;
      }
      $canvas-> clear( 0, $aty, $atx - 1, $aty + $imYz - 1) if $atx > 0;
      $canvas-> clear( $atx + $imXz, $aty, $winX - 1, $aty + $imYz - 1) if $atx + $imXz < $winX;
      $xDest = 0;
   } else {
      $atx   = -($self->{deltaX} % $iS);
      $xDest = ($self->{deltaX} + $atx) / $iS * $iI;
      $imXz = int(($winX - $atx + $iS - 1) / $iS) * $iS;
      $imX = $imXz / $iS * $iI;
   }

   $canvas-> put_image_indirect(
      $self->{image},
      $atx, $aty,
      $xDest, $yDest,
      $imXz, $imYz, $imX, $imY,
      rop::CopyPut
   );
}

sub set_alignment
{
   $_[0]->{alignment} = $_[1];
   $_[0]->repaint;
}

sub set_valignment
{
   $_[0]->{valignment} = $_[1];
   $_[0]->repaint;
}

sub set_image
{
   my ( $self, $img) = @_;
   $self->{image} = $img;
   unless ( defined $img) {
      $self->{imageX} = $self->{imageY} = 0;
      $self->limits(0,0);
      $self->palette([]);
      return;
   }
   my ( $x, $y) = ($img-> width, $img-> height);
   $self-> {imageX} = $x;
   $self-> {imageY} = $y;
   $x *= $self->{zoom};
   $y *= $self->{zoom};
   $self-> limits($x,$y);
   $self-> palette( $img->palette) if $self->{quality};
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

sub set_quality
{
   my ( $self, $quality) = @_;
   return if $quality == $self->{quality};
   $self->{quality} = $quality;
   return unless defined $self->{image};
   $self-> palette( $quality ? $self->{image}-> palette : []);
   $self-> repaint;
}

sub set_zoom
{
   my ( $self, $zoom) = @_;

   $zoom = 100 if $zoom > 100;
   $zoom = 1 if $zoom <= 0.01;

   my $dv = int( 100 * ( $zoom - int( $zoom)) + 0.5);
   $dv-- if ($dv % 2) and ( $dv % 5);
   $zoom = int($zoom) + $dv / 100;
   $dv = 0 if $dv >= 100;
   my ($r,$n,$m) = (1,100,$dv);
   while(1) {
      $r = $m % $n;
      last unless $r;
      ($m,$n) = ($n,$r);
   }
   return if $zoom == $self->{zoom};

   $self->{zoom} = $zoom;
   $self->{integralScreen} = int( 100 / $n) * int( $zoom) + int( $dv / $n);
   $self->{integralImage}  = int( 100 / $n);


   return unless defined $self->{image};
   my ( $x, $y) = ($self->{image}-> width, $self-> {image}->height);
   $x *= $self->{zoom};
   $y *= $self->{zoom};

   $self-> limits($x,$y);
   $self-> repaint;
   $self->{hScrollBar}->set_steps( $zoom, $zoom * 10) if $self->{hScroll};
   $self->{vScrollBar}->set_steps( $zoom, $zoom * 10) if $self->{vScroll};
}

sub screen2point
{
   my $self = shift;
   my @ret = ();
   my ( $i, $wx, $wy, $z, $dx, $dy, $ha, $va) =
      @{$self}{qw(indents winX winY zoom deltaX deltaY alignment valignment)};
   my $maxy = ( $wy < $self->{limitY}) ? $self->{limitY} - $wy : 0;
   unless ( $maxy) {
       if ( $va == ta::Top) {
          $maxy += $self-> {imageY} * $z - $wy;
       } elsif ( $va != ta::Bottom) {
          $maxy += ( $self-> {imageY} * $z - $wy) / 2;
       }
   }
   my $maxx = 0;
   if ( $wx > $self->{limitX}) {
       if ( $ha == ta::Right) {
          $maxx += $self-> {imageX} * $z - $wx;
       } elsif ( $ha != ta::Left) {
          $maxx += ( $self-> {imageX} * $z - $wx) / 2;
       }
   }

   while ( scalar @_) {
      my ( $x, $y) = ( shift, shift);
      $x += $dx - $$i[0];
      $y += $maxy - $dy - $$i[1];
      $x += $maxx;
      push @ret, $x / $z, $y / $z;
   }
   return @ret;
}

sub point2screen
{
   my $self = shift;
   my @ret = ();
   my ( $i, $wx, $wy, $z, $dx, $dy, $ha, $va) =
      @{$self}{qw(indents winX winY zoom deltaX deltaY alignment valignment)};
   my $maxy = ( $wy < $self->{limitY}) ? $self->{limitY} - $wy : 0;
   unless ( $maxy) {
       if ( $va == ta::Top) {
          $maxy += $self-> {imageY} * $z - $wy;
       } elsif ( $va != ta::Bottom) {
          $maxy += ( $self-> {imageY} * $z - $wy) / 2;
       }
   }
   my $maxx = 0;
   if ( $wx > $self->{limitX}) {
       if ( $ha == ta::Right) {
          $maxx += $self-> {imageX} * $z - $wx;
       } elsif ( $ha != ta::Left) {
          $maxx += ( $self-> {imageX} * $z - $wx) / 2;
       }
   }
   while ( scalar @_) {
      my ( $x, $y) = ( $z * shift, $z * shift);
      $x -= $maxx + $self-> {deltaX} - $$i[0];
      $y -= $maxy - $self-> {deltaY} - $$i[1];
      push @ret, $x, $y;
   }
   return @ret;
}


sub alignment    {($#_)?($_[0]->set_alignment(    $_[1]))               :return $_[0]->{alignment}    }
sub valignment   {($#_)?($_[0]->set_valignment(    $_[1]))              :return $_[0]->{valignment}   }
sub image        {($#_)?$_[0]->set_image($_[1]):return $_[0]->{image} }
sub imageFile    {($#_)?$_[0]->set_image_file($_[1]):return $_[0]->{imageFile}}
sub zoom         {($#_)?$_[0]->set_zoom($_[1]):return $_[0]->{zoom}}
sub quality      {($#_)?$_[0]->set_quality($_[1]):return $_[0]->{quality}}

1;
