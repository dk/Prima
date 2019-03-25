use strict;
use warnings;
use Test::More;
plan tests => 1;

SKIP : {
	skip "not under CPAN", 1 unless $ENV{PERL5_CPAN_IS_RUNNING};
	skip "no makepl.log", 1 unless open F, "<", "makepl.log";
	local $/;
	diag(<F>);
	close F;
	ok(1, "dummy");
}
