use strict;
use warnings;

use Prima::sys::Test qw(noX11);
use Test::More;
use Prima::Image::Loader;


sub ix($) {
	Prima::Image->new(
		type => im::Byte,
		size => [1,1],
		data => $_[0],
	)
}
my @ix = map { ix chr } 0..2;

sub test_singleframe_codec
{
	my ($codecID, $name) = @_;

	my $buf1 = '';
	open F, "+>", \ $buf1 or die $!;

	my $ix = $ix[1];
	my $ok = Prima::Image->save( \*F, images => [$ix], codecID => $codecID);
	ok( $ok, "$name: traditional save".( $ok ? '' : ":$@"));

	my $buf2 = '';
	open G, "+>", \ $buf2 or die $!;

	my ($s,$err) = Prima::Image::Saver->new( \*G, codecID => $codecID, frames => 1 ) ;
	ok( $s, "$name: open_save".($s ? '' : ":$err"));

	($ok,$err) = $s->save($ix);
	ok($ok, "$name: save frame ".($ok ? '' : ":$err"));
	undef $s;
	close G;
	ok( $buf1 eq $buf2, "$name: traditional and framed saves produce identical results");

	seek(F,0,0);
	my @nx = Prima::Image->load(\*F, loadAll => 1, loadExtras => 1 );
	is( scalar(@nx), 1, "$name: traditional load returned 1 frame");
	is( $nx[0] && $nx[0]->{extras} && $nx[0]->{extras}->{frames}, 1, "$name: extras.frames is okay");
	$_->type(im::Byte) for grep { defined } @nx;
	is($nx[0]->pixel(0,0), 1, "$name/loadAll: image is loaded correctly") if $nx[0];

	seek(F,0,0);
	@nx = Prima::Image->load(\*F);
	$_->type(im::Byte) for grep { defined } @nx;
	is( scalar(@nx), 1, "$name/index: noindex loaded ok");
	is($nx[0]->pixel(0,0), 1, "$name/index: image is loaded correctly") if $nx[0];

	seek(F,0,0);
	@nx = Prima::Image->load(\*F, index => 0);
	$_->type(im::Byte) for grep { defined } @nx;
	is( scalar(@nx), 1, "$name/index: index 0 loaded ok");
	is($nx[0]->pixel(0,0), 1, "$name/index: image is loaded correctly") if $nx[0];

	my ($lx,$i);
	$i = Prima::Image->new;
	seek(F,0,0);
	($ok,$err) = $i->load(\*F, index => 0);
	$i->type(im::Byte);
	ok( $ok, "$name/inplace: index 0 loaded ok");
	is($i->pixel(0,0), 1, "$name/inplace: image is loaded correctly");

	seek(F,0,0);
	@nx = Prima::Image->load(\*F, map => [0]);
	$_->type(im::Byte) for grep { defined } @nx;
	is( scalar(@nx), 1, "$name/map: index 0 loaded ok");
	is($nx[0]->pixel(0,0), 1, "$name/map: image 0 is loaded correctly") if $nx[0];

	$i = Prima::Image->new;
	seek(F,0,0);
	@nx = $i->load(\*F, map => [], loadExtras => 1, wantFrames => 1);
	$_->type(im::Byte) for grep { defined } @nx;
	is(scalar(@nx), 0, "$name: traditional null load request is ok");
	is($i->{extras}->{frames}, 1, "$name: traditional null load request says 1 frame");

	seek(F,0,0);
	($lx,$err) = Prima::Image::Loader->new(\*F);
	ok( $lx, "$name: loader created".($ok?'':":$err"));

	is($lx->frames, 1, "$name/loader: null load request says 1 frame");

	ok(!$lx->eof, "$name/loader: lx(0) is not eof");
	($i,$err) = $lx->next;
	$i->type(im::Byte) if $i;
	ok( $i, "$name/loader: image loaded".($i?'':":$err"));
	is($i->pixel(0,0), 1, "$name/loader: image is loaded correctly")
		if $i;
	ok($lx->eof,"$name/loader: lx(1) is eof");
	($i,$err) = $lx->next;
	ok(!$i, "$name/loader: image(1) is null");

	close F;
}

sub test_multiframe_codec
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
	$_->type(im::Byte) for grep { defined } @nx;
	for my $n ( 0..2 ) {
		is($nx[$n]->pixel(0,0), $n, "$name/loadAll: image $n is loaded correctly") if $nx[$n];
	}

	for my $n (0..2) {
		seek(F,0,0);
		@nx = Prima::Image->load(\*F, index => $n);
		$_->type(im::Byte) for grep { defined } @nx;
		is( scalar(@nx), 1, "$name/index: index $n loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/index: image $n is loaded correctly") if $nx[0];

		my $i = Prima::Image->new;
		seek(F,0,0);
		($ok,$err) = $i->load(\*F, index => $n);
		$i->type(im::Byte);
		ok( $ok, "$name/inplace: index $n loaded ok");
		is($i->pixel(0,0), $n, "$name/inplace: image $n is loaded correctly");

		seek(F,0,0);
		@nx = Prima::Image->load(\*F, map => [$n]);
		$_->type(im::Byte) for grep { defined } @nx;
		is( scalar(@nx), 1, "$name/map: index $n loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/map: image $n is loaded correctly") if $nx[0];

		my $m = ($n < 2) ? $n + 1 : 0;
		seek(F,0,0);
		@nx = Prima::Image->load(\*F, map => [$n, $m]);
		$_->type(im::Byte) for grep { defined } @nx;
		is( scalar(@nx), 2, "$name/map($n,$m): images $n and $m loaded ok");
		is($nx[0]->pixel(0,0), $n, "$name/map($n,$m): image $n is loaded correctly") if $nx[0];
		is($nx[1]->pixel(0,0), $m, "$name/map($n,$m): image $m is loaded correctly") if $nx[1];
	}

	my ($lx,$i);
	$i = Prima::Image->new;
	seek(F,0,0);
	@nx = $i->load(\*F, map => [], loadExtras => 1, wantFrames => 1);
	$_->type(im::Byte) for grep { defined } @nx;
	is(scalar(@nx), 0, "$name: traditional null load request is ok");
	is($i->{extras}->{frames}, 3, "$name: traditional null load request says 3 frames");

	seek(F,0,0);
	($lx,$err) = Prima::Image::Loader->new(\*F);
	ok( $lx, "$name: loader created".($ok?'':":$err"));

	is($lx->frames, 3, "$name/loader: null load request says 3 frames");

	for my $n (0..2) {
		ok(!$lx->eof, "$name/loader: lx($n) is not eof");
		($i,$err) = $lx->next;
		$i->type(im::Byte) if $i;
		ok( $i, "$name/loader: image $n loaded".($i?'':":$err"));
		is($i->pixel(0,0), $n, "$name/loader: image $n is loaded correctly")
			if $i;
	}
	ok($lx->eof,"$name/loader: lx(3) is eof");
	($i,$err) = $lx->next;
	ok(!$i, "$name/loader: image(3) is null");

	close F;
}

my $tests_run = 0;
for ( @{ Prima::Image->codecs }) {
	next unless $_->{canSaveStream} && $_->{canLoadStream};
	if ( $_->{canSaveMultiple} && $_->{canLoadMultiple} ) {
		test_multiframe_codec($_->{codecID}, $_->{fileShortType});
		$tests_run++;
	} else {
		test_singleframe_codec($_->{codecID}, $_->{fileShortType});
		$tests_run++;
	}
}

plan skip_all => 'no codecs to test' unless $tests_run;

done_testing;
