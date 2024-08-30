=pod

=head1 NAME

examples/rotation.pl - custom drawing example

=head1 FEATURES

Needs custom fonts for antialiasing emulation.

=cut

use strict;
use warnings;

use Prima qw(Application);

#my @data = (
#  'XXXXXXXX',
#  'X......X',
#  'X......X',
#  'X......X',
#  'X......X',
#  'X......X',
#  'X......X',
#  'XXXXXXXX',
#);

my @data = (
'XXXX    XXXX    X X     X  XXXX ',
'X   X   X   X   X XX   XX X    X',
'X    X  X    X  X X X X X X    X',
'X   X   X   X   X X  X  X X    X',
'XXXX    XXXX    X X     X XXXXXX',
'X       X   X   X X     X X    X',
'X       X   X   X X     X X    X',
'X       X    X  X X     X X    X',
'X       X    X  X X     X X    X',
);


my $xdim = length( $data[0]) - 1;
my $ydim = $#data;
my $antialias = 1;
my @box  = ([0,$ydim], [$xdim,$ydim], [$xdim,0], [0,0]);

sub f
{
	my ( $x, $y, $a) = @_;
	my $r = sqrt( $x * $x + $y * $y);
	return
		$x * cos( $a) - $y * sin( $a),
		$x * sin( $a) + $y * cos( $a);
}

sub fc
{
	my ( $x, $y, $sin, $cos) = @_;
	my $r = sqrt( $x * $x + $y * $y);
	return
		$x * $cos - $y * $sin,
		$x * $sin + $y * $cos;
}


sub round
{
	return ( $_[0] < 0) ? int( $_[0] - 0.5) : int( $_[0] + 0.5);
}

sub imgbin
{
	return 0 if $_[1] < 0 || $_[0] < 0 || $_[1] > $ydim || $_[0] > $xdim;
	return ( substr( $data[ $ydim - $_[1]], $_[0], 1) eq 'X') ? 1 : 0;
}

sub ds
{
	if ( $_[0] < 0.125) { return ' '}
	elsif ( $_[0] < 0.375) { return '.'}
	elsif ( $_[0] < 0.625) { return ':'}
	elsif ( $_[0] < 0.875) { return '+'}
	else {  return 'x'};
}


sub rotate {
my $a = $_[0];
my ( $s1, $c1) = ( sin($a), cos($a));
my ( $s2, $c2) = ( sin(-$a), cos(-$a));
my @sbox;
my @dbox = ([-1,$ydim+1], [$xdim+1,$ydim+1], [$xdim+1,-1], [-1,-1]);
for ( 0..3) {
	my @x = fc( @{$box[$_]}, $s1, $c1);
	$sbox[$_] = [ round( $x[0]), round( $x[1])];
	@x = fc( @{$dbox[$_]}, $s1, $c1);
	$dbox[$_] = [ round( $x[0]), round( $x[1])];
}
my @r = (0,0,0,0);
for ( 0..3 ) { $r[0] = $sbox[$_]-> [0] if $r[0] > $sbox[$_]-> [0];}
for ( 0..3 ) { $r[1] = $sbox[$_]-> [1] if $r[1] > $sbox[$_]-> [1];}
for ( 0..3 ) { $r[2] = $sbox[$_]-> [0] if $r[2] < $sbox[$_]-> [0];}
for ( 0..3 ) { $r[3] = $sbox[$_]-> [1] if $r[3] < $sbox[$_]-> [1];}


my @rs = (('.'x($r[2]-$r[0]+1)) x ($r[3]-$r[1]+1));

my ( $x, $y);
for ( $y = $r[1]; $y <= $r[3]; $y++) {
	for ( $x = $r[0]; $x <= $r[2]; $x++) {
		my ( $sx, $sy) = fc( $x, $y, $s2, $c2);
		unless  ( $antialias) {
			$sx = round( $sx);
			$sy = round( $sy);
			substr( $rs[ $y - $r[1]], $x - $r[0], 1, imgbin( $sx, $sy) ? '*' : ' ');
		} else {
			my $fx = int( $sx) - (( $sx > 0) ? 0 : 1);
			my $fy = int( $sy) - (( $sy > 0) ? 0 : 1);
			substr( $rs[ $y - $r[1]], $x - $r[0], 1,  ds(
				( imgbin( $fx,     $fy) * ( 1 - $sx + $fx) * ( 1 - $sy + $fy)) +
				( imgbin( $fx + 1, $fy) * ( $sx - $fx) * ( 1 - $sy + $fy)) +
				( imgbin( $fx, $fy + 1) * ( 1 - $sx + $fx) * ( $sy - $fy)) +
				( imgbin( $fx + 1, $fy + 1) * ( $sx - $fx) * ( $sy - $fy)))
			);
		}
	}
}
return $r[0], $r[1], @rs;
};

my $a = 1;
my $w = Prima::MainWindow-> new
(
text => 'Rotating line',
font => { pitch => fp::Fixed, style => fs::Bold },
menuItems =>
	[[ '~Options' => [
		[ '@*a' => '~Antialias' => sub { $antialias = $_[2] }],
	],
]],

buffered => 1,
onPaint => sub {
	my ( $self, $canvas) = @_;
	$canvas-> color( cl::Back);
	$canvas-> bar( 0, 0, $canvas-> size);
	$canvas-> color( cl::Fore);
	my ( $x, $y, @lines) = rotate( $a);
	my ( $fh, $fw) = ( $canvas-> font-> height, $canvas-> font-> width);
	my $dy = 0;
	my ( $X, $Y) = map { $_ / 2 } $self-> size;
	for ( @lines) {
		$canvas-> text_out( $_, $X + $x * $fw, $Y + ( $dy + $y) * $fh );
		$dy++;
	}
	$canvas-> text_out( "$x $y ".(int($a * 180 / 3.14159)), 0, 0);
},
);

$w-> insert( Timer =>
	timeout => 100,
	onTick => sub {
		$a += 0.1;
		$a -= 6.28 if $a > 6.28;
		$w-> repaint;
	},
)-> start;

run Prima;
