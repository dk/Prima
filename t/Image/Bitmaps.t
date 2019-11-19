use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima qw(Application);

plan tests => 1835;

my ($src, $mask, $dst);
my $can_argb = $::application->get_system_value(sv::LayeredWidgets);

sub test_src
{
	my $descr = shift;
	$src->pixel(0,0,cl::Black);
	$src->pixel(1,0,cl::White);
	my $ok = $dst->put_image(0,0,$src);
	ok( $ok, "put $descr" );
	is( $dst->pixel(0,0), cl::Black, "$descr 0");
	is( $dst->pixel(1,0), cl::White, "$descr 1");
}

sub bitop
{
	my ( $pix, $descr, $s, $m, $d ) = @_;
	my $res = ( $d & $m ) ^ $s;
	my $clr = $res ? cl::White : cl::Black;
	is($pix, $clr, "$descr ($s & $m ^ $d == $res)");
}

sub fill_dst
{
	my $dst = shift;
	$dst->rop(rop::CopyPut);
	$dst->pixel(0,0,cl::Black);
	$dst->pixel(1,0,cl::Black);
	$dst->pixel(2,0,cl::Black);
	$dst->pixel(3,0,cl::Black);
	$dst->pixel(0,1,cl::White);
	$dst->pixel(1,1,cl::White);
	$dst->pixel(2,1,cl::White);
	$dst->pixel(3,1,cl::White);
}

sub test_mask
{
#  ....   .*.*   ..**   ..**
#  **** & .*.* ^ ..** = .**.
#
#  this doesn't work with RGBA blending because XOR can do inversions, while alpha channel cannot
#
	my $descr = shift;


	$mask->pixel(0,0,cl::Black);
	$mask->pixel(1,0,cl::White);
	$mask->pixel(2,0,cl::Black);
	$mask->pixel(3,0,cl::White);
	# convert AND-mask to alpha-channel (only to be converted back, but still..)
	$mask->put_image( 0, 0, $mask, rop::NotPut) if $mask->type == im::Byte;

	$src->pixel(0,0,cl::Black);
	$src->pixel(1,0,cl::Black);
	$src->pixel(2,0,cl::White);
	$src->pixel(3,0,cl::White);

	fill_dst($dst);

	my $icon = Prima::Icon->new;
	$icon->combine($src,$mask);

	my $ok = 1;
	$ok &= $dst->put_image(0,0,$icon);
	$ok &= $dst->put_image(0,1,$icon);
	ok( $ok, "put $descr" );

	bitop( $dst->pixel(0,0), $descr, 0,0,0);
	bitop( $dst->pixel(1,0), $descr, 0,1,0);
	bitop( $dst->pixel(2,0), $descr, 1,0,0);
	bitop( $dst->pixel(3,0), $descr, 1,1,0);

	bitop( $dst->pixel(0,1), $descr, 0,0,1);
	bitop( $dst->pixel(1,1), $descr, 0,1,1);
	bitop( $dst->pixel(2,1), $descr, 1,0,1);
	bitop( $dst->pixel(3,1), $descr, 1,1,1);

	# test 2: test a8 source in paint
}

