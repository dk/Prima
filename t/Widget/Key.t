# $Id$
print "1..3 send,post,simulation\n";

$dong = 0;
my @keydata = ();
my $c = $w-> insert( Widget =>
	onCreate  => \&__dong,
	onDestroy => \&__dong,
	onKeyDown => sub { $dong = 1; push( @keydata, [@_]); },
	onKeyUp   => sub { $dong = 1; push( @keydata, [ $_[0]-> get_shift_state, @_])  },
);

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( $dong && scalar @keydata);
@keydata = ();

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 1);
my $ww = &__wait;
@keydata = grep {  scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( $ww && scalar @keydata);
@keydata = ();

$c-> key_event( cm::KeyUp, 0, kb::Down, km::Ctrl|km::Shift, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[3] == kb::Down && $$_[4] == (km::Ctrl|km::Shift) } @keydata;
ok($dong && scalar @keydata);
@keydata = ();


$c-> destroy;

1;
