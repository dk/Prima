use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
my $unix = Prima::Application-> get_system_info-> {apc} == apc::Unix;

my $d = Prima::Drawable-> create( width => 1, height => 1, type => im::RGB, antialias => 1);
my (@z, $i, @fpo, $fillPatternCount);

my $can_antialias = $d->antialias;
my $can_alpha     = $d->can_draw_alpha;
$d->antialias(0);

sub test
{
	my ( $method, $value1, $value2) = @_;
	$d->$method($value1);
	$d->graphic_context( $method => $value2, sub {
		is( $d->$method(), $value2, 'in.'.$method);
	});
	is( $d->$method(), $value1, 'out.' . $method);
}

test( antialias => 0, 1 ) if $can_antialias;
test( alpha => 255, 10 ) if $can_alpha;
test( color => 0x123456, 0x654321 );
test( backColor => 0x654321, 0x123456 );

$d-> fillPattern(fp::Solid);
$d->graphic_context( fillPattern => [0..7], sub {
	$i = 0;
	$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
	is( $fillPatternCount, 0, 'in.fillPattern' );
});
$i = 0;
$fillPatternCount = scalar grep { $_ == 0xff } @{$d-> fillPattern};
is( $fillPatternCount, 8, 'out.fillPattern' );

$d-> fillPatternOffset( 1,2);
$d->graphic_context( fillPatternOffset =>[5,4], sub {
	@fpo = $d-> fillPatternOffset;
	is( $fpo[0], 5, 'in.fillPatternOffset.x' );
	is( $fpo[1], 4, 'in.fillPatternOffset.y' );
});
@fpo = $d-> fillPatternOffset;
is( $fpo[0], 1, 'out.fillPatternOffset.x' );
is( $fpo[1], 2, 'out.fillPatternOffset.y' );

my $fpx1 = Prima::Image->new( size => [1,1], type => im::RGB, data => "\xff\x00\xff" ); # for non-24 bit displays
my $fpx2 = Prima::Image->new( size => [1,1], type => im::RGB, data => "\x00\xff\x00" );

test( fillPattern => $fpx1, $fpx2);
$d->fillPattern(fp::Solid);

test( fillMode => fm::Alternate, fm::Winding);

$d->lineEnd([ [line=>[1,2]], 1]);
$d->graphic_context( lineEnd => 0, sub {
	is( $d->lineEnd, 0, 'in.lineEnd complex');
});
my $le = $d->lineEnd;
is_deeply($le, [[line => [1,2]], 1], 'out.lineEnd complex 1');
$d->lineEnd([line => [1,2]]);
is_deeply($d->lineEnd, [line => [1,2]]);
test( lineEnd => le::Square, le::Flat);

test( lineJoin => lj::Round, lj::Bevel);
test( linePattern => lp::Dash, lp::Dot);
test( lineWidth => 5, 2);
test( miterLimit => 5, 2);
test( rop => rop::NotSrcXor, rop::AndPut);
test( rop2 => rop::NotSrcXor, rop::AndPut);

is_deeply( $d->get_matrix, [1,0,0,1,0,0], 'default.matrix');
$d->set_matrix([1,0,0,2,0,0]);
$d->graphic_context( matrix => [1..6], sub {
	is_deeply( $d->get_matrix, [1,4,3,8,5,12], 'in.matrix');
});
is_deeply( $d->get_matrix, [1,0,0,2,0,0], 'out.matrix');
$d->set_matrix([1,0,0,1,0,0]);

$d-> translate( 2, 1);
$d-> graphic_context( translate =>[1, 2], sub {
	@z = $d-> translate;
	is_deeply( \@z, [1,2], 'in.translate' );
});
@z = $d-> translate;
is_deeply( \@z, [2,1], 'in.translate' );

test( textOpaque => 1,0);
test( textOutBaseline => 1,0);

$d-> begin_paint;
$d-> end_paint;
is( $d-> color, 0x123456, 'color' );
is( $d-> backColor, 0x654321, 'backColor' );

