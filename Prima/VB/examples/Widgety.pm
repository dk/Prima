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

#     Example of how to implement mirror to VB widget palette.
#     In fact, you don't need to keep runtime package and VB
#     implementation in one file.
#
#     Package Prima::VB::examples::Widgety must be presented
#     first in file, located in Prima/VB/examples/Widgety.pm,
#     but runtime package could be located somewhere else, like
#     Prima::VB::CoreClasses.pm is a shell to Prima/*.pm
#
#     Another example here is to introduce custom property editor,
#     included into Object Inspector. Here's defined 'lineRoundStyle',
#     property, same as rare-used 'lineEnd'.
#

##########################  VB mirror package #########################

package Prima::VB::examples::Widgety;

sub classes
{
   return (
      'Prima::SampleWidget' => {
         RTModule => 'Prima::VB::examples::Widgety',
         class    => 'Prima::VB::examples::SampleWidget',
         page     => 'Samples',
         module   => 'Prima::VB::examples::Widgety',
         icon     => 'VB::VB.gif:0',
      },
   );
}

use Prima::VB::Classes;

package Prima::VB::examples::SampleWidget;
use vars qw(@ISA);
@ISA = qw( Prima::VB::CommonControl);

sub prf_types
{
   my $pt = $_[ 0]-> SUPER::prf_types;
   my %de = (
      lineRoundStyle => ['lineRoundStyle'],
   );
   $_[0]-> prf_types_add( $pt, \%de);
   return $pt;
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $self-> size;
   my $c = $self-> color;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 0, 0, @sz);
   $canvas-> color( $c);
   $canvas-> lineWidth( 8);
   $canvas-> lineEnd( $self-> prf('lineRoundStyle'));
   $canvas-> line( 20, 20, $sz[0] - 21, $sz[1] - 21);
   $canvas-> draw_text( $self-> prf('text'), 0, 0, @sz, dt::Center | dt::VCenter);
   $canvas-> lineWidth( 1);
   $self-> common_paint($canvas);
}


sub prf_lineRoundStyle { $_[0]-> repaint; }

package Prima::VB::Types::lineRoundStyle;
use vars qw(@ISA);
@ISA = qw(Prima::VB::Types::radio);
sub IDS    { qw( Flat Square Round); }
sub packID { 'le'; }

sub open
{
   my $self = shift;
   $self-> SUPER::open( @_);
   $self->{A}-> set(
      bottom => $self->{A}->bottom + 36,
      height => $self->{A}->height - 36,
   );
   $self->{B} = $self->{container}-> insert( Widget =>
      origin => [ 5, 5],
      size   => [ $self->{A}->width, 32],
      growMode => gm::GrowHiX,
      onPaint  => sub {
         my ( $me, $canvas) = @_;
         my @sz = $canvas-> size;
         $canvas-> color( cl::White);
         $canvas-> bar(0,0,@sz);
         $canvas-> lineEnd( $self-> get);
         $canvas-> lineWidth( 14);
         $canvas-> color( cl::Gray);
         $canvas-> line( 14, $sz[1]/2, $sz[0]-14, $sz[1]/2);
         $canvas-> lineWidth( 2);
         $canvas-> color( cl::Black);
         $canvas-> lineEnd( le::Round);
         $canvas-> line( 8, $sz[1]/2, 20, $sz[1]/2);
         $canvas-> line( $sz[0]-20, $sz[1]/2, $sz[0]-8, $sz[1]/2);
         $canvas-> line( 14, $sz[1]/2-6, 14, $sz[1]/2+6);
         $canvas-> line( $sz[0]-14, $sz[1]/2-6, $sz[0]-14, $sz[1]/2+6);
      },
   );
}

sub on_change
{
   my $self = $_[0];
   $self-> {B}-> repaint;
}

###############################  runtime package ##########################################

use Prima::Classes;

package Prima::SampleWidget;
use vars qw(@ISA);
@ISA = qw( Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      lineRoundStyle => le::Round,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self->  SUPER::init( @_);
   $self-> lineRoundStyle( $profile{lineRoundStyle});
   return %profile;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @sz = $self-> size;
   my $c = $self-> color;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 0, 0, @sz);
   $canvas-> color( $c);
   $canvas-> lineWidth( 8);
   $canvas-> lineEnd( $self-> lineRoundStyle);
   $canvas-> line( 20, 20, $sz[0] - 21, $sz[1] - 21);
   $canvas-> draw_text( $self-> text, 0, 0, @sz, dt::Center | dt::VCenter);
}

sub lineRoundStyle
{
   return $_[0]-> {lineRoundStyle} unless $#_;
   $_[0]-> {lineRoundStyle} = $_[1];
   $_[0]-> repaint;
}

1;
