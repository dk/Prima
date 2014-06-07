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

examples/mdi.pl - A MDI ( multiple-document interface ) example

=item FEATURES

Outlines usage of Prima::MDI module.
Note the MDI window are not subject for a window-manager 
decorations and do not conform to the system user interaction scheme.

=cut

use strict;
use Prima qw( InputLine ImageViewer MDI Application);

package Generic;

my $w;

sub icons
{
	my ( $self, $menu, $const) = @_;
	my $set = $self-> menu-> toggle( $menu);
	my $bi  = $w-> borderIcons;
	$set ? ( $bi |= $const) : ( $bi &= ~$const);
	$w-> borderIcons( $bi);
}

my $wwx = Prima::MDIWindowOwner-> create(
	size => [ 300, 300],
	text => 1,
	name => 1,
	selectable => 1,
	onDestroy => sub { $::application-> close},
	menuItems => [
		[ '~Style' => [
			[ '~Sizeable' => sub { $w-> borderStyle( bs::Sizeable);}],
			[ 'S~ingle'   => sub { $w-> borderStyle( bs::Single);}],
			[ '~Dialog'   => sub { $w-> borderStyle( bs::Dialog);}],
			[ '~None'     => sub { $w-> borderStyle( bs::None);}],
		]],
		[ 'S~tate' => [
			[ '~Minimize' => sub { $w-> minimize;}],
			[ 'Ma~ximize' => sub { $w-> maximize;}],
			[ 'Restore'   => sub { $w-> restore;}],
		]],
		[ '~Icons' => [
			[ '*titlebar' => '~Title bar'  => sub { icons( @_, mbi::TitleBar)}, ],
			[ '*sys' => '~System menu'     => sub { icons( @_, mbi::SystemMenu)}, ],
			[ '*min' => '~Minimize button' => sub { icons( @_, mbi::Minimize)}, ],
			[ '*max' => 'Ma~ximize button' => sub { icons( @_, mbi::Maximize)}, ],
			[ '*cls' => '~Close button'    => sub { icons( @_, mbi::Close)}, ],
		]],
		[ '~Drag mode' => [
			[ '~System defined' => sub { $w-> dragMode( undef);}],
			[ '~Dynamic' => sub { $w-> dragMode( 1);}],
			[ '~Old fashioned' => sub { $w-> dragMode( 0);}],
		]],
		[ '~Windows' => [
			[ '~New' => 'Ctrl+N' => '^N' => sub{ $_[0]-> insert( 'MDI'); }],
			[ '~Arrange icons' => sub{ $_[0]-> arrange_icons;} ],
			[ '~Cascade' => sub{ $_[0]-> cascade; } ],
			[ '~Tile' => sub{ $_[0]-> tile;} ],
		]],
	],
);

$w = Prima::MDI-> create(
	owner => $wwx,
	clipOwner => 0,
	size => [200, 200],
	icon => Prima::StdBitmap::icon(sbmp::DriveCDROM),
	font => { size => 6 },
	titleHeight => 12,
);


my $i = Prima::Image-> create;
$i-> load('Hand.gif');

$w-> client-> insert( ImageViewer =>
image => $i,
pack  => { expand => 1, fill => 'both' },
);

run Prima;
