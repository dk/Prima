use strict;
use warnings;

use Prima::sys::Test qw(noX11);
use Test::More;

my $codecs = Prima::Image-> codecs;
if ( !defined $codecs || ref($codecs) ne 'ARRAY') {
BASE_ERROR:
	fail("basic functionality");
}

my @names;
for ( @$codecs) {
	goto BASE_ERROR if ref($_) ne 'HASH' || ! defined $$_{fileShortType};
	push @names, $$_{fileShortType};
}

# testing if can write file
my $fileok = 0;
if ( open F, "> ./test.test" ) {
	if ( print F ('0'x10240)) {
		$fileok = 1 if close F;
	} else {
		close F;
	}
	unlink "./test.test";
}

# test if has BMP support compiled in
my $has_bmp = grep { $_->{fileShortType} =~ /^bmp$/i } @$codecs;

SKIP : {
    unless ( $fileok ) {
        pass("basic functionality");
        ok($has_bmp, "BMP supported");
        skip "skipping tests", 4;
    }
    my $cdx = scalar(@$codecs) + 3;
    my @test_names = ( "basic functionality",
                       "BMP supported",
                       join(",", 'codecs', @names) );
    pass( $test_names[ 0 ] );
    ok($has_bmp, $test_names[ 1 ] );
    pass( $test_names[ 2 ] );

    my $i = Prima::Image-> create(
        width => 16,
        height => 16,
        type => im::bpp1,
        palette => [0,0,0,255,0,0],
        data =>
        "\x00\x00\x00\x00\x7f\xfe\x00\x00\@\x02\x00\x00_\xfa\x00\x00P\x0a\x00\x00".
        "W\xea\x00\x00T\*\x00\x00U\xaa\x00\x00U\xaa\x00\x00T\*\x00\x00".
        "W\xea\x00\x00P\x0a\x00\x00_\xfa\x00\x00\@\x02\x00\x00\x7f\xfe\x00\x00".
        "\x00\x00\x00\x00"
        );

    my $cid = -1;
    for ( @$codecs) {
        $cid++;
        my $ci = $_;
        SKIP : {
            skip "can't save, hence skipping". $names[ $cid ], 1,  unless $ci-> {canSave};
            my $name = "./test.test." . $ci-> {fileExtensions}->[0];

            my $xi = $i-> dup;
            fail($names[ $cid ]), unlink( $name), next unless $xi-> save( $name);
            my $xl = Prima::Image-> load( $name, loadExtras => 1);
            unlink $name;
            pass( $names[ $cid] ), next unless $xl;
            skip "skipping ".$names[ $cid ], 1  if $xl-> {extras}-> {codecID} != $cid;
            pass( $names[ $cid ] );
        };
    }
};

done_testing();
