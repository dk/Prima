use strict;
use warnings;

use Test::More;
use Prima::Test;

plan tests => 21;

my ($src, $dst);

sub test_src
{
	my $descr = shift;
	$src->pixel(0,0,cl::Black);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::Black, "$descr 0");
	$src->pixel(0,0,cl::White);
	$dst->put_image(0,0,$src);
	is( $dst->pixel(0,0), cl::White, "$descr 1");
}

sub test_dst
{
	my $target = shift;
	$src = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 1);
	$dst->set(color => cl::Black, backColor => cl::White);
	test_src( "bitmap on $target");

	$dst->set(color => cl::White, backColor => cl::Black);
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

	$src = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 0);
	test_src( "pixmap on $target");

	$src = Prima::Image->create( width => 1, height => 1, type => im::BW);
	test_src( "im::BW on $target");
	is( unpack('C', $src->data), 0x80, "im::BW pixel(white) = 1");

	$src->begin_paint;
	test_src( "im::BW/paint on $target");
	$src->end_paint;

	$src->type(im::bpp1);
	$src->colormap(cl::White, cl::Black);
	test_src( "im::bpp1 on $target");

	$src->colormap(cl::White, cl::Black);
	$src->begin_paint;
	test_src( "im::bpp1/paint on $target");
	$src->end_paint;

	test_src( "im::Byte on $target");
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
}

$dst = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 1);
test_dst("bitmap");
$dst = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 0);
test_dst("pixmap");

#
# put stuff on pixmap
#
