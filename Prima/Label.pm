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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
#  $Id$
package Prima::Label;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

use Carp;
use Prima::Const;
use Prima::Classes;
use strict;

sub profile_default
{
   my $font = $_[ 0]-> get_default_font;
   return {
      %{$_[ 0]-> SUPER::profile_default},
      alignment      => ta::Left,
      autoWidth      => 1,
      autoHeight     => 0,
      focusLink      => undef,
      height         => 4 + $font-> { height},
      ownerBackColor => 1,
      selectable     => 0,
      showAccelChar  => 0,
      showPartial    => 1,
      tabStop        => 0,
      widgetClass    => wc::Label,
      wordWrap       => 0,
      valignment     => ta::Top,
   }
}


sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $p-> { autoWidth} = 0
      if exists $p->{width}  || exists $p->{size} || exists $p-> {rect} || ( exists $p->{left} && exists $p->{right});
   $p-> {autoHeight} = 0
      if exists $p->{height} || exists $p->{size} || exists $p-> {rect} || ( exists $p->{top} && exists $p->{bottom});
   $self-> SUPER::profile_check_in( $p, $default);
   my $vertical = exists $p-> {vertical} ? $p-> {vertical} : $default->{ vertical};
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> { alignment}     = $profile{ alignment};
   $self-> { valignment}    = $profile{ valignment};
   $self-> { autoHeight}    = $profile{ autoHeight};
   $self-> { autoWidth}     = $profile{ autoWidth};
   $self-> { wordWrap}      = $profile{ wordWrap};
   $self-> { focusLink}     = $profile{ focusLink};
   $self-> { showAccelChar} = $profile{ showAccelChar};
   $self-> { showPartial}   = $profile{ showPartial};
   $self-> check_auto_size;
   return %profile;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @size = $canvas-> size;
   my @clr;
   if ( $self-> enabled)
   {
     if ( $self-> focused) {
        @clr = ($self-> hiliteColor, $self-> hiliteBackColor);
     } else { @clr = ($self-> color, $self-> backColor); }
   } else { @clr = ($self-> disabledColor, $self-> disabledBackColor); }

   unless ( $self-> transparent)
   {
      $canvas-> color( $clr[1]);
      $canvas-> bar(0,0,@size);
   }

   my $fh = $canvas-> font-> height;
   my $ta = $self->{alignment};
   my $wx = $self->{widths};
   my $ws = $self->{words};
   my ($starty,$ycommon) = (0, scalar @{$ws} * $fh);
   if ( $self->{valignment} == ta::Top)       { $starty = $size[1] - $fh;}
   elsif ( $self->{valignment} == ta::Bottom) { $starty = $ycommon - $fh;}
   else { $starty = ( $size[1] + $ycommon)/2 - $fh; }
   my $y   = $starty;
   my $tl  = $self->{tildeLine};
   my $i;
   my $paintLine = !$self->{showAccelChar} && $tl < scalar @{$ws} && $tl >= 0;

   unless ( $self-> enabled)
   {
      $canvas-> color( $self-> light3DColor);
      for ( $i = 0; $i < scalar @{$ws}; $i++)
      {
         my $x = 0;
         if ( $ta == ta::Center) { $x = ( $size[0] - $$wx[$i]) / 2; }
         elsif ( $ta == ta::Right) { $x = $size[0] - $$wx[$i]; }
         $canvas-> text_out( $$ws[$i], $x + 1, $y - 1);
         $y -= $fh;
      }
      $y   = $starty;
      if ( $paintLine) {
         my $x = 0;
         if ( $ta == ta::Center) { $x = ( $size[0] - $$wx[$tl]) / 2; }
         elsif ( $ta == ta::Right) { $x = $size[0] - $$wx[$tl]; }
         $canvas-> line( $x + $self->{tildeStart} + 1, $starty - $fh * $tl - 1,
                         $x + $self->{tildeEnd} + 1,   $starty - $fh * $tl - 1);
      }
   }

   $canvas-> color( $clr[0]);
   for ( $i = 0; $i < scalar @{$ws}; $i++)
   {
      my $x = 0;
      if ( $ta == ta::Center) { $x = ( $size[0] - $$wx[$i]) / 2; }
      elsif ( $ta == ta::Right) { $x = $size[0] - $$wx[$i]; }
      $canvas-> text_out( $$ws[$i], $x, $y);
      $y -= $fh;
   }
   if ( $paintLine) {
      my $x = 0;
      if ( $ta == ta::Center) { $x = ( $size[0] - $$wx[$tl]) / 2; }
      elsif ( $ta == ta::Right) { $x = $size[0] - $$wx[$tl]; }
      $canvas-> line( $x + $self->{tildeStart}, $starty - $fh * $tl,
                      $x + $self->{tildeEnd},   $starty - $fh * $tl);
   }
}


