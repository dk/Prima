use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

my @codecs;

@codecs = grep {
	$_-> {canSaveStream} and $_-> {canLoadStream}
} @{Prima::Image-> codecs};

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
unless ( $fileok) {
	print "1..1 load/save to streams";
	skip("skipping load/save to streams");
	return 1;
}

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

for ( @codecs) {
    SKIP : {
       my $ci = $_;
       my $name = "./test.test." . $ci-> {fileExtensions}->[0];

       my $xi = $i-> dup;
       unless( open F, ">", $name) {
           warn("Cannot open $name:$!");
           fail("load ".$ci->{fileShortType});
           skip "skipping load ".$ci->{fileShortType}, 2;
       }
       binmode F;
       unless ( $xi-> save( \*F, codecID => $ci-> {codecID})) {
           warn("Cannot save: $@");
           fail("save ".$ci->{fileShortType});
           skip "skipping save ".$ci->{fileShortType}, 2;
           close F;
           unlink $name;
       }
       close F;
       pass("save ".$ci->{fileShortType});

       unless ( open F, "<", $name) {
           warn("Cannot open $name:$! / $ci->{versionMajor}.$ci->{versionMinor}");
           fail($ci->{fileShortType});
           unlink $name;
           skip "load ".$ci->{fileShortType}, 1;
       }
       binmode F;

       my $xl = Prima::Image-> load( \*F, loadExtras => 1);
       close F;
       unlink $name;
       unless ( $xl) {
           warn("Cannot load:$@");
           fail($ci->{fileShortType});
           skip "load ".$ci->{fileShortType}, 1;
       }

       pass("load ".$ci->{fileShortType});
   };
}

done_testing();
