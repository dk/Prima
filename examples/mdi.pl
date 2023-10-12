=pod

=head1 NAME

examples/mdi.pl - MDI ( multiple-document interface ) example

=head1 FEATURES

Demonstrates usage of the Prima::MDI module.  Note the MDI windows are not
subject for window-manager decorations and do not conform to the system user
interaction policy.

=cut

use strict;
use warnings;
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
