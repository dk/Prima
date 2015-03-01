use Test::More;
use Prima::Test qw(noX11 w);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

$dong = 0;
my @keydata = ();
my $c = $Prima::Test::w-> insert( Widget =>
	onCreate  => \&Prima::Test::set_dong,
	onDestroy => \&Prima::Test::set_dong,
	onKeyDown => sub { $Prima::Test::dong = 1; push( @keydata, [@_]); },
	onKeyUp   => sub { $Prima::Test::dong = 1; push( @keydata, [ $_[0]-> get_shift_state, @_])  },
);

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( $Prima::Test::dong && scalar @keydata, "send" );
@keydata = ();

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 1);
my $ww = &Prima::Test::wait;
@keydata = grep {  scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( $ww && scalar @keydata, "post" );
@keydata = ();

$c-> key_event( cm::KeyUp, 0, kb::Down, km::Ctrl|km::Shift, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[3] == kb::Down && $$_[4] == (km::Ctrl|km::Shift) } @keydata;
ok($Prima::Test::dong && scalar @keydata, "simulation" );
@keydata = ();


$c-> destroy;

done_testing();
