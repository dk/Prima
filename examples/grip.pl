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
use strict;
use Prima;
use Prima::Const;
use Prima::Classes;
use Prima::ImageViewer;


package MonoDeviceBitmap;
use vars qw( @ISA);
@ISA = qw( Prima::DeviceBitmap);

sub set_color
{
   my ( $self, $color) = @_;
   $self-> SUPER::set_color( $self-> get_nearest_color( $color));
}

sub set_back_color
{
   my ( $self, $color) = @_;
   $self-> SUPER::set_back_color( $self-> get_nearest_color( $color));
}

package Generic;

$::application = Prima::Application-> create( name => "Generic.pm");

my $imgType = im::bpp1;

sub canvas
{
    my $self = $_[0];
    my $i = MonoDeviceBitmap-> create(
       type         => $imgType,
       width        => $self-> width,
       height       => $self-> height,
       monochrome   => 1,
       preserveType => 1,
    );
    $i-> begin_paint;
    $i-> set(
       color      => $self-> color,
       backColor  => $self-> backColor,
       font       => $self-> font,
    );
    return $i;
}

sub paint
{
   my ( $self, $canvas) = @_;
   $self-> notify(q(Paint), $canvas);
   $canvas-> clipRect( 0, 0, $self-> size);
   $canvas-> transform(0, 0);
   $canvas-> palette([]);
   my @c = $self-> widgets;
   for ( @c) {
      next unless $_-> visible;
      my @org = $_-> origin;
      my $i = canvas( $_);
      if ( $_-> transparent) {
         $i-> put_image( -$org[0], -$org[1], $canvas);
      }
      paint( $_, $i);
      $i-> end_paint;
      $canvas-> put_image( @org, $i);
   }
}

sub xordraw
{
   my $self = shift;
   my $o = $::application;
   my @xrect = @_;
   $o-> begin_paint;
   $o-> rect_focus( $self->{capx},$self->{capy}, @xrect, 1) if scalar @xrect == 2;
   $o-> rect_focus( $self->{capx},$self->{capy}, $self->{dx},$self->{dy}, 1);
   $o-> end_paint;
}

my $w = Prima::Window-> create(
   size => [ 200, 200],
   menuItems => [
      [ "~Grip" => sub {
         my $self = $_[0];
         $self-> capture( 1);
         $self-> pointer( cr::Move);
         $self-> { cap} = 1;
      }],
      [ "G~rab" => sub {
         my $self = $_[0];
         $self-> capture( 1);
         $self-> { cap} = 2;
      }],
   ],
   onDestroy => sub { $::application-> close; },
   onMouseDown => sub {
      my ( $self, $btn, $mod, $x, $y) = @_;
      return unless defined $self->{cap};
      return unless $self->{cap} == 2;
      return unless $btn == mb::Left;
      $self->{cap} = 3;
      ($self->{capx},$self->{capy}) = $self-> client_to_screen( $x, $y);
      ($self->{dx},$self->{dy}) = ($self->{capx},$self->{capy});
      xordraw( $self);
   },
   onMouseMove => sub {
      my ( $self, $mod, $x, $y) = @_;
      return unless defined $self->{cap};
      return unless $self->{cap} == 3;
      my @od = ($self->{dx},$self->{dy});
      ($self->{dx},$self->{dy}) = $self-> client_to_screen( $x, $y);
      xordraw( $self, @od);
   },
   onMouseUp => sub {
      my ( $self, $btn, $mod, $x, $y) = @_;
      return unless defined $self-> { cap};
      my $cap = $self-> { cap};
      $self-> { cap} = undef;
      $self-> capture(0);
      if ( $cap == 1) {
         $self-> pointer( cr::Default);
         my $v = $::application-> get_view_from_point( $self-> client_to_screen( $x, $y));
         return unless $v;
         my $i = canvas( $v);
         paint( $v, $i);
         $i-> end_paint;
         $self-> IV-> image( $i);
      } elsif ( $cap == 3) {
         ($self->{dx},$self->{dy}) = $self-> client_to_screen( $x, $y);
         xordraw( $self);
         my ( $xl, $yl) = (abs($self->{dx} - $self->{capx}), abs($self->{dy} - $self->{capy}));
         $x = $self->{capx} > $self->{dx} ? $self->{dx} : $self->{capx};
         $y = $self->{capy} > $self->{dy} ? $self->{dy} : $self->{capy};
         my $i = $::application-> get_image( $x, $y, $xl, $yl);
         $self-> IV-> image( $i);
      }
   },
);

$w-> insert( ImageViewer =>
   origin => [ 0, 0],
   size   => [ $w-> size],
   growMode => gm::Client,
   hScroll => 1,
   vScroll => 1,
   name    => 'IV',
   valignment  => ta::Center,
   alignment   => ta::Center,
   quality => 1,
);





run Prima;
