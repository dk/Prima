use strict;
use warnings;

use Test::More;
use Prima::sys::Test;

my $d = Prima::Drawable-> create( width => 1, height => 1, type => im::RGB);
my (@z, $i, @fpo, $fillPatternCount);
goto R;

$d-> color( 0x123456);
is( $d-> color, 0x123456, 'color' );

$d-> backColor( 0x654321);
is( $d-> backColor, 0x654321, 'backColor' );

$d-> fillPattern( [0..7]); $i = 0;
$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
is( $fillPatternCount, 0, 'fillPattern' );

$d-> fillPatternOffset( 5,4);
@fpo = $d-> fillPatternOffset;
is( $fpo[0], 5, 'fillPatternOffset.x' );
is( $fpo[1], 4, 'fillPatternOffset.y' );

$d-> lineEnd( le::Square);
is( $d-> lineEnd, le::Square, 'lineEnd' );

$d-> miterLimit( 5.0 );
is( int( $d-> miterLimit + .5), 5, 'miterLimit' );

$d-> lineWidth( 5);
is( $d-> lineWidth, 5, 'lineWidth' );

$d-> linePattern( lp::Dash);
is( $d-> linePattern, lp::Dash, 'linePattern' );

$d-> rop( rop::NotSrcXor);
is( $d-> rop, rop::NotSrcXor, 'rop' );

$d-> rop2( rop::NotSrcXor);
is( $d-> rop2, rop::NotSrcXor, 'rop2' );

$d-> translate( 1, 2);
@z = $d-> translate;
is_deeply( \@z, [1,2], 'translate' );

$d-> textOpaque( 1);
is( $d-> textOpaque, 1, 'textOpaque' );

$d-> textOutBaseline( 1);
is( $d-> textOutBaseline, 1, 'textOutBaseline' );

$d-> lineJoin( lj::Bevel);
is( $d-> lineJoin ,lj::Bevel, "lineJoin");

$d-> fillMode( fm::Alternate);
is( $d-> fillMode, fm::Alternate, "fillMode");

$d-> begin_paint;
$d-> end_paint;
is( $d-> color, 0x123456, 'color' );
is( $d-> backColor, 0x654321, 'backColor' );

$i = 0;
$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
is( $fillPatternCount, 0, "fillPattern" );
is( $d-> lineEnd, le::Square, "lineEnd" );
is( $d-> lineWidth, 5, "lineWidth" );
is( $d-> linePattern, lp::Dash, "linePattern" );
is( int( $d-> miterLimit + .5), 5, 'miterLimit' );
is( $d-> rop, rop::NotSrcXor, "rop" );
is( $d-> rop2 , rop::NotSrcXor, "rop2");

@z = $d-> translate;
is_deeply( \@z, [1,2], "translate" );
is( $d-> textOpaque, 1, "textOpaque" );
is( $d-> textOutBaseline, 1, "textOutBaseline" );
is( $d-> lineJoin, lj::Bevel, "lineJoin" );
is( $d-> fillMode, fm::Alternate, "fillMode");

R:
$d = Prima::DeviceBitmap-> new( width => 8, height => 8, type => dbt::Pixmap);

sub bits
{
	my $type = $_[0] // im::Byte;
	my $i = $d->image;
	$i->type($type) if $i->type != $type;
	$i = $i->data;
	my $lw = ($type == im::RGB) ? 24 : 8;
	return join("\n", '',map { unpack("H*", substr( $i, $_ * $lw, $lw )) } reverse 0..$d->height-1);
}

my ($bits1, $bits2);

sub check
{
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

check( antialias => 0,   1,  act => sub { $d->polyline([2,2,6,2,6,6]); });
check( alpha     => 200, 50, act => sub { $d->bar(2,2,6,6); });
$d->alpha(255);
$d->antialias(0);

for my $aa ( 0, 1) {
	$d->antialias($aa);

	check( color     => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) } );
	check( backColor => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) });

	$d->color(cl::White);
	$d->backColor(cl::Black);
	check( fillMode => fm::Alternate, fm::Winding, act => sub { $d->fillpoly([1,1,6,6,6,1,1,6,6,3]) });

	$d-> fillPattern(fp::Solid);
	$d->graphic_context( fillPattern => [0..7], sub {
		$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
		is( $fillPatternCount, 0, 'gc.in.fillPattern' );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	$i = 0;
	$fillPatternCount = scalar grep { $_ == 0xff } @{$d-> fillPattern};
	is( $fillPatternCount, 8, 'gc.out.fillPattern' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPattern");

	$d-> fillPattern([0xff,0xff,0x00,0x00,0xCA,0xCA,0xAC,0xAC]);
	$d-> fillPatternOffset( 1,2);
	$d->graphic_context( fillPatternOffset => [5,4], sub {
		@fpo = $d-> fillPatternOffset;
		is( $fpo[0], 5, 'gc.in.fillPatternOffset.x' );
		is( $fpo[1], 4, 'gc.in.fillPatternOffset.y' );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	@fpo = $d-> fillPatternOffset;
	is( $fpo[0], 1, 'gc.out.fillPatternOffset.x' );
	is( $fpo[1], 2, 'gc.out.fillPatternOffset.y' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPatternOffset");
	$d-> fillPattern(fp::Solid);

	$d->lineWidth(4);
	check( lineJoin => lj::Miter,  lj::Round, act => sub { $d->polyline([2,2,6,2,6,6]); });
	check( lineEnd  => le::Square, le::Round, act => sub { $d->polyline([2,2,6,2,6,6]) });
	$d->lineWidth(0);

	check( lineWidth => 2, 5, act => sub { $d->polyline([2,2,6,2,6,6]) });
	check( linePattern => lp::Dot, lp::Dash, act => sub { $d->polyline([2,2,6,2,6,6]) });

	$d->lineWidth(2);
	$d->linePattern(lp::Solid);
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
		is_deeply( [$d-> translate], [4,5], 'gc.in.translate' );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	is_deeply( [$d-> translate], [1,2], 'gc.out.translate' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.translate");
	$d->translate(0,0);

	$d->font->height(8);
	$d->backColor(cl::Yellow);
	$d->color(cl::Black);
	check( textOpaque => 1, 0, type => im::RGB, act => sub { $d->bar(0,0,8,8); $d->text_out("__",0,0); });
	$d->color(cl::White);
	$d->backColor(cl::Black);

	check( textOutBaseline => 1, 0, act => sub { $d->text_out("fg",0,0); });
}

done_testing;
