use strict;
use warnings;

use Test::More;
use Cwd ();
use Prima::sys::Test qw(noX11);
use Prima::Utils;
use Prima::sys::FS;
use Fcntl qw(:DEFAULT S_IFREG S_IFDIR);
use utf8;

my $fn = "f.f.f";
my $dn = "d.d.d";
unlink $fn;
unlink $dn;

plan skip_all => "cannot write file:$!"
	unless open F, ">", $fn;
close F;
ok( -f $fn, "file created");
unlink $fn;

unless ( mkdir $dn ) {
	unlink $fn;
	plan skip_all => "cannot mkdir: $!";
}
ok( -d $dn, "dir created");
rmdir $dn;

sub check
{
	my ( $id, $fn, $dn ) = @_;

	my $ok;
	my $fd = Prima::Utils::open_file($fn, O_CREAT|O_WRONLY);
	ok($fd, "$id: open file ok");
	diag($!) unless $fd;
	my $fh;
	$ok = CORE::open $fh, ">&=", $fd;
	ok($ok, "$id: fdopen");
	diag($!) unless $ok;
	$ok = print $fh "Hello world!\n";
	ok($ok, "$id: print");
	close $fh;

	ok($ok = mkdir($dn), "$id: mkdir ok");
	diag($!) unless $ok;

	my @l = getdir('.');
	my ($found_file, $found_dir) = (0,0);
	for (my $i = 0; $i < @l; $i += 2 ) {
		$found_file = $l[$i+1] if $l[$i] eq $fn;
		$found_dir  = $l[$i+1] if $l[$i] eq $dn;
	}
	ok( $found_file eq 'reg', "$id: getdir file");
	ok( $found_dir  eq 'dir', "$id: getdir dir");

	my $d;
	ok( opendir($d, '.'), "opendir");
	my $start = telldir($d);
	@l = readdir($d);
	($found_file, $found_dir) = (0,0);
	for (my $i = 0; $i < @l; $i++ ) {
		$found_file = 1 if $l[$i] eq $fn;
		$found_dir  = 1 if $l[$i] eq $dn;
	}
	ok( $found_file, "$id: readdir file");
	ok( $found_dir , "$id: readdir dir");
	seekdir($d, $start);
	@l = readdir($d);
	($found_file, $found_dir) = (0,0);
	for (my $i = 0; $i < @l; $i++ ) {
		$found_file = 1 if $l[$i] eq $fn;
		$found_dir  = 1 if $l[$i] eq $dn;
	}
	ok( $found_file, "$id: rewind/readdir file");
	ok( $found_dir , "$id: rewind/readdir dir");
	seekdir($d, $start);
	scalar readdir($d);
	my $pos = telldir $d;
	seekdir($d, $start);
	seekdir $d, $pos;
	is($pos, telldir $d, "telldir") if $^O !~ /freebsd/ || `uname -r` !~ /^([02-9]|10)/;
	my @r = readdir $d;
	ok( @r < @l, "seekdir/telldir");

	ok( closedir($d), "closedir");

	is( _f $fn, 1, "$id: _f file = 1");
 	is( _d $dn, 1, "$id: _d dir  = 1");
	is( _d $fn, 0, "$id: _d file = 0");
	is( _f $dn, 0, "$id: _f dir  = 0");

	@l = stat($fn);
	ok(scalar(@l), "$id: stat file");
	diag($!) unless @l;
	ok( $l[2] & S_IFREG, "$id: stat file is file");

	@l = stat($dn);
	ok(scalar(@l), "$id: stat dir");
	diag($!) unless @l;
	ok( $l[2] & S_IFDIR, "$id: stat dir is dir");

	my $cwd = getcwd;
	ok( $ok = chdir($dn), "$id: chdir");
	diag($!) unless $ok;
	my $ncwd = getcwd;
	setenv( PWD => $ncwd );
	is( getenv( 'PWD' ), $ncwd, "$id: getenv");

	my $dn_local = Prima::Utils::sv2local($dn);
	if ( defined $dn_local ) {
		my $cwd = Cwd::getcwd();
		CORE::chdir $dn_local;
		like( Cwd::getcwd(), qr/\Q$dn_local\E/, "$id: chdir back-compat");
		CORE::chdir $cwd;
		chdir($ncwd);

		my $test = "$dn\0$dn";
		my $loc  = Prima::Utils::sv2local($test);
		is( length($loc), length($dn_local) * 2 + 1, "$id: sv2local");
		is(Prima::Utils::local2sv($loc), $test, "$id: local2sv");
	}

	$ok = open(F, ">", 1);
	diag($!) unless $ok;
	close F;
	ok( $ok, "$id: create file in subdir");
	ok( $ok = rename('1', $fn), "$id: rename");
	diag($!) unless $ok;
	ok( $ok = chdir($cwd), "$id: chdir back");
	diag($!) unless $ok;

	ok($ok = unlink("$ncwd/$fn"), "$id: unlink in subdir");
	diag(getcwd, $!) unless $ok;

	ok($ok = unlink($fn), "$id: unlink file");
	diag(getcwd, $!) unless $ok;

	ok($ok = rmdir($dn), "$id: unlink dir");
	diag(getcwd, $!) unless $ok;

	ok( !scalar(stat($fn)), "$id: really unlink file");
	ok( !scalar(stat($dn)), "$id: really unlink dir");
}

check("en", $fn, $dn);
check("ru", "файл", "фолдер");
check("zh", "文件", "目录");

done_testing;