sub test_dst
{
	my ($target, %opt) = @_;
	$src = Prima::DeviceBitmap->create( width => 2, height => 1, type => dbt::Bitmap);
	$dst->set(color => cl::Black, backColor => cl::White);
	test_src( "bitmap on $target");

	$dst->set(color => cl::White, backColor => cl::Black);
	$dst->clear;
	$src->pixel(0,0,cl::Black);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::White, "inverse bitmap on $target 0");
	$src->pixel(0,0,cl::White);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::Black, "inverse bitmap on $target 1");

	$dst->set(color => cl::Black, backColor => cl::Black);
	$src->pixel(0,0,cl::Black);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::Black, "clear bitmap on $target 0");
	$src->pixel(0,0,cl::White);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::Black, "clear bitmap on $target 1");

	$dst->set(color => cl::White, backColor => cl::White);
	$src->pixel(0,0,cl::Black);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::White, "set bitmap on $target 0");
	$src->pixel(0,0,cl::White);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::White, "set bitmap on $target 1");

	$src = Prima::DeviceBitmap->create( width => 2, height => 1, type => dbt::Pixmap);
	test_src( "pixmap on $target");

	$src = Prima::Image->create( width => 2, height => 1, type => im::BW);
	test_src( "im::BW on $target");
	is( unpack('C', $src->data), 0x40, "im::BW pixel(white) = 1");

	$src->begin_paint;
	test_src( "im::BW/paint on $target");

	$dst->set(color => cl::White, backColor => cl::Black);
	test_src( "inverse im::BW/paint on $target");
	$dst->set(color => cl::Black, backColor => cl::Black);
	test_src( "clear im::BW/paint on $target");
	$dst->set(color => cl::White, backColor => cl::White);
	test_src( "set im::BW/paint on $target");
	$src->end_paint;

	$src->type(im::bpp1);
	$src->colormap(cl::Black, cl::White);
	test_src( "im::bpp1/BW on $target");
	$src->colormap(cl::White, cl::Black);
	test_src( "im::bpp1/WB on $target");

	$src->colormap(cl::White, cl::Black);
	$src->begin_paint;
	test_src( "im::bpp1/paint on $target");
	$src->end_paint;

	$src->type(im::bpp4);
	test_src( "im::bpp4 on $target");

	$src->type(im::bpp4);
	$src->colormap(cl::White, cl::Black);
	$src->begin_paint;
	test_src( "im::bpp4/paint on $target");
	$src->end_paint;

	$src->type(im::bpp8);
	test_src( "im::bpp8 on $target");

	$src->type(im::bpp8);
	$src->colormap(cl::White, cl::Black);
	$src->begin_paint;
	test_src( "im::bpp8/paint on $target");
	$src->end_paint;

	$src->set( type => im::RGB);
	test_src( "im::RGB on $target");
	$src->begin_paint;
	test_src( "im::RGB/paint on $target");
	$src->end_paint;

	$mask = Prima::Image->create( width => 4, height => 1, type => im::BW);
	$src = Prima::Image->create( width => 4, height => 1, type => im::BW);
	test_mask( "1-bit grayscale xor mask / 1-bit and mask on $target");
	for my $bit ( 4, 8, 24) {
		$src = Prima::Image->create( width => 4, height => 1, type => $bit);
		test_mask( "$bit-bit xor mask / 1-bit and mask on $target");
	}

	$mask = Prima::Image->create( width => 4, height => 1, type => im::Byte);
	$src = Prima::Image->create( width => 4, height => 1, type => im::BW);
	test_blend( "1-bit grayscale image / 8-bit alpha on $target");
	$src = Prima::Image->create( width => 4, height => 1, type => im::bpp1);
	test_blend( "1-bit image / 8-bit alpha on $target");

	for my $bit ( 4, 8, 24) {
		$src = Prima::Image->create( width => 4, height => 1, type => $bit);
		test_blend( "$bit-bit image / 8-bit alpha on $target");
	}
}

sub blendop
{
	my ( $pix, $descr, $s, $m, $d ) = @_;
	if ( $s == 1 && $m == 0 && $d == 0 ) {
		# this is win32/cygwin specific stuff; not that this behavior is
		# wrong for practical blending, but still a minor WTF
		ok( $pix == 0xffffff || $pix == 0, "$descr (($s + a$m) OVER $d ) == either 0 or 1 )");
	} else {
		my $res = $m ? $s : ( $s | $d );
		my $clr = $res ? cl::White : cl::Black;
		is($pix, $clr, "$descr (($s + a$m) OVER $d ) == $res)");
	}
}

sub test_blend_pixels
{
	my ($icon, $descr) = @_;

	my $ok = 1;
	$ok &= $dst->put_image(0,0,$icon);
	$ok &= $dst->put_image(0,1,$icon);

	ok( $ok, "put $descr" );

	blendop( $dst->pixel(0,0), $descr, 0,0,0);
	blendop( $dst->pixel(1,0), $descr, 0,1,0);
	blendop( $dst->pixel(2,0), $descr, 1,0,0);
	blendop( $dst->pixel(3,0), $descr, 1,1,0);

	blendop( $dst->pixel(0,1), $descr, 0,0,1);
	blendop( $dst->pixel(1,1), $descr, 0,1,1);
	blendop( $dst->pixel(2,1), $descr, 1,0,1);
	blendop( $dst->pixel(3,1), $descr, 1,1,1);
}

sub test_blend
{
SKIP: {
    	skip "no argb capability", 9 * 3 unless $can_argb;
#  0011 + ALPHA(1010) = 0.1*
#
#  0000      0.1* 0011 ( . - fully transparent )
#  1111 OVER 0.1* 0111 ( * - transparent white )

	my $descr = shift;


	$mask->pixel(0,0,cl::Black);
	$mask->pixel(1,0,cl::White);
	$mask->pixel(2,0,cl::Black);
	$mask->pixel(3,0,cl::White);

	$src->pixel(0,0,cl::Black);
	$src->pixel(1,0,cl::Black);
	$src->pixel(2,0,cl::White);
	$src->pixel(3,0,cl::White);
	$src->type(im::RGB);

	my $icon = Prima::Icon->new( autoMasking => am::None );
	$icon->combine($src,$mask);

	fill_dst($dst);
	$dst->rop(rop::SrcOver);
	test_blend_pixels($icon, $descr);

	fill_dst($dst);
	$dst->rop(rop::SrcOver);
	$icon->begin_paint;
	test_blend_pixels($icon, "$descr (in paint)");
	$icon->end_paint;

	fill_dst($dst);
	$dst->rop(rop::SrcOver);
	test_blend_pixels($icon->bitmap, "$descr (layered)");
}}