$i = 0;
$fillPatternCount = scalar grep { 0xff == $_ } @{$d-> fillPattern};
is( $fillPatternCount, 8, "fillPattern" );
is( $d-> lineEnd, le::Square, "lineEnd" );
is( $d-> lineJoin, lj::Round, "lineJoin" );
is( $d-> linePattern, lp::Dash, "linePattern" );
is( $d-> lineWidth, 5, "lineWidth" );
is( int( $d-> miterLimit + .5), 5, 'miterLimit' );
is( $d-> rop, rop::NotSrcXor, "rop" );
is( $d-> rop2 , rop::NotSrcXor, "rop2");

@z = $d-> translate;
is_deeply( \@z, [2,1], "translate" );
is( $d-> textOpaque, 1, "textOpaque" );
is( $d-> textOutBaseline, 1, "textOutBaseline" );
is( $d-> fillMode, fm::Alternate, "fillMode");

my $x = Prima::Image->new( size => [1,1]);
$x->fillPattern($fpx1);
$x-> begin_paint;
$x-> fillPattern(0);
$x-> end_paint;
is( ref($x->fillPattern), 'Prima::Image', 'fillPatternImage restored after end_paint');

$d = Prima::DeviceBitmap-> new( width => 8, height => 8, type => dbt::Pixmap);

sub bits
{
	my $type = $_[0] // im::Byte;
	my $i = (ref($d) eq 'Prima::Image') ? $d : $d->image;
	$i->type($type) if $i->type != $type;
	$i = $i->data;
	my $lw = ($type == im::RGB) ? 24 : 8;
	return join("\n", '',map { unpack("H*", substr( $i, $_ * $lw, $lw )) } reverse 0..$d->height-1);
}

my ($bits1, $bits2);

