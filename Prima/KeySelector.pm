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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  $Id$
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
		key                => kb::NoKey,
		scaleChildren      => 0,
		autoEnableChildren => 1,
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
	$self-> {mod} = $self-> insert( GroupBox =>
		origin   => [ 0, $sz[1]],
		size     => [ $sz[0], $fh * 4 + 28],
		growMode => gm::GrowHiX, 
		style    => cs::DropDown,
		text     => '',
	);

	my @esz = $self-> {mod}-> size;
	$esz[1] -= $fh * 2 + 4;
	$self-> {modShift} = $self-> {mod}-> insert( CheckBox =>
		name        => 'Shift',
		delegations => [$self, qw(Click)],
		origin   => [ 8, $esz[1]],
		size     => [ $esz[0] - 16, $fh + 6],
		text     => '~Shift',
		growMode => gm::GrowHiX,
	);

	$esz[1] -= $fh + 8;
	$self-> {modCtrl} = $self-> {mod}-> insert( CheckBox =>
		name        => 'Ctrl',
		delegations => [$self, qw(Click)], 
		origin  => [ 8, $esz[1]],
		size    => [ $esz[0] - 16, $fh + 6],
		text    => '~Ctrl',
		growMode => gm::GrowHiX,
	);
	$esz[1] -= $fh + 8;

	$self-> {modAlt} = $self-> {mod}-> insert( CheckBox =>
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
	$self-> {keyhook} = $self-> insert( Widget =>
		name        => 'Hook',
		delegations => [qw(Paint KeyDown TranslateAccel )],
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
	my $tx = $self-> {keys}-> text;
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

sub Hook_TranslateAccel
{
	my ( $self, $hook, $code, $key, $mod) = @_;
	return unless $hook-> focused;
	$hook-> clear_event unless $key == kb::Tab || $key == kb::BackTab;
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
		$txt = $code ? 
			( uc chr $code) : 
			( 'F' . (( $key - kb::F1) / ( kb::F2 - kb::F1) + 1));
		$txt = '^' . $txt if $mod & km::Ctrl;
		$txt = '@' . $txt if $mod & km::Alt;
		$txt = '#' . $txt if $mod & km::Shift;
	} else { 
		return export( $data);
	}
	return "'" . $txt . "'";
}

1;

__DATA__

=pod

=head1 NAME

Prima::KeySelector - key combination widget and routines

=head1 DESCRIPTION

The module provides a standard widget for selecting a user-defined
key combination. The widget class allows import, export, and modification of
key combinations.

The module provides a set of routines, useful for conversion of
a key combination between representations.

=head1 SYNOPSIS

	my $ks = Prima::KeySelector-> create( );
	$ks-> key( km::Alt | ord('X'));
	print Prima::KeySelector::describe( $ks-> key );

=head1 API

=head2 Properties

=over

=item key INTEGER

Selects a key combination in integer format. The format is
described in L<Prima::Menu/"Hot key">, and is a combination
of C<km::XXX> key modifiers and either a C<kb::XXX> virtual
key, or a character code value.

The property allows almost, but not all possible combinations of
key constants. Only C<km::Ctrl>, C<km::Alt>, and C<km::Shift> 
modifiers are allowed.

=back

=head2 Methods

All methods here can ( and must ) be called without the object
syntax; - the first parameter must not be neither package nor 
widget.

=over

=item describe KEY

Accepts KEY in integer format, and returns string
description of the key combination in human readable
format. Useful for supplying an accelerator text to
a menu.

	print describe( km::Shift|km::Ctrl|km::F10);
	Ctrl+Shift+F10

=item export KEY

Accepts KEY in integer format, and returns string
with a perl-evaluable expression, which after
evaluation resolves to the original KEY value. Useful for storing
a key into text config files, where value must be both 
human readable and easily passed to a program.

	print export( km::Shift|km::Ctrl|km::F10);
	km::Shift|km::Ctrl|km::F10 

=item shortcut KEY

Converts KEY from integer format to a string,
acceptable by C<Prima::AbstractMenu> input methods.

	print shortcut( km::Ctrl|ord('X'));
	^X

=item translate_codes KEY, [ USE_CTRL = 0 ]

Converts KEY in integer format to three integers
in the format accepted by L<Prima::Widget/KeyDown> event:
code, key, and modifier. USE_CTRL is only relevant when
KEY first byte ( C<KEY & 0xFF> ) is between 1 and 26, what
means that the key is a combination of an alpha key with the control key.
If USE_CTRL is 1, code result is unaltered, and is in range 1 - 26.
Otherwise, code result is converted to the character code
( 1 to ord('A'), 2 to ord('B') etc ).

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Menu>.

=cut
