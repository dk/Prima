use strict;
use warnings;
use Test::More;
use Prima::sys::Test qw(noX11);
       
my $w = Prima::Image-> create( type => dbt::Pixmap, width => 132, height => 32);

SKIP: {
	$w->font->vector(1);
	my $z = $w->text_shape(123, skip_if_simple => 1);
	skip("no vector fonts", 2) unless $z;
	$z = $w->text_shape("A\x{924}\x{627}\x{5d0}\x{c27}", polyfont => 1);
	skip("cannot polyfont", 2) unless $z->fonts;

	ok( 1, "got polyfont");
	my %f = map { $_ => 1 } @{$z->fonts};
	delete $f{0};
	skip("no such font", 1) unless keys %f;
}

done_testing;
