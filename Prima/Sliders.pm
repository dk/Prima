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
#
#  $Id$

# contains:
#   SpinButton
#   AltSpinButton
#   SpinEdit
#   Gauge
#   Slider
#   CircularSlider

use strict;
use Prima::Const;
use Prima::Classes;
use Prima::IntUtils;

package Prima::AbstractSpinButton;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
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
	return %profile;
}

sub on_mouseclick
{
	my $self = shift;
	$self-> clear_event;
	return unless pop;
	$self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub state         {($#_)?$_[0]-> set_state ($_[1]):return $_[0]-> {pressState}}

#sub on_trackend   {}
#sub on_increment  {
#  my ( $self, $increment) = @_;
#}

package Prima::SpinButton;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractSpinButton);

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
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	my $h = $self-> height;
	if ( $y >= $h * 0.6) { 
		$self-> { mouseTransaction} = 1; 
	} elsif ( $y <  $h * 0.4) { 
		$self-> { mouseTransaction} = 2;
	} else { 
		$self-> { mouseTransaction} = 3; 
	}
	$self-> { lastMouseOver}  = 1;
	$self-> { startMouseY  }  = $y;
	$self-> state( $self-> { mouseTransaction});
	$self-> capture(1);
	$self-> clear_event;
	$self-> {increment} = 0;
	if ( $self-> { mouseTransaction} != 3) {
		$self-> notify( 'Increment', $self-> { mouseTransaction} == 1 ? 1 : -1);
		$self-> scroll_timer_start;
		$self-> scroll_timer_semaphore(0);
	} else {
		$self-> {pointerSave} = $self-> pointer;
		$self-> pointer( cr::SizeWE);
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless $self-> {mouseTransaction};
	my $mt  = $self-> {mouseTransaction};
	my $inc = $mt != 2 ? 1 : -1;

	$self-> {mouseTransaction} = undef;
	$self-> {spaceTransaction} = undef;
	$self-> {lastMouseOver}    = undef;
	$self-> capture(0);
	$self-> scroll_timer_stop;
	$self-> state( 0);
	$self-> pointer( $self-> {pointerSave}), $self-> {pointerSave} = undef 
		if $mt == 3;
	$self-> {increment} = 0;
	$self-> notify( 'TrackEnd');
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	unless ( $self-> {mouseTransaction}) {
		my $h = $self-> height;
		$self-> pointer((( $y >= $h * 0.6) || ( $y < $h * 0.4)) ? cr::Default : cr::SizeWE);
		return;
	}
	my @size = $self-> size;
	my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
	$self-> state( $self-> {pressState} ? 0 : $self-> {mouseTransaction})
		if $self-> { lastMouseOver} != $mouseOver && $self-> {pressState} != 3;
	$self-> { lastMouseOver} = $mouseOver;

	if ( $self-> {pressState} == 3) {
		my $d  = ( $self-> {startMouseY} - $y) / 3; # 2 is mouse sensitivity
		$self-> notify( 'Increment', int($self-> {increment}) - int($d))
			if int( $self-> {increment}) != int( $d);
		$self-> {increment}  = $d;
	} elsif ( $self-> {pressState} > 0) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
		$self-> notify( 'Increment', $self-> {mouseTransaction} == 1 ? 1 : -1);
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
	my $p = $self-> {pressState};

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
	return if $s == $self-> {pressState};
	$self-> {pressState} = $s;
	$self-> repaint;
}

package Prima::AltSpinButton;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractSpinButton);

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
	$p-> {height} = $p-> {width}  if !exists( $p-> {height}) && exists( $p-> {width});
	$p-> {width}  = $p-> {height} if exists( $p-> {height}) && !exists( $p-> {width});
	$self-> SUPER::profile_check_in( $p, $default);
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	$self-> { mouseTransaction} = 
		(( $x * $self-> height / ( $self-> width || 1)) > $y) ? 
			2 : 1;
	$self-> { lastMouseOver}  = 1;
	$self-> state( $self-> { mouseTransaction});
	$self-> capture(1);
	$self-> clear_event;
	$self-> notify( 'Increment', $self-> { mouseTransaction} == 1 ? 1 : -1);
	$self-> scroll_timer_start;
	$self-> scroll_timer_semaphore(0);
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless $self-> {mouseTransaction};
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
	return unless $self-> {mouseTransaction};
	my @size = $self-> size;
	my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
	$self-> state( $self-> {pressState} ? 0 : $self-> {mouseTransaction})
		if $self-> { lastMouseOver} != $mouseOver;
	$self-> { lastMouseOver} = $mouseOver;
	if ( $self-> {pressState}) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
		$self-> notify( 'Increment', $self-> {mouseTransaction} == 1 ? 1 : -1);
	} else {
		$self-> scroll_timer_stop;
	}
}


sub on_paint
{
	my ( $self, $canvas) = @_;
	my @clr  = ( $self-> color, $self-> backColor);
	@clr = ( $self-> hiliteColor, $self-> hiliteBackColor)     if $self-> { default};
	@clr = ( $self-> disabledColor, $self-> disabledBackColor) if !$self-> enabled;
	my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
	my @size = $canvas-> size;
	$canvas-> color( $clr[ 1]);
	$canvas-> bar( 0, 0, $size[0]-1, $size[1]-1);
	my $p = $self-> {pressState};

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
	$canvas-> fillpoly([ 
		$size[0] * 0.2 + $p1, $size[1] * 0.65 - $p1,
		$size[0] * 0.3 + $p1, $size[1] * 0.77 - $p1,
		$size[0] * 0.4 + $p1, $size[1] * 0.65 - $p1
	]);
	$p1 = ( $p == 2) ? 1 : 0;
	$canvas-> fillpoly([ 
		$size[0] * 0.6 + $p1, $size[1] * 0.35 - $p1,
		$size[0] * 0.7 + $p1, $size[1] * 0.27 - $p1,
		$size[0] * 0.8 + $p1, $size[1] * 0.35 - $p1
	]);
}

sub set_state
{
	my ( $self, $s) = @_;
	$s = 0 if $s > 2;
	return if $s == $self-> {pressState};
	$self-> {pressState} = $s;
	$self-> repaint;
}

package Prima::SpinEdit;
use vars qw(@ISA %editProps %spinDynas);
use Prima::InputLine;
@ISA = qw(Prima::Widget);


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
   sub Prima::SpinEdit::DummyEdit::$_ { }
GENPROC
}

sub profile_default
{
	my $font = $_[ 0]-> get_default_font;
	my $fh   = $font-> {height} + 2;
	return {
		%{Prima::InputLine-> profile_default},
		%{$_[ 0]-> SUPER::profile_default},
		autoEnableChildren => 1,
		ownerBackColor => 1,
		selectable     => 0,
		scaleChildren  => 0,
		min            => 0,
		max            => 100,
		step           => 1,
		pageStep       => 10,
		value          => 0,
		circulate      => 0,
		height         => $fh < 20 ? 20 : $fh,
		editClass      => 'Prima::InputLine',
		spinClass      => 'Prima::AltSpinButton',
		editProfile    => {},
		spinProfile    => {},
		editDelegations=> [qw(KeyDown Change MouseWheel Enter Leave)],
		spinDelegations=> [qw(Increment)],
	}
}

sub init
{
	my $self = shift;
	my %profile = @_;
	my $visible = $profile{visible};
	$profile{visible} = 0;
	for (qw( min max step circulate pageStep)) {$self-> {$_} = 1;};
	$self-> {edit} = bless [], q\Prima::SpinEdit::DummyEdit\;
	%profile = $self-> SUPER::init(%profile);
	my ( $w, $h) = ( $self-> size);
	$self-> {spin} = $self-> insert( $profile{spinClass} =>
		ownerBackColor => 1,
		name           => 'Spin',
		bottom         => 1,
		right          => $w - 1,
		height         => $h - 1 * 2,
		growMode       => gm::Right,
		delegations    => $profile{spinDelegations},
		(map { $_ => $profile{$_}} grep { exists $profile{$_} ? 1 : 0} keys %spinDynas),
		%{$profile{spinProfile}},
	);
	$self-> {edit} = $self-> insert( $profile{editClass} =>
		name         => 'InputLine',
		origin      => [ 1, 1],
		size        => [ $w - $self-> {spin}-> width - 1 * 2, $h - 1 * 2],
		growMode    => gm::GrowHiX|gm::GrowHiY,
		selectable  => 1,
		tabStop     => 1,
		borderWidth => 0,
		current     => 1,
		delegations => $profile{editDelegations},
		(map { $_ => $profile{$_}} keys %editProps),
		%{$profile{editProfile}},
		text        => $profile{value},
	);
	for (qw( min max step value circulate pageStep)) {$self-> $_($profile{$_});};
	$self-> visible( $visible);
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @s = $canvas-> size;
	$canvas-> rect3d( 0, 0, $s[0]-1, $s[1]-1, 1, $self-> dark3DColor, $self-> light3DColor);
}

