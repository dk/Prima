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
#  Contains:
#       Prima::KeySelector
#  Provides:
#       Control set for assigning and exporting keys

package Prima::KeySelector;

use strict;
use Prima;
use Prima::Buttons;
use Prima::Label;
use Prima::ComboBox;

use vars qw(@ISA %vkeys);
@ISA = qw(Prima::Widget);

for ( keys %kb::) {
   next if $_ eq 'constant';
   next if $_ eq 'AUTOLOAD';
   next if $_ eq 'CharMask';
   next if $_ eq 'CodeMask';
   next if $_ eq 'ModMask';
   $vkeys{$_} = &{$kb::{$_}}();
}

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      key => kb::NoKey,
      scaleChildren => 0,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);
   my @sz = $self-> size;
   my $fh = $self-> font-> height;
   $sz[1] -= $fh + 4;
   $self-> {keys} = $self-> insert( ComboBox =>
      name     => 'Keys',
      delegations => [qw(Change)],
      origin   => [ 0, $sz[1]],
      size     => [ $sz[0], $fh + 4],
      growMode => gm::GrowHiX, 
      style    => cs::DropDownList,
      items    => [ sort keys %vkeys, 'A'..'Z', '0'..'9', '+', '-', '*'],
   );

   $sz[1] -= $fh * 4 + 28;
   $self->{mod} = $self-> insert( GroupBox =>
      origin   => [ 0, $sz[1]],
      size     => [ $sz[0], $fh * 4 + 28],
      growMode => gm::GrowHiX, 
      style    => cs::DropDown,
      text     => '',
   );

   my @esz = $self->{mod}->size;
   $esz[1] -= $fh * 2 + 4;
   $self->{modShift} = $self->{mod}->insert( CheckBox =>
      name        => 'Shift',
      delegations => [$self, qw(Click)],
      origin   => [ 8, $esz[1]],
      size     => [ $esz[0] - 16, $fh + 6],
      text     => '~Shift',
      growMode => gm::GrowHiX,
   );

   $esz[1] -= $fh + 8;
   $self->{modCtrl} = $self->{mod}->insert( CheckBox =>
      name        => 'Ctrl',
      delegations => [$self, qw(Click)], 
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 6],
      text    => '~Ctrl',
      growMode => gm::GrowHiX,
   );
   $esz[1] -= $fh + 8;

   $self->{modAlt} = $self->{mod}->insert( CheckBox =>
      name        => 'Alt',
      delegations => [$self, qw(Click)], 
      origin  => [ 8, $esz[1]],
      size    => [ $esz[0] - 16, $fh + 6],
      text    => '~Alt',
      growMode => gm::GrowHiX,
   );

   $sz[1] -= $fh + 8;
   $self-> insert( Label =>
      origin     => [ 0, $sz[1]],
      size       => [ $sz[0], $fh + 2],
      growMode   => gm::GrowHiX,
      text       => 'Press shortcut key:',
   );

   $sz[1] -= $fh + 6;
   $self->{keyhook} = $self->insert( Widget =>
      name        => 'Hook',
      delegations => [qw(Paint KeyDown)],
      origin     => [ 0, $sz[1]],
      size       => [ $sz[0], $fh + 2],
      growMode   => gm::GrowHiX, 
      selectable => 1,
      cursorPos  => [ 4, 1],
      cursorSize => [ 1, $fh],
      cursorVisible => [ 1, $fh],
      tabStop       => 1,
   );
   
   $self-> key( $profile{key});
   return %profile;
}

sub Keys_Change  { $_[0]-> _gather; }
sub Shift_Click  { $_[0]-> _gather; }
sub Ctrl_Click   { $_[0]-> _gather; }
sub Alt_Click    { $_[0]-> _gather; }

sub _gather
{
   my $self = $_[0];
   return if $self-> {blockChange}; 

   my $mod = ( $self-> {modAlt}-> checked ? km::Alt : 0) |
           ( $self-> {modCtrl}-> checked ? km::Ctrl : 0) |
           ( $self-> {modShift}-> checked ? km::Shift : 0);
   my $tx = $self->{keys}->text;
   my $vk = exists $vkeys{$tx} ? $vkeys{$tx} : kb::NoKey;
   my $ck;
   if ( exists $vkeys{$tx}) {
      $ck = 0;
   } elsif (( $mod & km::Ctrl) && ( ord($tx) >= ord('A')) && ( ord($tx) <= ord('z'))) {
      $ck = ord( uc $tx) - ord('@');
   } else {
      $ck = ord( $tx);
   }
   $self-> {key} = Prima::AbstractMenu-> translate_key( $ck, $vk, $mod);
   $self-> notify(q(Change));
}

