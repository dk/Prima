use strict;
use Prima;
use Prima::Classes;
use Prima::Const;
use Prima::Buttons;
use Prima::ScrollBar;
use IPA;
use IPA::Local;
use IPA::Point;
use IPA::Global;
use Prima::Application name => 'Test application';

print STDERR "=====\n";
my $im = Prima::Image-> create;
$im-> load( "P:/EUGENE/EU-DEMO/meet-hannover/pic1-coom.gif");
$im-> type( im::Byte);
$im = IPA::Local::unionFind( $im, method => 'ave', threshold => 40);
$im = IPA::Point::threshold( $im, minvalue => 0, maxvalue => 120);
$im = IPA::Global::fill_holes( $im);
$im = IPA::Global::area_filter( $im, minArea => 300);
my $c = IPA::Global::identify_contours( $im);
print STDERR "=====\n";


sub sizeprint {
   my $me = shift;
   print "Size of ", $me-> name, " is now ", $me-> width, "x", $me-> height, "\n";
}

my $w = Prima::Window-> create( #size => [400, 300], origin => [100, 100],
			 name => "Test Window",
			 text => "apc_graphics test",
			 backColor => cl::LightGray,
			 color => cl::Black,
			 onMouseDown => sub {
			    my ($me,$btn,$mod,$x,$y) = @_;
			    print "Down: B:$btn M:$mod X:$x Y:$y\n";
			    my (@xy) = $me-> get_pointer_pos;
			    print "@xy\n";
			 },
			 onMouseUp => sub {
			    my ($me,$btn,$mod,$x,$y) = @_;
			    print "Up: B:$btn M:$mod X:$x Y:$y\n";
			 },
			 onMouseClick => sub {
			    my ($me,$btn,$mod,$x,$y,$dbl) = @_;
			    print "Click: B:$btn M:$mod X:$x Y:$y D:$dbl\n";
			 },
			 onMouseWheel => sub {
			    my ($me,$mod,$x,$y,$btn) = @_;
			    $me-> ScrollBar1-> value( $me-> ScrollBar1-> value
						      + ($btn < 0?1:-1)*$me-> ScrollBar1-> pageStep);
			 },
			 onPaint => sub {
print STDERR "=== BEGIN PAINT ===\n";
			    my $me = $_[0];
			    my ($w,$h) = $me-> size;
			    my $fore = $me-> color;

			    $me-> color( $me-> backColor);
			    $me-> bar( 0, 0, $w, $h);

			    $me-> color( $fore);
			    $me-> text_out( "This is a very preliminary stuff", 10, 170);

			    $me-> font-> size( 24);
			    $me-> text_out( "This is a very preliminary stuff", 10, 200);

			    $me-> put_image( 10, 250, $im);
			    $me-> color( cl::LightGreen);
			    for my $pts (()) { # (@$c) {
			       my @pts = @$pts;
			       my (@x,@y);
			       while (@pts) {
				  push @x, 10 + shift @pts;
				  push @y, 250 + shift @pts;
			       }
			       while (@x > 1) {
				  $me-> line( $x[0], $y[0], $x[1], $y[1]);
				  shift @x;
				  shift @y;
			       }
			    }

			    $me-> color( 0xa0a0a0);
			    $me-> bar( 40, 40, 160, 160);

			    # check for horizontal lines
			    $me-> color( cl::LightRed);
			    $me-> line( 50, 98, 150, 98);
			    $me-> line( 150, 102, 50, 102);
			    $me-> pixel( 50, 100, cl::LightCyan);
			    $me-> pixel( 150, 100, cl::LightCyan);

			    # check for vertical lines
			    $me-> line( 98, 50, 98, 150);
			    $me-> line( 102, 150, 102, 50);
			    $me-> pixel( 100, 50, cl::LightCyan);
			    $me-> pixel( 100, 150, cl::LightCyan);

			    # check for bar
			    $me-> color( cl::Red);
			    $me-> bar( 70, 70, 80, 80);
			    $me-> pixel( 70, 70, cl::LightCyan);
			    $me-> pixel( 80, 80, cl::LightCyan);
			    $me-> pixel( 70, 80, cl::LightCyan);
			    $me-> pixel( 80, 70, cl::LightCyan);

			    # check for rectangle
			    $me-> rectangle( 120, 120, 130, 130);
			    $me-> pixel( 120, 120, cl::LightCyan);
			    $me-> pixel( 130, 130, cl::LightCyan);
			    $me-> pixel( 120, 130, cl::LightCyan);
			    $me-> pixel( 130, 120, cl::LightCyan);

print STDERR "=== END PAINT ===\n";
			 },
			onSize => sub { sizeprint $_[0]; },
			onMove => sub {
			   my ($me) = @_;
			   my ($x,$y) = $me-> origin;
			},
			onClose => sub {
			   print "Closing me!\n";
			   $::application-> destroy;
			},
		       );

$w-> insert( Button =>
	     origin => [ 170, 70],
	     text => "~Press!",
	     onClick => sub {
		$::application-> destroy;
	     },
	     onMouseEnter => sub { $_[0]-> backColor( cl::LightCyan); },
	     onMouseLeave => sub { $_[0]-> backColor( cl::Back); },
	     onSize => sub { sizeprint $_[0]; },
	     onMouseWheel => sub {
		my ($me,$mod,$x,$y,$btn) = @_;
		print "Wheel: B:$btn M:$mod X:$x Y:$y\n";
	     },
	   );

print "Window width: ", $w-> width, "\n";
$w-> insert( ScrollBar =>
	     vertical => 1,
	     left    => $w-> width - 16,
	     width    => 16,
	     height   => $w-> height,
	     bottom   => 0,
#	     growMode => gm::GrowHiY,
#	     growMode => 0, #gf::GrowLoX | gf::GrowHiY,
	     value    => 30,
	     onSize => sub { sizeprint $_[0]; },
	   );

$w-> insert( Button =>
	     origin => [ 170, 30],
	     text => "~Don't",
	     # enabled => "",
	     onSize => sub { sizeprint $_[0]; },
	   );

run Prima;

