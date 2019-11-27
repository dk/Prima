use strict;
use warnings;

use Test::More;
use Prima::Test qw(noX11);

sub bytes { unpack('H*', shift ) }

sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   bytes(" . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
#	exit unless $ok;
}

my $i = Prima::Image->create(
	width    => 2,
	height   => 4,
	type     => im::Byte,
	data     => "12345678",
	lineSize => 2,
);

my $j = $i->dup;
$j->rotate(90);
is( $j->width, 4, "rotate(90) width ok");
is( $j->height, 2, "rotate(90) height ok");
is($j->data, "75318642", "rotate(90) data ok");

$j = $i->dup;
$j->rotate(270);
is( $j->width, 4, "rotate(270) width ok");
is( $j->height, 2, "rotate(270) height ok");
is($j->data, "24681357", "rotate(270) data ok");

$j->rotate(180);
is( $j->width, 4, "rotate(180) width ok");
is( $j->height, 2, "rotate(180) height ok");
is($j->data, "75318642", "rotate(180) data ok");

my $k = Prima::Image->create(
	width    => 2,
	height   => 2,
	type     => im::Short,
	data     => "12345678",
	lineSize => 4,
);

$k->rotate(90);
is($k->data, "56127834", "short: rotate(90) data ok");

$k->data("12345678");
$k->rotate(270);
is($k->data, "34781256", "short: rotate(270) data ok");

$k->data("12345678");
$k->rotate(180);
is($k->data, "78563412", "short: rotate(180) data ok");

# mirroring
$j->data("12345678");
$j->mirror(1);
is( $j->data, "56781234", "byte: vertical mirroring ok");
$j->data("12345678");
$j->mirror(0);
is( $j->data, "43218765", "byte: horizontal mirroring ok");

$j->type(im::Short);
$j->data("123456789ABCDEFG");
$j->mirror(1);
is( $j->data, "9ABCDEFG12345678", "short: vertical mirroring ok");
$j->data("123456789ABCDEFG");
$j->mirror(0);
is( $j->data, "78563412FGDEBC9A", "short: horizontal mirroring ok");

# rotation
$k = Prima::Image->create(
	width    => 140,
	height   => 140,
	type     => im::Byte,
);

$k->bar(0,0,$k->size);
$k->pixel(138, 70, cl::White);

my $p = Prima::Image->create(
	width    => 200,
	height   => 200,
	type     => im::Byte,
);
$p->bar(0,0,$k->size);
for (my $i = 0; $i < 360; $i++) {
	my $d = $k->dup;
	$d->rotate($i);
	my $dx = ($p-> width  - $d->width) / 2; 
	my $dy = ($p-> height - $d->height) / 2; 
	$p->put_image($dx,$dy,$d,rop::OrPut);
}
my $sum = $p->sum / 255;
ok(( $sum > 250 && $sum < 430), "rotation 360 seems performing");
$p->color(cl::Black);
$p->lineWidth(8);
$p->ellipse( 100, 100, 138, 138 );
is( $p->sum, 0, "rotation 360 is correct");

$p = Prima::Icon->create(
	width    => 2,
	height   => 2,
	type     => im::Byte,
	maskType => 8,
	mask     => "\1\2..\3\4",
	data     => "\4\5..\6\7",
);
$p->shear(2,0);
is_bytes( $p->data, "\4\5\0\0\0\0\6\7", "xshear(2) integral data");
is_bytes( $p->mask, "\1\2\0\0\0\0\3\4", "xshear(2) integral mask");

$p = Prima::Image->create(
	width    => 2,
	height   => 2,
	type     => im::Byte,
	data     => "\1\4..\3\6");
$p->shear(0,0.5);
$p = $p->data;
$p = substr($p,0,2) . substr($p,4,2) . substr($p,8,2);
is_bytes( $p, join('', map { chr } (1, (0+4)/2), 3, (4+6)/2, 0, (6+0)/2), "yshear subpixel");

done_testing;
