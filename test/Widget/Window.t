# $Id$
print "1..15 activate,onActivate,deactivate,onDeactivate,maximize,onWindowState,".
		"restore from maximized,minimize,restore from minimized,restore max->min->normal,".
		"user modality,execute,show,hide,close\n";

my %id;
my $xw = Prima::Window-> create(
	size => [ 100, 100],
	onActivate    => sub { $dong = 1; $id{Activate}   = 1;},
	onDeactivate  => sub { $dong = 1; $id{Deactivate} = 1;},
	onExecute     => sub { $dong = 1; $id{Execute}    = 1;},
	onWindowState => sub { $dong = 1; $id{State}      = 1;},
	onClose       => sub { $dong = 1; $id{Close}      = 1; $_[0]-> clear_event; },
	onShow        => sub { $dong = 1; $id{Show}       = 1; },
	onHide        => sub { $dong = 1; $id{Hide}       = 1; },
);

$w-> focus;

$dong = 0;
$xw-> focus;
ok( $xw-> selected);
ok(( $dong || &__wait) && $id{Activate});
%id=();

$dong = 0;
$w-> focus;
ok( !$xw-> selected);
ok(( $dong || &__wait) && $id{Deactivate});
%id=();

$dong = 0;
$xw-> windowState( ws::Maximized);
ok( $xw-> windowState == ws::Maximized);
ok(( $dong || &__wait) && $id{State});
$xw-> windowState( ws::Normal);
ok( $xw-> windowState == ws::Normal);
%id=();

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

%id=();
$dong = 0;
$xw-> insert( Timer =>
	timeout => 250,
	onTick => sub {
	$_[0]-> stop;
	$w-> focus;
	ok( !$w-> selected);
	$xw-> ok;
	$_[0]-> destroy;
})-> start;
my $mr = $xw-> execute;
ok( $dong && $mr == mb::OK);

$dong = 0;
%id=();
$xw-> show;
ok(( $dong || &__wait) && $id{Show} && $xw-> visible);

$dong = 0;
%id=();
$xw-> hide;
ok(( $dong || &__wait) && $id{Hide} && !$xw-> visible);

%id=();
$dong = 0;
$xw-> close;
ok(( $dong || &__wait) && $id{Close} && $xw-> alive);

$xw-> destroy;


1;
