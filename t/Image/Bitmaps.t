use strict;
use warnings;

use Test::More;
use Prima::Test;

plan tests => 4;

# put stuff on bitmap
my $dst = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 1);

my $src = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 1);
$src->pixel(0,0,cl::Clear);
$dst->put_image(0,0,$src);
is( $dst->pixel(0,0), cl::Black, "bitmap on bitmap 0");
$src->pixel(0,0,cl::Set);
$dst->put_image(0,0,$src);
is( $dst->pixel(0,0), cl::White, "bitmap on bitmap 1");

$src = Prima::DeviceBitmap->create( width => 1, height => 1, monochrome => 0);
$src->pixel(0,0,cl::Black);
$dst->put_image(0,0,$src);
is( $dst->pixel(0,0), cl::Black, "pixmap on bitmap 0");
$src->pixel(0,0,cl::White);
$dst->put_image(0,0,$src);
is( $dst->pixel(0,0), cl::White, "pixmap on bitmap 1");