sub InputLine_MouseWheel
{
	my ( $self, $edit, $mod, $x, $y, $z) = @_;
	$z = int($z/120);
	$z *= $self-> {pageStep} if $mod & km::Ctrl;
	my $value = $self-> value;
	$self-> value( $value + $z * $self-> {step});
	$self-> value( $z > 0 ? $self-> min : $self-> max)
		if $self-> {circulate} && ( $self-> value == $value);
	$edit-> clear_event;
}

sub Spin_Increment
{
	my ( $self, $spin, $increment) = @_;
	my $value = $self-> value;
	$self-> value( $value + $increment * $self-> {step});
	$self-> value( $increment > 0 ? $self-> min : $self-> max)
		if $self-> {circulate} && ( $self-> value == $value);
}

sub InputLine_KeyDown
{
	my ( $self, $edit, $code, $key, $mod) = @_;
	$edit-> clear_event, return if 
		$key == kb::NoKey && !($mod & (km::Alt | km::Ctrl)) &&
		chr($code) !~ /^[.\d+-]$/;
	if ( $key == kb::Up || $key == kb::Down || $key == kb::PgDn || $key == kb::PgUp) {
		my ($s,$pgs) = ( $self-> step, $self-> pageStep);
		my $z = ( $key == kb::Up) ? $s : (( $key == kb::Down) ? -$s :
			(( $key == kb::PgUp) ? $pgs : -$pgs));
		if (( $mod & km::Ctrl) && ( $key == kb::PgDn || $key == kb::PgUp)) {
			$self-> value( $key == kb::PgDn ? $self-> min : $self-> max);
		} else {
			my $value = $self-> value;
			$self-> value( $value + $z);
			$self-> value( $z > 0 ? $self-> min : $self-> max)
				if $self-> {circulate} && ( $self-> value == $value);
		}
		$edit-> clear_event;
		return;
	}
	if ($key == kb::Enter) {
		my $value = $edit-> text;
		$self-> value( $value);
		$edit-> clear_event if $value ne $self-> value;
		return;
	}
}

sub InputLine_Change
{
	my ( $self, $edit) = @_;
	$self-> notify(q(Change));
}

sub InputLine_Enter
{
	my ( $self, $edit) = @_;
	$self-> notify(q(Enter));
}

sub InputLine_Leave
{
	my ( $self, $edit) = @_;
	$self-> notify(q(Leave));
}

sub set_bounds
{
	my ( $self, $min, $max) = @_;
	$max = $min if $max < $min;
	( $self-> { min}, $self-> { max}) = ( $min, $max);
	my $oldValue = $self-> value;
	$self-> value( $max) if $max < $self-> value;
	$self-> value( $min) if $min > $self-> value;
}

sub set_step
{
	my ( $self, $step) = @_;
	$step  = 0 if $step < 0;
	$self-> {step} = $step;
}

sub circulate
{
	return $_[0]-> {circulate} unless $#_;
	$_[0]-> {circulate} = $_[1];
}

sub pageStep
{
	return $_[0]-> {pageStep} unless $#_;
	$_[0]-> {pageStep} = $_[1];
}


