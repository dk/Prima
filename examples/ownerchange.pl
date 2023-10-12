=pod

=head1 NAME

examples/ownerchange.pl - implicit re-creation of widgets

=head1 FEATURES

Widgets that dynamically change owners are re-created internally - a system
window gets destroyed and another created thereafter. The example tests the
correct implementation of the owner-changing functionality

=cut

use strict;
use warnings;
use Prima qw(Buttons Application);

my $w = Prima::Window-> create(
	name       => "D1",
	text    => "Window Number One",
	origin     =>  [ 100, 300],
	designScale => [7, 16],
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
	designScale => [7, 16],
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
	onClick => sub { $_[0]-> owner-> borderIcons(bi::Minimize|bi::TitleBar)},
);
$w-> insert( Button =>
	rect => [ 60 , 10, 100, 30],
	text => ">",
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
