=pod

=head1 NAME

examples/buttons.pl - Prima button widgets

=head1 FEATURES

Demonstrates basic use of Prima toolkit:
creation of built-in push-buttons, radio-buttons, and
a customized subclassed button

=cut

use strict;
use warnings;

use Prima qw(Buttons Application Drawable::Metafile);

package UserButton;
use vars qw(@ISA);
@ISA = qw(Prima::Button);

sub on_click
{
	#print "User button has been pressed\n";
	print ".";
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my ($x, $y) = $canvas-> size;
	my @fcap = ( $canvas-> get_text_width( $self-> text), $canvas-> font-> height);
	$canvas-> color( $self-> pressed ? cl::LightGreen : cl::Green);
	$canvas-> bar(0, 0, $x - 1 , $y - 1);

	$canvas-> color( $self-> pressed ? cl::Green : cl::LightGreen);
	$canvas-> line ( 0, 0, 0, $y - 1);
	$canvas-> line ( 1, 1, 1, $y - 2);
	$canvas-> line ( 0, $y - 1, $x - 1, $y - 1);
	$canvas-> line ( 1, $y - 2, $x - 2, $y - 2);
	$canvas-> color( $self-> pressed ? cl::White : cl::Gray);
	$canvas-> line ( 0, 0, $x - 1, 0);
	$canvas-> line ( 1, 1, $x - 2, 1);
	$canvas-> line ( $x - 1, 0, $x - 1, $y - 1);
	$canvas-> line ( $x - 2, 1, $x - 2, $y - 2);

	$canvas-> color(cl::Black);
	$canvas-> text_out ( $self-> text,
		($x - $fcap[ 0]) / 2,
		($y - $fcap[ 1]) / 2
	);
	if ( $self-> default) {
		$canvas-> linePattern( lp::Dot);
		$canvas-> rectangle(
			($x - $fcap[ 0]) / 2 - 3,
			($y - $fcap[ 1]) / 2 - 3,
			($x + $fcap[ 0]) / 2 + 3,
			($y + $fcap[ 1]) / 2 + 3
		);
		$canvas-> linePattern( lp::Solid);
	}
}

package main;

sub set_style
{
	$_[0]->skin($_[1]);
	$_[0]->repaint;
}

my $w = Prima::MainWindow-> create(
	text   => "Button example",
	centered  => 1,
	height    => 300,
	width     => 400,
	designScale => [7, 16],
	menuItems => [
		['~Skin' => [
			[ '(classic'  => 'Classic 3D' => \&set_style ],
			[ ')flat'     => 'Flat'       => \&set_style ],
		]],
	],
);
$w->insert( "Button",
	origin => [  50,180], 
	pressed => 1, 
	onClick => sub {
		$w->Meta->enabled(!$w->Meta->enabled);
	}
);
my $l = $w->insert( "UserButton" , origin => [ 250,180], autoRepeat => 1);

$w->insert( "Radio"      , origin => [  50,140]);


my $c = 3.14159 * 2 / 7;
my @pts = map { 35 + int(25 * $_ + .5) } (
	1,0,
	cos(3*$c), sin(-3*$c),
	cos($c), sin($c), 
	cos(2*$c), sin(-2*$c),
	cos(2*$c), sin(2*$c),
	cos($c), -sin($c),
	cos(3*$c), sin(3*$c),
	1,0,
);

my $metafile = Prima::Drawable::Metafile->new( size => [70, 70] );
$metafile->begin_paint;
$metafile->antialias(1);
$metafile->lineJoin(lj::Miter);
$metafile->call( sub {
	my ($self, $canvas) = @_;
	my $f = $canvas->fader_current_value // 0;
	$canvas->color(cl::blend( $canvas->map_color(cl::Fore), cl::Green, $f));
	$canvas->lineWidth( 5 * ( 1 - $f) );
	my $method = ( $f == 1 ) ? 'fillpoly' : 'polyline';
	$canvas->$method(\@pts);
});
$metafile->end_paint;

$w->insert(
	"Button"     ,
	origin => [ 50,50],
	size => [80,80],
	image => $metafile,
	text => '',
	name => 'Meta',
);

run Prima;