sub min          {($#_)?$_[0]-> set_bounds($_[1], $_[0]-> {'max'})      : return $_[0]-> {min};}
sub max          {($#_)?$_[0]-> set_bounds($_[0]-> {'min'}, $_[1])      : return $_[0]-> {max};}
sub step         {($#_)?$_[0]-> set_step         ($_[1]):return $_[0]-> {step}}
sub value
{
	if ($#_) {
		my ( $self, $value) = @_;
		if ( $value =~ m/^\s*([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?\s*$/) {
			$value = $self-> {min} if $value < $self-> {min};
			$value = $self-> {max} if $value > $self-> {max};
		} else {
			$value = $self-> {min};
		}
		return if $value eq $self-> {edit}-> text;
		$self-> {edit}-> text( $value);
	} else {
		my $self = $_[0];
		my $value = $self-> {edit}-> text;
		if ( $value =~ m/^\s*([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?\s*$/) {
			$value = $self-> {min} if $value < $self-> {min};
			$value = $self-> {max} if $value > $self-> {max};
		} else {
			$value = $self-> {min};
		}
		return $value;
	}
}


# gauge reliefs
package 
    gr;
use constant Sink         =>  -1;
use constant Border       =>  0;
use constant Raise        =>  1;


package Prima::Gauge;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Stringify => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		indent         => 1,
		relief         => gr::Sink,
		ownerBackColor => 1,
		hiliteBackColor=> cl::Blue,
		hiliteColor    => cl::White,
		min            => 0,
		max            => 100,
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
	{$self-> {$_} = 0}
	$self-> {string} = '';
	for (qw( vertical threshold min max relief indent value))
	{$self-> $_($profile{$_}); }
	return %profile;
}

sub setup
{
	$_[0]-> SUPER::setup;
	$_[0]-> value($_[0]-> {value});
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my $i = $self-> indent;
	my ($clComplete,$clBack,$clFore,$clHilite) = ($self-> hiliteBackColor, $self-> backColor, $self-> color, $self-> hiliteColor);
	my $v = $self-> {vertical};
	my $complete = $v ? $y : $x;
	my $range = ($self-> {max} - $self-> {min}) || 1;
	$complete = int(($complete - $i*2) * $self-> {value} / $range + 0.5);
	my ( $l3, $d3) = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> color( $clComplete);
	$canvas-> bar ( $v ? ($i, $i, $x-$i-1, $i+$complete) : ( $i, $i, $i + $complete, $y-$i-1));
	$canvas-> color( $clBack);
	$canvas-> bar ( $v ? ($i, $i+$complete+1, $x-$i-1, $y-$i-1) : ( $i+$complete+1, $i, $x-$i-1, $y-$i-1));
	# draw the border
	my $relief = $self-> relief;
	$canvas-> color(( $relief == gr::Sink) ? $d3 : (( $relief == gr::Border) ? cl::Black : $l3));
	for ( my $j = 0; $j < $i; $j++)
	{
		$canvas-> line( $j, $j, $j, $y - $j - 1);
		$canvas-> line( $j, $y - $j - 1, $x - $j - 1, $y - $j - 1);
	}
	$canvas-> color(( $relief == gr::Sink) ? $l3 : (( $relief == gr::Border) ? cl::Black : $d3));
	for ( my $j = 0; $j < $i; $j++)
	{
		$canvas-> line( $j + 1, $j, $x - $j - 1, $j);
		$canvas-> line( $x - $j - 1, $j, $x - $j - 1, $y - $j - 1);
	}

	# draw the text, if neccessary
	my $s = $self-> {string};
	if ( $s ne '')
	{
		my ($fw, $fh) = ( $canvas-> get_text_width( $s), $canvas-> font-> height);
		my $xBeg = int(( $x - $fw) / 2 + 0.5);
		my $xEnd = $xBeg + $fw;
		my $yBeg = int(( $y - $fh) / 2 + 0.5);
		my $yEnd = $yBeg + $fh;
		my ( $zBeg, $zEnd) = $v ? ( $yBeg, $yEnd) : ( $xBeg, $xEnd);
		if ( $zBeg > $i + $complete) {
			$canvas-> color( $clFore);
			$canvas-> text_out( $s, $xBeg, $yBeg);
		} elsif ( $zEnd < $i + $complete + 1) {
			$canvas-> color( $clHilite);
			$canvas-> text_out( $s, $xBeg, $yBeg);
		} else {
			$canvas-> clipRect( $v ? 
				( 0, 0, $x, $i + $complete) : 
				( 0, 0, $i + $complete, $y)
			);
			$canvas-> color( $clHilite);
			$canvas-> text_out( $s, $xBeg, $yBeg);
			$canvas-> clipRect( $v ? 
				( 0, $i + $complete + 1, $x, $y) : 
				( $i + $complete + 1, 0, $x, $y)
			);
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
	my $oldValue = $self-> {value};
	$self-> value( $max) if $self-> {value} > $max;
	$self-> value( $min) if $self-> {value} < $min;
}

sub value
{
	return $_[0]-> {value} unless $#_;
	my $v = $_[1] < $_[0]-> {min} ? $_[0]-> {min} : ($_[1] > $_[0]-> {max} ? $_[0]-> {max} : $_[1]);
	$v -= $_[0]-> {min};
	my $old = $_[0]-> {value};
	if (abs($old - $v) >= $_[0]-> {threshold}) {
		my ($x, $y) = $_[0]-> size;
		my $i = $_[0]-> {indent};
		my $range = ( $_[0]-> {max} - $_[0]-> {min}) || 1;
		my $x1 = $i + ($x - $i*2) * $old / $range;
		my $x2 = $i + ($x - $i*2) * $v   / $range;
		($x1, $x2) = ( $x2, $x1) if $x1 > $x2;
		my $s = $_[0]-> {string};
		$_[0]-> {value} = $v;
		$_[0]-> notify(q(Stringify), $v, \$_[0]-> {string});
		( $_[0]-> {string} eq $s) ?
			$_[0]-> invalidate_rect( $x1, 0, $x2+1, $y) :
			$_[0]-> repaint;
	}
}

sub on_stringify
{
	my ( $self, $value, $sref) = @_;
	$$sref = sprintf( "%2d%%", $value * 100.0 / (($_[0]-> {max} - $_[0]-> {min})||1));
	$self-> clear_event;
}

sub indent    {($#_)?($_[0]-> {indent} = $_[1],$_[0]-> repaint)  :return $_[0]-> {indent};}
sub relief    {($#_)?($_[0]-> {relief} = $_[1],$_[0]-> repaint)  :return $_[0]-> {relief};}
sub vertical  {($#_)?($_[0]-> {vertical} = $_[1],$_[0]-> repaint):return $_[0]-> {vertical};}
sub min       {($#_)?$_[0]-> set_bounds($_[1], $_[0]-> {'max'})  : return $_[0]-> {min};}
sub max       {($#_)?$_[0]-> set_bounds($_[0]-> {'min'}, $_[1])  : return $_[0]-> {max};}
sub threshold {($#_)?($_[0]-> {threshold} = $_[1]):return $_[0]-> {threshold};}

# slider standard schemes
package 
    ss;
use constant Gauge        => 0;
use constant Axis         => 1;
use constant Thermometer  => 2;
use constant StdMinMax    => 3;

package Prima::AbstractSlider;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Track   => nt::Default,
);
sub notification_types { return \%RNT; }
}

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		autoTrack      => 1,
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
	for ( qw( min max readOnly snap value autoTrack))
		{$self-> {$_}=0}
	for ( qw( tickVal tickLen tickTxt )) { $self-> {$_} = [] };
	my %profile = $self-> SUPER::init( @_);
	for ( qw( step min max increment readOnly ticks snap value autoTrack))
	{$self-> $_($profile{$_});}
	$self-> scheme( $profile{scheme}) if defined $profile{scheme};
	return %profile;
}

sub autoTrack
{
	return $_[0]-> {autoTrack} unless $#_;
	$_[0]-> {autoTrack} = $_[1];
}

sub on_mouseclick
{
	my $self = shift;
	$self-> clear_event;
	return unless pop;
	$self-> clear_event unless $self-> notify( "MouseDown", @_);
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$self-> set_next_value( $self-> {step} * $z / 120);
	$self-> clear_event;
}

sub set_next_value
{
	my ( $self, $dir) = @_;
	$dir *= -1 if $self-> {min} > $self-> {max};
	if ( $self-> snap) {
		my $v = $self-> value;
		my $w = $v;
		return if ( $v + $dir > $self-> {min} and $v + $dir > $self-> {max}) or
			( $v + $dir < $self-> {min} and $v + $dir < $self-> {max});
		$self-> value( $v += $dir) while $self-> {value} == $w;
	} else {
		$self-> value( $self-> value + $dir);
	}
}

sub set_read_only
{
	$_[0]-> {readOnly} = $_[1];
	$_[0]-> repaint;
	$_[0]-> notify(q(MouseUp),0,0,0) if defined $_[0]-> {mouseTransaction};
}


sub set_snap
{
	$_[0]-> {snap} = $_[1];
	$_[0]-> value( $_[0]-> value) if $_[1];
}

sub set_step
{
	my $i = $_[1];
	$i = 1 if $i == 0;
	$_[0]-> {step} = $i;
}

sub get_ticks
{
	my $self  =  $_[0];
	my $i;
	my ( $tv, $tl, $tt) = ($self-> {tickVal}, $self-> {tickLen}, $self-> {tickTxt});
	my @t;
	for ( $i = 0; $i < scalar @{$tv}; $i++) {
		push ( @t, { 
			value => $$tv[$i], 
			height => $$tl[$i], 
			text => $$tt[$i] 
		});
	}
	return @t;
}

sub set_ticks
{
	my $self  = shift;
	return unless defined $_[0];
	my @ticks = (@_ == 1 and ref($_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;
	my @val;
	my @len;
	my @txt;
	for ( @ticks) {
		next unless exists $$_{value};
		push( @val, $$_{value});
		push( @len, exists($$_{height})    ? $$_{height}    : 0);
		push( @txt, exists($$_{text})      ? $$_{text}      : undef);
	}
	$self-> {tickVal} = \@val;
	$self-> {tickLen} = \@len;
	$self-> {tickTxt} = \@txt;
	$self-> {scheme}  = undef;
	$self-> value( $self-> value);
	$self-> repaint;
}

sub set_bound
{
	my ( $self, $val, $bound) = @_;
	$self-> {$bound} = $val;
	$self-> scheme($self-> {scheme}) if defined $self-> {scheme};
	$self-> repaint;
}

sub set_scheme
{
	my ( $self, $s) = @_;
	unless ( defined $s) {
		$self-> {scheme} = undef;
		return;
	}
	my ( $max, $min) = ( $self-> {max}, $self-> {min});
	$self-> ticks([]), return if $max == $min;

	my @t;
	my $i;
	my $inc = $self-> {increment};
	if ( $s == ss::Gauge) {
		for ( $i = $min; $i <= $max; $i += $inc) {
			push ( @t, { value => $i, height => 4, text => $i });
		}
	} elsif ( $s == ss::Axis) {
		for ( $i = $min; $i <= $max; $i += $inc) {
			push ( @t, { value => $i, height => 6,   text => $i });
			if ( $i < $max) {
				for ( 1..4) {
					my $v = $i + $inc / 5 * $_;
					last if $v > $max;
					push ( @t, { value => $v, height => 3 });
					push ( @t, { value => $v, height => 3 });
					push ( @t, { value => $v, height => 3 });
					push ( @t, { value => $v, height => 3 });
				}
			}
		}
	} elsif ( $s == ss::StdMinMax) {
		push ( @t, { value => $min, height => 6,   text => "Min" });
		push ( @t, { value => $max, height => 6,   text => "Max" });
	} elsif ( $s == ss::Thermometer) {
		for ( $i = $min; $i <= $max; $i += $inc) {
			push ( @t, { 
				value => $i, 
				height => 6,   
				text => $i 
			});
			if ( $i < $max) {
				my $j;
				for ( $j = 1; $j < 10; $j++) {
					my $v = $i + $inc / 10 * $j;
					last if $v > $max;
					push ( @t, { 
						value => $v,
						height => $j == 5 ? 5 : 3 
					});
				}
			}
		}
	}
	$self-> ticks( @t);
	$self-> {scheme} = $s;
}

sub increment
{
	return $_[0]-> {increment} unless $#_;
	my ( $self, $increment) = @_;
	$self-> {increment} = $increment;
	if ( defined $self-> {scheme}) {
		$self-> scheme( $self-> {scheme});
		$self-> repaint;
	}
}
sub readOnly    {($#_)?$_[0]-> set_read_only   ($_[1]):return $_[0]-> {readOnly};}
sub ticks       {($#_)?shift-> set_ticks          (@_):return $_[0]-> get_ticks;}
sub snap        {($#_)?$_[0]-> set_snap        ($_[1]):return $_[0]-> {snap};}
sub step        {($#_)?$_[0]-> set_step        ($_[1]):return $_[0]-> {step};}
sub scheme      {($#_)?shift-> set_scheme         (@_):return $_[0]-> {scheme}}
sub value       {($#_)?$_[0]-> {value} =       $_[1]  :return $_[0]-> {value};}
sub min         {($#_)?$_[0]-> set_bound($_[1],q(min)):return $_[0]-> {min};}
sub max         {($#_)?$_[0]-> set_bound($_[1],q(max)):return $_[0]-> {max};}


# linear slider tick alignment
package 
    tka;
use constant Normal      => 0;
use constant Alternative => 1;
use constant Dual        => 2;

package Prima::Slider;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractSlider);

use constant DefButtonX => 12;

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		borderWidth    => 0,
		ribbonStrip    => 0,
		shaftBreadth   => 6,
		tickAlign      => tka::Normal,
		vertical       => 0,
	}
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 
		for qw( vertical tickAlign ribbonStrip shaftBreadth borderWidth);
	my %profile = $self-> SUPER::init( @_);
	$self-> $_($profile{$_}) 
		for qw( vertical tickAlign ribbonStrip shaftBreadth borderWidth);
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @clr  = ( $self-> color, $self-> backColor);
	@clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
	my @c3d  = ( $self-> dark3DColor, $self-> light3DColor);
	my @cht  = ( $self-> hiliteColor, $self-> hiliteBackColor);
	my @size = $canvas-> size;
	my ( 
		$sb, $v, 
		$range, $min, 
		$tval, $tlen, $ttxt, 
		$ta
	) = ( 
		$self-> {shaftBreadth}, $self-> {vertical}, 
		abs($self-> {max} - $self-> {min}) || 1, $self-> {min},
		$self-> {tickVal}, $self-> {tickLen}, $self-> {tickTxt}, 
		$self-> {tickAlign}
	);
	if ( $ta == tka::Normal) { 
		$ta = 1; 
	} elsif ( $ta == tka::Alternative) { 
		$ta = 2; 
	} else { 
		$ta = 3; 
	}

	unless ( $self-> transparent) {
		$canvas-> color( $clr[1]);
		$canvas-> bar(0,0,@size);
	}
	$sb = ( $v ? $size[0] : $size[1]) / 6 unless $sb;
	$sb = 2 unless $sb;
	if ( $v) {
		my $bh = $canvas-> font-> height;
		my $bw  = ( $size[0] - $sb) / 2;
		return if $size[1] <= DefButtonX * ($self-> {readOnly} ? 1 : 0) + 2 * $bh + 2;

		$canvas-> translate((( $ta == 1) ? 1 : -1) * ( $bw - $sb - DefButtonX), 0) 
			if $ta < 3;
		my $br  = $size[1] - 2 * $bh - 2;
		$canvas-> rect3d( 
			$bw, $bh, $bw + $sb - 1, $bh + $br - 1, 1, 
			@c3d, $cht[1]
		), return unless $range;

		my $val = $bh + 1 + abs( $self-> {value} - $min) * ( $br - 3) / $range;
		if ( $self-> {ribbonStrip}) {
			$canvas-> rect3d( $bw, $bh, $bw + $sb - 1, $bh + $br - 1, 1, @c3d);
			$canvas-> color( $cht[0]);
			$canvas-> bar( $bw + 1, $bh + 1, $bw + $sb - 2, $val);
			$canvas-> color( $cht[1]);
			$canvas-> bar( $bw + 1, $val + 1, $bw + $sb - 2, $bh + $br - 2);
		} else {
			$canvas-> rect3d( $bw, $bh, $bw + $sb - 1, $bh + $br - 1, 
				1, @c3d, $cht[1]);
			$canvas-> color( $clr[0]);
			$canvas-> line( $bw + 1, $val, $bw + $sb - 2, $val) 
				if $self-> {readOnly};
		}
		my $i;
		$canvas-> color( $clr[0]);
		for ( $i = 0; $i < scalar @{$tval}; $i++) {
			my $val = $bh + 1 + abs( $$tval[$i] - $min) * ( $br - 3) / $range;
			if ( $$tlen[ $i]) {
				$canvas-> line( 
					$bw + $sb + 3, $val, 
					$bw + $sb + $$tlen[ $i] + 3, $val
				) if $ta & 2;
				$canvas-> line( 
					$bw - 4, $val, 
					$bw - 4 - $$tlen[ $i], $val
				) if $ta & 1;
			}
			$canvas-> text_out( $$ttxt[ $i],
				( $ta == 2) ? 
					$bw + $sb + $$tlen[ $i] + 5 : 
					$bw - $$tlen[ $i] - 5 - $canvas-> get_text_width( $$ttxt[ $i]),
				$val - $bh / 2
			) if defined $$ttxt[ $i];
		}
		unless ( $self-> {readOnly}) {
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
		my $bw = $canvas-> font-> width + $self-> {borderWidth};
		my $bh  = ( $size[1] - $sb) / 2;
		my $fh = $canvas-> font-> height;
		return if $size[0] <= DefButtonX * ($self-> {readOnly} ? 1 : 0) + 2 * $bw + 2;

		$canvas-> translate( 0, (( $ta == 1) ? -1 : 1) * ( $bh - $sb - DefButtonX)) 
			if $ta < 3;
		my $br  = $size[0] - 2 * $bw - 2;
		$canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d, $cht[1]), return 
			unless $range;
		my $val = $bw + 1 + abs( $self-> {value} - $min) * ( $br - 3) / $range;

		if ( $self-> {ribbonStrip}) {
			$canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d);
			$canvas-> color( $cht[0]);
			$canvas-> bar( $bw+1, $bh+1, $val, $bh + $sb - 2);
			$canvas-> color( $cht[1]);
			$canvas-> bar( $val+1, $bh+1, $bw + $br - 2, $bh + $sb - 2);
		} else  {
			$canvas-> rect3d( $bw, $bh, $bw + $br - 1, $bh + $sb - 1, 1, @c3d, $cht[1]);
			$canvas-> color( $clr[0]);
			$canvas-> line( $val, $bh+1, $val, $bh + $sb - 2) if $self-> {readOnly};
		}
		my $i;

		$canvas-> color( $clr[0]);
		my @texts;
		for ( $i = 0; $i < scalar @{$tval}; $i++) {
			my $val = int( 1 + $bw + abs( $$tval[$i] - $min) * ( $br - 3) / $range + .5);
			if ( $$tlen[ $i]) {
				$canvas-> line( $val, $bh + $sb + 3, $val, $bh + $sb + $$tlen[ $i] + 3) 
					if $ta & 1;
				$canvas-> line( $val, $bh - 4, $val, $bh - 4 - $$tlen[ $i]) 
					if $ta & 2;
			}

			next unless defined $$ttxt[ $i]; 
			my $tw = int( $canvas-> get_text_width( $$ttxt[ $i]) / 2 + .5);
			my $x = $val - $tw;
			next if $x >= $size[0] or $val + $tw < 0;
			push @texts, [
				$$ttxt[$i], $val, $tw,
				( $ta == 2) ? $bh - $$tlen[ $i] - 5 - $fh : $bh + $sb + $$tlen[ $i] + 5
			];
		}

		
		if ( @texts) {
			# see that leftmost val fits
			if ( $texts[0]->[1] - $texts[0]->[2] < 0) {
				$texts[0]->[1] = $texts[0]->[2];
				shift @texts
					if $texts[0]->[1] + $texts[0]->[2] > $size[0];
				goto NO_LABELS unless @texts;
			}

			# see that rightmost text fits
			my ( $rightmost_val, $rightmost_label_width) = (
				$texts[-1]->[1], $texts[-1]->[2]);
			$rightmost_val = $size[0] - 1 - $rightmost_label_width
				if $rightmost_val > $size[0] - 1 - $rightmost_label_width;
			if ( 1 < @texts and $rightmost_val < 0) {
				# skip it
				pop @texts;
				goto NO_LABELS unless @texts;
			} else {
				my $dx = $texts[-1]->[1] - $rightmost_val;
				$texts[-1]->[1] = $rightmost_val;
				# push the label next to it (but not the 1st one)
				$texts[-2]->[1] -= $dx if 2 < @texts;
			}

			# draw labels
			my $lastx = 0;
			for ( @texts) {
				my ( $text, $val, $width, $y) = @$_;
				my $x = $val - $width;
				next if $x < $lastx or $x < 0 or $val + $width >= $size[0];
				$lastx = $val + $width;
				$canvas-> text_out( $text, $x, $y);
			}
		}
		NO_LABELS:

		unless ( $self-> {readOnly}) {
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
	return if $self-> {max} == $self-> {min};
	if ( $self-> {vertical}) {
		my $bh  = $self-> font-> height;
		my $val = 
			$bh + 
			1 + 
			abs( $self-> {value} - $self-> {min}) *
				( $size[1] - 2 * $bh - 5) / 
				( abs($self-> {max} - $self-> {min}) || 1);
		my $ret1 = 
			$self-> {min} + 
			( $y - $bh - 1) * 
				abs($self-> {max} - $self-> {min}) / 
				(( $size[1] - 2 * $bh - 5) || 1);

		if ( $y < $val - DefButtonX / 2 or $y >= $val + DefButtonX / 2) {
			return 0, $ret1;
		} else {
			return 1, $ret1, $y - $val;
		}
	} else {
		my $bw = $self-> font-> width + $self->{borderWidth};
		my $val = 
			$bw + 
			1 + 
			abs( $self-> {value} - $self-> {min}) *
				( $size[0] - 2 * $bw - 5) / 
				(abs($self-> {max} - $self-> {min}) || 1);
		my $ret1 = 
			$self-> {min} + 
			( $x - $bw - 1) * 
				abs($self-> {max} - $self-> {min}) / 
				(( $size[0] - 2 * $bw - 5) || 1);

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
	return if $self-> {readOnly};
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	my ($info, $pos, $ap) = $self-> pos2info( $x, $y);
	return unless defined $info;
	if ( $info == 0) {
		$self-> value( $pos);
		return;
	}
	$self-> {aperture} = $ap;
	$self-> {mouseTransaction} = 1;
	$self-> capture(1);
	$self-> clear_event;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless $self-> {mouseTransaction};
	$self-> {mouseTransaction} = undef;
	$self-> capture(0);
	$self-> notify( 'Change') unless $self-> {autoTrack};
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	$self-> {vertical} ? $y : $x   -= $self-> {aperture};
	my ( $info, $pos) = $self-> pos2info( $x, $y);
	return unless defined $info;
	my $ov = $self-> {value};
	$self-> {suppressNotify} = 1 unless $self-> {autoTrack};
	$self-> value( $pos);
	$self-> {suppressNotify} = 0;
	$self-> notify(q(Track)) if !$self-> {autoTrack} && $ov != $self-> {value};
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
		my $s = $self-> {step};
		$self-> clear_event;
		$self-> set_next_value(( $key == kb::Left || $key == kb::Down) ? -$s : $s);
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
	$ta = tka::Normal if $ta != tka::Alternative and $ta != tka::Dual;
	return if $ta == $self-> {tickAlign};
	$self-> {tickAlign} = $ta;
	$self-> repaint;
}

sub set_ribbon_strip
{
	$_[0]-> {ribbonStrip} = $_[1];
	$_[0]-> repaint;
}

sub set_shaft_breadth
{
	my ( $self, $sb) = @_;
	$sb = 0 if $sb < 0;
	return if $sb == $self-> {shaftBreadth};
	$self-> {shaftBreadth} = $sb;
	$self-> repaint;
}

sub set_bound
{
	my ( $self, $val, $bound) = @_;
	$self-> {$bound} = $val;
	$self-> scheme($self-> {scheme}) if defined $self-> {scheme};
	$self-> repaint;
}

sub value
{
	if ($#_) {
		my ( $self, $value) = @_;
		my ( $min, $max) = ( $self-> {min}, $self-> {max});
		my $old = $self-> {value};
		if ( $self-> {snap}) {
			my ( $minDist, $thatVal, $i) = ( abs( $min - $max));
			my $tval = $self-> {tickVal};
			for ( $i = 0; $i < scalar @{$tval}; $i++) {
				my $j = $$tval[ $i];
				$minDist = abs(($thatVal = $j) - $value) 
					if abs( $j - $value) < $minDist;
			}
			$value = $thatVal if defined $thatVal;
		} elsif ( $self-> {step} != 0 ) {
			$value = int ( $value / $self-> {step} ) * $self-> {step};
		}
		$value = $min if $value < $min;
		$value = $max if $value > $max;
		return if $old == $value;
		$self-> {value} = $value;
		my @size = $self-> size;
		my $sb = $self-> {shaftBreadth};
		if ( $self-> {vertical}) {
			$sb = $size[0] / 6 unless $sb;
			$sb = 2 unless $sb;
			my $bh = $self-> font-> height;
			my $bw  = ( $size[0] - $sb) / 2;
			my $v1  = $bh + 1 + abs( $self-> {value} - $self-> {min}) *
				( $size[1] - 2 * $bh - 5) / (abs($self-> {max} - $self-> {min})||1);
			my $v2  = $bh + 1 + abs( $old - $self-> {min}) *
				( $size[1] - 2 * $bh - 5) / (abs($self-> {max} - $self-> {min})||1);
			( $v2, $v1) = ( $v1, $v2) if $v1 > $v2;
			$v1 -= DefButtonX / 2;
			$v2 += DefButtonX / 2 + 1;
			my $xd = 0;
			$xd = (( $self-> {tickAlign} == tka::Normal) ? 1 : -1) *
			( $bw - $sb - DefButtonX) if $self-> {tickAlign} != tka::Dual;
			$self-> invalidate_rect( $bw - 4 + $xd, $v1, $bw + $sb + 9 + $xd, $v2);
		} else {
			$sb = $size[1] / 6 unless $sb;
			$sb = 2 unless $sb;
			my $bw = $self-> font-> width + $self-> {borderWidth};
			my $bh  = ( $size[1] - $sb) / 2;
			my $v1  = $bw + 1 + abs( $self-> {value} - $self-> {min}) *
				( $size[0] - 2 * $bw - 5) / (abs($self-> {max} - $self-> {min})||1);
			my $v2  = $bw + 1 + abs( $old - $self-> {min}) *
				( $size[0] - 2 * $bw - 5) / (abs($self-> {max} - $self-> {min})||1);
			( $v2, $v1) = ( $v1, $v2) if $v1 > $v2;
			$v1 -= DefButtonX / 2;
			$v2 += DefButtonX / 2 + 1;
			my $yd = 0;
			$yd = (( $self-> {tickAlign} == tka::Normal) ? -1 : 1) *
			( $bh - $sb - DefButtonX) if $self-> {tickAlign} != tka::Dual;
			$self-> invalidate_rect( $v1, $bh - 9 + $yd, $v2, $bh + $sb + 4 + $yd);
		}
		$self-> notify(q(Change)) unless $self-> {suppressNotify};
	} else {
		return $_[0]-> {value};
	}
}
sub vertical    {($#_)?$_[0]-> set_vertical    ($_[1]):return $_[0]-> {vertical};}
sub tickAlign   {($#_)?$_[0]-> set_tick_align  ($_[1]):return $_[0]-> {tickAlign};}
sub ribbonStrip {($#_)?$_[0]-> set_ribbon_strip($_[1]):return $_[0]-> {ribbonStrip};}
sub shaftBreadth{($#_)?$_[0]-> set_shaft_breadth($_[1]):return $_[0]-> {shaftBreadth};}

sub borderWidth
{
	return $_[0]-> {borderWidth} unless $#_;
	my ( $self, $bw) = @_;
	$bw = 0 if $bw < 0;
	$self-> {borderWidth} = $bw;
	$self-> repaint;
}

package Prima::CircularSlider;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractSlider Prima::MouseScroller);

{
my %RNT = (
	%{Prima::AbstractSlider-> notification_types()},
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
	}
}

sub init
{
	my $self = shift;
	for ( qw( buttons pressState circX circY br butt1X butt1Y butt2X))
		{$self-> {$_}=0}
	$self-> {string} = '';
	my %profile = $self-> SUPER::init( @_);
	for ( qw( buttons stdPointer))
	{$self-> $_($profile{$_});}
	$self-> reset;
	return %profile;
}

sub setup
{
	$_[0]-> SUPER::setup;
	$_[0]-> notify(q(Stringify), $_[0]-> {value}, \$_[0]-> {string});
	$_[0]-> repaint;
}

sub text
{
	return $_[0]-> SUPER::text unless $#_;
	$_[0]-> SUPER::text( $_[1]);
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
	my $a = 225 * 3.14159 / 180 - ( 270 * 3.14159 / 180) * ( $value - $self-> {min}) /
		(abs( $self-> {min} - $self-> {max})||1);
	return $width + $radius * cos($a), $height + $radius * sin($a);
}

sub offset2data
{
	my ( $self, $value) = @_;
	my $a = 225 * 3.14159 / 180 - ( 270 * 3.14159 / 180) * abs( $value - $self-> {min})/
		(abs( $self-> {min} - $self-> {max})||1);
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
	( abs($self-> {max} - $self-> {min}), $self-> {min}, $self-> {tickVal},
	$self-> {tickLen}, $self-> {tickTxt} );

	if ( defined $self-> {singlePaint}) {
		my @clip1 = @{$self-> {expectedClip}};
		my @clip2 = $self-> clipRect;
		my $i;
		for ( $i = 0; $i < 4; $i++) {
			$self-> {singlePaint} = undef, last if $clip1[$i] != $clip2[$i];
		}
	}

	$canvas-> color( $clr[1]);
	$canvas-> bar( 0, 0, @size) if !$self-> transparent && !defined $self-> {singlePaint};
	my $fh  = $canvas-> font-> height;
	my $br  = $self-> {br};
	my $rad = $br * 0.3;
	my @cpt = ( $self-> {circX}, $self-> {circY}, $rad*2+1, $rad*2+1);
	if ( $self-> {circAlive}) {
		if ( defined $self-> {singlePaint}) {
			$canvas-> fill_ellipse( @cpt[0..1], $rad*2-5, $rad*2-5);
			$canvas-> color( $clr[0]);
		} else {
			$canvas-> color( $c3d[1]);
			$canvas-> lineWidth(2);
			$canvas-> arc( @cpt[0..1], $cpt[2]-2, $cpt[3]-2, 65, 235);
			$canvas-> color( $c3d[0]);
			$canvas-> arc( @cpt[0..1], $cpt[2]-2, $cpt[3]-2, 255, 405);
			$canvas-> lineWidth(0);
			$canvas-> color( $clr[0]);
			$canvas-> ellipse( @cpt);
		}

		if ( $self-> {stdPointer}) {
			my $dev = $range * 0.03;
			my @j = (
				$self-> offset2pt( @cpt[0,1], $self-> {value}, $rad * 0.8),
				$self-> offset2pt( @cpt[0,1], $self-> {value} + $dev, $rad * 0.6),
				$self-> offset2pt( @cpt[0,1], $self-> {value} - $dev, $rad * 0.6),
			);
			$self-> fillpoly( \@j);
		} else {
			my @cxt = ( $self-> offset2pt( @cpt[0,1], $self-> {value}, $rad - 10), 4, 4);
			$canvas-> lineWidth(2);
			$canvas-> color( $c3d[0]);
			$canvas-> arc( @cxt[0..1], 3, 3, 65, 235);
			$canvas-> color( $c3d[1]);
			$canvas-> arc( @cxt[0..1], 3, 3, 255, 405);
			$canvas-> lineWidth(0);
			$canvas-> color( $clr[0]);
		}

		my $i;
		unless ( defined $self-> {singlePaint}) {
			for ( $i = 0; $i < scalar @{$tval}; $i++) {
				my $r = $rad + 3 + $$tlen[ $i];
				my ( $cos, $sin) = $self-> offset2data( $$tval[$i]);
				$canvas-> line( $self-> offset2pt( @cpt[0,1], $$tval[$i], $rad + 3),
					$cpt[0] + $r * $cos, $cpt[1] + $r * $sin
				) if $$tlen[ $i];
				$r += 3;
				if ( defined $$ttxt[ $i]) {
					my $y = $cpt[1] + $r * $sin - $fh / 2 * ( 1 - $sin);
					my $x = $cpt[0] + $r * $cos - 
						( 1 - $cos) * 
						$canvas-> get_text_width( $$ttxt[ $i]) / 2;
					$canvas-> text_out( $$ttxt[ $i], $x, $y);
				}
			}
		}
	} else {
		$canvas-> bar( 0, 0, @size);
		$canvas-> color( $clr[0]);
	}

	my $ttw = $canvas-> get_text_width( $self-> {string});
	$canvas-> text_out( $self-> {string}, ( $size[0] - $ttw) / 2, $size[1] / 2 - $fh / 3)
	if $ttw < $rad || !$self-> {circAlive};
	return if defined $self-> {singlePaint};

	$ttw = $canvas-> get_text_width( $self-> text);
	$canvas-> text_out( $self-> text, ( $size[0] - $ttw) / 2, 2);

	if ( $self-> {buttons}) {
		my $s = $self-> {pressState};
		my @cbd = reverse @c3d;
		my $at  = 0;
		$at = 1, @cbd = reverse @cbd if $s & 1;
		
		$canvas-> rect3d( 
			$self-> { butt1X}, $self-> { butt1Y}, $self-> { butt1X} + DefButtonX,
			$self-> { butt1Y} + DefButtonX, 1, @cbd, $clr[1]
		);
		$canvas-> line( 
			$self-> { butt1X} + 2 + $at, $self-> { butt1Y} + DefButtonX / 2 - $at,
			$self-> { butt1X} - 2 + + DefButtonX + $at, $self-> {butt1Y} + DefButtonX / 2 - $at
		);

		@cbd = reverse @c3d; $at = 0;
		$at = 1, @cbd = reverse @cbd if $s & 2;
		$canvas-> rect3d( 
			$self-> { butt2X}, $self-> { butt1Y}, $self-> { butt2X} + DefButtonX,
			$self-> { butt1Y} + DefButtonX, 1, @cbd, $clr[1]
		);
		$canvas-> line( 
			$self-> { butt2X} + 2 + $at, $self-> { butt1Y} + DefButtonX / 2 - $at,
			$self-> { butt2X} - 2 + + DefButtonX + $at, $self-> {butt1Y} + DefButtonX / 2 - $at
		);
		$canvas-> line( 
			$self-> { butt2X} + DefButtonX / 2 + $at, $self-> { butt1Y} + 2 - $at,
			$self-> { butt2X} + DefButtonX / 2 + $at, $self-> { butt1Y} - 2 - $at + DefButtonX
		);
	}

	$canvas-> rect_focus(
		( $size[0] - $ttw) / 2 - 1, 1, 
		( $size[0] + $ttw) / 2 + 1, $fh + 2
	) if $self-> focused && ( length( $self-> text) > 0);
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
		my $s = $self-> {step};
		$self-> clear_event;
		$self-> set_next_value(( $key == kb::Left || $key == kb::Down) ? -$s : $s);
	}
}

sub xy2val
{
	my ( $self, $x, $y) = @_;
	$x -= $self-> {circX};
	$y -= $self-> {circY};
	my $a  = atan2( $y, $x);
	my $pi = atan2( 0, -1);
	$a += $pi / 2;
	$a += $pi * 2 if $a < 0;
	$a = $self-> {min} + abs( $self-> {max} - $self-> {min}) * ( $pi * 1.75 - $a) * 2 / ( 3 * $pi);
	my $s = $self-> {step};
	$a = int( $a) if int( $s) - $s == 0;
	my $inCircle = ( abs($x) < $self-> {br} * 0.3 + 3 and abs($y) < $self-> {br} * 0.3 + 3);
	return $a, $inCircle;
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {readOnly};
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	my @butt = ( 
		$self-> {butt1X}, $self-> {butt1Y}, 
		$self-> {butt2X}, $self-> {butt1X} + DefButtonX, 
		$self-> {butt1Y} + DefButtonX, $self-> {butt2X} + DefButtonX
	);
	if ( $self-> {buttons} and $y >= $butt[1] and $y < $butt[4]) {
		if ( $x >= $butt[0] and $x < $butt[3]) {
			$self-> {pressState} = 1;
			$self-> invalidate_rect( @butt[0..1], $butt[3] + 1, $butt[4] + 1);
		}
		if ( $x >= $butt[2] and $x < $butt[5]) {
			$self-> {pressState} = 2;
			$self-> invalidate_rect( $butt[2], $butt[1], $butt[5] + 1, $butt[4] + 1);
		}
		if ( $self-> {pressState} > 0) {
			$self-> {mouseTransaction} = $self-> {pressState};
			$self-> update_view;
			$self-> capture(1);
			$self-> scroll_timer_start;
			$self-> scroll_timer_semaphore(0);
			$self-> value( $self-> value + 
				$self-> step * (($self-> {pressState} == 1) ? -1 : 1));
			return;
		}
	}

	my ( $val, $inCircle) = $self-> xy2val( $x, $y);
	return unless $inCircle;
	$self-> {mouseTransaction} = 3;
	$self-> value( $val);
	$self-> capture(1);
	$self-> clear_event;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless $self-> {mouseTransaction};
	my @butt = ( 
		$self-> {butt1X}, $self-> {butt1Y}, $self-> {butt2X},
		$self-> {butt1X} + DefButtonX, $self-> {butt1Y} + DefButtonX, 
		$self-> {butt2X} + DefButtonX
	);
	$self-> scroll_timer_stop;
	$self-> {pressState} = 0;
	if ( $self-> {mouseTransaction} == 1) {
		$self-> invalidate_rect( @butt[0..1], $butt[3] + 1, $butt[4] + 1);
		$self-> update_view;
	}
	if ( $self-> {mouseTransaction} == 2) {
		$self-> invalidate_rect( $butt[2], $butt[1], $butt[5] + 1, $butt[4] + 1);
		$self-> update_view;
	}
	my $mt = $self-> {mouseTransaction};
	$self-> {mouseTransaction} = undef;
	$self-> capture(0);
	$self-> notify( 'Change') if $mt == 3 && !$self-> {autoTrack};
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	if ( $self-> {mouseTransaction} == 3) {
		my $ov = $self-> {value};
		$self-> {suppressNotify} = 1 unless $self-> {autoTrack};
		$self-> value( $self-> xy2val( $x, $y));
		$self-> {suppressNotify} = 0;
		$self-> notify(q(Track)) if !$self-> {autoTrack} && $ov != $self-> {value};
	} elsif ( $self-> {pressState} > 0) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
		$self-> value( $self-> value + 
			$self-> step * (( $self-> {mouseTransaction} == 1) ? -1 : 1));
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

sub set_buttons
{
	$_[0]-> {buttons} = $_[1];
	$_[0]-> repaint;
}

sub set_std_pointer
{
	$_[0]-> {stdPointer} = $_[1];
	$_[0]-> repaint;
}

sub stdPointer  {($#_)?$_[0]-> set_std_pointer ($_[1]):return $_[0]-> {stdPointer};}
sub buttons     {($#_)?$_[0]-> set_buttons     ($_[1]):return $_[0]-> {buttons};}

sub value
{
	return $_[0]-> {value} unless $#_;
	my ( $self, $value) = @_;
	my ( $min, $max) = ( $self-> {min}, $self-> {max});
	my $old = $self-> {value};
	$value = $min if $value < $min;
	$value = $max if $value > $max;
	if ( $self-> {snap}) {
		my ( $minDist, $thatVal, $i) = ( abs( $min - $max));
		my $tval = $self-> {tickVal};
		for ( $i = 0; $i < scalar @{$tval}; $i++) {
			my $j = $$tval[ $i];
			$minDist = abs(($thatVal = $j) - $value) if abs( $j - $value) < $minDist;
		}
		$value = $thatVal if defined $thatVal;
	} elsif ( $self-> {step} != 0 ) {
		$value = int ( $value / $self-> {step} ) * $self-> {step};
	}
	return if $old == $value;

	$self-> {value} = $value;
	$self-> notify(q(Stringify), $value, \$self-> {string});
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
	$self-> notify(q(Change)) unless $self-> {suppressNotify};
}

1;

__DATA__

=pod

=head1 NAME

Prima::Sliders - sliding bars, spin buttons and input lines, dial widget etc.

=head1 DESCRIPTION

The module is a set of widget classes, with one
common property; - all of these provide input and / or output of an integer value.
This property unites the following set of class hierarchies:

	Prima::AbstractSpinButton
		Prima::SpinButton
		Prima::AltSpinButton

	Prima::SpinEdit

	Prima::Gauge

	Prima::AbstractSlider
		Prima::Slider
		Prima::CircularSlider

=head1 Prima::AbstractSpinButton

Provides a generic interface to spin-button class functionality, which includes
range definition properties and events. Neither C<Prima::AbstractSpinButton>, nor
its descendants store the integer value. These provide a mere possibility for
the user to send incrementing or decrementing commands.

The class is not usable directly.

=head2 Properties

=over

=item state INTEGER

Internal state, reflects widget modal state, for example,
is set to non-zero when the user performs a mouse drag action. The exact meaning of C<state>
is defined in the descendant classes.

=back

=head2 Events

=over

=item Increment DELTA

Called when the user presses a part of a widget that is responsible for
incrementing or decrementing commands. DELTA is an integer value, 
indicating how the associated value must be modified.

=item TrackEnd

Called when the user finished the mouse transaction.

=back

=head1 Prima::SpinButton

A rectangular spin button, consists of three parts, divided horizontally. 
The upper and the lower parts are push-buttons associated with singular
increment and decrement commands. The middle part, when dragged by mouse,
fires C<Increment> events with delta value, based on a vertical position
of the mouse pointer.

=head1 Prima::AltSpinButton

A rectangular spin button, consists of two push-buttons, associated
with singular increment and decrement command. Comparing to C<Prima::SpinButton>,
the class is less functional but has more stylish look.

=head1 Prima::SpinEdit

The class is a numerical input line, paired with a spin button.
The input line value can be change three ways - either as a direct
traditional keyboard input, or as spin button actions, or as mouse
wheel response. The class provides value storage and range 
selection properties.

=head2 Properties

=over

=item circulate BOOLEAN

Selects the value modification rule when the increment or decrement
action hits the range. If 1, the value is changed to the opposite limit
value ( for example, if value is 100 in range 2-100, and the user
clicks on 'increment' button, the value is changed to 2 ). 

If 0, the value does not change.

Default value: 0

=item editClass STRING

Assigns an input line class.

Create-only property.

Default value: C<Prima::InputLine>

=item editDelegations ARRAY

Assigns the input line list of delegated notifications.

Create-only property.

=item editProfile HASH

Assigns hash of properties, passed to the input line during the creation.

Create-only property.

=item max INTEGER

Sets the upper limit for C<value>.

Default value: 100.

=item min INTEGER

Sets the lower limit for C<value>.

Default value: 0

=item pageStep INTEGER

Determines the multiplication factor for incrementing/decrementing
actions of the mouse wheel.

Default value: 10

=item spinClass STRING

Assigns a spin-button class.

Create-only property.

Default value: C<Prima::AltSpinButton>

=item spinProfile ARRAY

Assigns the spin-button list of delegated notifications.

Create-only property.

=item spinDelegations HASH

Assigns hash of properties, passed to the spin-button during the creation.

Create-only property.

=item step INTEGER

Determines the multiplication factor for incrementing/decrementing
actions of the spin-button.

Default value: 1

=item value INTEGER

Selects integer value in range from C<min> to C<max>, reflected in the input line.

Default value: 0.

=back

=head2 Methods

=over

=item set_bounds MIN, MAX

Simultaneously sets both C<min> and C<max> values.

=back

=head2 Events

=over

=item Change

Called when C<value> is changed.

=back

=head1 Prima::Gauge

An output-only widget class, displays a progress bar and an eventual percentage string.
Useful as a progress indicator.

=head2 Properties

=over

=item indent INTEGER

Selects width of a border around the widget.

Default value: 1

=item max INTEGER

Sets the upper limit for C<value>.

Default value: 100.

=item min INTEGER

Sets the lower limit for C<value>.

Default value: 0

=item relief INTEGER

Selects the style of a border around the widget. Can be one of the
following C<gr::XXX> constants:

	gr::Sink    - 3d sunken look
	gr::Border  - uniform black border
	gr::Raise   - 3d risen look

Default value: C<gr::Sink>.

=item threshold INTEGER

Selects the threshold value used to determine if the changes to C<value>
are reflected immediately or deferred until the value is changed more
significantly. When 0, all calls to C<value> result in an immediate
repaint request.

Default value: 0

=item value INTEGER

Selects integer value between C<min> and C<max>, reflected in the progress bar and 
eventual text.

Default value: 0.

=item vertical BOOLEAN

If 1, the widget is drawn vertically, and the progress bar moves from bottom to top.
If 0, the widget is drawn horizontally, and the progress bar moves from left to right.

Default value: 0

=back

=head2 Methods

=over

=item set_bounds MIN, MAX

Simultaneously sets both C<min> and C<max> values.

=back

=head2 Events

=over

=item Stringify VALUE, REF

Converts integer VALUE into a string format and puts into REF scalar reference.
Default stringifying conversion is identical to C<sprintf("%2d%%")> one.

=back

=head1 Prima::AbstractSlider

The class provides basic functionality of a sliding bar, equipped with
tick marks. Tick marks are supposed to be drawn alongside the main sliding axis or 
circle and provide visual feedback for the user.

The class is not usable directly.

=head2 Properties

=over

=item autoTrack BOOLEAN

A boolean flag, selects the way notifications execute when the user mouse-drags
the sliding bar. If 1, C<Change> notification is executed as soon as C<value>
is changed. If 0, C<Change> is deferred until the user finished the mouse drag;
instead, C<Track> notification is executed when the bar is moved.

This property can be used when the action, called on C<Change> performs very 
slow, so the eventual fast mouse interactions would not thrash down the program.

Default value: 1

=item increment INTEGER

A step range value, used in C<scheme> for marking the key ticks.
See L<scheme> for details.

Default value: 10

=item max INTEGER

Sets the upper limit for C<value>.

Default value: 100.

=item min INTEGER

Sets the lower limit for C<value>.

Default value: 0

=item readOnly BOOLEAN

If 1, the use cannot change the value by moving the bar or otherwise.

Default value: 0

=item ticks ARRAY

Selects the tick marks representation along the sliding axis or circle.
ARRAY consists of hashes, each for one tick. The hash must contain
at least C<value> key, with integer value. The two additional keys,
C<height> and C<text>, select the height of a tick mark in pixels
and the text drawn near the mark, correspondingly.

If ARRAY is C<undef>, no ticks are drawn.

=item scheme INTEGER

C<scheme> is a property, that creates a set of tick marks
using one of the predefined scale designs, selected by C<ss::XXX> constants.
Each constant produces different scale; some make use of C<increment> integer
property, which selects a step by which the additional 
text marks are drawn. As an example, C<ss::Thermometer> design with
default C<min>, C<max>, and C<increment> values would look like that:

	0   10   20        100
	|    |    |          |
	|||||||||||||||....|||

The module defines the following constants:

	ss::Axis          - 5 minor ticks per increment
	ss::Gauge         - 1 tick per increment
	ss::StdMinMax     - 2 ticks at the ends of the bar
	ss::Thermometer   - 10 minor ticks per increment, longer text ticks

When C<tick> property is set, C<scheme> is reset to C<undef>.

=item snap BOOLEAN

If 1, C<value> cannot accept values that are not on the tick scale.
When set such a value, it is rounded to the closest tick mark.
If 0, C<value> can accept any integer value in range from C<min> to C<max>.

Default value: 0

=item step INTEGER

Integer delta for singular increment / decrement commands and
a threshold for C<value> when C<snap> value is 0.

Default value: 1

=item value INTEGER

Selects integer value between C<min> and C<max> and the corresponding sliding bar
position.

Default value: 0.

=back

=head2 Events

=over

=item Change

Called when C<value> value is changed, with one exception:
if the user moves the sliding bar while C<autoTrack> is 0, C<Track>
notification is called instead.

=item Track

Called when the user moves the sliding bar while C<autoTrack> value is 0;
this notification is a substitute to C<Change>.

=back

=head1 Prima::Slider

Presents a linear sliding bar, movable along a linear shaft. 

=head2 Properties

=over

=item borderWidth INTEGER

In horizontal mode, sets extra margin space between the slider line and
the widget boundaries. Can be used for fine tuning of displaying text
labels from <ticks()>, where the default spacing (0) or spacing procedure 
(drop overlapping labels) is not enough.

=item ribbonStrip BOOLEAN

If 1, the parts of shaft are painted with different colors, to increase
visual feedback. If 0, the shaft is painted with single default background color.

Default value: 0

=item shaftBreadth INTEGER

Breadth of the shaft in pixels.

Default value: 6

=item tickAlign INTEGER

One of C<tka::XXX> constants, that correspond to the situation of tick marks:

	tka::Normal        - ticks are drawn on the left or on the top of the shaft
	tka::Alternative   - ticks are drawn on the right or at the bottom of the shaft
	tka::Dual          - ticks are drawn both ways

The ticks orientation ( left or top, right or bottom ) is dependant on C<vertical>
property value.

Default value: C<tka::Normal>

=item vertical BOOLEAN

If 1, the widget is drawn vertically, and the slider moves from bottom to top.
If 0, the widget is drawn horizontally, and the slider moves from left to right.

Default value: 0

=back

=head2 Methods

=over

=item pos2info X, Y

Translates integer coordinates pair ( X, Y ) into the value corresponding to the scale,
and returns three scalars: 

=over

=item info INTEGER

If C<undef>, the user-driven positioning is not possible ( C<min> equals to C<max> ).

If 1, the point is located on the slider.

If 0, the point is outside the slider.

=item value INTEGER

If C<info> is 0 or 1, contains the corresponding C<value>.

=item aperture INTEGER

Offset in pixels along the shaft axis.

=back

=back

=head1 Prima::CircularSlider

Presents a slider widget with the dial and two increment / decrement buttons.
The tick marks are drawn around the perimeter of the dial; current value
is displayed in the center of the dial.

=head2 Properties

=over

=item buttons BOOLEAN

If 1, the increment / decrement buttons are shown at the bottom of the dial,
and the user can change the value either by the dial or by the buttons.
If 0, the buttons are not shown.

Default values: 0

=item stdPointer BOOLEAN

Determines the style of a value indicator ( pointer ) on the dial.
If 1, it is drawn as a black triangular mark. 
If 0, it is drawn as a small circular knob.

Default value: 0

=back

=head2 Methods

=over

=item offset2data VALUE

Converts integer value in range from C<min> to C<max> into
the corresponding angle, and return two real values:
cosine and sine of the angle.

=item offset2pt X, Y, VALUE, RADIUS

Converts integer value in range from C<min> to C<max> into the
point coordinates, with the RADIUS and dial center coordinates
X and Y. Return the calculated point coordinates
as two integers in (X,Y) format.

=item xy2val X, Y

Converts widget coordinates X and Y into value in range from C<min> 
to C<max>, and return two scalars: the value and the boolean flag,
which is set to 1 if the (X,Y) point is inside the dial circle,
and 0 otherwise.

=back

=head2 Events

=over

=item Stringify VALUE, REF

Converts integer VALUE into a string format and puts into REF scalar reference.
The resulting string is displayed in the center of the dial. 

Default conversion routine simply copies VALUE to REF as is.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>,
Anton Berezin E<lt>tobez@tobez.orgE<gt>.

=head1 SEE ALSO

L<Prima>, F<examples/fontdlg.pl>

=cut
