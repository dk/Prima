use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

plan tests => 3;

my $utf8_line = "line\x{2028}line";
my @r = @{$::application-> text_wrap( $utf8_line, 1000, tw::NewLineBreak)};
is( scalar @r, 2, "wrap utf8 text");
ok( @r, "wrap utf8 text"  );
is( $r[0], $r[1], "wrap utf8 text" );