sub check
{
	local $Test::Builder::Level = $Test::Builder::Level + 1;
	my ( $method, $value1, $value2, %opt) = @_;
	my $xaa = $d->antialias ? '.aa' : '';
	$d->$method($value1);
	$d->graphic_context( $method => $value2, sub { is(
		$d->$method(), $value2, 'gc.in.'.$method);
		$d->clear;
		$opt{act}->();
		$bits1 = bits($opt{type} // im::Byte);
	});
	is( $d->$method(), $value1, 'gc.out.' . $method . $xaa);
	$d->clear;
	$opt{act}->();
	$bits2 = bits();
	isnt($bits1,$bits2,"gc.bits.$method$xaa");
}

check( antialias => 0,   1,  act => sub { $d->polyline([2,2,6,2,6,6]); }) if $can_antialias;
check( alpha     => 200, 50, act => sub { $d->bar(2,2,6,6); }) if $can_alpha;
$d->alpha(255);
$d->antialias(0);

for my $aa ( 0, 1) {
	next if $aa && !$can_antialias;
	$d->rop2(rop::NoOper);
	$d->antialias($aa);
	$d->clipRect(0,0,8,8);
	my $xaa = $aa ? '.aa' : '';

	check( color     => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) } );
	check( backColor => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) });

	$d->color(cl::White);
	$d->backColor(cl::Black);
	check( fillMode => fm::Alternate, fm::Winding, act => sub { $d->fillpoly([1,1,6,6,6,1,1,6,6,3]) });

	$d-> fillPattern(fp::Solid);
	$d->graphic_context( fillPattern => [0..7], sub {
		$i = 0;
		$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
		is( $fillPatternCount, 0, 'gc.in.fillPattern' );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	$i = 0;
	$fillPatternCount = scalar grep { $_ == 0xff } @{$d-> fillPattern};
	is( $fillPatternCount, 8, 'gc.out.fillPattern'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPattern".$xaa);

	$d-> fillPattern([0xff,0xff,0x00,0x00,0xCA,0xCA,0xAC,0xAC]);
	$d-> fillPatternOffset( 1,2);
	$d->graphic_context( fillPatternOffset => [5,4], sub {
		@fpo = $d-> fillPatternOffset;
		is( $fpo[0], 5, 'gc.in.fillPatternOffset.x'.$xaa );
		is( $fpo[1], 4, 'gc.in.fillPatternOffset.y'.$xaa );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	@fpo = $d-> fillPatternOffset;
	is( $fpo[0], 1, 'gc.out.fillPatternOffset.x'.$xaa );
	is( $fpo[1], 2, 'gc.out.fillPatternOffset.y'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPatternOffset".$xaa);
	$d-> fillPattern(fp::Solid);

	check( fillPattern => $fpx1, $fpx2, act => sub { $d->bar(0,0,7,7); });
	$d-> fillPattern(fp::Solid);

	$d->lineWidth(4);
	$d->lineEnd(le::Square);
	check( lineJoin => lj::Miter,  lj::Round, act => sub { $d->polyline([0,2,5,2,5,7]); });
	check( lineEnd  => le::Square, le::Round, act => sub { $d->polyline([2,2,6,2,6,6]) });
	$d->lineWidth(0);

	check( lineWidth => 2, 5, act => sub { $d->polyline([2,2,6,2,6,6]) });
	check( linePattern => lp::Dot, lp::Dash, act => sub { $d->polyline([2,2,6,2,6,6]) });
	$d->linePattern(lp::Solid);

	$d->lineWidth(2);
	check( miterLimit => 1, 5, act => sub { $d->polyline([0,0,3,3,3,0]) });
	$d->lineWidth(0);

	check( rop  => rop::NotPut, rop::CopyPut, act => sub { $d->bar(1,1,6,6) });
	$d->rop(rop::CopyPut);
	$d->fillPattern(fp::CloseDot);
	$d->backColor(cl::Yellow);
	check( rop2 => rop::NoOper, rop::CopyPut, type => im::RGB, act => sub { $d->bar(1,1,6,6) });
	$d->fillPattern(fp::Solid);
	$d->backColor(cl::Black);
	$d->rop2(rop::CopyPut);

	$d-> translate( 1, 2);
	$d->graphic_context( translate => [4,5], sub {
		is_deeply( [$d-> translate], [4,5], 'gc.in.translate'.$xaa );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	is_deeply( [$d-> translate], [1,2], 'gc.out.translate'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.translate".$xaa);
	$d->translate(0,0);

	$d->font->height(8);
	$d->backColor(cl::Yellow);
	$d->color(cl::Black);
	check( textOpaque => 1, 0, type => im::RGB, act => sub { $d->bar(0,0,8,8); $d->text_out("__",0,0); });
	$d->backColor(cl::Black);
	$d->color(cl::White);
	check( textOutBaseline => 1, 0, act => sub { $d->text_out("fg",0,0) });

	$d->clear;
	$d->clipRect(1,1,6,6);
	$d->graphic_context( clipRect => [2,2,5,5], sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.clipRect1".$xaa);
	$d->clipRect(0,0,$d->size);
	$d->clear;
	$d->graphic_context( clipRect => [2,2,5,5], sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.clipRect2".$xaa);

	$d->clear;
	$d->region( Prima::Region->new( rect =>[1,1,6,6] ));
	$d->graphic_context( region => Prima::Region->new( rect =>[2,2,5,5] ), sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.region1".$xaa);
	$d->clipRect(0,0,$d->size);
	$d->clear;
	$d->graphic_context( region => Prima::Region->new( rect =>[2,2,5,5] ), sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.region2".$xaa);
}

$d = Prima::Image->new( size => [8,8], type => im::Byte);
$d->clipRect(0,0,$d->size);
$d->clear;
$d->graphic_context( region => Prima::Region->new( rect =>[2,2,5,5] ), sub {
	$d->bar(0,0,7,7);
	$bits1 = bits;
});
$d->bar(0,0,7,7);
$bits2 = bits;
isnt($bits1,$bits2,"gc.image.region");

$d->color(0x123456);
$d->graphic_context_push;
$d->color(0x234567);
$d->begin_paint;
$d->color(0x345678);
$d->graphic_context( color => 0x456789, sub {} );
is( $d->color, 0x345678, "color in paint after gc");
$d->end_paint;
is( $d->color, 0x234567, "color outside paint before gc");
$d->graphic_context_pop;
is( $d->color, 0x123456, "color outside paint after gc");

done_testing;
