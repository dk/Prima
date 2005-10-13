# $Id$
print "1..9 send,post,mouse up,mouse move,click,doubleclick,capture,positioning,simulated movement\n";

$dong = 0;
my @keydata = ();
my $c = $w-> insert( Widget =>
	onCreate  => \&__dong,
	onDestroy => \&__dong,
	onMouseDown  => sub { $dong = 1; push( @keydata, [@_]); },
	onMouseUp    => sub { $dong = 1; },
	onMouseMove  => sub { $dong = 1; },
	onMouseClick => sub { $dong = 1; push( @keydata, [@_]);},
);

$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok( $dong && scalar @keydata);
@keydata = ();

my $w;
$c-> mouse_event( cm::MouseDown, mb::Left, 0, 1, 2, 0, 1);
$w = &__wait;
@keydata = grep { scalar @$_ == 5 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2} @keydata;
ok($w && scalar @keydata);

$dong = 0;
$c-> mouse_event( cm::MouseUp, mb::Left, 0, 1, 2, 0, 0);
ok( $dong);

$dong = 0;
$c-> mouse_event( cm::MouseMove, mb::Left, 0, 1, 2, 0, 0);
ok( $dong);

$dong = 0;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 0, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 0 } @keydata;
ok( $dong && scalar @keydata);

$dong = 0;
@keydata = ();
$c-> mouse_event( cm::MouseClick, mb::Left, 0, 1, 2, 1, 0);
@keydata = grep { scalar @$_ == 6 && $$_[1] == mb::Left && $$_[2] == 0 && $$_[3] == 1 && $$_[4] == 2 && $$_[5] == 1 } @keydata;
ok( $dong && scalar @keydata);


my @ppx = $c-> pointerPos;
$c-> capture(1);
$c-> focus;
ok( $c-> capture);

$dong = 0;
$c-> pointerPos( 10, 10);
my @pp = $c-> pointerPos;
ok( $pp[0] == 10 && $pp[1] == 10);
$c-> pointerPos( 11, 11);
ok( $dong || &__wait);

$c-> pointerPos( @ppx);
$c-> capture(0);
$c-> destroy;

1;