sub Hook_KeyDown 
{
   my ( $self, $hook, $code, $key, $mod) = @_;
   $self-> key( Prima::AbstractMenu-> translate_key( $code, $key, $mod));
}   

sub Hook_Paint
{
   my ( $self, $hook, $canvas) = @_;
   $canvas-> rect3d( 0, 0, $canvas-> width - 1, $canvas-> height - 1, 1,
       $hook-> dark3DColor, $hook-> light3DColor, $hook-> backColor);
}

sub translate_codes
{
   my ( $data, $useCTRL) = @_;
   my ( $code, $key, $mod);
   if ((( $data & 0xFF) >= ord('A')) && (( $data & 0xFF) <= ord('z'))) {
      $code = $data & 0xFF;
      $key  = kb::NoKey;
   } elsif ((( $data & 0xFF) >= 1) && (( $data & 0xFF) <= 26)) { 
      $code  = $useCTRL ? ( $data & 0xFF) : ord( lc chr(ord( '@') + $data & 0xFF));
      $key   = kb::NoKey; 
      $data |= km::Ctrl;
   } elsif ( $data & 0xFF) {
      $code = $data & 0xFF;
      $key  = kb::NoKey;
   } else {
      $code = 0;
      $key  = $data & kb::CodeMask;
   }
   $mod = $data & kb::ModMask;   
   return $code, $key, $mod;
}

sub key
{
   return $_[0]-> {key} unless $#_;
   my ( $self, $data) = @_;
   my ( $code, $key, $mod) = translate_codes( $data, 0);
   
   if ( $code) {
      $self-> {keys}-> text( chr($code));
   } else {
      my $x = 'NoKey';
      for ( keys %vkeys) {
         next if $_ eq 'constant';
         $x = $_, last if $key == $vkeys{$_};
      }
      $self-> {keys}-> text( $x);
   }
   $self-> {key} = $data;
   $self-> {blockChange} = 1;
   $self-> {modAlt}-> checked( $mod & km::Alt);
   $self-> {modCtrl}-> checked( $mod & km::Ctrl);
   $self-> {modShift}-> checked( $mod & km::Shift);
   delete $self-> {blockChange};
   $self-> notify(q(Change));
}

# static functions

# exports binary value to a reasonable and perl-evaluable expression

sub export
{
   my $data = $_[0];
   my ( $code, $key, $mod) = translate_codes( $data, 1);
   my $txt = '';
   if ( $code) {
      if (( $code >= 1) && ($code <= 26)) {
         $code += ord('@');
         $txt = '(ord(\''.uc chr($code).'\')-64)'; 
      } else {
         $txt = 'ord(\''.lc chr($code).'\')'; 
      }
   } else {
      my $x = 'NoKey';
      for ( keys %vkeys) {
         next if $_ eq 'constant';
         $x = $_, last if $vkeys{$_} == $key;
      }
      $txt = 'kb::'.$x;
   }
   $txt .= '|km::Alt' if $mod & km::Alt;
   $txt .= '|km::Ctrl' if $mod & km::Ctrl;
   $txt .= '|km::Shift' if $mod & km::Shift;
   return $txt;
}

# creates a key description, suitable for a menu accelerator text

sub describe
{
   my $data = $_[0];
   my ( $code, $key, $mod) = translate_codes( $data, 0);
   my $txt = '';
   my $lonekey;
   if ( $code) {
      $txt = uc chr $code;
   } elsif ( $key == kb::NoKey) {
      $lonekey = 1;
   } else {
      for ( keys %vkeys) {
         next if $_ eq 'constant';
         $txt = $_, last if $vkeys{$_} == $key;
      }
   }
   $txt = 'Shift+' . $txt if $mod & km::Shift;
   $txt = 'Alt+'   . $txt if $mod & km::Alt;
   $txt = 'Ctrl+'  . $txt if $mod & km::Ctrl; 
   $txt =~ s/\+$// if $lonekey;
   return $txt;
}

# exports binary value to AbstractMenu-> translate_shortcut input

sub shortcut
{
   my $data = $_[0];
   my ( $code, $key, $mod) = translate_codes( $data, 0);
   my $txt = '';

   if ( $code || (( $key >= kb::F1) && ( $key <= kb::F30))) {
      $txt = $code ? ( uc chr $code) : ( 'F' . (( $key - kb::F1) / ( kb::F2 - kb::F1) + 1));
      $txt = '^' . $txt if $mod & km::Ctrl;
      $txt = '@' . $txt if $mod & km::Alt;
      $txt = '#' . $txt if $mod & km::Shift;
   } else { 
      return export( $data);
   }
   return "'" . $txt . "'";
}

1;
