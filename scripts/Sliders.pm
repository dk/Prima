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
package Sliders;

# contains:
#   SpinButton
#   AltSpinButton
#   SpinEdit
#   Gauge
#   Slider
#   CircularSlider

use strict;
use Const;
use Classes;
use IntUtils;


package AbstractSpinButton;
use vars qw(@ISA);
@ISA = qw(Widget MouseScroller);

{
my %RNT = (
   %{Widget->notification_types()},
   Increment  => nt::Default,
   TrackEnd   => nt::Default,
);
sub notification_types { return \%RNT; }
}

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      ownerBackColor => 1,
      color          => cl::Black,
      selectable     => 0,
      tabStop        => 0,
      widgetClass    => wc::Button,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   $self-> { pressState} = 0;
   $self-> {__DYNAS__}->{onIncrement}  = $profile{onIncrement};
   $self-> {__DYNAS__}->{onTrackEnd}   = $profile{onTrackEnd};
   return %profile;
}

sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onIncrement} = $profile{onIncrement},
      delete $profile{onIncrement} if exists $profile{onIncrement};
   $self->{__DYNAS__}->{onTrackEnd} = $profile{onTrackEnd},
      delete $profile{onTrackEnd} if exists $profile{onTrackEnd};
   $self-> SUPER::set( %profile);
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub state         {($#_)?$_[0]->set_state ($_[1]):return $_[0]->{pressState}}

sub on_trackstart {}
sub on_trackend   {}
sub on_increment  {
#  my ( $self, $increment) = @_;
}

package SpinButton;
use vars qw(@ISA);
@ISA = qw(AbstractSpinButton);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      width        => 17,
      height       => 24,
   }
}


sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;
   my $h = $self-> height;
   if    ( $y >= $h * 0.6) { $self->{ mouseTransaction} = 1; }
   elsif ( $y <  $h * 0.4) { $self->{ mouseTransaction} = 2; }
   else                                       { $self->{ mouseTransaction} = 3; }
   $self->{ lastMouseOver}  = 1;
   $self->{ startMouseY  }  = $y;
   $self-> state( $self->{ mouseTransaction});
   $self-> capture(1);
   $self-> clear_event;
   $self->{increment} = 0;
   if ( $self->{ mouseTransaction} != 3) {
      $self-> notify( 'Increment', $self->{ mouseTransaction} == 1 ? 1 : -1);
      $self-> scroll_timer_start;
      $self-> scroll_timer_semaphore(0);
   } else {
      $self->{pointerSave} = $self-> pointer;
      $self->pointer( cr::SizeWE);
   }
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self->{mouseTransaction};
   my $mt  = $self-> {mouseTransaction};
   my $inc = $mt != 2 ? 1 : -1;
   $self-> {mouseTransaction} = undef;
   $self-> {spaceTransaction} = undef;
   $self-> {lastMouseOver}    = undef;
   $self-> capture(0);
   $self-> scroll_timer_stop;
   $self-> state( 0);
   $self-> pointer( $self->{pointerSave}), $self->{pointerSave} = undef if $mt == 3;
   $self->{increment} = 0;
   $self-> notify( 'TrackEnd');
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   unless ( $self->{mouseTransaction}) {
      my $h = $self-> height;
      $self-> pointer((( $y >= $h * 0.6) || ( $y < $h * 0.4)) ? cr::Default : cr::SizeWE);
      return;
   }
   my @size = $self-> size;
   my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
   $self-> state( $self->{pressState} ? 0 : $self->{mouseTransaction})
      if $self-> { lastMouseOver} != $mouseOver && $self->{pressState} != 3;
   $self-> { lastMouseOver} = $mouseOver;
   if ( $self->{pressState} == 3) {
      my $d  = ( $self->{startMouseY} - $y) / 3; # 2 is mouse sensitivity
      $self-> notify( 'Increment', int($self->{increment}) - int($d))
        if int( $self-> {increment}) != int( $d);
      $self-> {increment}  = $d;
   } elsif ( $self->{pressState} > 0) {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self-> scroll_timer_semaphore;
      $self-> scroll_timer_semaphore(0);
      $self-> notify( 'Increment', $self->{mouseTransaction} == 1 ? 1 : -1);
   } else {
      $self-> scroll_timer_stop;
   }
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
   my @size = $canvas-> size;
   my $p = $self->{pressState};
   $canvas-> rect3d( 0, 0, $size[0] - 1, $size[1] * 0.4 - 1, 2,
        (($p != 2) ? @c3d : reverse @c3d), $clr[1]);
   $canvas-> rect3d( 0, $size[1] * 0.4, $size[0] - 1, $size[1] * 0.6 - 1, 2,
      (($p != 3) ? @c3d : reverse @c3d), $clr[1]);
   $canvas-> rect3d( 0, $size[1] * 0.6, $size[0] - 1, $size[1] - 1, 2,
        (($p != 1) ? @c3d : reverse @c3d), $clr[1]);
   $canvas-> color( $clr[0]);
   my $p1 = ( $p == 1) ? 1 : 0;
   $canvas-> fillpoly([
      $size[0] * 0.3 + $p1, $size[1] * 0.73 - $p1,
      $size[0] * 0.5 + $p1, $size[1] * 0.87 - $p1,
      $size[0] * 0.7 + $p1, $size[1] * 0.73 - $p1
   ]);
   $p1 = ( $p == 2) ? 1 : 0;
   $canvas-> fillpoly([
      $size[0] * 0.3 + $p1, $size[1] * 0.27 - $p1,
      $size[0] * 0.5 + $p1, $size[1] * 0.13 - $p1,
      $size[0] * 0.7 + $p1, $size[1] * 0.27 - $p1
   ]);
}

sub set_state
{
   my ( $self, $s) = @_;
   $s = 0 if $s > 3;
   return if $s == $self->{pressState};
   $self-> {pressState} = $s;
   $self-> repaint;
}

