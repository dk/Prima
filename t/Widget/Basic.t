use Test::More;
use Prima::Test qw(noX11 w dong);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

$Prima::Test::dong = 0;
my @xpm = (0,0);
my $c = $Prima::Test::w-> insert( Widget =>
	onCreate  => \&Prima::Test::set_dong,
	onDestroy => \&Prima::Test::set_dong,
	onPostMessage => sub { $Prima::Test::dong = 1; @xpm = ($_[1],$_[2])}
);
ok($c, "create" );
ok($Prima::Test::dong, "onCreate" );
$c-> post_message("abcd", [1..200]);
$c-> owner( $::application);
$c-> owner( $Prima::Test::w);
ok(&Prima::Test::wait, "onPostMessage" );
is($xpm[0], 'abcd', "onPostMessage" );
cmp_ok( @{$xpm[1]}, '==', 200, "onPostMessage" );
$Prima::Test::dong = 0;
$c-> destroy;
ok($Prima::Test::dong, "onDestroy" );

done_testing();
