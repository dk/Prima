use strict;
use Prima::Classes;
use Prima::InputLine;
use Prima::ImageViewer;
use Prima::MDI;

package Generic;
$::application = Prima::Application-> create(name => 'mdi');

my $w;

sub icons
{
   my ( $self, $menu, $const) = @_;
   my $set = $self-> menu-> toggle( $menu);
   my $bi  = $w-> borderIcons;
   $set ? ( $bi |= $const) : ( $bi &= ~$const);
   $w-> borderIcons( $bi);
}

my $ww = Prima::MDIWindow-> create(
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
          [ '*titlebar' => '~Title bar'  => sub { icons( @_, bi::TitleBar)}, ],
          [ '*sys' => '~System menu'     => sub { icons( @_, bi::SystemMenu)}, ],
          [ '*min' => '~Minimize button' => sub { icons( @_, bi::Minimize)}, ],
          [ '*max' => 'Ma~ximize button' => sub { icons( @_, bi::Maximize)}, ],
       ]],
       [ '~Drag mode' => [
          [ '~System defined' => sub { $w-> dragMode( undef);}],
          [ '~Dynamic' => sub { $w-> dragMode( 1);}],
          [ '~Old fashioned' => sub { $w-> dragMode( 0);}],
       ]],
       [ '~Windows' => [
         [ '~New' => 'Ctrl+N' => '^N' => sub{ $_[0]->insert( 'MDI')}],
         [ '~Arrange icons' => sub{ $_[0]-> arrange_icons;} ],
         [ '~Cascade' => sub{ $_[0]-> cascade;} ],
         [ '~Tile' => sub{ $_[0]-> tile;} ],
       ]],
    ],
);


$ww-> insert( "InputLine",
   text => '1002',
   onChange => sub { $w-> text( $_[0]-> text);},
) if 1;

$w = Prima::MDI-> create(
   owner => $ww,
   size => [200, 200],
  icon => Prima::StdBitmap::icon(sbmp::DriveCDROM),
)-> client if 1;

my $i = Prima::Image-> create;
$i->load('winnt256.bmp');

$w-> insert( ImageViewer =>
  origin => [0,0],
  size   => [ $w-> size],
  hScroll=> 1,
  vScroll=> 1,
  growMode => gm::Client,
  image => $i,
) if 1;

$w = $w-> owner;


run Prima;