sub test_blend_native
{
#  0011 + ALPHA(1010) = 0.1*
#
#  0000      0.1* 0011 ( . - fully transparent )
#  1111 OVER 0.1* 0111 ( * - transparent white )

	my $descr = shift;


	$mask->pixel(0,0,cl::Black);
	$mask->pixel(1,0,cl::White);
	$mask->pixel(2,0,cl::Black);
	$mask->pixel(3,0,cl::White);

	$src->pixel(0,0,cl::Black);
	$src->pixel(1,0,cl::Black);
	$src->pixel(2,0,cl::White);
	$src->pixel(3,0,cl::White);

	my $icon = Prima::Icon->new( autoMasking => am::None );
	$icon->combine($src,$mask);

	fill_dst($dst);
	$dst->rop(rop::SrcOver);

	my $ok = 1;
	$ok &= $dst->put_image(0,0,$icon);
	$ok &= $dst->put_image(0,1,$icon);

	my $save = $dst;

	$dst = $dst->dup;
	$dst->type(im::RGB); # to convert 0xff into 0xffffff

	ok( $ok, "put $descr" );

	blendop( $dst->pixel(0,0), $descr, 0,0,0);
	blendop( $dst->pixel(1,0), $descr, 0,1,0);
	blendop( $dst->pixel(2,0), $descr, 1,0,0);
	blendop( $dst->pixel(3,0), $descr, 1,1,0);

	blendop( $dst->pixel(0,1), $descr, 0,0,1);
	blendop( $dst->pixel(1,1), $descr, 0,1,1);
	blendop( $dst->pixel(2,1), $descr, 1,0,1);
	blendop( $dst->pixel(3,1), $descr, 1,1,1);

	$dst = $save;
}

$dst = Prima::Image->create( width => 4, height => 2, type => im::RGB);
$src  = Prima::Image->create( width => 4, height => 1, type => im::RGB);
$mask = Prima::Image->create( width => 4, height => 1, type => im::BW);
test_mask( "reference implementation / 1bit mask");

$mask = Prima::Image->create( width => 4, height => 1, type => im::Byte);
my $target = "reference implementation / 8bit mask";
$mask = Prima::Image->create( width => 4, height => 1, type => im::Byte);
$src = Prima::Image->create( width => 4, height => 1, type => im::Byte);
$dst = Prima::Image->create( width => 4, height => 2, type => im::Byte);
test_blend_native( "8-bit grayscale image / 8-bit alpha on $target");
$dst = Prima::Image->create( width => 4, height => 2, type => im::RGB);
$src = Prima::Image->create( width => 4, height => 1, type => im::RGB);
test_blend_native( "24-bit image / 8-bit alpha on $target");

$dst = Prima::DeviceBitmap->create( width => 4, height => 2, type => dbt::Bitmap);
test_dst("bitmap");

$dst = Prima::DeviceBitmap->create( width => 4, height => 2, type => dbt::Pixmap);
test_dst("pixmap");

$dst = Prima::Image->create( width => 4, height => 2, type => im::BW);
$dst->begin_paint;
test_dst("im::BW");
$dst->end_paint;

$dst = Prima::Image->create( width => 4, height => 2, type => im::bpp1);
$dst->begin_paint;
test_dst("im::bpp1");
$dst->end_paint;

$dst = Prima::Image->create( width => 4, height => 2, type => im::RGB);
$dst->begin_paint;
test_dst("im::RGB");
$dst->end_paint;

# Because get_pixel from non-buffered guarantees nothing.
# .buffered is also not guaranteed, but for 8 pixel widget that shouldn't be a problem
#
# also, do test inside onPaint to make sure it's on the buffer, not on the screen
$dst = Prima::Widget->create( width => 4, height => 2, buffered => 1, onPaint => sub {
	return if get_flag;
	set_flag;
	test_dst("widget");
});
$dst->bring_to_front;
SKIP: {
	skip "cannot get widget to paint", 226 unless wait_flag;
}

SKIP: {
    skip "no argb capability", 226 unless $can_argb;
    reset_flag;
    $dst = Prima::Widget->create( width => 4, height => 2, buffered => 1, layered => 1, onPaint => sub {
	return if get_flag;
	set_flag;
        test_dst("argb widget");
    });

    $dst->bring_to_front;
    skip "cannot get widget to paint", 226 unless wait_flag;
}

SKIP: {
    skip "no argb capability", 226 unless $can_argb;
    $dst = Prima::DeviceBitmap->create( width => 4, height => 2, type => dbt::Layered);
    test_dst("layered");
}
