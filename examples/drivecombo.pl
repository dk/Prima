use strict;
use Carp;
use Prima::ComboBox;
use Prima::Classes;

package UserInit;
$::application = Prima::Application-> create( name => "DriveCombo.pm");

my $w = Prima::Window-> create(
   text   => "Combo box",
   left   => 100,
   bottom => 300,
   width  => 250,
   height => 250,
   menuItems => [
      [ "~Normal" => sub {$_[0]->ComboBox1->style(cs::Simple);}],
      [ "~Drop down" => sub {$_[0]->ComboBox1->style(cs::DropDown);}],
      [ "~List" => sub {$_[0]->ComboBox1->style(cs::DropDownList);}],
   ],
   onDestroy=> sub { $::application-> destroy},
);

$w-> insert( DriveComboBox =>
   origin => [ 10, 10],
   width  => 200,
   name => 'ComboBox1',
#  height => 200,
   onChange => sub { $w-> DirectoryListBox1->path( $_[0]->text); },
);

$w-> insert( DirectoryListBox =>
   origin => [ 10, 40],
   width  => 200,
   height => 160,
   growMode => gm::Client,
   onChange => sub { print $_[0]-> path."\n"},
   name => 'DirectoryListBox1',
#  path => 'w:/',
) if 1;

run Prima;
