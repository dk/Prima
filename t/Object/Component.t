use Test::More tests => 8;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

{ # block for mys
$dong = 0;
my @xpm = (0,0);
my $c = $w-> insert( Component =>
	onCreate  => \&__dong,
	onDestroy => \&__dong,
	onPostMessage => sub { $dong = 1; @xpm = ($_[1],$_[2])},
	name => 'gumbo jumbo',
);
# 1
ok($c);
ok($dong);
ok($c-> name eq 'gumbo jumbo');
$c-> post_message("abcd", [1..200]);
$c-> owner( $::application);
$c-> owner( $w);
# 4
ok(&__wait);
ok($xpm[0] eq 'abcd' && @{$xpm[1]} == 200);
$dong = 0;
$c-> destroy;
ok($dong);
# 7
$dong = 0;
Prima::Drawable-> create( onDestroy => sub { $dong = 1 } );
ok( $dong);
return 1;
}
1;

