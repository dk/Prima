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
#  $Id$
=pod 

=head1 NAME

examples/macro.pl - A macro recording program

=head1 FEATURES

Demonstration and test of mouse_event() function usage.

=cut

use strict;
use Prima;
use Prima::Classes;
use Prima::Buttons;
use Prima::MsgBox;
use Prima::StdDlg;
use Prima::Application;


my $state = 'empty';
my $wtx   = 'Macros';
my @data  = ();
my @savepos  = ();
my $ipos  = 0;
my $mstate;
my $doMouseMove = 0;

sub mopen
{
	my $f = Prima::open_file(
		filter    => [
			['Macro record' => '*.mrec'],
			['All files' => '*']
		],
	);
	return unless defined $f;
	if ( open F, $f) {
		stop();
		@data = ();
		while (<F>) {
			push ( @data, [ split(' ', $_)]);
		}
		close F;
		update();
	} else {
		MsgBox::message("Cannot load $f");
	}
}



sub msave
{
	unless ( scalar @data) {
		MsgBox::message("Nothing to save");
		return;
	}
	my $f = Prima::save_file(
		filter    => [
			['Macro record' => '*.mrec'],
			['All files' => '*']
		],
	);
	return unless defined $f;
	if ( open F, "> $f") {
		for ( @data) {
			print F "@$_\n";
		}
		close F;
	} else {
		MsgBox::message("Cannot save $f");
	}
}


my $w = Prima::MainWindow-> create(
	text => $wtx,
	size => [ 436, 54],
	menuItems => [[ '~File' => [
			['~Open...' => 'F3' => 'F3' => \&mopen],
			['~Save...' => 'F2' => 'F2' => \&msave],
		],
	]],
);


sub tick
{
	if ( $state eq 'rec') {
		push( @data, [ 
			$::application-> pointerPos, 
			$::application-> get_mouse_state, 
			$::application-> get_shift_state
		]);
		return;
	}
# play
	if ( ++$ipos >= scalar @data) {
		stop();
		return;
	}
	my $d = $data[$ipos];
	$::application-> pointerPos( $$d[0], $$d[1]);
	my $v = $::application-> get_widget_from_point( $$d[0], $$d[1]);
	if ( $v) {
		my ( $x, $y) = $v-> screen_to_client( $$d[0], $$d[1]);
		if ( $mstate != $$d[2]) {
			my $btn  = $mstate > $$d[2] ? ( $mstate - $$d[2]) : ( $$d[2] - $mstate);
			my $cmd  = $mstate > $$d[2] ? cm::MouseUp : cm::MouseDown;
			$v-> select if $cmd == cm::MouseDown && $btn == mb::Left;
			$v-> mouse_event( $cmd, $btn, $$d[3], $x, $y, 0, 0);
			$mstate = $$d[2];
		} elsif ( $doMouseMove) {
			$v-> mouse_event( cm::MouseMove, $$d[3], $x, $y, 0, 0);
		}
	}
}

$w-> insert( Timer =>
	timeout => 10,
	name    => 'Timer1',
	onTick  => \&tick,
);


sub update
{
	if ( $state eq 'empty') {
		$w-> Start -> enabled(0);
		$w-> Stop  -> enabled(0);
		$w-> Record-> enabled(1);
		$w-> Clean -> enabled(0);
		$w-> text( "$wtx");
	} elsif ( $state eq 'rec') {
		$w-> Start -> enabled(0);
		$w-> Stop  -> enabled(1);
		$w-> Record-> enabled(0);
		$w-> Clean -> enabled(0);
		$w-> text( "$wtx - recording");
	} elsif ( $state eq 'stop') {
		$w-> Start -> enabled(1);
		$w-> Stop  -> enabled(0);
		$w-> Record-> enabled(1);
		$w-> Clean -> enabled(1);
		$w-> text( "$wtx - stopped");
	} elsif ( $state eq 'play') {
		$w-> Start -> enabled(0);
		$w-> Stop  -> enabled(1);
		$w-> Record-> enabled(0);
		$w-> Clean -> enabled(0);
		$w-> text( "$wtx - playing");
	}
}

sub record
{
	clean();
	$state = 'rec';
	update;
	$w-> Timer1-> start;
}

sub start
{
	$state = 'play';
	update;
	@savepos = $::application-> pointerPos;
	$mstate  = $::application-> get_mouse_state;
	$w-> Timer1-> start;
}


sub stop
{
	$w-> Timer1-> stop;
	$::application-> pointerPos( @savepos) if $state eq 'play';
	$state = 'stop';
	update;
	$ipos = 0;
}

sub clean
{
	$w-> Timer1-> stop;
	$state = 'empty';
	update;
	@data = ();
	$ipos = 0;
}



$w-> insert( [ Button =>
	text    => '~Start',
	pack    => { side => 'left', padx => 10 },
	name    => 'Start',
	selectable => 0,
	onClick => \&start,
], [ Button =>
	text    => 'S~top',
	pack    => { side => 'left', padx => 10 },
	name    => 'Stop',
	selectable => 0,
	onClick => \&stop,
], [ Button =>
	text    => '~Record',
	pack    => { side => 'left', padx => 10 },
	name    => 'Record',
	selectable => 0,
	onClick => \&record,
], [ Button =>
	text    => '~Clean',
	pack    => { side => 'left', padx => 10 },
	name    => 'Clean',
	selectable => 0,
	onClick => \&clean,
],
);

update;

run Prima;
