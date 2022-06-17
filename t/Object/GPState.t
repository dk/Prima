use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
my $unix = Prima::Application-> get_system_info-> {apc} == apc::Unix;

my $d = Prima::Drawable-> create( width => 1, height => 1, type => im::RGB);
my (@z, $i, @fpo, $fillPatternCount);
<<<<<<< HEAD
<<<<<<< HEAD

sub test
{
	my ( $method, $value1, $value2) = @_;
	$d->$method($value1);
	$d->graphic_context( $method => $value2, sub {
		is( $d->$method(), $value2, 'in.'.$method);
	});
	is( $d->$method(), $value1, 'out.' . $method);
}

test( antialias => 0, 1 );
test( alpha => 255, 10 );
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

my $fpx1 = Prima::Image->new( size => [1,1], type => im::RGB, data => "\x11\x11\x11" );
my $fpx2 = Prima::Image->new( size => [1,1], type => im::RGB, data => "\x22\x22\x22" );
test( fillPattern => $fpx1, $fpx2);
$d->fillPattern(fp::Solid);

test( fillMode => fm::Alternate, fm::Winding);
test( lineEnd => le::Square, le::Flat);
test( lineJoin => lj::Round, lj::Bevel);
test( linePattern => lp::Dash, lp::Dot);
test( lineWidth => 5, 2);
test( miterLimit => 5, 2);
test( rop => rop::NotSrcXor, rop::AndPut);
test( rop2 => rop::NotSrcXor, rop::AndPut);

$d-> translate( 2, 1);
$d-> graphic_context( translate =>[1, 2], sub {
	@z = $d-> translate;
	is_deeply( \@z, [1,2], 'in.translate' );
});
@z = $d-> translate;
is_deeply( \@z, [2,1], 'in.translate' );

test( textOpaque => 1,0);
test( textOutBaseline => 1,0);

=======
goto R;
=======
#goto R;
>>>>>>> 3e818d51 (apply graphic_context())

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

>>>>>>> e700564f (first shot at graphic_context push and pop)
$d-> begin_paint;
$d-> end_paint;
is( $d-> color, 0x123456, 'color' );
is( $d-> backColor, 0x654321, 'backColor' );

$i = 0;
<<<<<<< HEAD
$fillPatternCount = scalar grep { 0xff == $_ } @{$d-> fillPattern};
is( $fillPatternCount, 8, "fillPattern" );
is( $d-> lineEnd, le::Square, "lineEnd" );
is( $d-> lineJoin, lj::Round, "lineJoin" );
is( $d-> linePattern, lp::Dash, "linePattern" );
is( $d-> lineWidth, 5, "lineWidth" );
=======
$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
is( $fillPatternCount, 0, "fillPattern" );
is( $d-> lineEnd, le::Square, "lineEnd" );
is( $d-> lineWidth, 5, "lineWidth" );
is( $d-> linePattern, lp::Dash, "linePattern" );
>>>>>>> e700564f (first shot at graphic_context push and pop)
is( int( $d-> miterLimit + .5), 5, 'miterLimit' );
is( $d-> rop, rop::NotSrcXor, "rop" );
is( $d-> rop2 , rop::NotSrcXor, "rop2");

@z = $d-> translate;
<<<<<<< HEAD
is_deeply( \@z, [2,1], "translate" );
is( $d-> textOpaque, 1, "textOpaque" );
is( $d-> textOutBaseline, 1, "textOutBaseline" );
is( $d-> fillMode, fm::Alternate, "fillMode");

=======
is_deeply( \@z, [1,2], "translate" );
is( $d-> textOpaque, 1, "textOpaque" );
is( $d-> textOutBaseline, 1, "textOutBaseline" );
is( $d-> lineJoin, lj::Bevel, "lineJoin" );
is( $d-> fillMode, fm::Alternate, "fillMode");

R:
>>>>>>> e700564f (first shot at graphic_context push and pop)
$d = Prima::DeviceBitmap-> new( width => 8, height => 8, type => dbt::Pixmap);

