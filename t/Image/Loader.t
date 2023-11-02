use strict;
use warnings;

use Prima::sys::Test qw(noX11);
use Test::More;
use Prima::Image::Loader;

my @codecs;
my @names;

for ( @{ Prima::Image->codecs }) {
	next unless $_->{canLoad} && $_->{canSave} && $_->{canSaveMultiple} && $_->{canLoadMultiple} && $_->{canSaveStream} && $_->{canLoadStream};
	push @codecs, $_->{codecID};
	push @names, $_->{fileShortType};
}
plan skip_all => 'No multiframe-capable codecs' unless @codecs;

sub ix($) {
	Prima::Image->new(
		type => im::Byte,
		size => [1,1],
		data => $_[0],
	)
}
my @ix = map { ix chr } 0..2;

sub test_codec
{
	my ($codecID, $name) = @_;

	my $buf1 = '';
	open F, "+>", \ $buf1 or die $!;

	my $ok = Prima::Image->save( \*F, images => \@ix, codecID => $codecID);
	ok( $ok, "$name: traditional save".( $ok ? '' : ":$@"));

	my $buf2 = '';
	open G, "+>", \ $buf2 or die $!;

	my ($s,$err) = Prima::Image::Saver->new( \*G, codecID => $codecID, frames => scalar(@ix) ) ;
	ok( $s, "$name: open_save".($s ? '' : ":$err"));

	for (0..2) {
		($ok,$err) = $s->save($ix[$_]);
		ok($ok, "$name: save frame $_".($ok ? '' : ":$err"));
	}
	undef $s;
	close G;
	ok( $buf1 eq $buf2, "$name: traditional and framed saves produce identical results");


	seek(F,0,0);
	my @nx = Prima::Image->load(\*F, loadAll => 1, loadExtras => 1 );
	is( scalar(@nx), 3, "$name: traditional load returned 3 frames");
	is( $nx[0] && $nx[0]->{extras} && $nx[0]->{extras}->{frames}, 3, "$name: extras.frames is okay");
	for my $n ( 0..2 ) {
		is($nx[$n]->pixel(0,0), $n, "$name/loadAll: image $n is loaded correctly") if $nx[$n];
	}

	for my $n (0..2) {
		seek(F,0,0);
		@nx = Prima::Image->load(\*F, index => $n);
		is( scalar(@nx), 1, "$name/index: index $n loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/index: image $n is loaded correctly") if $nx[0];

		my $i = Prima::Image->new;
		seek(F,0,0);
		($ok,$err) = $i->load(\*F, index => $n);
		ok( $ok, "$name/inplace: index $n loaded ok");
		is($i->pixel(0,0), $n, "$name/inplace: image $n is loaded correctly");

		seek(F,0,0);
		@nx = Prima::Image->load(\*F, map => [$n]);
		is( scalar(@nx), 1, "$name/map: index $n loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/map: image $n is loaded correctly") if $nx[0];

		my $m = ($n < 2) ? $n + 1 : 0;
		seek(F,0,0);
		@nx = Prima::Image->load(\*F, map => [$n, $m]);
		is( scalar(@nx), 2, "$name/map($n,$m): images $n and $m loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/map($n,$m): image $n is loaded correctly") if $nx[0];
		is($nx[1]->pixel(0,0), $m, "$name/map($n,$m): image $m is loaded correctly") if $nx[1];
	}

	my ($lx,$i);
	$i = Prima::Image->new;
	seek(F,0,0);
	@nx = $i->load(\*F, map => [], loadExtras => 1, wantFrames => 1);
	is(scalar(@nx), 0, "$name: traditional null load request is ok");
	is($i->{extras}->{frames}, 3, "$name: traditional null load request says 3 frames");

	seek(F,0,0);
	($lx,$err) = Prima::Image::Loader->new(\*F);
	ok( $lx, "$name: loader created".($ok?'':":$err"));

	is($lx->frames, 3, "$name/loader: null load request says 3 frames");

	for my $n (0..2) {
		ok(!$lx->eof, "$name/loader: lx($n) is not eof");
		($i,$err) = $lx->next;
		ok( $i, "$name/loader: image $n loaded".($i?'':":$err"));
		is($i->pixel(0,0), $n, "$name/loader: image $n is loaded correctly")
			if $i;
	}
	ok($lx->eof,"$name/loader: lx(3) is eof");
	($i,$err) = $lx->next;
	ok(!$i, "$name/loader: image(3) is null");

	close F;
}

for ( my $i = 0; $i < @codecs; $i++) {
	test_codec($codecs[$i], $names[$i]);
#	last; # XXX
}


done_testing;
