# $Id$
print "1..5 create,onCreate,onPostMessage,onPostMessage,onDestroy\n";

$dong = 0;
my @xpm = (0,0);
my $c = $w-> insert( Widget =>
	onCreate  => \&__dong,
	onDestroy => \&__dong,
	onPostMessage => sub { $dong = 1; @xpm = ($_[1],$_[2])}
);
ok($c);
ok($dong);
$c-> post_message("abcd", [1..200]);
$c-> owner( $::application);
$c-> owner( $w);
ok(&__wait);
ok($xpm[0] eq 'abcd' && @{$xpm[1]} == 200);
$dong = 0;
$c-> destroy;
ok($dong);

1;

