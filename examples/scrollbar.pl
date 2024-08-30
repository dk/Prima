=pod

=head1 NAME

examples/scrollbar.pl - scrollbar example

=head1 FEATURES

Tests correct representation of colors created by combinations of R,G, and B
components. The Prima::Widget::sizeMin and Prima::Widget::sizeMax
implementation is tested too - the Area widget changes its height on a mouse
click.  Note how the Area widget maintains its maximum size when the window
gets maximized.

=cut

use strict;
use warnings;
use Prima qw(ScrollBar);
use Prima::Application name => 'scrollbar';

package MyWindow;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);

sub updateArea
{
	$_[0]-> Area-> backColor(
		$_[0]-> Blue-> value|($_[0]-> Green-> value<<8)|($_[0]-> Red-> value<<16)
	);
}

sub Timer1_Tick
{
	$_[ 0]-> Red  -> backColor(( $_[ 0]-> Red  -> backColor == cl::Red  ) ? cl::LightRed   : cl::Red  );
	$_[ 0]-> Green-> backColor(( $_[ 0]-> Green-> backColor == cl::Green) ? cl::LightGreen : cl::Green);
	$_[ 0]-> Blue -> backColor(( $_[ 0]-> Blue -> backColor == cl::Blue ) ? cl::LightBlue  : cl::Blue );
}

package UserInit;

my $w = MyWindow-> new(
	text => "Scrollbar & timer example",
	left    => 100,
	bottom  => 100,
	width   => 300,
	height  => 300,
	designScale => [7, 16],
	borderStyle => bs::Sizeable,
);

$w-> insert(
	"ScrollBar",
	 name     => "Red",
	 origin   => [ 2, 20],
	 vertical => 0,
	 max      => 255,
	 width    => 298,
	 onChange => sub { $w-> updateArea },
);
$w-> insert(
	"ScrollBar",
	 name     => "Green",
	 origin   => [ 2, 40],
	 vertical => 0,
	 max      => 255,
	 width    => 298,
	 onChange => sub { $w-> updateArea },
);
$w-> insert(
	"ScrollBar",
	 name     => "Blue",
	 origin   => [ 2, 60],
	 vertical => 0,
	 max      => 255,
	 width    => 298,
	 onChange => sub { $w-> updateArea },
);

$w-> insert(
	"ScrollBar",
	 name     => "Bluex",
	 origin   => [ 2, 80],
	 vertical => 1,
	 max      => 255,
	 height    => 218,
);


$w-> insert(
	"Widget",
	name      => "Area",
	rect      => [ 20, 100, 280, 280],
	backColor => cl::Black,
	growMode  => gm::GrowHiX | gm::GrowHiY,
	sizeMin   => [120, 120],
	sizeMax   => [420, 420],
	onPaint   => sub {
		my ($x,$y)=$_[0]-> size;
		$_[0]-> color($_[0]-> backColor);
		$_[0]-> bar(0,0,$x,$y);
		$_[0]-> color(cl::Set);
		$_[0]-> rop(rop::XorPut);
		$_[0]-> line(0,0,$x,$y);
		$_[0]-> line(0,$y,$x,0);
	},
	onMouseDown =>sub{
		$_[0]-> height($_[0]-> height+(($_[1]==1)?1:-1));
	},
);
my $t = $w-> insert( Timer=>
	timeout=> 2000,
	name => 'Timer1',
	delegations => ['Tick'],
);
$t-> start;

run Prima;