sub text
{
   return $_[0]->SUPER::text unless $#_;
   my $self = $_[0];
   $self-> SUPER::text( $_[1]);
   $self-> check_auto_size;
   $self-> repaint;
}


sub on_translateaccel
{
   my ( $self, $code, $key, $mod) = @_;
   if ( defined $self->{accel} && ( $key == kb::NoKey) && lc chr $code eq $self-> { accel})
   {
      $self-> clear_event;
      $self-> notify( 'Click');
   }
}

sub on_click
{
   my ( $self, $f) = ( $_[0], $_[0]->{focusLink});
   $f-> select if defined $f && $f-> alive && $f-> enabled;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   if ( defined $self->{accel} && ( $key == kb::NoKey) && lc chr $code eq $self-> { accel})
   {
      $self-> notify( 'Click');
      $self-> clear_event;
   }
}

sub on_mousedown
{
   my $self = $_[0];
   $self-> notify( 'Click');
   $self-> clear_event;
}

sub on_fontchanged
{
   $_[0]-> check_auto_size;
}

sub on_size
{
   $_[0]-> reset_lines;
}

sub on_enable { $_[0]-> repaint } sub on_disable { $_[0]-> repaint }

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


sub reset_lines
{
   my $self = $_[0];
   my @res;
   my $maxLines = int($self-> height / $self-> font-> height);
   $maxLines++ if $self->{showPartial} and (($self-> height % $self-> font-> height) > 0);
   my $opt   = tw::NewLineBreak|tw::ReturnLines|tw::WordBreak|tw::CalcMnemonic|tw::ExpandTabs|tw::CalcTabs;
   my $width = 1000000;
   $opt |= tw::CollapseTilde unless $self->{showAccelChar};
   $width = $self-> width if $self->{wordWrap};
   my $lines = $self-> text_wrap( $self-> text, $width, $opt);
   my $lastRef = pop @{$lines};
   $self->{textLines} = scalar @$lines;
   for( qw( tildeStart tildeEnd tildeLine)) {$self->{$_} = $lastRef->{$_}}
   $self-> {accel} = defined($self->{tildeStart}) ? lc( $lastRef->{tildeChar}) : undef;
   splice( @{$lines}, $maxLines) if scalar @{$lines} > $maxLines;
   $self-> {words} = $lines;
   my @len;
   for ( @{$lines}) { push @len, $self-> get_text_width( $_); }
   $self-> {widths} = [@len];
   $self-> repaint;
}

sub check_auto_size
{
   my $self = $_[0];
   my $cap = $self-> text;
   unless ( $self->{wordWrap})
   {
      $cap =~ s/~//s unless $self->{showAccelChar};
      my %sets;
      $sets{ width}  = $self-> get_text_width( $cap) + 6 if $self->{autoWidth};
      $sets{ height} = $self-> font-> height + 2 if $self->{autoHeight};
      $self-> set( %sets);
   }
   $self-> reset_lines;
   if ( $self->{wordWrap} && $self->{autoHeight}) {
      $self-> height( $self-> font-> height * $self->{textLines} + 2);
   }
}

sub set_auto_width
{
   $_[0]->{autoWidth} = $_[1];
   $_[0]-> check_auto_size;
}

sub set_auto_height
{
   $_[0]->{autoHeight} = $_[1];
   $_[0]-> check_auto_size;
}

sub set_word_wrap
{
   $_[0]->{wordWrap} = $_[1];
   $_[0]->repaint;
}

sub set_show_accel_char
{
   $_[0]->{showAccelChar} = $_[1];
   $_[0]-> check_auto_size;
}

sub set_show_partial
{
   $_[0]->{showPartial} = $_[1];
   $_[0]-> check_auto_size;
}


sub get_lines
{
   return @{$_[0]->{words}};
}


sub showAccelChar {($#_)?($_[0]->set_show_accel_char($_[1]))             :return $_[0]->{showAccelChar}}
sub showPartial   {($#_)?($_[0]->set_show_partial($_[1]))                :return $_[0]->{showPartial}}
sub focusLink     {($#_)?($_[0]->{focusLink}     = $_[1])                :return $_[0]->{focusLink}    }
sub alignment     {($#_)?($_[0]->set_alignment(    $_[1]))               :return $_[0]->{alignment}    }
sub valignment    {($#_)?($_[0]->set_valignment(    $_[1]))              :return $_[0]->{valignment}   }
sub autoWidth     {($#_)?($_[0]->set_auto_width(   $_[1]))               :return $_[0]->{autoWidth}    }
sub autoHeight    {($#_)?($_[0]->set_auto_height(  $_[1]))               :return $_[0]->{autoHeight}   }
sub wordWrap      {($#_)?($_[0]->set_word_wrap(    $_[1]))               :return $_[0]->{wordWrap}     }

1;
