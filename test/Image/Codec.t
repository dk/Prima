# $Id$

my $codecs = Prima::Image-> codecs;
if ( !defined $codecs || ref($codecs) ne 'ARRAY') {
BASE_ERROR:
	print "1..1 basic functionality";
	ok(0);
	return 1;
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

unless ( $fileok) {
	print "1..3 basic functionality,BMP supported,codecs";
	ok(1);
	ok($has_bmp);
	skip;
	return 1;
}

my $cdx = scalar(@$codecs) + 3;
print "1..$cdx basic functionality,BMP supported,", join(',', 'codecs', @names);
# 1-3
ok(1);
ok($has_bmp);
ok(1);

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
	skip, next unless $ci-> {canSave};    
	my $name = "./test.test." . $ci-> {fileExtensions}->[0];
	
	my $xi = $i-> dup;
	ok(0), unlink( $name), next unless $xi-> save( $name);
	my $xl = Prima::Image-> load( $name, loadExtras => 1);
	unlink $name;
	ok(0), next unless $xl;
	skip, next if $xl-> {extras}-> {codecID} != $cid;
	ok( 1);
}

1;

