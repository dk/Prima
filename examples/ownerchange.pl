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
=item NAME

A recreate functionality example.

=item FEATURES

Widgets that change owner dynamically often gets 
recreated internally - one system window gets
destroyed and another created then. Test the correct
implementation of Prima owner change functionality.

=cut

use strict;
use Prima qw(Buttons Application);

my $w = Prima::Window-> create(
	name       => "D1",
	text    => "Window Number One",
	origin     =>  [ 100, 300],
	borderStyle=> bs::Sizeable,
	size       =>  [ 350, 100],
	backColor  => cl::Green,
	popupItems => [["Change owner"=> sub { $_[0]-> popup-> owner (( $_[0]-> name eq "D1") ? $::application-> D2 : $::application-> D1); }]],
	menuItems  => [["Change owner"=> sub { $_[0]-> menu-> owner (( $_[0]-> name eq "D1") ? $::application-> D2 : $::application-> D1); }]],
	onTimer    => sub { $_[0]-> backColor(($_[0]-> backColor == cl::Green) ? cl::LightGreen : cl::Green)},
	onMouseDown => sub {
	my ( $self, $btn, @k) = @_;
		$_[0]-> borderStyle( ($btn  == mb::Left) ? bs::Dialog : bs::Sizeable);
	},
);

my $w2 = Prima::Window-> create(
	name      => "D2",
	text   => "Window Number Two",
	origin    =>  [ 500, 300],
	size      =>  [ 450, 200],
	font      => { name=>"System VIO",size=>18},
	backColor => cl::Yellow,
	onTimer   => sub { 
		$_[0]-> backColor(($_[0]-> backColor == cl::Yellow) ? 
			cl::White : cl::Yellow)
	},
);

$w-> insert( Button =>
	rect => [ 10 ,10, 50, 30],
	text => "<",
	font => { height => 18},
	onClick => sub { $_[0]-> owner-> borderIcons(bi::Minimize|bi::TitleBar)},
);
$w-> insert( Button =>
	rect => [ 60 , 10, 100, 30],
	text => ">",
	font => { height => 18},
	onClick => sub { $_[0]-> owner-> borderIcons(
		bi::TitleBar|bi::SystemMenu|bi::Minimize|bi::Maximize)},
);


$w-> insert( Button =>
	growMode => gm::Center,
	text  => "Change owner",
	onClick  => sub {
		my $oldOwner = $_[0]-> owner;
		$_[0]-> owner (( $_[0]-> owner-> name eq "D1") ? 
			$::application-> D2 : $::application-> D1);
		my $timer = $::application-> Timer1;
		if ( $timer-> {win} == $w)
		{
			$timer-> {win} = $w2;
		} else {
			$timer-> {win} = $w;
		}
	},
);

$::application-> insert( Timer =>
	timeout  => 1000,
	name     => Timer1 =>
	onCreate => sub { $_[0]-> start; $_[0]-> {win} = $w; },
	onTick   => sub {
		return unless $_[0]-> {win}-> alive;
		$_[0]-> {win}-> backColor( $_[0]-> {win}-> backColor ^ 0x00FFFFFF);
	},
);

run Prima;
