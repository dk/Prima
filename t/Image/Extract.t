
use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima;

plan tests => 20;

for my $bpp ( 1, 4, 8, 24 ) {
	my $i = Prima::Image-> create(
	       width  => 16,
	       height => 16,
	       type   => $bpp,
	);
	$i->color(cl::White);
	$i->bar(0,0,15,15);
	$i->color(cl::Black);
	$i->bar(6,6,12,12);

	my $x = $i->extract(5,5,8,8);
	my $cmp = Prima::Image-> create(
	       width  => 8,
	       height => 8,
	       type   => $bpp,
	);
	$cmp->color(cl::White);
	$cmp->bar(0,0,8,8);
	$cmp->color(cl::Black);
	$cmp->bar(1,1,7,7);

	$cmp->type(im::Byte); # do not compare tailing bytes
	$x->type(im::Byte);
	ok($cmp->data eq $x->data, "$bpp bit extract");

	my $mask = $i->dup;
	$mask->type(1);
	my $icon = Prima::Icon->new;
	$icon->combine($i,$mask);
	my ( $xor, $and ) = $icon->extract(5,5,8,8)->split;
	$xor->type(im::Byte);
	$and->type(im::Byte);
	ok($cmp->data eq $xor->data, "$bpp bit / 1-bit mask extract data");
	ok($cmp->data eq $and->data, "$bpp bit / 1-bit mask extract mask");

	$mask = $i->dup;
	$mask->type(im::Byte);
	$icon = Prima::Icon->new;
	$icon->combine($i,$mask);
	( $xor, $and ) = $icon->extract(5,5,8,8)->split;
	$xor->type(im::Byte);
	$and->type(im::Byte);
	ok($cmp->data eq $xor->data, "$bpp bit / 8-bit mask extract data");
	ok($cmp->data eq $and->data, "$bpp bit / 8-bit mask extract mask");
}
