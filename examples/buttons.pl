=pod

=head1 NAME

examples/buttons.pl - Prima button widgets

=head1 FEATURES

Demonstrates basic use of Prima toolkit, in particular
creation of built-in push-buttons and radio-buttons. L<Prima::Buttons>
A customized button creation with subclassing is exemplified

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

sub clone
{
	my ( $self, %profile) = @_;
	my %d = %{$self-> profile_default};
	my %readfilter = (
		x_centered  => 1,
		y_centered  => 1,
		centered    => 1,
		delegations => 1,
		designScale => 1,
	);
	my %hashed = (
		font      => 1,
		popupFont => 1,
		menuFont  => 1,
	);
	for ( keys %readfilter) { delete $d{$_}};
	for ( keys %d) {
		my @res = $self-> $_();
		$d{$_} = @res ? ($#res ? (
			$hashed{$_} ? {@res} : [@res]
		) : $res[0]) : undef;
	}
	return ref($self)-> create( %d, %profile);
}

package main;

my $w = Prima::MainWindow-> create(
	text   => "Button example",
	centered  => 1,
	height    => 300,
	width     => 400,
	designScale => [7, 16],
);
$w->insert( "Button"     , origin => [  50,180], pressed => 1);
my $l = $w->insert( "UserButton" , origin => [ 250,180], autoRepeat => 1);
$w->insert( "Radio"      , origin => [  50,140]);
my $metafile = Prima::Drawable::Metafile->new( size => [25, 25] );
$metafile->begin_paint;
$metafile->lineJoin(lj::Miter);
$metafile->lineWidth(5);
my $c = 3.14159 * 2 / 7;
my @pts = map { 25 * ($_ + .5)  } (
	1,0,
	cos(3*$c), sin(-3*$c),
	cos($c), sin($c), 
	cos(2*$c), sin(-2*$c),
	cos(2*$c), sin(2*$c),
	cos($c), -sin($c),
	cos(3*$c), sin(3*$c),
	1,0,
);
$metafile->polyline(\@pts);
$metafile->end_paint;
$w->insert( "Button"     , origin => [ 50,50], size => [80,80],image => $metafile, text => '' );

run Prima;
