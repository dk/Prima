print "1..13 activate,onActivate,deactivate,onDeactivate,maximize,onWindowState,".
      "restore from maximized,minimize,restore from minimized,restore max->min->normal,".
      "user modality,execute,close\n";

my $id = 0;
my $xw = Prima::Window-> create(
   size => [ 100, 100],
   onActivate    => sub { $dong = 1; $id = 1;},
   onDeactivate  => sub { $dong = 1; $id = 2;},
   onExecute     => sub { $dong = 1; $id = 3;},
   onWindowState => sub { $dong = 1; $id = 4;},
   onClose       => sub { $dong = 1; $id = 5; $_[0]-> clear_event; },
);

$w-> focus;

$dong = 0;
$xw-> focus;
ok( $xw-> selected);
ok(( $dong || &__wait) && $id == 1);

$dong = 0;
$w-> focus;
ok( !$xw-> selected);
ok(( $dong || &__wait) && $id == 2);

$dong = 0;
$xw-> windowState( ws::Maximized);
ok( $xw-> windowState == ws::Maximized);
ok(( $dong || &__wait) && $id == 4);
$xw-> windowState( ws::Normal);
ok( $xw-> windowState == ws::Normal);

$dong = 0;
$xw-> windowState( ws::Minimized);
ok( $xw-> windowState == ws::Minimized);

$dong = 0;
$xw-> windowState( ws::Normal);
ok( $xw-> windowState == ws::Normal);

$xw-> windowState( ws::Maximized);
$xw-> windowState( ws::Minimized);
$xw-> windowState( ws::Normal);
ok( $xw-> windowState == ws::Normal);

$dong = 0;
$xw-> insert( Timer =>
   timeout => 500,
   onTick => sub {
   $_[0]-> stop;
   $w-> focus;
   ok( !$w-> selected && $xw-> selected);
   $xw-> ok;
   $_[0]-> destroy;
})-> start;
my $mr = $xw-> execute;
ok( $dong && $mr == cm::OK);


$dong = 0;
$xw-> close;
ok(( $dong || &__wait) && $id == 5 && $xw-> alive);

$xw-> destroy;


1;
