# $Id$

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
	skip(1);
	return 1;
}

print "1..", 2 * scalar(@codecs), ' ', 
	join(',', map { "save $_->{fileShortType},load $_->{fileShortType}"} @codecs);

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
		ok(0);
		skip(1);
		next;
	}
	binmode F;
	unless ( $xi-> save( \*F, codecID => $ci-> {codecID})) {
		ok(0);
		skip(1);
		close F;
		unlink $name;
		next;
	}
	close F;
	ok(1);

	unless ( open F, "<", $name) {
		ok(0);
		unlink $name;
		next;
	}
	binmode F;

	my $xl = Prima::Image-> load( \*F, loadExtras => 1);
	close F;
	unlink $name;
	unless ( $xl) {
		ok(0);
		next;
	}

	ok( 1);
}

1;

