# overrides rect3d calls for some classes

use strict;
use Prima qw(Themes);

package Round3D;
use vars qw(@ISA);
@ISA=qw(Prima::Themes::Proxy);

sub rect3d
{
   my ( $self, $x, $y, $x1, $y1, $width, $lColor, $rColor, $backColor) = @_;
	my $canvas = $self-> {object};
	if ( defined $backColor) {
	   my $c = $canvas-> color;
      $canvas-> color( $backColor);
		$canvas-> bar( $x, $y, $x1, $y1);
      $canvas-> color( $c);
	}
	oval3d( $canvas, $x, $y, $x1, $y1, $width, $lColor, $rColor, 40);
}

sub oval3d
{
   my ( $self, $x, $y, $x1, $y1, $width, $lColor, $rColor, $maxd) = @_;
	( $x1, $x) = ( $x, $x1) if $x > $x1;
	( $y1, $y) = ( $y, $y1) if $y > $y1;
   my $bias = int($width / 2);
	$x += $bias;
	$y += $bias;
	$x1 -= $bias;
	$y1 -= $bias;
	my ( $dx, $dy) = ( $x1 - $x, $y1 - $y);
	$dx = $maxd if $dx > $maxd;
	$dy = $maxd if $dy > $maxd;
	my $d = ( $dx < $dy ) ? $dx : $dy;
	my $r = int($d/2);
#  plots 3-d roundrect:
# A'        B'
#  /------\  lines: AC, AB - light, BD, CD - dark
#  |A    B|  arcs: A - light, B - l/d, C - l/d, D - dark
#  |      |  arcs cannot have diameter larger than $maxd
#  |C    D|
#  \------/
# C'        D'
   my @r = ( 
	# coordinates of C and B, so A=r[0,3],B=r[2,3],C=r[0,1],D=[2,1]
	   $x + $r, $y + $r,
		$x1 - $r, $y1 - $r,
	# coordinates of C' and B'
	   $x, $y,
		$x1, $y1,
	);
   my $c = $self-> color;
	my $w = $self-> lineWidth;
	$self-> lineWidth( $width) if $width != $w;
   $self-> color( $lColor) if $lColor != $c;
	# light color
	$self-> line( @r[0,7,2,7]) if $r[0] < $r[2];
	$self-> line( @r[4,1,4,3]) if $r[1] < $r[3];
	$self-> arc( @r[0,3], $d, $d, 90, 180);
	$self-> arc( @r[2,3], $d, $d, 45, 90);
	$self-> arc( @r[0,1], $d, $d, 180, 225);
   $self-> color( $rColor);
	# dark color
	$self-> line( @r[0,5,2,5]) if $r[0] < $r[2];
	$self-> line( @r[6,1,6,3]) if $r[1] < $r[3];
	$self-> arc( @r[2,1], $d, $d, 270, 360);
	$self-> arc( @r[2,3], $d, $d, 0, 45);
	$self-> arc( @r[0,1], $d, $d, 225, 270);
	# restore pen style
   $self-> color( $c) if $c != $rColor;
	$self-> lineWidth( $w) if $width != $w;
}

my %wrap_paint = (
  onPaint => sub {
   $_[0]-> on_paint( Round3D-> new($_[1]));
}
);

Prima::Themes::register( 'Prima::themes::round3d', 'round3d', [ 
	'Prima::Button' => \%wrap_paint,
	'Prima::ScrollBar' => \%wrap_paint,
	'Prima::InputLine' => \%wrap_paint,
	],
);
