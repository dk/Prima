print "1..8 onMove message - pass 1,correct movement,parameters consistency - pass 1,child move,child move consistency,onMove message - pass 2,parameters consistency - pass 2,gmDontCare\n";

my $dong2 = 0;

my @mrep;
my @mrep2;
my $wx = Prima::Window-> create(
   originDontCare => 1,
   sizeDontCare => 1,
   onMove => sub { $dong = 1; shift; @mrep = @_;  },
   name => 'TEST',
);


my $wl = $wx-> insert( Prima::Widget =>
   clipOwner => 0,
   growMode  => 0,
   onMove => sub { $dong2 = 1; shift; @mrep2 = @_; },
);

my @or = $wx-> origin;
my @or2 = $wl-> origin;
$dong = 0;
$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);

sub __mywait
{
   my $count = 20000;
   while ( $count--) {
      last if $dong && $dong2;
      $::application-> yield;
   }
   return $dong && $dong2;
}

ok(( $dong && $dong2) || &__mywait);
my @nor = $wx-> origin;
if ( $nor[0] == $or[0] + 1 && $nor[1] == $or[1] + 1) {
   ok(1);
} elsif ( Prima::Application-> get_system_info->{apc} != apc::Unix) {
   ok(0);
} else {
   print "ok # skip";
}
ok( $mrep[0] == $or[0] && $mrep[1] == $or[1] && $mrep[2] == $nor[0] && $mrep[3] == $nor[1]);
ok( scalar(@mrep2));
ok( $mrep2[0] == $or2[0] && $mrep2[1] == $or2[1] && $mrep2[2] == $mrep2[0] + 1 && $mrep2[3] == $mrep2[1] + 1);

$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);
$wx-> size( $wx-> width + 1, $wx-> height + 1);


@or = $wx-> origin;
@or2 = $wl-> origin;
@mrep = @mrep2 = ();

$wl-> growMode( gm::DontCare);
$wx-> origin( $wx-> left + 1, $wx-> bottom + 1);
ok( $dong || &__wait);
@nor = $wx-> origin;
ok( $mrep[0] == $or[0] && $mrep[1] == $or[1] && $mrep[2] == $nor[0] && $mrep[3] == $nor[1]);
@or = $wl-> origin;
ok( $or[0] == $or2[0] && $or[1] == $or2[1]);

$wx-> destroy;

1;
