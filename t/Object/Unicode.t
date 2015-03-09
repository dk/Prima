use strict;
use warnings;

use Test::More tests => 4;
use Prima::Test;
use Prima::Application;

SKIP : {
    unless ( $] >= 5.006 &&
             $::application-> get_system_value( sv::CanUTF8_Output)
        ) {
        skip "$] >= 5.006 && UTF8_Output not supported", 4;
    }

    pass("support");

    my $utf8_line;
    eval '$utf8_line="line\\x{2028}line"';
    my @r = @{$::application-> text_wrap( $utf8_line, 1000, tw::NewLineBreak)};
    is( scalar @r, 2, "wrap utf8 text");
    ok( @r, "wrap utf8 text"  );
    is( $r[0], $r[1], "wrap utf8 text" );
};
