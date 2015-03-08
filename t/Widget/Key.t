use Test::More tests => 3;
use Prima::Test;

use strict;
use warnings;

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

reset_flag();
my @keydata = ();
my $window = create_window();
my $c = $window-> insert( Widget =>
	onCreate  => \&Prima::Test::set_flag,
	onDestroy => \&Prima::Test::set_flag,
	onKeyDown => sub { set_flag(); push( @keydata, [@_]); },
	onKeyUp   => sub { set_flag(); push( @keydata, [ $_[0]-> get_shift_state, @_])  },
);

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( get_flag() && scalar @keydata, "send" );
@keydata = ();

$c-> key_event( cm::KeyDown, ord(' '), kb::Space, 0, 1, 1);
my $ww = &Prima::Test::wait;
@keydata = grep {  scalar @$_ == 5 && $$_[1] == ord(' ') && $$_[2] == kb::Space && $$_[3] == 0} @keydata;
ok( $ww && scalar @keydata, "post" );
@keydata = ();

$c-> key_event( cm::KeyUp, 0, kb::Down, km::Ctrl|km::Shift, 1, 0);
@keydata = grep { scalar @$_ == 5 && $$_[3] == kb::Down && $$_[4] == (km::Ctrl|km::Shift) } @keydata;
ok(get_flag() && scalar @keydata, "simulation" );
@keydata = ();

$c-> destroy;

done_testing();
