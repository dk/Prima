#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
   my @size   = $self-> size;
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

   $canvas-> clear( $atx, $aty, $atx + $imXz, $aty + $imYz) if $self-> {icon};

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

my @cubic_palette;

sub set_image
{
   my ( $self, $img) = @_;
   unless ( defined $img) {
      $self->{imageX} = $self->{imageY} = 0;
      $self->limits(0,0);
      $self->palette([]);
      $self-> repaint if defined $self-> {image};
      $self->{image} = $img;
      return;
   }
   $self->{image} = $img;
   my ( $x, $y) = ($img-> width, $img-> height);
   $self-> {imageX} = $x;
   $self-> {imageY} = $y;
   $x *= $self->{zoom};
   $y *= $self->{zoom};
   $self-> {icon} = $img-> isa('Prima::Icon');
   $self-> limits($x,$y);
   if ( $self->{quality}) {
      if (( $img-> type & im::BPP) > 8) {
         my $depth = $self-> get_bpp;
         if (($depth > 2) && ($depth <= 8)) {
            unless ( scalar @cubic_palette) {
               my ( $r, $g, $b) = (6, 6, 6);
               @cubic_palette = ((0) x 648);
               for ( $b = 0; $b < 6; $b++) {
                  for ( $g = 0; $g < 6; $g++) {
                     for ( $r = 0; $r < 6; $r++) {
                        my $ix = $b + $g * 6 + $r * 36;
                        @cubic_palette[ $ix, $ix + 1, $ix + 2] = 
                           map {$_*51} ($b,$g,$r); 
            }}}}
            $self-> palette( \@cubic_palette);
         }
      } else {
         $self-> palette( $img->palette);
      }
   }
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

__DATA__

=pod

=head1 NAME

Prima::ImageViewer - standard image, icon, and bitmap viewer class.

=head1 DESCRIPTION

The module contains C<Prima::ImageViewer> class, which provides
image displaying functionality, including different zoom levels.

C<Prima::ImageViewer> is a descendant of C<Prima::ScrollWidget>
and inherits its document scrolling behavior and programming interface.
See L<Prima::ScrollWidget> for details.

=head1 API

=head2 Properties

=over

=item alignment INTEGER

One of the following C<ta::XXX> constants:

   ta::Left
   ta::Center 
   ta::Right

Selects the horizontal image alignment.

Default value: C<ta::Left>

=item image OBJECT

Selects the image object to be displayed. OBJECT can be
an instance of C<Prima::Image>, C<Prima::Icon>, or C<Prima::DeviceBitmap> class.

=item imageFile FILE

Set the image FILE to be loaded and displayed. Is rarely used since does not return
a loading success flag.

=item quality BOOLEAN

A boolean flag, selecting if the palette of C<image> is to be 
copied into the widget palette, providing higher visual
quality on paletted displays. See also L<Prima::Widget/palette>.

Default value: 1

=item valignment INTEGER

One of the following C<ta::XXX> constants:

   ta::Top
   ta::Middle or ta::Center
   ta::Bottom

Selects the vertical image alignment.

NB: C<ta::Middle> value is not equal to C<ta::Center>'s, however
the both constants produce equal effect here.

Default value: C<ta::Bottom>

=item zoom FLOAT

Selects zoom level for image display. The acceptable value range
is between 0.02 and 100. The zoom value
is rounded to fiftieth and twentieth fractionals - 
.02, .04, .05, .06, .08, and 0.1 .

Default value: 1

=back

=head2 Methods

=over

=item screen2point X, Y, [ X, Y, ... ]

Performs translation of integer pairs integers as (X,Y)-points from widget coordinates 
to pixel offset in image coordinates. Takes in account zoom level,
image alignments, and offsets. Returns array of same length as the input.

Useful for determining correspondence, for example, of a mouse event
to a image point.

The reverse function is C<point2screen>.

=item point2screen   X, Y, [ X, Y, ... ]

Performs translation of integer pairs as (X,Y)-points from image pixel offset
to widget image coordinates. Takes in account zoom level,
image alignments, and offsets. Returns array of same length as the input.

Useful for detemining a screen location of an image point.

The reverse function is C<screen2point>.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Image>, L<Prima::ScrollWidget>, F<examples/iv.pl>.

=cut
