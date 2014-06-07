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
#
=pod 

=head1 NAME

examples/transparent.pl - Prima toolkit example

=head1 FEATURES

Demonstrates the usage of Prima::Widget::transparent property.
Test the certain events: for example, when two transparent
widgets are located one upon another, when a graphic content changed
under a transparent widget, etc.

Note that the $tt widget is not transparent but uses shape extension.

=cut

use strict;
use warnings;

use Prima;
use Prima::Const;
use Prima::Application name => 'Generic.pm';

my $w = Prima::MainWindow-> create(
	size => [ 300, 300],
	borderStyle => bs::Dialog,
	backColor => cl::Green,
	onCreate => sub {
		$_[0]-> {delta} = 0;
	},
	onPaint => sub {
		my ($self,$canvas) = @_;
		my $c = $self-> color;
		$canvas-> color( $self-> backColor);
		$canvas-> bar(0,0,$self-> size);
		$canvas-> color( $c);
		my $d = $self-> {delta};
		my $i;
		for ( $i = -1; $i < 7; $i++)
		{
			$canvas-> text_out("Hello!", $d + $i * 40, $d + $i * 40);
		}
	},
);

$w-> insert( Timer =>
	timeout => 100,
	onTick  => sub {
		$w-> {delta} += 2;
		$w-> { delta} = 0 if $w-> { delta} >= 40;
		$w-> repaint;
	}
)-> start;

$w-> insert(
	Widget =>
	origin => [ 90, 90],
	transparent => 1,
	onPaint => sub {
		my $self = $_[0];
		$self-> color( cl::LightGreen);
		$self-> lineWidth( 4);
		$self-> line( 3, 3, $self-> size);
		$self-> ellipse( 50, 50, 80, 80);
	},
	onMouseDown => sub {
		$_[0]-> bring_to_front;
	},
);


my $string = "Hello from Prima::OnScreenDisplay!";
my $tt = Prima::Widget-> create(
	name => 'W1',
	onPaint => sub {
		$_[1]-> color( cl::LightRed);
		$_[1]-> font-> size( 36);
		$_[1]-> text_out( $string, 0, 0);
	},
	onMouseDown => sub {
		$_[0]-> {drag}    = [ $_[3], $_[4]];
		$_[0]-> {lastPos} = [ $_[0]-> left, $_[0]-> bottom];
		$_[0]-> capture(1);
		$_[0]-> repaint;
	},
	onMouseMove => sub{
		return unless exists $_[0]-> { drag};
		my @org = $_[0]-> origin;
		my @st  = @{$_[0]-> {drag}};
		my @new = ( $org[0] + $_[2] - $st[0], $org[1] + $_[3] - $st[1]);
		$_[0]-> origin( $new[0], $new[1]) if $org[1] != $new[1] || $org[0] != $new[0];
	},
	onMouseUp => sub {
		$_[0]-> capture(0);
		delete $_[0]-> {drag};
	},
);

$tt-> begin_paint_info;
$tt-> font-> size(36);
my $font = $tt-> font;
$tt-> width( $tt-> get_text_width( $string));
$tt-> end_paint_info;

my $i = Prima::Image-> create( width => $tt-> width, height => $tt-> height,
type => im::BW, conversion => ict::None);
$i-> begin_paint;
$i-> color( cl::Black);
$i-> bar(0,0,$i-> size);
$i-> color( cl::White);
$i-> font($font);
$i-> text_out( $string, 0, 0);
$i-> end_paint;
$tt-> shape($i);
$tt-> bring_to_front;

run Prima;
