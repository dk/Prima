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
package Editor;

use Prima;
use Prima::Classes;
use Prima::Widgets;
use Prima::Application;

package Painter;
use vars qw(@ISA);
@ISA = qw(Prima::Scroller);

sub profile_default
{
   my %d = %{$_[ 0]-> SUPER::profile_default};
   return {
      %d,
      standardScrollBars => 1,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   my $i = $self->{image} = Prima::Image-> create(
      width  => $profile{limitX},
      height => $profile{limitY},
      type   => im::RGB,
   );
   $i-> begin_paint;
   $i-> color( cl::White);
   $i-> bar( 0,0,$i->size);
   $i-> end_paint;
   return %profile;
}

sub on_paint
{
   my ($self, $canvas)  = @_;
   my ($im, $x, $y) = ( $self-> {image}, $self-> size);
   $canvas-> color( $self-> backColor);
   if ( $im)
   {
      my ($w, $h) = $im-> size;
      $canvas-> bar ( $w, 0, $x, $y) if $x > $w;
      $canvas-> bar ( 0, $h, $w, $y) if $y > $h;
      $canvas-> put_image_indirect(
         $im, 0, 0, $self-> deltaX,
         $im-> height - $self-> height - $self-> deltaY >= 0 ?
         $im-> height - $self-> height - $self-> deltaY : 0,
         $w, $h, $w, $h, rop::CopyPut
      );
   } else { $canvas-> bar(0, 0, $x, $y); }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction};
   $self->{mouseTransaction} = 1;
   $self-> capture(1);
   $self->{lmx} = $x;
   $self->{lmy} = $y;
   $self->{mxy} = $self-> {vscroll}-> max;
   $self-> {estorage} = undef;
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   my ( $dx, $dy, $mxy) = ( $self->{deltaX}, $self->{deltaY}, $self->{mxy});
   my @ln = ( $self->{lmx} + $dx, $self->{lmy} + $mxy - $dy, $x + $dx, $y + $mxy - $dy);
   $self-> begin_paint;
   $self-> line( $self->{lmx}, $self->{lmy}, $x, $y);
   $self-> end_paint;
   push( @{$self-> {estorage}-> {line}}, [@ln]);
   $self->{lmx} = $x;
   $self->{lmy} = $y;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   $self->{mouseTransaction} = undef;
   $self-> capture(0);
   $self->{lmx} = undef;
   $self->{lmy} = undef;
   return unless defined $self->{estorage};
   my $i = $self-> {image};
   $i-> begin_paint;
   if ( exists $self->{estorage}->{line})
   {
      for ( @{$self->{estorage}->{line}})
      {
         $i-> line( @$_);
      }
   }
   $i-> end_paint;
}

sub done
{
   my $self = $_[0];
   $self->{image}-> destroy;
   $self-> SUPER::done;
}


my $w = Prima::Window-> create(
   text    => 'Edit sample',
   size       => [ 400, 400],
   centered   => 1,
   active     => 1,
   onDestroy  => sub {$::application-> close},
);

my $e = $w-> insert( Painter =>
   origin     => [ 64, 64],
   size       => [ $w-> width - 64, $w-> height - 64],
   limitX     => 320,
   limitY     => 320,
);

run Prima;