package AltSpinButton;
use vars qw(@ISA);
@ISA = qw(AbstractSpinButton);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      width        => 18,
      height       => 18,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $p->{height} = $p->{width}  if !exists( $p->{height}) && exists( $p->{width});
   $p->{width}  = $p->{height} if exists( $p->{height}) && !exists( $p->{width});
   $self-> SUPER::profile_check_in( $p, $default);
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;
   $self->{ mouseTransaction} = (( $x * $self-> height / $self-> width) > $y) ? 2 : 1;
   $self->{ lastMouseOver}  = 1;
   $self-> state( $self->{ mouseTransaction});
   $self-> capture(1);
   $self-> clear_event;
   $self-> notify( 'Increment', $self->{ mouseTransaction} == 1 ? 1 : -1);
   $self-> scroll_timer_start;
   $self-> scroll_timer_semaphore(0);
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self->{mouseTransaction};
   $self-> {mouseTransaction} = undef;
   $self-> {spaceTransaction} = undef;
   $self-> {lastMouseOver}    = undef;
   $self-> capture(0);
   $self-> scroll_timer_stop;
   $self-> state( 0);
   $self-> notify( 'TrackEnd');
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   my @size = $self-> size;
   my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
   $self-> state( $self->{pressState} ? 0 : $self->{mouseTransaction})
      if $self-> { lastMouseOver} != $mouseOver;
   $self-> { lastMouseOver} = $mouseOver;
   if ( $self->{pressState}) {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self-> scroll_timer_semaphore;
      $self-> scroll_timer_semaphore(0);
      $self-> notify( 'Increment', $self->{mouseTransaction} == 1 ? 1 : -1);
   } else {
      $self-> scroll_timer_stop;
   }
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> hiliteColor, $self-> hiliteBackColor)     if ( $self-> { default});
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
   my @size = $canvas-> size;
   $canvas-> color( $clr[ 1]);
   $canvas-> bar( 0, 0, $size[0]-1, $size[1]-1);
   my $p = $self->{pressState};

   $canvas-> color( $p == 1 ? $c3d[1] : $c3d[ 0]);
   $canvas-> line( 0, 0, 0, $size[1] - 1);
   $canvas-> line( 1, 1, 1, $size[1] - 2);
   $canvas-> line( 2, $size[1] - 2, $size[0] - 3, $size[1] - 2);
   $canvas-> line( 1, $size[1] - 1, $size[0] - 2, $size[1] - 1);

   $canvas-> color( $p == 2 ? $c3d[0] : $c3d[ 1]);
   $canvas-> line( 1, 0, $size[0] - 1, 0);
   $canvas-> line( 2, 1, $size[0] - 1, 1);
   $canvas-> line( $size[0] - 2, 1, $size[0] - 2, $size[1] - 2);
   $canvas-> line( $size[0] - 1, 1, $size[0] - 1, $size[1] - 1);

   $canvas-> color( $p == 1 ? $c3d[ 0] : $c3d[ 1]);
   $canvas-> line( -1, 0, $size[0] - 2, $size[1] - 1);
   $canvas-> line( 0, 0, $size[0] - 1, $size[1] - 1);
   $canvas-> color( $p == 2 ? $c3d[ 1] : $c3d[ 0]);
   $canvas-> line( 1, 0, $size[0], $size[1] - 1);

   $canvas-> color( $clr[0]);
   my $p1 = ( $p == 1) ? 1 : 0;
   $canvas-> fillpoly([ $size[0] * 0.2 + $p1, $size[1] * 0.65 - $p1,
                      $size[0] * 0.3 + $p1, $size[1] * 0.77 - $p1,
                      $size[0] * 0.4 + $p1, $size[1] * 0.65 - $p1]);
   $p1 = ( $p == 2) ? 1 : 0;
   $canvas-> fillpoly([ $size[0] * 0.6 + $p1, $size[1] * 0.35 - $p1,
                      $size[0] * 0.7 + $p1, $size[1] * 0.27 - $p1,
                      $size[0] * 0.8 + $p1, $size[1] * 0.35 - $p1]);
}

sub set_state
{
   my ( $self, $s) = @_;
   $s = 0 if $s > 2;
   return if $s == $self->{pressState};
   $self-> {pressState} = $s;
   $self-> repaint;
}

package SpinEdit;
use vars qw(@ISA %editProps %spinDynas);
use InputLine;
@ISA = qw(Widget);


%editProps = (
   alignment      => 1, autoScroll  => 1, text        => 1,
   charOffset     => 1, maxLen      => 1, insertMode  => 1, firstChar   => 1,
   selection      => 1, selStart    => 1, selEnd      => 1, writeOnly   => 1,
   copy           => 1, cut         => 1, 'delete'    => 1, paste       => 1,
   wordDelimiters => 1, readOnly    => 1, passwordChar=> 1, focus       => 1,
   select_all     => 1,
);

%spinDynas = ( onIncrement => 1, onTrackEnd => 1,);

for ( keys %editProps) {
   eval <<GENPROC;
   sub $_ { return shift-> {edit}-> $_(\@_); }
GENPROC
}

sub profile_default
{
   my $font = $_[ 0]-> get_default_font;
   my $fh   = $font-> {height} + 2;
   return {
      %{InputLine->profile_default},
      %{$_[ 0]-> SUPER::profile_default},
      ownerBackColor => 1,
      selectable     => 1,
      scaleChildren  => 0,
      min            => 0,
      max            => 100,
      step           => 1,
      value          => 0,
      height         => $fh < 20 ? 20 : $fh,
      editClass      => 'InputLine',
      spinClass      => 'AltSpinButton',
      editProfile    => {},
      spinProfile    => {},
   }
}

sub init
{
   my $self = shift;
   my %profile = @_;
   my $visible = $profile{visible};
   $profile{visible} = 0;
   for (qw( min max step value)) {$self->{$_} = 1;};
   %profile = $self-> SUPER::init(%profile);
   my ( $w, $h) = ( $self-> size);
   $self-> {spin} = $self-> insert( $profile{spinClass} =>
      ownerBackColor => 1,
      name           => 'Spin',
      bottom         => 1,
      right          => $w - 1,
      height         => $h - 1 * 2,
      growMode       => gm::Right,
      (map { $_ => $profile{$_}} keys %spinDynas),
      %{$profile{spinProfile}},
   );
   $self-> {edit} = $self-> insert( $profile{editClass} =>
      name         => 'InputLine',
      origin      => [ 1, 1],
      size        => [ $w - $self->{spin}->width - 1 * 2, $h - 1 * 2],
      growMode    => gm::GrowHiX|gm::GrowHiY,
      selectable  => 1,
      tabStop     => 1,
      borderWidth => 0,
      text        => '',
      (map { $_ => $profile{$_}} keys %editProps),
      %{$profile{editProfile}},
   );
   $self-> {lastEditText} = 0;
   for (qw( min max step value)) {$self->$_($profile{$_});};
   $self-> visible( $visible);
   return %profile;
}

sub set
{
   my ( $self, %profile) = @_;
   my %spinPasse;
   for ( keys %spinDynas) {
      $spinPasse{$_} = $profile{$_}, delete $profile{$_} if exists $profile{$_};
   }
   $self->{list}->set( %spinPasse) if scalar keys %spinPasse;
   $self-> SUPER::set( %profile);
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @s = $canvas-> size;
   $canvas-> rect3d( 0, 0, $s[0]-1, $s[1]-1, 1, $self-> dark3DColor, $self-> light3DColor);
}

sub Spin_Increment
{
   my ( $self, $spin, $increment) = @_;
   my $value = $self-> value;
   $self-> value( $value + $increment * $self->{step});
   $self-> value( $increment > 0 ? $self-> min : $self-> max) if $self-> value == $value;
}

sub InputLine_KeyDown
{
   my ( $self, $edit, $code, $key, $mod) = @_;
   $edit-> clear_event if $key == kb::NoKey and lc( chr $code) =~ m/[a-df-z]/;
}

