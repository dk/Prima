# $Id$

print "1..3 create,get,set,nearest\n";

my $p = [0,1,2,3,4,5];
my $c = [0x020100, 0x050403];
my $i = Prima::Image-> create(
	width => 1,
	height => 1,
	type => im::Mono,
	colormap => $c,
);

sub cmpx
{
	my ( $a, $b) = @_;
	return 0 unless @$a == @$b;
	my $i;
	for ( $i = 0; $i < @$a; $i++) {
		return 0 unless $a->[$i] == $b-> [$i];
	}
	return 1;
}

# 1 
ok( cmpx( $i-> palette, $p));

# 2
ok( cmpx( [ $i-> colormap ], $c)); 

# 3
$i-> colormap( @$c);
ok( cmpx( $i-> palette, $p));

# 4
my $cc = $i-> get_nearest_color(0x030101);
ok( $cc == 0x020100);
