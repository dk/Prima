use strict;
use warnings;
use utf8;

use Test::More;
use Prima::sys::Test;
use Prima::noX11;
use Prima qw(Config);

my $noX11 = defined Prima::XOpenDisplay();

if ( $^O !~ /win32/i && 0 == grep { $_ eq 'freetype' } @{ $Prima::Config::Config{ldlibs} } ) {
	plan skip_all => 'not compiled with freetype';
}

my $i = Prima::Image->new( size => [50,50], type => im::BW );

is( $i-> get_paint_state, ps::Disabled, 'get_paint_state == ps::Disabled');
$i->get_font;
is( $i-> get_paint_state, ps::Disabled, 'get_paint_state == ps::Disabled');

unless ( $noX11 ) {
	ok( $i->begin_paint_info, 'begin paint info');
	is( $i-> get_paint_state, ps::Information, 'get_paint_state == ps::Information');
	$i->end_paint_info;
	is( $i-> get_paint_state, ps::Disabled, 'end_font_query == ps::Disabled');
}

my $x = scalar @{ $i->fonts // [] };
cmp_ok( $x, '>=', 1, "$x direct access fonts are reported");

$x = $i->font_mapper->count;
cmp_ok($x, '>=', 1, "$x fonts for shaping are reported");

$x = scalar @{ $i->font_encodings // [] };
cmp_ok($x, '>=', 1, "$x font encodings reported");

$i->clear;
is( $i->clone(type => im::Byte)->sum, $i->width * $i->height * 255, 'white background');
$i->text_out( '123', 10, 10);
isnt( $i->clone(type => im::Byte)->sum, $i->width * $i->height * 255, 'black text');

if (my $s = $i->text_shape("f\x{444}\x{555}")) {
	is( scalar @{$s->glyphs}, 3, "text shape");
	$i->clear;
	$i->text_out( $s, 10, 10);
	isnt( $i->clone(type => im::Byte)->sum, $i->width * $i->height * 255, 'shaped text');
}

for my $str (qw(im::bpp4 im::Byte im::RGB im::Short)) {
	my $type = eval $str;
	$i = Prima::Image->new( size => [50,50], type => $type);
	$i->clear;
	$i->text_out('123', 10, 10);
	cmp_ok( $i->clone(type => im::Byte)-> sum / $i->width / $i->height, '<', 255, "mono text on $str");
}

$i = Prima::Image->new( size => [50,50], type => im::Byte);
$i->clear;
$i->antialias(1);
$i->text_out('123', 10, 10);
cmp_ok( $i->sum / $i->width / $i->height, '<', 255, "blended black text on graybyte image");

$i = Prima::Image->new( size => [50,50], type => im::RGB);
$i->clear;
$i->antialias(1);
$i->color(cl::Black);
$i->text_out('123', 10, 10);
my $black = $i->clone(type => im::Byte)->sum / $i->width / $i->height;
cmp_ok( $black, '<', 255, "blended black text on RGB image");
$i->clear;
$i->color(0x808080);
$i->text_out('123', 10, 10);
my $gray = $i->clone(type => im::Byte)->sum / $i->width / $i->height;
cmp_ok($gray, '>', $black, 'blended gray text on RGB image is brighter than black');
cmp_ok($gray, '<', 255, 'blended gray text on RGB image is darker than white');
$i->clear;
$i->color(cl::Black);
$i->alpha(0x80);
$i->text_out('123', 10, 10);
my $alpha = $i->clone(type => im::Byte)->sum / $i->width / $i->height;
cmp_ok(abs($alpha-$gray), '<', 0.5, "gray(a=1) and black(a=0.5) are basically the same");

done_testing;
