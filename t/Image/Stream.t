use Test::More tests => 4;
use Prima::Test;

use strict;
use warnings;

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
	my $ci = $_;
	my $name = "./test.test." . $ci-> {fileExtensions}->[0];
	
	my $xi = $i-> dup;
	unless( open F, ">", $name) {
		fail("load ".$ci->{fileShortType});
		skip;
		next;
	}
	binmode F;
	unless ( $xi-> save( \*F, codecID => $ci-> {codecID})) {
		fail("save ".$ci->{fileShortType});
		skip;
		close F;
		unlink $name;
		next;
	}
	close F;
	pass("save ".$ci->{fileShortType});

	unless ( open F, "<", $name) {
		fail($ci);
		unlink $name;
		next;
	}
	binmode F;

	my $xl = Prima::Image-> load( \*F, loadExtras => 1);
	close F;
	unlink $name;
	unless ( $xl) {
		fail($ci);
		next;
	}

	pass("load ".$ci->{fileShortType});
}

done_testing();