sub bits
{
	my $type = $_[0] // im::Byte;
<<<<<<< HEAD
	my $i = (ref($d) eq 'Prima::Image') ? $d : $d->image;
=======
	my $i = $d->image;
>>>>>>> e700564f (first shot at graphic_context push and pop)
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
<<<<<<< HEAD
	$d->clipRect(0,0,8,8);
	my $xaa = $aa ? '.aa' : '';
=======
>>>>>>> e700564f (first shot at graphic_context push and pop)

	check( color     => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) } );
	check( backColor => 0x654321, 0x123456, type => im::RGB, act => sub { $d->polyline([2,2,6,2,6,6]) });

	$d->color(cl::White);
	$d->backColor(cl::Black);
	check( fillMode => fm::Alternate, fm::Winding, act => sub { $d->fillpoly([1,1,6,6,6,1,1,6,6,3]) });

	$d-> fillPattern(fp::Solid);
	$d->graphic_context( fillPattern => [0..7], sub {
<<<<<<< HEAD
<<<<<<< HEAD
		$i = 0;
=======
>>>>>>> e700564f (first shot at graphic_context push and pop)
=======
		$i = 0;
>>>>>>> 3e818d51 (apply graphic_context())
		$fillPatternCount = scalar grep { $i++ != $_ } @{$d-> fillPattern};
		is( $fillPatternCount, 0, 'gc.in.fillPattern' );
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	$i = 0;
	$fillPatternCount = scalar grep { $_ == 0xff } @{$d-> fillPattern};
<<<<<<< HEAD
	is( $fillPatternCount, 8, 'gc.out.fillPattern'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPattern".$xaa);
=======
	is( $fillPatternCount, 8, 'gc.out.fillPattern' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPattern");
>>>>>>> e700564f (first shot at graphic_context push and pop)

	$d-> fillPattern([0xff,0xff,0x00,0x00,0xCA,0xCA,0xAC,0xAC]);
	$d-> fillPatternOffset( 1,2);
	$d->graphic_context( fillPatternOffset => [5,4], sub {
		@fpo = $d-> fillPatternOffset;
<<<<<<< HEAD
		is( $fpo[0], 5, 'gc.in.fillPatternOffset.x'.$xaa );
		is( $fpo[1], 4, 'gc.in.fillPatternOffset.y'.$xaa );
=======
		is( $fpo[0], 5, 'gc.in.fillPatternOffset.x' );
		is( $fpo[1], 4, 'gc.in.fillPatternOffset.y' );
>>>>>>> e700564f (first shot at graphic_context push and pop)
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
	@fpo = $d-> fillPatternOffset;
<<<<<<< HEAD
	is( $fpo[0], 1, 'gc.out.fillPatternOffset.x'.$xaa );
	is( $fpo[1], 2, 'gc.out.fillPatternOffset.y'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPatternOffset".$xaa);
	$d-> fillPattern(fp::Solid);

	check( fillPattern => $fpx1, $fpx2, act => sub { $d->bar(0,0,7,7); });
	$d-> fillPattern(fp::Solid);

	$d->lineWidth(5);
	$d->lineEnd(le::Square);
	check( lineJoin => lj::Miter,  lj::Round, act => sub { $d->polyline([0,2,5,2,5,7]); });
=======
	is( $fpo[0], 1, 'gc.out.fillPatternOffset.x' );
	is( $fpo[1], 2, 'gc.out.fillPatternOffset.y' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.fillPatternOffset");
	$d-> fillPattern(fp::Solid);

	$d->lineWidth(4);
	check( lineJoin => lj::Miter,  lj::Round, act => sub { $d->polyline([2,2,6,2,6,6]); });
>>>>>>> e700564f (first shot at graphic_context push and pop)
	check( lineEnd  => le::Square, le::Round, act => sub { $d->polyline([2,2,6,2,6,6]) });
	$d->lineWidth(0);

	check( lineWidth => 2, 5, act => sub { $d->polyline([2,2,6,2,6,6]) });
	check( linePattern => lp::Dot, lp::Dash, act => sub { $d->polyline([2,2,6,2,6,6]) });
<<<<<<< HEAD
	$d->linePattern(lp::Solid);

	unless ( $unix ) {
		# X11 cannot do variable mitering
		$d->lineWidth(2);
		check( miterLimit => 1, 5, act => sub { $d->polyline([0,0,3,3,3,0]) });
		$d->lineWidth(0);
	}
=======

	$d->lineWidth(2);
	$d->linePattern(lp::Solid);
	check( miterLimit => 1, 5, act => sub { $d->polyline([0,0,3,3,3,0]) });
	$d->lineWidth(0);
>>>>>>> e700564f (first shot at graphic_context push and pop)

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
<<<<<<< HEAD
		is_deeply( [$d-> translate], [4,5], 'gc.in.translate'.$xaa );
=======
		is_deeply( [$d-> translate], [4,5], 'gc.in.translate' );
>>>>>>> e700564f (first shot at graphic_context push and pop)
		$d->clear;
		$d->bar(1,1,6,6);
		$bits1 = bits;
	});
<<<<<<< HEAD
	is_deeply( [$d-> translate], [1,2], 'gc.out.translate'.$xaa );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.translate".$xaa);
=======
	is_deeply( [$d-> translate], [1,2], 'gc.out.translate' );
	$d->clear;
	$d->bar(1,1,6,6);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.translate");
>>>>>>> e700564f (first shot at graphic_context push and pop)
	$d->translate(0,0);

	$d->font->height(8);
	$d->backColor(cl::Yellow);
	$d->color(cl::Black);
	check( textOpaque => 1, 0, type => im::RGB, act => sub { $d->bar(0,0,8,8); $d->text_out("__",0,0); });
<<<<<<< HEAD
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
=======
	$d->color(cl::White);
	$d->backColor(cl::Black);

	check( textOutBaseline => 1, 0, act => sub { $d->text_out("fg",0,0); });

	$d->clear;
	$d->clipRect(1,1,6,6);
	$d->graphic_context( clipRect => [2,2,5,5], sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.clipRect1");
	$d->clipRect(0,0,$d->size);
	$d->clear;
	$d->graphic_context( clipRect => [2,2,5,5], sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.clipRect2");

	$d->clear;
	$d->region( Prima::Region->new( rect =>[1,1,6,6] ));
	$d->graphic_context( region => Prima::Region->new( rect =>[2,2,5,5] ), sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.region1");
	$d->clipRect(0,0,$d->size);
	$d->clear;
	$d->graphic_context( region => Prima::Region->new( rect =>[2,2,5,5] ), sub {
		$d->bar(0,0,7,7);
		$bits1 = bits;
	});
	$d->bar(0,0,7,7);
	$bits2 = bits;
	isnt($bits1,$bits2,"gc.bits.region2");
}
>>>>>>> e700564f (first shot at graphic_context push and pop)

done_testing;