sub InputLine_Change
{
   my ( $self, $edit) = @_;
   unless ( exists $self->{validDisabled}) {
      my $ch = undef;
      my $value = $edit-> text;
      if ( $value =~ m/^\s*([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?\s*$/) {
         $ch = $value = $self->{min} if $value < $self->{min};
         $ch = $value = $self->{max} if $value > $self->{max};
      } else {
         $ch = $self->{lastEditText};
      }
      $edit-> text( $ch), return if defined $ch;
   }
   $self-> {lastEditText} = $edit-> text;
   $self-> notify(q(Change));
}

sub set_bounds
{
   my ( $self, $min, $max) = @_;
   $max = $min if $max < $min;
   ( $self-> { min}, $self-> { max}) = ( $min, $max);
   my $oldValue = $self->{value};
   $self-> value( $max) if $self->{value} > $max;
   $self-> value( $min) if $self->{value} < $min;
}

sub set_step
{
   my ( $self, $step) = @_;
   $step  = 0 if $step < 0;
   $self-> {step} = $step;
}

sub set_value
{
   my ( $self, $value) = @_;
   if ( $value =~ m/^\s*([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?\s*$/) {
      $value = $self->{min} if $value < $self->{min};
      $value = $self->{max} if $value > $self->{max};
   } else {
      $value = $self->{min};
   }
   return if $value eq $self-> {edit}-> text;
   $self-> {validDisabled} = 1;
   $self-> {edit}-> text( $value);
   delete $self-> {validDisabled};
}

sub min          {($#_)?$_[0]->set_bounds($_[1], $_[0]-> {'max'})      : return $_[0]->{min};}
sub max          {($#_)?$_[0]->set_bounds($_[0]-> {'min'}, $_[1])      : return $_[0]->{max};}
sub step         {($#_)?$_[0]->set_step         ($_[1]):return $_[0]->{step}}
sub value        {($#_)?$_[0]->set_value        ($_[1]):return $_[0]->{edit}->text;}

package Gauge;
use vars qw(@ISA);
@ISA = qw(Widget);

{
my %RNT = (
   %{Widget->notification_types()},
   Stringify => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      indent         => 1,
      relief         => -1,       # -1 = sink, 0 = border, 1 = raise
      ownerBackColor => 1,
      hiliteBackColor=> cl::Blue,
      hiliteColor    => cl::White,
      min            => 0,
      max            => 1,
      value          => 0,
      threshold      => 0,
      vertical       => 0,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   for (qw( relief value indent min max threshold vertical))
     {$self->{$_} = 0}
   $self-> {__DYNAS__}->{onStringify} = $profile{onStringify};
   $self->{string} = '';
   for (qw( vertical threshold min max relief indent value))
     {$self->$_($profile{$_}); }
   return %profile;
}

sub setup
{
   $_[0]->SUPER::setup;
   $_[0]->value($_[0]->{value});
}

sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onStringify} = $profile{onStringify},
      delete $profile{onStringify} if exists $profile{onStringify};
   $self-> SUPER::set( %profile);
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my ($x, $y) = $canvas-> get_size;
   my $i = $self-> indent;
   my ($clComplete,$clBack,$clFore,$clHilite) = ($self-> hiliteBackColor, $self-> backColor, $self-> color, $self-> hiliteColor);
   my $v = $self-> {vertical};
   my $complete = $v ? $y : $x;
   $complete = int(($complete - $i*2) * $self->{value} / ($self-> {max} - $self-> {min}) + 0.5);
   my ( $l3, $d3) = ( $self-> light3DColor, $self-> dark3DColor);
   $canvas-> color( $clComplete);
   $canvas-> bar ( $v ? ($i, $i, $x-$i-1, $i+$complete) : ( $i, $i, $i + $complete, $y-$i-1));
   $canvas-> color( $clBack);
   $canvas-> bar ( $v ? ($i, $i+$complete+1, $x-$i-1, $y-$i-1) : ( $i+$complete+1, $i, $x-$i-1, $y-$i-1));
   # draw the border
   my $relief = $self-> relief;
   $canvas-> color(( $relief < 0) ? $d3 : (( $relief == 0) ? cl::Black : $l3));
   for ( my $j = 0; $j < $i; $j++)
   {
      $canvas-> line( $j, $j, $j, $y - $j - 1);
      $canvas-> line( $j, $y - $j - 1, $x - $j - 1, $y - $j - 1);
   }
   $canvas-> color(( $relief < 0) ? $l3 : (( $relief == 0) ? cl::Black : $d3));
   for ( my $j = 0; $j < $i; $j++)
   {
      $canvas-> line( $j + 1, $j, $x - $j - 1, $j);
      $canvas-> line( $x - $j - 1, $j, $x - $j - 1, $y - $j - 1);
   }

   # draw the text, if neccessary
   my $s = $self->{string};
   if ( $s ne '')
   {
      my ($fw, $fh) = ( $canvas-> get_text_width( $s), $canvas-> font-> height);
      my $xBeg = int(( $x - $fw) / 2 + 0.5);
      my $xEnd = $xBeg + $fw;
      my $yBeg = int(( $y - $fh) / 2 + 0.5);
      my $yEnd = $yBeg + $fh;
      my ( $zBeg, $zEnd) = $v ? ( $yBeg, $yEnd) : ( $xBeg, $xEnd);
      if ( $zBeg > $i + $complete)
      {
         $canvas-> color( $clFore);
         $canvas-> text_out( $s, $xBeg, $yBeg);
      }
      elsif ( $zEnd < $i + $complete + 1)
      {
         $canvas-> color( $clHilite);
         $canvas-> text_out( $s, $xBeg, $yBeg);
      }
      else
      {
         $canvas-> clipRect( $v ? ( 0, 0, $x, $i + $complete) : ( 0, 0, $i + $complete, $y));
         $canvas-> color( $clHilite);
         $canvas-> text_out( $s, $xBeg, $yBeg);
         $canvas-> clipRect( $v ? ( 0, $i + $complete + 1, $x, $y) : ( $i + $complete + 1, 0, $x, $y));
         $canvas-> color( $clFore);
         $canvas-> text_out( $s, $xBeg, $yBeg);
      }
   }
}

sub set_bounds
{
   my ( $self, $min, $max) = @_;
   $max = $min if $max < $min;
   ( $self-> { min}, $self-> { max}) = ( $min, $max);
   my $oldValue = $self->{value};
   $self-> value( $max) if $self->{value} > $max;
   $self-> value( $min) if $self->{value} < $min;
}

sub value
{
   return $_[0]-> {value} unless $#_;
   my $v = $_[1] < $_[0]-> {min} ? $_[0]-> {min} : ($_[1] > $_[0]-> {max} ? $_[0]-> {max} : $_[1]);
   my $old = $_[0]-> {value};
   if (abs($old - $v) >= $_[0]-> {threshold})
   {
      my ($x, $y) = $_[0]-> get_size;
      my $i = $_[0]-> {indent};
      my $x1 = $i + ($x - $i*2) * $old / ($_[0]-> {max} - $_[0]-> {min});
      my $x2 = $i + ($x - $i*2) * $v   / ($_[0]-> {max} - $_[0]-> {min});
      if ( $x1 > $x2)
      {
         $i = $x1;
         $x1 = $x2;
         $x2 = $i;
      }
      my $s = $_[0]->{string};
      $_[0]-> {value} = $v;
      $_[0]-> notify(q(Stringify), $v, \$_[0]->{string});
      ( $_[0]->{string} eq $s) ?
         $_[0]-> invalidate_rect( $x1, 0, $x2+1, $y) :
         $_[0]-> repaint;
   }
}

sub on_stringify
{
   my ( $self, $value, $sref) = @_;
   $$sref = sprintf( "%2d%%", $value * 100.0 / ($_[0]-> max - $_[0]-> min));
   $self-> clear_event;
}

sub indent    {($#_)?($_[0]->{indent} = $_[1],$_[0]-> repaint)  :return $_[0]->{indent};}
sub relief    {($#_)?($_[0]->{relief} = $_[1],$_[0]-> repaint)  :return $_[0]->{relief};}
sub vertical  {($#_)?($_[0]->{vertical} = $_[1],$_[0]-> repaint):return $_[0]->{vertical};}
sub min       {($#_)?$_[0]->set_bounds($_[1], $_[0]-> {'max'})  : return $_[0]->{min};}
sub max       {($#_)?$_[0]->set_bounds($_[0]-> {'min'}, $_[1])  : return $_[0]->{max};}
sub threshold {($#_)?($_[0]->{threshold} = $_[1]):return $_[0]->{threshold};}

# slider standard schemes
package ss;
use constant Gauge        => 0;
use constant Axis         => 1;
use constant Thermometer  => 2;
use constant StdMinMax    => 3;

package AbstractSlider;
use vars qw(@ISA);
@ISA = qw(Widget);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      increment      => 10,
      min            => 0,
      max            => 100,
      ownerBackColor => 1,
      readOnly       => 0,
      scheme         => undef,
      selectable     => 1,
      snap           => 0,
      step           => 1,
      ticks          => undef,
      value          => 0,
      widgetClass    => wc::Slider,
   }
}


sub init
{
   my $self = shift;
   for ( qw( min max readOnly snap value))
      {$self->{$_}=0}
   for ( qw( tickVal tickLen tickTxt )) { $self->{$_} = [] };
   my %profile = $self-> SUPER::init( @_);
   for ( qw( step min max increment readOnly ticks snap value))
     {$self->$_($profile{$_});}
   $self-> scheme( $profile{scheme}, 1) if defined $profile{scheme};
   return %profile;
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}


sub set_read_only
{
   $_[0]-> {readOnly} = $_[1];
   $_[0]-> repaint;
   $_[0]-> notify(q(MouseUp),0,0,0) if defined $_[0]->{mouseTransaction};
}


sub set_snap
{
   $_[0]->{snap} = $_[1];
   $_[0]->value( $_[0]-> value) if $_[1];
}

sub set_step
{
   my $i = $_[1];
   $i = 1 if $i == 0;
   $_[0]->{step} = $i;
}


sub get_ticks
{
   my $self  =  $_[0];
   my $i;
   my ( $tv, $tl, $tt) = ($self->{tickVal}, $self->{tickLen}, $self->{tickTxt});
   my @t = ();
   for ( $i = 0; $i < scalar @{$tv}; $i++) {
      push ( @t, { value => $$tv[$i], height => $$tl[$i], text => $$tt[$i] });
   }
   return @t;
}

sub set_ticks
{
   my $self  = shift;
   return unless defined $_[0];
   my @ticks = (@_ == 1 and ref($_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;
   my @val = ();
   my @len = ();
   my @txt = ();
   for ( @ticks) {
      next unless exists $$_{value};
      push( @val, $$_{value});
      push( @len, exists($$_{height})    ? $$_{height}    : 0);
      push( @txt, exists($$_{text})      ? $$_{text}      : undef);
   }
   $self->{tickVal} = \@val;
   $self->{tickLen} = \@len;
   $self->{tickTxt} = \@txt;
   $self-> value( $self-> value);
   $self-> repaint;
}

sub set_bound
{
   my ( $self, $val, $bound) = @_;
   $self->{$bound} = $val;
   $self->repaint;
}


sub set_scheme
{
   my ( $self, $s, $addFlag) = @_;
   return unless defined $s;
   my ( $max, $min) = ( $self->{max}, $self->{min});
   ( $addFlag ? 0 : $self-> ticks([])), return if $max == $min;
   my @t = $addFlag ? $self-> ticks : ();
   my $i;
   my $inc = $self->{increment};
   if ( $s == ss::Gauge) {
      for ( $i = $min; $i <= $max; $i += $inc) {
         push ( @t, { value => $i, height => 4, text => $i });
      }
   } elsif ( $s == ss::Axis) {
      for ( $i = $min; $i <= $max; $i += $inc) {
         push ( @t, { value => $i, height => 6,   text => $i });
         if ( $i < $max) {
            push ( @t, { value => $i + $inc / 5 * 1, height => 3 });
            push ( @t, { value => $i + $inc / 5 * 2, height => 3 });
            push ( @t, { value => $i + $inc / 5 * 3, height => 3 });
            push ( @t, { value => $i + $inc / 5 * 4, height => 3 });
         }
      }
   } elsif ( $s == ss::StdMinMax) {
      push ( @t, { value => $min, height => 6,   text => "Min" });
      push ( @t, { value => $max, height => 6,   text => "Max" });
   } elsif ( $s == ss::Thermometer) {
      for ( $i = $min; $i <= $max; $i += $inc) {
         push ( @t, { value => $i, height => 6,   text => $i });
         if ( $i < $max) {
            my $j;
            for ( $j = 1; $j < 10; $j++) {
               push ( @t, { value => $i + $inc / 10 * $j, height => $j == 5 ? 5 : 3 });
            }
         }
      }
   }
   $self-> ticks( @t);
}

sub increment   {($#_)?$_[0]-> {increment}   =  $_[1] :return $_[0]->{increment};}
sub readOnly    {($#_)?$_[0]-> set_read_only   ($_[1]):return $_[0]->{readOnly};}
sub ticks       {($#_)?shift-> set_ticks          (@_):return $_[0]->get_ticks;}
sub snap        {($#_)?$_[0]-> set_snap        ($_[1]):return $_[0]->{snap};}
sub step        {($#_)?$_[0]-> set_step        ($_[1]):return $_[0]->{step};}
sub scheme      {($#_)?shift-> set_scheme         (@_):$_[0]->raise_wo("scheme");}
sub value       {($#_)?$_[0]-> set_value       ($_[1]):return $_[0]->{value};}
sub min         {($#_)?$_[0]-> set_bound($_[1],q(min)):return $_[0]->{min};}
sub max         {($#_)?$_[0]-> set_bound($_[1],q(max)):return $_[0]->{max};}


# linear slider tick alignment
package ta;
use constant Normal      => 0;
use constant Alternative => 1;
use constant Dual        => 2;

package Slider;
use vars qw(@ISA);
@ISA = qw(AbstractSlider);

{
my %RNT = (
   %{Widget->notification_types()},
   TrackEnd   => nt::Default,
);
sub notification_types { return \%RNT; }
}

use constant DefButtonX => 12;

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      ribbonStrip    => 0,
      shaftBreadth   => 6,
      tickAlign      => ta::Normal,
      vertical       => 0,
      onTrackEnd     => undef,
   }
}

sub init
{
   my $self = shift;
   for ( qw( vertical tickAlign ribbonStrip shaftBreadth))
      {$self->{$_}=0}
   my %profile = $self-> SUPER::init( @_);
   for ( qw( vertical tickAlign ribbonStrip shaftBreadth))
     {$self->$_($profile{$_});}
   $self-> {__DYNAS__}->{onTrackEnd}   = $profile{onTrackEnd};
   return %profile;
}

sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onTrackEnd} = $profile{onTrackEnd},
      delete $profile{onTrackEnd} if exists $profile{onTrackEnd};
   $self-> SUPER::set( %profile);
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> dark3DColor, $self-> light3DColor);
   my @cht  = ( $self-> hiliteColor, $self-> hiliteBackColor);
   my @size = $canvas-> size;
   my ( $sb, $v, $range, $min, $tval, $tlen, $ttxt, $ta) =
   ( $self->{shaftBreadth}, $self->{vertical}, abs($self->{max} - $self->{min}), $self->{min},
     $self->{tickVal}, $self-> {tickLen}, $self->{tickTxt}, $self->{tickAlign},
   );
   if ( $ta == ta::Normal)      { $ta = 1; } elsif
      ( $ta == ta::Alternative) { $ta = 2; } else { $ta = 3; }

   unless ( $self->transparent) {
      $canvas-> color( $clr[1]);
      $canvas-> bar(0,0,@size);
   }
   $sb = ( $v ? $size[0] : $size[1]) / 6 unless $sb;
   $sb = 2 unless $sb;
   if ( $v) {
      my $bh = $canvas->font->height;
      my $bw  = ( $size[0] - $sb) / 2;
      return if $size[1] <= DefButtonX * ($self->{readOnly} ? 1 : 0) + 2 * $bh + 2;
      my $br  = $size[1] - 2 * $bh - 2;
      $canvas-> rect3d( $bw, $bh, $bw + $sb - 1, $bh + $br - 1, 1, @c3d, $cht[1]), return unless $range;
      my $val = $bh + 1 + abs( $self->{value} - $min) * ( $br - 3) / $range;
      if ( $self->{ribbonStrip}) {
         $canvas-> rect3d( $bw, $bh, $bw + $sb - 1, $bh + $br - 1, 1, @c3d);
         $canvas-> color( $cht[0]);
         $canvas-> bar( $bw + 1, $bh + 1, $bw + $sb - 2, $val);
         $canvas-> color( $cht[1]);
         $canvas-> bar( $bw + 1, $val + 1, $bw + $sb - 2, $bh + $br - 2);
      } else {
         $canvas-> rect3d( $bw, $bh, $bw + $sb - 1, $bh + $br - 1, 1, @c3d, $cht[1]);
         $canvas-> color( $clr[0]);
         $canvas-> line( $bw + 1, $val, $bw + $sb - 2, $val) if $self->{readOnly};
      }
      my $i;
      $canvas-> color( $clr[0]);
      for ( $i = 0; $i < scalar @{$tval}; $i++) {
         my $val = $bh + 1 + abs( $$tval[$i] - $min) * ( $br - 3) / $range;
         if ( $$tlen[ $i]) {
            $canvas-> line( $bw + $sb + 3, $val, $bw + $sb + $$tlen[ $i] + 3, $val) if $ta & 2;
            $canvas-> line( $bw - 4, $val, $bw - 4 - $$tlen[ $i], $val) if $ta & 1;
         }
         $canvas-> text_out( $$ttxt[ $i],
            ( $ta == 2) ? $bw + $sb + $$tlen[ $i] + 5 : $bw - $$tlen[ $i] - 5 - $canvas-> get_text_width( $$ttxt[ $i]),
            $val - $bh / 2
         ) if defined $$ttxt[ $i];
      }
      unless ( $self->{readOnly}) {
         my @jp = (
                     $bw - 4,       $val - DefButtonX / 2,
                     $bw - 4,       $val + DefButtonX / 2,
                     $bw + $sb + 1, $val + DefButtonX / 2,
                     $bw + $sb + 8, $val,
                     $bw + $sb + 1, $val - DefButtonX / 2,
                  );
         $canvas-> color( $clr[1]);
         $canvas-> fillpoly( \@jp);
         $canvas-> color( $c3d[0]);
         $canvas-> polyline([@jp[6..9, 0, 1]]);
         $canvas-> line($bw - 3, $jp[7]+1, $jp[6]-1, $jp[7]+1);
         $canvas-> color( $c3d[1]);
         $canvas-> polyline([@jp[0..7]]);
         $canvas-> line($bw - 3, $jp[7]-1, $jp[6]-1, $jp[7]-1);
      }
   } else {
      my $bw = $canvas->font->width;
      my $bh  = ( $size[1] - $sb) / 2;
      my $fh = $canvas-> font-> height;
      return if $size[0] <= DefButtonX * ($self->{readOnly} ? 1 : 0) + 2 * $bw + 2;
      my $br  = $size[0] - 2 * $bw - 2;
      $canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d, $cht[1]), return unless $range;
      my $val = $bw + 1 + abs( $self->{value} - $min) * ( $br - 3) / $range;
      if ( $self->{ribbonStrip}) {
         $canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d);
         $canvas-> color( $cht[0]);
         $canvas-> bar( $bw+1, $bh+1, $val, $bh + $sb - 2);
         $canvas-> color( $cht[1]);
         $canvas-> bar( $val+1, $bh+1, $bw + $br - 2, $bh + $sb - 2);
      } else  {
         $canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d, $cht[1]);
         $canvas-> color( $clr[0]);
         $canvas-> line( $val, $bh+1, $val, $bh + $sb - 2) if $self->{readOnly};
      }
      my $i;
      $canvas-> color( $clr[0]);
      for ( $i = 0; $i < scalar @{$tval}; $i++) {
         my $val = 1 + $bw + abs( $$tval[$i] - $min) * ( $br - 3) / $range;
         if ( $$tlen[ $i]) {
            $canvas-> line( $val, $bh + $sb + 3, $val, $bh + $sb + $$tlen[ $i] + 3) if $ta & 1;
            $canvas-> line( $val, $bh - 4, $val, $bh - 4 - $$tlen[ $i]) if $ta & 2;
         }
         $canvas-> text_out( $$ttxt[ $i],
            $val - $canvas-> get_text_width( $$ttxt[ $i]) / 2,
            ( $ta == 2) ? $bh - $$tlen[ $i] - 5 - $fh : $bh + $sb + $$tlen[ $i] + 5
         ) if defined $$ttxt[ $i];
      }
      unless ( $self->{readOnly}) {
         my @jp = (
                     $val - DefButtonX / 2, $bh - 2,
                     $val - DefButtonX / 2, $bh + $sb + 3,
                     $val + DefButtonX / 2, $bh + $sb + 3,
                     $val + DefButtonX / 2, $bh - 2,
                     $val,                  $bh - 9,
                  );
         $canvas-> color( $clr[1]);
         $canvas-> fillpoly( \@jp);
         $canvas-> color( $c3d[0]);
         $canvas-> polyline([@jp[4..9]]);
         $canvas-> line($val-1,$jp[3]-1,$val-1,$jp[9]+1);
         $canvas-> color( $c3d[1]);
         $canvas-> polyline([@jp[8,9,0..5]]);
         $canvas-> line($val+1,$jp[3]-1,$val+1,$jp[9]+1);
      }
   }
}

sub pos2info
{
   my ( $self, $x, $y) = @_;
   my @size = $self-> size;
   return if $self->{max} == $self->{min};
   if ( $self-> {vertical}) {
      my $bh  = $self-> font-> height;
      my $val = $bh + 1 + abs( $self->{value} - $self->{min}) *
                ( $size[1] - 2 * $bh - 5) / abs($self->{max} - $self->{min});
      my $ret1 = $self->{min} + ( $y - $bh - 1) * abs($self->{max} - $self->{min}) / ( $size[1] - 2 * $bh - 5);
      if ( $y < $val - DefButtonX / 2 or $y >= $val + DefButtonX / 2) {
         return 0, $ret1;
      } else {
         return 1, $ret1, $y - $val;
      }
   } else {
      my $bw = $self->font->width;
      my $val = $bw + 1 + abs( $self->{value} - $self->{min}) *
                ( $size[0] - 2 * $bw - 5) / abs($self->{max} - $self->{min});
      my $ret1 = $self->{min} + ( $x - $bw - 1) * abs($self->{max} - $self->{min}) / ( $size[0] - 2 * $bw - 5);
      if ( $x < $val - DefButtonX / 2 or $x >= $val + DefButtonX / 2) {
         return 0, $ret1;
      } else {
         return 1, $ret1, $x - $val;
      }
   }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{readOnly};
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;
   my ($info, $pos, $ap) = $self-> pos2info( $x, $y);
   return unless defined $info;
   if ( $info == 0) {
      $self-> value( $pos);
      return;
   }
   $self-> {aperture} = $ap;
   $self->{mouseTransaction} = 1;
   $self-> capture(1);
   $self-> clear_event;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self->{mouseTransaction};
   $self-> {mouseTransaction} = undef;
   $self-> capture(0);
   $self-> notify( 'TrackEnd');
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   $self->{vertical} ? ( $y -= $self->{aperture}) : ( $x -= $self->{aperture});
   my ( $info, $pos) = $self-> pos2info( $x, $y);
   return unless defined $info;
   $self-> value( $pos);
}


sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   return if $self-> {readOnly};
   if ( $key == kb::Home || $key == kb::PgUp) {
      $self-> value( $self-> {vertical} ? $self-> {max} : $self-> {min});
      $self-> clear_event;
      return;
   }
   if ( $key == kb::End || $key == kb::PgDn) {
      $self-> value( $self-> {vertical} ? $self-> {min} : $self-> {max});
      $self-> clear_event;
      return;
   }
   if ( $key == kb::Left || $key == kb::Right || $key == kb::Up || $key == kb::Down) {
       my $s = $self->{step};
       $self-> clear_event;
       my $dir = ( $key == kb::Left || $key == kb::Down) ? -$s : $s;
       $dir *= -1 if $self-> {min} > $self-> {max};
       $dir *= -1 if $self-> {vertical};
       if ( $self-> snap) {
          my $v = $self-> value;
          my $w = $v;
          return if ( $v + $dir > $self->{min} and $v + $dir > $self->{max}) or
                    ( $v + $dir < $self->{min} and $v + $dir < $self->{max});
          $self-> value( $v += $dir) while $self->{value} == $w;
       } else {
          $self-> value( $self-> value + $dir);
       }
   }
}

sub set_vertical
{
   $_[0]-> {vertical} = $_[1];
   $_[0]-> repaint;
}

sub set_tick_align
{
   my ( $self, $ta) = @_;
   $ta = ta::Normal if $ta != ta::Alternative and $ta != ta::Dual;
   return if $ta == $self-> {tickAlign};
   $self-> {tickAlign} = $ta;
   $self-> repaint;
}

sub set_ribbon_strip
{
   $_[0]-> {ribbonStrip} = $_[1];
   $_[0]-> repaint;
}

sub set_value
{
   my ( $self, $value) = @_;
   my ( $min, $max) = ( $self->{min}, $self->{max});
   my $old = $self-> {value};
   $value = $min if $value < $min;
   $value = $max if $value > $max;
   if ( $self->{snap}) {
      my ( $minDist, $thatVal, $i) = ( abs( $min - $max));
      my $tval = $self->{tickVal};
      for ( $i = 0; $i < scalar @{$tval}; $i++) {
         my $j = $$tval[ $i];
         $minDist = abs(($thatVal = $j) - $value) if abs( $j - $value) < $minDist;
      }
      $value = $thatVal if defined $thatVal;
   }
   return if $old == $value;
   $self->{value} = $value;
   my @size = $self-> size;
   my $sb = $self->{shaftBreadth};
   if ( $self-> {vertical}) {
      $sb = $size[0] / 6 unless $sb;
      $sb = 2 unless $sb;
      my $bh = $self->font->height;
      my $bw  = ( $size[0] - $sb) / 2;
      my $v1  = $bh + 1 + abs( $self->{value} - $self->{min}) *
         ( $size[1] - 2 * $bh - 5) / abs($self->{max} - $self->{min});
      my $v2  = $bh + 1 + abs( $old - $self->{min}) *
         ( $size[1] - 2 * $bh - 5) / abs($self->{max} - $self->{min});
      ( $v2, $v1) = ( $v1, $v2) if $v1 > $v2;
      $v1 -= DefButtonX / 2;
      $v2 += DefButtonX / 2 + 1;
      $self-> invalidate_rect( $bw - 4, $v1, $bw + $sb + 9, $v2);
   } else {
      $sb = $size[1] / 6 unless $sb;
      $sb = 2 unless $sb;
      my $bw = $self->font->width;
      my $bh  = ( $size[1] - $sb) / 2;
      my $v1  = $bw + 1 + abs( $self->{value} - $self->{min}) *
         ( $size[0] - 2 * $bw - 5) / abs($self->{max} - $self->{min});
      my $v2  = $bw + 1 + abs( $old - $self->{min}) *
         ( $size[0] - 2 * $bw - 5) / abs($self->{max} - $self->{min});
      ( $v2, $v1) = ( $v1, $v2) if $v1 > $v2;
      $v1 -= DefButtonX / 2;
      $v2 += DefButtonX / 2 + 1;
      $self-> invalidate_rect( $v1, $bh - 9, $v2, $bh + $sb + 4);
   }
   $self-> notify(q(Change));
}

sub set_shaft_breadth
{
   my ( $self, $sb) = @_;
   $sb = 0 if $sb < 0;
   return if $sb == $self->{shaftBreadth};
   $self->{shaftBreadth} = $sb;
   $self-> repaint;
}

sub set_bound
{
   my ( $self, $val, $bound) = @_;
   $self->{$bound} = $val;
   $self->repaint;
}

sub vertical    {($#_)?$_[0]-> set_vertical    ($_[1]):return $_[0]->{vertical};}
sub tickAlign   {($#_)?$_[0]-> set_tick_align  ($_[1]):return $_[0]->{tickAlign};}
sub ribbonStrip {($#_)?$_[0]-> set_ribbon_strip($_[1]):return $_[0]->{ribbonStrip};}
sub shaftBreadth{($#_)?$_[0]-> set_shaft_breadth($_[1]):return $_[0]->{shaftBreadth};}

package CircularSlider;
use vars qw(@ISA);
@ISA = qw(AbstractSlider MouseScroller);

{
my %RNT = (
   %{Widget->notification_types()},
   TrackEnd   => nt::Default,
   Stringify  => nt::Action,
);
sub notification_types { return \%RNT; }
}

use constant DefButtonX => 10;

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      buttons        => 1,
      stdPointer     => 0,
      onTrackEnd     => undef,
      onStringify    => undef,
   }
}

sub init
{
   my $self = shift;
   for ( qw( buttons pressState circX circY br butt1X butt1Y butt2X))
      {$self->{$_}=0}
   $self-> {string} = '';
   my %profile = $self-> SUPER::init( @_);
   $self-> {__DYNAS__}->{onTrackEnd}   = $profile{onTrackEnd};
   $self-> {__DYNAS__}->{onStringify}  = $profile{onStringify};
   for ( qw( buttons stdPointer))
     {$self->$_($profile{$_});}
   $self-> reset;
   return %profile;
}

sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onTrackEnd} = $profile{onTrackEnd},
      delete $profile{onTrackEnd} if exists $profile{onTrackEnd};
   $self->{__DYNAS__}->{onStringify} = $profile{onStringify},
      delete $profile{onStringify} if exists $profile{onStringify};
   $self-> SUPER::set( %profile);
}

sub setup
{
   $_[0]->SUPER::setup;
   $_[0]-> notify(q(Stringify), $_[0]->{value}, \$_[0]->{string});
   $_[0]-> repaint;
}

sub set_text
{
   $_[0]-> SUPER::set_text( $_[1]);
   $_[0]-> repaint;
}

sub reset
{
   my $self = $_[0];
   my @size = $self-> size;
   my $fh  = $self-> font-> height;
   my $br  = $size[0] > ( $size[1] - $fh) ? ( $size[1] - $fh) : $size[0];
   $self-> {br}        = $br;
   $self-> {circX}     = $size[0]/2;
   $self-> {circY}     = ($size[1] + $fh) / 2;
   $self-> {butt1X}    = $size[0] / 2 - $br * 0.4 - DefButtonX / 2;
   $self-> {butt1Y}    = ( $size[1] + $fh) / 2 - $br * 0.4;
   $self-> {butt2X}    = $size[0] / 2 + $br * 0.4 - DefButtonX / 2;
   $self-> {circAlive} = $br > DefButtonX * 5;
}

sub offset2pt
{
   my ( $self, $width, $height, $value, $radius) = @_;
   my $a = 225 * 3.14159 / 180 - ( 270 * 3.14159 / 180) * abs( $value - $self->{min}) /
           abs( $self->{min} - $self->{max});
   return $width + $radius * cos($a), $height + $radius * sin($a);
}

sub offset2data
{
   my ( $self, $value) = @_;
   my $a = 225 * 3.14159 / 180 - ( 270 * 3.14159 / 180) * abs( $value - $self->{min})/
           abs( $self->{min} - $self->{max});
   return cos($a), sin($a);
}


sub on_paint
{
   my ( $self, $canvas) = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> dark3DColor, $self-> light3DColor);
   my @cht  = ( $self-> hiliteColor, $self-> hiliteBackColor);
   my @size = $canvas-> size;
    my ( $range, $min, $tval, $tlen, $ttxt) =
   ( abs($self->{max} - $self->{min}), $self->{min}, $self->{tickVal},
     $self-> {tickLen}, $self->{tickTxt} );

   if ( defined $self->{singlePaint}) {
      my @clip1 = @{$self->{expectedClip}};
      my @clip2 = $self-> clipRect;
      my $i;
      for ( $i = 0; $i < 4; $i++) {
         $self->{singlePaint} = undef, last if $clip1[$i] != $clip2[$i];
      }
   }

   $canvas-> color( $clr[1]);
   $canvas-> bar( 0, 0, @size) if !$self-> transparent && !defined $self->{singlePaint};
   my $fh  = $canvas-> font-> height;
   my $br  = $self-> {br};
   my $rad = $br * 0.3;
   my @cpt = ( $self->{circX}, $self->{circY}, $rad, $rad);
   if ( $self-> {circAlive}) {
      if ( defined $self->{singlePaint}) {
         $canvas-> fill_ellipse( @cpt[0..1], $rad-3, $rad-3);
         $canvas-> color( $clr[0]);
      } else {
         $canvas-> color( $c3d[1]);
         $canvas-> lineWidth(2);
         $canvas-> arc( @cpt[0..1], $cpt[2]-1, $cpt[3]-1, 65, 235);
         $canvas-> color( $c3d[0]);
         $canvas-> arc( @cpt[0..1], $cpt[2]-1, $cpt[3]-1, 255, 405);
         $canvas-> lineWidth(0);
         $canvas-> color( $clr[0]);
         $canvas-> ellipse( @cpt);
      }

      if ( $self->{stdPointer}) {
         my $dev = $range * 0.03;
         my @j = (
            $self-> offset2pt( @cpt[0,1], $self->{value}, $rad * 0.8),
            $self-> offset2pt( @cpt[0,1], $self->{value} + $dev, $rad * 0.6),
            $self-> offset2pt( @cpt[0,1], $self->{value} - $dev, $rad * 0.6),
         );
         $self-> fillpoly( \@j);
      } else {
         my @cxt = ( $self-> offset2pt( @cpt[0,1], $self->{value}, $rad - 10), 4, 4);
         $canvas-> lineWidth(2);
         $canvas-> color( $c3d[0]);
         $canvas-> arc( @cxt[0..1], 3, 3, 65, 235);
         $canvas-> color( $c3d[1]);
         $canvas-> arc( @cxt[0..1], 3, 3, 255, 405);
         $canvas-> lineWidth(0);
         $canvas-> color( $clr[0]);
      }

      my $i;
      unless ( defined $self->{singlePaint}) {
         for ( $i = 0; $i < scalar @{$tval}; $i++) {
            my $r = $rad + 3 + $$tlen[ $i];
            my ( $cos, $sin) = $self-> offset2data( $$tval[$i]);
            $canvas-> line( $self-> offset2pt( @cpt[0,1], $$tval[$i], $rad + 3),
               $cpt[0] + $r * $cos, $cpt[1] + $r * $sin
            ) if $$tlen[ $i];
            $r += 3;
            if ( defined $$ttxt[ $i]) {
               my $y = $cpt[1] + $r * $sin - $fh / 2 * ( 1 - $sin);
               my $x = $cpt[0] + $r * $cos - ( 1 - $cos) * $canvas-> get_text_width( $$ttxt[ $i]) / 2;
               $canvas-> text_out( $$ttxt[ $i], $x, $y);
            }
         }
      }
   } else {
      $canvas-> color( $clr[0]);
   }

   my $ttw = $canvas-> get_text_width( $self-> {string});
   $canvas-> text_out( $self-> {string}, ( $size[0] - $ttw) / 2, $size[1] / 2 - $fh / 3)
     if $ttw < $rad || !$self->{circAlive};
   return if defined $self->{singlePaint};

   $ttw = $canvas-> get_text_width( $self-> text);
   $canvas-> text_out( $self-> text, ( $size[0] - $ttw) / 2, 2);

   if ( $self-> {buttons}) {
      my $s = $self-> {pressState};
      my @cbd = reverse @c3d;
      my $at  = 0;
      $at = 1, @cbd = reverse @cbd if $s & 1;
      $canvas-> rect3d( $self-> { butt1X}, $self-> { butt1Y}, $self-> { butt1X} + DefButtonX,
         $self-> { butt1Y} + DefButtonX, 1, @cbd, $clr[1]);
      $canvas-> line( $self-> { butt1X} + 2 + $at, $self-> { butt1Y} + DefButtonX / 2 - $at,
         $self-> { butt1X} - 2 + + DefButtonX + $at, $self-> {butt1Y} + DefButtonX / 2 - $at);
      @cbd = reverse @c3d; $at = 0;
      $at = 1, @cbd = reverse @cbd if $s & 2;
      $canvas-> rect3d( $self-> { butt2X}, $self-> { butt1Y}, $self-> { butt2X} + DefButtonX,
         $self-> { butt1Y} + DefButtonX, 1, @cbd, $clr[1]);
      $canvas-> line( $self-> { butt2X} + 2 + $at, $self-> { butt1Y} + DefButtonX / 2 - $at,
         $self-> { butt2X} - 2 + + DefButtonX + $at, $self-> {butt1Y} + DefButtonX / 2 - $at);
      $canvas-> line( $self-> { butt2X} + DefButtonX / 2 + $at, $self-> { butt1Y} + 2 - $at,
         $self-> { butt2X} + DefButtonX / 2 + $at, $self-> { butt1Y} - 2 - $at + DefButtonX);
   }

   $canvas-> rect_focus(( $size[0] - $ttw) / 2 - 1, 1, ( $size[0] + $ttw) / 2 + 1, $fh + 2)
      if $self-> focused && ( length( $self-> text) > 0);
}

sub on_keydown
{
   my ( $self, $code, $key, $mod) = @_;
   return if $self-> {readOnly};
   if ( $key == kb::Home || $key == kb::PgUp) {
      $self-> value( $self-> {min});
      $self-> clear_event;
      return;
   }
   if ( $key == kb::End || $key == kb::PgDn) {
      $self-> value( $self-> {max});
      $self-> clear_event;
      return;
   }
   if ( $key == kb::Left || $key == kb::Right || $key == kb::Up || $key == kb::Down) {
       my $s = $self->{step};
       $self-> clear_event;
       my $dir = ( $key == kb::Left || $key == kb::Down) ? -$s : $s;
       $dir *= -1 if $self-> {min} > $self-> {max};
       if ( $self-> snap) {
          my $v = $self-> value;
          my $w = $v;
          return if ( $v + $dir > $self->{min} and $v + $dir > $self->{max}) or
                    ( $v + $dir < $self->{min} and $v + $dir < $self->{max});
          $self-> value( $v += $dir) while $self->{value} == $w;
       } else {
          $self-> value( $self-> value + $dir);
       }
   }
}

sub xy2val
{
   my ( $self, $x, $y) = @_;
   $x -= $self->{circX};
   $y -= $self->{circY};
   my $a  = atan2( $y, $x);
   my $pi = atan2( 0, -1);
   $a += $pi / 2;
   $a += $pi * 2 if $a < 0;
   $a = $self->{min} + abs( $self->{max} - $self->{min}) * ( $pi * 1.75 - $a) * 2 / ( 3 * $pi);
   my $s = $self->{step};
   $a = int( $a) if int( $s) - $s == 0;
   my $inCircle = ( abs($x) < $self->{br} * 0.3 + 3 and abs($y) < $self->{br} * 0.3 + 3);
   return $a, $inCircle;
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{readOnly};
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;
   my @butt = ( $self->{butt1X}, $self->{butt1Y}, $self->{butt2X},
                $self->{butt1X} + DefButtonX, $self->{butt1Y} + DefButtonX, $self->{butt2X} + DefButtonX);
   if ( $self->{buttons} and $y >= $butt[1] and $y < $butt[4]) {
      if ( $x >= $butt[0] and $x < $butt[3]) {
         $self->{pressState} = 1;
         $self-> invalidate_rect( @butt[0..1], $butt[3] + 1, $butt[4] + 1);
      }
      if ( $x >= $butt[2] and $x < $butt[5]) {
         $self->{pressState} = 2;
         $self-> invalidate_rect( $butt[2], $butt[1], $butt[5] + 1, $butt[4] + 1);
      }
      if ( $self->{pressState} > 0) {
         $self-> {mouseTransaction} = $self-> {pressState};
         $self-> update_view;
         $self-> capture(1);
         $self-> scroll_timer_start;
         $self-> scroll_timer_semaphore(0);
         $self-> value( $self-> value + $self-> step * (($self->{pressState} == 1) ? -1 : 1));
         return;
      }
   }

   my ( $val, $inCircle) = $self-> xy2val( $x, $y);
   return unless $inCircle;
   $self->{mouseTransaction} = 3;
   $self-> value( $val);
   $self-> capture(1);
   $self-> clear_event;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self->{mouseTransaction};
   my @butt = ( $self->{butt1X}, $self->{butt1Y}, $self->{butt2X},
                $self->{butt1X} + DefButtonX, $self->{butt1Y} + DefButtonX, $self->{butt2X} + DefButtonX);
   $self-> scroll_timer_stop;
   $self-> {pressState} = 0;
   if ( $self->{mouseTransaction} == 1) {
      $self-> invalidate_rect( @butt[0..1], $butt[3] + 1, $butt[4] + 1);
      $self-> update_view;
   }
   if ( $self->{mouseTransaction} == 2) {
      $self-> invalidate_rect( $butt[2], $butt[1], $butt[5] + 1, $butt[4] + 1);
      $self-> update_view;
   }
   $self-> {mouseTransaction} = undef;
   $self-> capture(0);
   $self-> notify( 'TrackEnd');
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   if ( $self->{mouseTransaction} == 3) {
      $self-> value( $self-> xy2val( $x, $y));
   } elsif ( $self->{pressState} > 0) {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self-> scroll_timer_semaphore;
      $self-> scroll_timer_semaphore(0);
      $self-> value( $self-> value + $self-> step * (( $self->{mouseTransaction} == 1) ? -1 : 1));
   } else {
      $self-> scroll_timer_stop;
   }
}

sub on_mouseclick
{
   my $self = shift;
   $self-> clear_event;
   return unless pop;
   $self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub on_size        { $_[0]-> reset; }
sub on_fontchanged { $_[0]-> reset; }
sub on_enter { $_[0]-> repaint; }
sub on_leave { $_[0]-> repaint; }

sub on_stringify
{
   my ( $self, $value, $sref) = @_;
   $$sref = $value;
   $self-> clear_event;
}


sub set_value
{
   my ( $self, $value) = @_;
   my ( $min, $max) = ( $self->{min}, $self->{max});
   my $old = $self-> {value};
   $value = $min if $value < $min;
   $value = $max if $value > $max;
   if ( $self->{snap}) {
      my ( $minDist, $thatVal, $i) = ( abs( $min - $max));
      my $tval = $self->{tickVal};
      for ( $i = 0; $i < scalar @{$tval}; $i++) {
         my $j = $$tval[ $i];
         $minDist = abs(($thatVal = $j) - $value) if abs( $j - $value) < $minDist;
      }
      $value = $thatVal if defined $thatVal;
   }
   return if $old == $value;
   $self->{value} = $value;
   $self-> notify(q(Stringify), $value, \$self->{string});
   $self-> {singlePaint} = 1;
   my @clip = (
      int( $self-> {circX} - $self-> {br} * 0.3),
      int( $self-> {circY} - $self-> {br} * 0.3),
      int( $self-> {circX} + $self-> {br} * 0.3),
      int( $self-> {circY} + $self-> {br} * 0.3),
   );
   $self-> {expectedClip} = \@clip;
   $self-> invalidate_rect( @clip[0..1], $clip[2]+1, $clip[3]+1);
   $self-> update_view;
   $self-> {singlePaint} = undef;
   $self-> notify(q(Change));
}

sub set_buttons
{
   $_[0]->{buttons} = $_[1];
   $_[0]-> repaint;
}

sub set_std_pointer
{
   $_[0]->{stdPointer} = $_[1];
   $_[0]-> repaint;
}

sub stdPointer  {($#_)?$_[0]-> set_std_pointer ($_[1]):return $_[0]->{stdPointer};}
sub buttons     {($#_)?$_[0]-> set_buttons     ($_[1]):return $_[0]->{buttons};}

1;
