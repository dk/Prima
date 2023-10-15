# contains
#    DockManager
#    DockManager::LaunchPad
#    DockManager::Toolbar
#    DockManager::ToolbarDocker
#    DockManager::Panelbar
#    DockManager::S::SpeedButton;
#

use strict;
use warnings;
use Prima;
use Prima::Utils;
use Prima::Docks;
use Prima::Notebooks;
use Prima::Lists;
use Prima::StdBitmap;

package Prima::DockManager::LaunchPad;
use vars qw(@ISA);
@ISA = qw(Prima::Notebook Prima::SimpleWidgetDocker);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		fingerprint => 0x0000FFFF,
		dockup      => undef,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw(fingerprint dockup));
	return %profile;
}

# inner part of toolbar tandem

package Prima::DockManager::ToolbarDocker;
use vars qw(@ISA);
@ISA = qw(Prima::SingleLinearWidgetDocker);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		parentDocker => undef,
		instance     => undef,
		x_sizeable   => 0,
		y_sizeable   => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my ($self, %profile) = @_;
	%profile = $self-> SUPER::init( %profile);
	$self-> $_($profile{$_}) for qw( parentDocker instance);
	return %profile;
}

sub get_extent
{
	my $self = $_[0];
	my @ext = (0,0);
	for ($self-> docklings) {
		my @z = $_-> rect;
		for (0,1) {
			$ext[$_] = $z[$_+2] if $ext[$_] < $z[$_+2]
		}
	}
	return @ext;
}

sub update_size
{
	my ( $self, @sz) = @_;
	my $o = $self-> parentDocker;
	return unless $o;
	@sz = $self-> size unless scalar @sz;
	my @r  = $o-> client2frame( 0, 0, @sz);
	$o-> size( $r[2] - $r[0], $r[3] - $r[1]);
	$self-> size( @sz);
	if ( $o-> dock) {
		$o-> redock;
	} else {
		@r = $o-> externalDocker-> client2frame( 0,0, @sz);
		$o-> externalDocker-> size($r[2] - $r[0], $r[3] - $r[1]);
		$self-> rect( 0,0,@sz); # respect growMode
	}
}

# this part is responsible for changing toolbar's size when new tools are docked in
sub dock
{
	return $_[0]-> {dock} unless $#_;
	my $self = shift;
	my @sz = $self-> size;
	$self-> SUPER::dock(@_);
	my @sz1 = $self-> size;
	my $resize = 0;
	my @ext = $self-> get_extent;
	for ( 0, 1) {
		$resize = 1, $sz1[$_] = $ext[$_] if $sz1[$_] > $ext[$_];
	}
	return if !$resize && ($sz[0] == $sz1[0] && $sz[1] == $sz1[1]);
	$self-> size( @sz1);
	$self-> update_size( @sz1);
}

sub rearrange
{
	my $self = $_[0];
# fast version of rearrange, without real redocking
	my $v = $self-> vertical;
	my @ext = (0,0);
	my ( $xid, $yid) = ( $v ? 0 : 1, $v ? 1 : 0);
	my $a;
	for ( $self-> docklings) {
		$a = $_ unless $a; #
		my @sz = $_-> size;
		$_-> origin( $v ? ( 0, $ext[1]) : ( $ext[0], 0));
		$ext[$xid] = $sz[$xid] if $ext[$xid] < $sz[$xid];
		$ext[$yid] += $sz[$yid];
	}
	if ( $a) {
		$self-> size( @ext);
		#innvoke dock-undock, just to be sure, but for only one widget
		$self-> redock_widget( $a);
	}
}

sub parentDocker
{
	return $_[0]-> {parentDocker} unless $#_;
	$_[0]-> {parentDocker} = $_[1];
}

sub instance
{
	return $_[0]-> {instance} unless $#_;
	$_[0]-> {instance} = $_[1];
}

sub on_dockerror
{
	my ( $self, $urchin) = @_;
	$self-> redock_widget( $urchin);
}

sub on_undock
{
	my $self = $_[0];
	return if scalar(@{$self->{docklings}});
	my $i = $self-> instance;
	$i-> post( sub {
		return if scalar(@{$self->{docklings}});
		if ( $self-> parentDocker-> autoClose) {
			$self-> parentDocker-> destroy;
		} else {
			$i-> toolbar_visible( $self-> parentDocker, 0);
			$i-> notify(q(ToolbarChange));
		}
	});
}

package Prima::DockManager::Toolbar;
use vars qw(@ISA);
@ISA = qw(Prima::LinearDockerShuttle);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		instance    => undef,
		childDocker => undef,
		autoClose   => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw(instance childDocker autoClose));
	return %profile;
}

sub autoClose
{
	return $_[0]-> {autoClose} unless $#_;
	$_[0]-> {autoClose} = $_[1];
}

sub childDocker
{
	return $_[0]-> {childDocker} unless $#_;
	$_[0]-> {childDocker} = $_[1];
}

sub instance
{
	return $_[0]-> {instance} unless $#_;
	$_[0]-> {instance} = $_[1];
}

sub on_getcaps
{
	my ( $self, $docker, $prf) = @_;
	$self-> SUPER::on_getcaps( $docker, $prf);
	my @cd = $self-> {childDocker}-> size;
	my @i = @{$self-> {indents}};
	my @sz = ($i[2] + $i[0] + $cd[0], $i[3] + $i[1] + $cd[1]);
	$prf-> {sizeMin} = [ @sz];
	my $vs = $docker-> can('vertical') ? $docker-> vertical : 0;
	my $v  = $self-> {vertical};
	$prf-> {sizes} = ( $v == $vs) ? [[@sz]] : [[@sz[1,0]]];
}

sub on_dock
{
	my $self = $_[0];
	my $nv   = $self-> dock-> can('vertical') ? $self-> dock-> vertical : 0;
	return if $nv == $self-> {vertical};
	$self-> vertical( $nv);
	my $c = $self-> {childDocker};
	$c-> vertical( $nv);
	$c-> rect( $self-> frame2client( 0, 0, $self-> size));
	$c-> rearrange;
}

package Prima::DockManager::Panelbar;
use vars qw(@ISA);
@ISA = qw(Prima::LinearDockerShuttle);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		vertical    => 1,
		instance    => undef,
		externalDockerProfile =>  { borderStyle => bs::Sizeable },
		x_sizeable  => 1,
		y_sizeable  => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_( $profile{$_}) for ( qw(instance));
	return %profile;
}

sub instance
{
	return $_[0]-> {instance} unless $#_;
	$_[0]-> {instance} = $_[1];
}


# flags for fingerprints - for different dockers and stages.
package
    dmfp;

use constant Tools     => 0x0F000; # those who want tools, must set this
use constant Toolbar   => 0x10000; # those who want toolbars, must set this
use constant LaunchPad => 0x20000; # tools that want to be disposable, set this

package Prima::DockManager;
use vars qw(@ISA);
@ISA = qw(Prima::Component Prima::AbstractDocker::Interface);

{
my %RNT = (
	%{Prima::Component->notification_types()},
	CommandChange  => nt::Notification,
	ToolbarChange  => nt::Notification,
	PanelChange    => nt::Notification,
	Command        => nt::Command,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		interactiveDrag => 0,
		commands        => {},
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {commands} = {};
	$self-> {interactiveDrag} = 0;
	my %profile = $self-> SUPER::init( @_);
	$self-> {classes} = [];
	$self-> {hiddenToolbars} = [];
	$self-> {toolbars} = [];
	$self-> {panels} = [];
	$self-> $_( $profile{$_}) for ( qw( commands interactiveDrag));
	return %profile;
}

sub interactiveDrag
{
	return $_[0]-> {interactiveDrag} unless $#_;
	my ( $self, $id) = @_;
	return if $id == $self-> {interactiveDrag};
	$self-> {interactiveDrag} = $id;
	if ( $id) {
		for ( $self-> toolbars)  {
			$_-> enabled(1) for $_-> childDocker-> docklings;
		}
	} else {
		my $c = $self-> {commands};
		for ( $self-> toolbars)  {
			for ( $_-> childDocker-> docklings) {
				next if !defined $_->{CLSID} || !exists($c-> {$_->{CLSID}}) || $c-> {$_->{CLSID}};
				$_-> enabled(0);
			}
		}
	}
}

sub toolbars { return @{$_[0]->{toolbars}}}
sub panels   { return @{$_[0]->{panels}}}

sub fingerprint { return 0xFFFFFFFF }

sub register_tool
{
	my ( $self, $CLSID, $hash) = @_;
	$hash-> {tool} = 1;
	push @{$self-> {classes}}, $CLSID, $hash;
}

sub register_panel
{
	my ( $self, $CLSID, $hash) = @_;
	$hash-> {tool} = 0;
	push @{$self-> {classes}}, $CLSID, $hash;
}

sub get_class
{
	my ( $self, $CLSID) = @_;
	my $i;
	my $c = $self-> {classes};
	my $x = scalar @$c;
	for ( $i = 0; $i < $x; $i+=2) {
		return $$c[$i+1] if $CLSID eq $$c[$i];
	}
}

sub panel_by_id
{
	my ( $self, $name) = @_;
	for ( @{$self-> {panels}}) {
		return $_ if $_-> {CLSID} eq $name;
	}
}

sub toolbar_by_name
{
	my ( $self, $name) = @_;
	for ( @{$self-> {toolbars}}) {
		return $_ if $_-> name eq $name;
	}
}

sub post
{
	my ( $self, $sub, @parms) = @_;
	$self-> post_message( 'sub', [ $sub, @parms]);
}

sub on_postmessage
{
	my ( $self, $id, $val) = @_;
	return unless $id eq 'sub';
	my $sub = shift @$val;
	$sub->( $self, @$val);
}

sub create_manager
{
	my ( $self, $where, %profile) = @_;
	my @o   = exists($profile{origin}) ? @{$profile{origin}} : (0,0);
	my @sz  = exists($profile{size})   ? @{$profile{size}}   : ($where-> size);
	my ( @items, %items);
	my @lclasses;
	my $i = 0;
	my $cls = $self-> {classes};
	my $xcls = scalar @$cls;
	for ( $i = 0; $i < $xcls; $i += 2) {
		my ( $CLSID, $hash) = @$cls[$i, $i+1];
		next unless $hash->{tool};
		push ( @lclasses, $CLSID);
		$CLSID =~ m/^([^\:]+)(?:\:|$)/;
		next if !$1 || exists($items{$1});
		$items{$1} = 1;
		push( @items, $1);
	}
	$i = 0;
	my $nb;
	my $lb = $where-> insert( ListBox =>
		origin  => [@o],
		size    => [ int($sz[0] / 3), $sz[1]],
		name    => 'GroupSelector',
		vScroll => 1,
		items   => \@items,
		growMode=> gm::Client,
		focusedItem => 0,
		onSelectItem => sub {
			my ($self,$lst,$state) = @_;
			return unless $state;
			$nb-> pageIndex( $self-> focusedItem);
		},
		exists($profile{listboxProfile}) ? %{$profile{listboxProfile}} : (),
	);
	$nb = $where-> insert( 'Prima::DockManager::LaunchPad' =>
		exists($profile{dockerProfile}) ? %{$profile{dockerProfile}} : (),
		origin      => [ $o[0] + int($sz[0] / 3), 0],
		size        => [ $sz[0] - int($sz[0] / 3), $sz[1]],
		name        => 'PageFlipper',
		pageCount   => scalar(@items),
		growMode    => gm::GrowHiY|gm::GrowLoX,
		fingerprint => dmfp::LaunchPad,
		dockup      => $self,
	);
	$nb-> {dockingRoot} = $self;
	$i = 0;
	my %x = @$cls;
	my @szn = $nb-> size;

	for ( @items) {
		my $iid = $_;
		my @d = grep { m/^([^\:]+)(?:\:|$)/; my $j = $1 || ''; $j eq $iid; } @lclasses;
		my @org  = (5,5);
		my $maxy = 0;
		my @ctrls;
		my $ix = 0;
		for ( @d) {
			my %acp = exists($x{$_}-> {profile}) ? %{$x{$_}-> {profile}} : ();
			my $ctrl = $nb-> insert_to_page( $i, $x{$_}->{class} =>
				growMode => gm::GrowLoY,
				%acp,
				onMouseDown => \&Control_MouseDown_FirstStage,
				onKeyDown   => \&Control_KeyDown,
				origin      => [ @org],
			);
			$ctrl-> {CLSID} = $_;
			$ctrl-> {DOCKMAN} = $self;
			push ( @ctrls, $ctrl);
			my @s = $ctrl-> size;
			if (( $s[0] + $org[0] > $szn[0] - 5) && ( $ix > 0)) {
				$ctrl-> origin( $org[0] = 5, $org[1] += $maxy + 3);
				$maxy = 0;
			} else {
				$org[0] += $s[0] + 1;
				$maxy = $s[1] if $maxy < $s[1];
			}
			$ix++;
		}
		if ( $org[1] + $maxy < $szn[1] - 5) {
			my $d = $sz[1] - 5 - $org[1] - $maxy;
			for ( @ctrls) {
				my @o = $_-> origin;
				$_-> origin( $o[0], $o[1] + $d);
			}
		}
		$i++;
	}
	return $lb, $nb;
}

sub create_tool
{
	my ( $self, $where, $CLSID, @rect) = @_;
	my $x = $self-> get_class( $CLSID);
	return unless $x;
	my %acp = exists($x-> {profile}) ? %{$x-> {profile}} : ();
	%acp = ( %acp,
		onMouseDown => \&Control_MouseDown,
		onKeyDown   => \&Control_KeyDown,
		onDestroy   => \&Control_Destroy,
	);
	$acp{rect} = \@rect if 4 == scalar @rect;
	my $ctrl = $where-> insert( $x->{class} => %acp);
	$ctrl-> {CLSID}   = $CLSID;
	$ctrl-> {DOCKMAN} = $self;
	$ctrl-> enabled(0) if !$self-> {interactiveDrag} && exists( $self-> {commands}->{$CLSID})
	&& !$self-> {commands}->{$CLSID};
	return $ctrl;
}

sub create_toolbar
{
	my ( $self, %profile) = @_;
	my $v    = $profile{vertical} || 0;
	my $dock = $profile{dock};
	my $auto = exists( $profile{autoClose}) ? $profile{autoClose} : ( exists($profile{name}) ? 1 : 0);
	my $name = $profile{name} || $self-> auto_toolbar_name;
	my $visible = exists($profile{visible}) ? $profile{visible} : 1;
	my @r    = $profile{rect} ? @{$profile{rect}} : ( 0, 0, 10, 10);
	my $acd  = $profile{dockerProfile}  || {};
	my $aci  = $profile{toolbarProfile} || {};

	my $x = Prima::DockManager::Toolbar-> create(
		dockingRoot => $self,
		name        => $name,
		text        => $name,
		visible     => $visible,
		vertical    => $v,
		instance    => $self,
		autoClose   => $auto,
		onEDSClose  => \&Toolbar_EDSClose,
		%$acd,
	);

	my @i = @{$x-> indents};
	$x-> rect( $r[0] - $i[0], $r[1] - $i[1], $r[2] + $i[2], $r[3] + $i[3]);
	@r = $x-> frame2client( $x-> rect);

	my $xcl = $x-> insert( 'Prima::DockManager::ToolbarDocker' =>
		%$aci,
		origin       => [ @i[0,1]],
		size         => [ $r[2] - $r[0], $r[3] - $r[1]],
		vertical     => $v,
		growMode     => gm::Client,
		fingerprint  => dmfp::Toolbar,
		parentDocker => $x,
		name         => $name,
		instance     => $self,
		onDestroy   => \&Toolbar_Destroy,
	);
	$x-> client( $xcl);
	$x-> childDocker( $xcl);
	$self-> add_subdocker( $xcl);
	if ( $profile{dock}) {
		$x-> dock( $dock, $dock-> client_to_screen( $x-> rect));
	} else {
		$x-> externalDocker-> rect( $x-> externalDocker-> client2frame( @r));
	}
	push ( @{$self-> {toolbars}}, $x);
	$self-> toolbar_visible( $x, 0) unless $visible;
	$self-> notify(q(ToolbarChange));
	return $x, $xcl;
}

sub create_panel
{
	my ( $self, $CLSID, %profile) = @_;
	my $prf = $self-> get_class( $CLSID);
	return unless $prf;
	my %prf = (
		dockingRoot         => $self,
		x_sizeable          => 1,
		y_sizeable          => 1,
		instance            => $self,
	);
	$prf{text} = $prf-> {text} if exists $prf-> {text};
	my $x = Prima::DockManager::Panelbar-> create( %prf,
		$prf-> {dockerProfile} ? %{$prf-> {dockerProfile}} : (),
		$profile{dockerProfile} ? %{$profile{dockerProfile}} : (),
		dock  => undef,
	);
	$x-> onEDSClose( \&Panel_EDSClose);
	$prf-> {text} = $x-> text unless exists $prf-> {text};
	my @rrc = $x-> frame2client( 0, 0, $x-> size);
	my $xcl = $x-> insert( $prf->{class} =>
		growMode => gm::Client,
		$prf-> {profile} ? %{$prf-> {profile}} : (),
		$profile{profile} ? %{$profile{profile}} : (),
		rect      => [@rrc],
	);
	$xcl-> add_notification( 'Destroy', \&Panelbar_Destroy, $x);
	$x-> client( $xcl);
	push( @{$self-> {panels}}, $x);
	$x-> {CLSID} = $CLSID;
	if ( $prf-> {dockerProfile}-> {dock} || $profile{dockerProfile}-> {dock}) {
		my $dock = $prf-> {dockerProfile}-> {dock} || $profile{dockerProfile}-> {dock};
		$x-> dock( $dock, $dock-> client_to_screen( $x-> rect));
	}
	$self-> notify(q(PanelChange));
	return ( $x, $xcl);
}

sub auto_toolbar_name
{
	my $name;
	my $self = $_[0];
	my @ids;
	for ( @{$self->{toolbars}}) {
		my $x = $_-> name;
		next unless $x =~ m/^ToolBar(\d+)$/;
		$ids[$1] = 1;
	}
	my $i = 0;
	for ( @ids) {
		$i++, next unless $i; # skip ToolBar0
		$name = $i, last unless $_;
		$i++;
	}
	$name = scalar(@ids) unless defined $name;
	$name++ unless $name;
	return "ToolBar$name";
}

sub toolbar_menuitems
{
	my ( $self, $sub) = @_;
	my @items;
	for ( @{$self->{toolbars}}) {
		my $vis = $_-> dock() ? $_-> visible : $_-> externalDocker-> visible;
		$vis = ( $vis ? '*' : '') . $_-> name;
		push ( @items, [ $vis => $_-> name => $sub ]);
	}
	return \@items;
}

sub panel_menuitems
{
	my ( $self, $sub) = @_;
	my @items;
	my %h = map { $_->{CLSID} => 1} @{$self-> {panels}};
	my $i;
	my $c = $self-> {classes};
	for ( $i = 0; $i < @$c; $i += 2) {
		my ( $CLSID, $hash) = @$c[$i,$i+1];
		next if $hash->{tool};
		my $vis = ( $h{$CLSID} ? '*' : '') . $CLSID;
		push ( @items, [ $vis => $hash-> {text} => $sub ]);
	}
	return \@items;
}

sub toolbar_visible
{
	my ( $self, $d, $visible) = @_;
	return unless $d;
	if ( $d-> dock) {
		return if $visible == $d-> visible;
		if ( $visible) {
			$d-> dock_back;
		} else {
			$d-> dock( undef);
			$d-> externalDocker-> visible( $visible);
		}
	} else {
		return if $visible == $d-> externalDocker-> visible;
		$d-> externalDocker-> visible( $visible);
	}
}

sub panel_visible
{
	my ( $self, $panelbarCLSID, $visible) = @_;
	my $d = $self-> panel_by_id( $panelbarCLSID);
	my $hash = $self-> get_class( $panelbarCLSID);
	if ( $visible) {
		return if $d;
		my %pf;
		if ( $hash-> {lastUsedDock} && Prima::Object::alive($hash-> {lastUsedDock})) {
			$pf{dockerProfile}->{dock} = $hash-> {lastUsedDock};
		}
		if ( $hash-> {lastUsedRect}) {
			$pf{dockerProfile}->{rect} = $hash-> {lastUsedRect};
		}
		my ( $x, $xcl) = $self-> create_panel( $panelbarCLSID, %pf);
		$x-> bring_to_front;
	} else {
		return unless $d;
		$hash-> {lastUsedDock} = $d-> dock;
		$hash-> {lastUsedRect} = [$d-> dock ? $d-> rect : $d-> externalDocker-> rect],
		$d-> close;
	}
}

sub predefined_toolbars
{
	my $self = shift;
	my %toolbars = map { $_-> name => $_ } @{$self-> {toolbars}};
	my %c = @{$self-> {classes}};
	my @o  = ( 10, $::application-> height - 100);
	my @as = $::application-> size;
	for ( @_) {
		my $rec = $_;
		next if $toolbars{$_-> {name}};
		my @org = (0,0);
		my $maxy = 0;
		my @ctrls;
		my @list = $rec->{list} ? @{$rec->{list}} : ();
		for ( @list) {
			my $ctrl = $self-> create_tool( $::application, $_);
			next unless $ctrl;
			$ctrl-> origin( @org);
			my @sz = $ctrl-> size;
			$org[0] += $sz[0];
			$maxy = $sz[1] if $maxy < $sz[1];
			push ( @ctrls, $ctrl);
		}
		my @oz = $rec->{origin} ? @{$rec->{origin}} : ( $rec->{dock} ? (0,0) : @o);
		my ( $x, $xcl) = $self-> create_toolbar(
			name    => $_->{name},
			rect    => [ @oz, $oz[0]+$org[0], $oz[1]+$maxy],
			visible => 1,
			dock    => $rec->{dock},
		);
		for ( @ctrls) {
			$_-> owner( $xcl);
			$xcl-> dock( $_);
		}
		$xcl-> rearrange;
		$o[0] += 25;
		$o[1] -= 25;
	}
}

sub predefined_panels
{
	my ( $self, @rec) = @_;
	my $i;
	my %pan = map { $_-> {CLSID} => 1 } @{$self-> {panels}};
	for ( $i = 0; $i < scalar @rec; $i += 2) {
		my ( $CLSID, $dock) = @rec[$i, $i+1];
		next if $pan{$CLSID};
		my ( $a, $b) = $self-> create_panel( $CLSID, dockerProfile => {dock => $dock});
	}
}

sub activate
{
	my $self = $_[0];
	for ( @{$self->{panels}}, @{$self-> {toolbars}}) {
		next if $_-> dock;
		$_-> externalDocker-> bring_to_front if $_-> externalDocker;
	}
}

sub windowState
{
	my ( $self, $ws) = @_;
	if ( $ws == ws::Minimized) {
		for ( @{$self->{panels}}, @{$self-> {toolbars}}) {
			next if $_-> dock;
			my $e = $_-> externalDocker;
			next unless $e;
			$e-> hide;
			push ( @{$self->{hiddenToolbars}}, $e);
		}
	} else {
		$_-> show for @{$self->{hiddenToolbars}};
		@{$self->{hiddenToolbars}} = ();
	}
}

sub commands_enable
{
	my ( $self, $enable) = ( shift, shift);
	my %cmds = map { $_ => 1 } @_;
	unless ( $self-> interactiveDrag) {
		for ( $self-> toolbars)  {
			for ( $_-> childDocker-> docklings) {
				next if !defined $_->{CLSID} || !$cmds{$_->{CLSID}} || $enable == $self-> {commands}->{$_->{CLSID}};
				$_-> enabled( $enable);
			}
		}
	}
	for ( keys %{$self->{commands}}) {
		next unless $cmds{$_};
		$self-> {commands}-> {$_} = $enable;
	}
	$self-> notify(q(CommandChange));
}

sub commands
{
	return $_[0]-> {commands} unless $#_;
	my ( $self, $cmds) = @_;
	$self-> {commands} = $cmds;
	unless ( $self-> interactiveDrag) {
		for ( $self-> toolbars)  {
			for ( $_-> childDocker-> docklings) {
				next if !defined $_->{CLSID} || !$cmds-> {$_->{CLSID}};
				$_-> enabled( $cmds-> {$_-> {CLSID}});
			}
		}
	}
	$self-> notify(q(CommandChange));
}

# internals

sub autodock
{
	my ( $self, $ctrl) = @_;
	my $dock = $ctrl-> owner;
	$dock-> undock( $ctrl);

	my ( $x, $xcl) = $self-> create_toolbar(
		vertical  => $dock-> can('vertical') ? $dock-> vertical : 0,
		dock      => $dock,
		rect      => [$ctrl-> rect],
		autoClose => 1,
	);
	$ctrl-> owner( $xcl);
	$ctrl-> origin( 0,0);
	$xcl-> dock( $ctrl);
	return $x;
}


sub Control_KeyDown
{
	return unless $_[0]-> {DOCKMAN}-> interactiveDrag;
	$_[0]-> clear_event;
}

sub Control_MouseDown_FirstStage
{
	my ($self,$btn, $mod, $x,$y) = @_;
	return unless $btn == mb::Left;
	my $man = $self-> {DOCKMAN};
	my $c = Prima::InternalDockerShuttle-> create(
		owner       => $self-> owner,
		rect        => [$self-> rect],
		dockingRoot => $man,
		fingerprint => dmfp::LaunchPad | dmfp::Toolbar | dmfp::Tools, # allow all docks
		backColor   => cl::Yellow,
		onLanding   => \&InternalDockerShuttle_Landing,
		name        => 'FirstStage',
		onDock      => sub {
			my $me = $_[0];
			if ( $me-> owner-> isa(q(Prima::DockManager::LaunchPad))) {
				$man-> post( sub { $me-> destroy; });
				return;
			}
			my $ctrl = $man-> create_tool( $me-> owner, $self-> {CLSID}, $me-> rect);
			return unless $ctrl;
			$me-> {dock} = undef;
			$me-> owner-> replace( $me, $ctrl);
			$man-> post( sub { $me-> destroy; });
			$man-> autodock( $ctrl)
				unless $me-> owner-> isa(q(Prima::DockManager::ToolbarDocker));
		},
		onFailDock => sub {
			my ( $me, $ax, $ay) = @_;
			my ( $x, $xcl) = $man-> create_toolbar( rect => [$me-> rect], autoClose => 1);
			$xcl-> dock( $man-> create_tool( $xcl, $self-> {CLSID}));
			$x-> externalDocker-> origin( $ax, $ay);
			$man-> post( sub { $me-> destroy; });
		},
	);
	$c-> externalDocker-> hide;
	$::application-> yield;
	$c-> drag( 1, [ $self-> client_to_screen(0,0,$self-> size)], $c-> screen_to_client( $self-> client_to_screen($x, $y)));
	$self-> clear_event;
}

sub Control_Destroy
{
	$_[0]-> owner-> undock( $_[0]);
}

sub Control_MouseDown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	my $man = $self-> {DOCKMAN};
	return unless $man-> interactiveDrag;
	$self-> clear_event;
	return unless $btn == mb::Left;

	my $c;
	$c = Prima::InternalDockerShuttle-> create(
		owner       => $self-> owner,
		rect        => [$self-> rect],
		dockingRoot => $man,
		fingerprint => dmfp::LaunchPad | dmfp::Toolbar | dmfp::Tools, # allow all docks
		backColor   => cl::White,
		onLanding   => \&InternalDockerShuttle_Landing,
		name        => 'SecondStage',
		onDock      => sub {
			my $me = $_[0];
			$me-> {dock} = undef;
			$me-> owner-> replace( $me, $self);
			$man-> post( sub { $me-> destroy; });
			if ( $me-> owner-> isa(q(Prima::DockManager::LaunchPad))) {
				$man-> post( sub { $self-> destroy; });
				return;
			}
			$man-> autodock( $self) unless $me-> owner-> isa(q(Prima::DockManager::ToolbarDocker));
		},
		onFailDock => sub {
			$self-> owner-> replace( $c, $self);
			$c-> {dock} = undef;
			$man-> post( sub { $c-> destroy; });
		},
	);
	$c-> {dock} = $self-> owner;
	$self-> owner-> replace( $self, $c);
	$c-> externalDocker-> hide;
	$c-> hide;
	$::application-> yield;
	$c-> drag( 1, [ $self-> client_to_screen(0,0,$self-> size)], $c-> screen_to_client( $self-> client_to_screen($x, $y)));
}

sub Panelbar_Destroy
{
	my $self = $_[0];
	my $i = $self-> instance;
	return unless $i;
	@{$i-> {panels}} = grep { $_ != $self } @{$i->{panels}};
	@{$i-> {hiddenToolbars}} = grep { $_ != $self } @{$i->{hiddenToolbars}};
	$i-> notify(q(PanelChange));
}

sub Toolbar_Destroy
{
	my $self = $_[0]-> parentDocker;
	my $i = $self-> instance;
	return unless $i;
	@{$i-> {toolbars}} = grep { $_ != $self } @{$i->{toolbars}};
	@{$i-> {hiddenToolbars}} = grep { $_ != $self } @{$i->{hiddenToolbars}};
	$i-> notify(q(ToolbarChange));
}

sub Toolbar_EDSClose
{
	my $e = $_[0]-> externalDocker;
	$e-> hide;
	$_[0]-> clear_event;
	$_[0]-> instance-> notify(q(ToolbarChange));
}

sub Panel_EDSClose
{
	my $hash = $_[0]-> instance-> get_class($_[0]-> {CLSID});
	return unless $hash;
	$hash-> {lastUsedDock} = undef;
	$hash-> {lastUsedRect} = [ $_[0]-> externalDocker-> rect ];
	$_[0]-> instance-> notify(q(PanelChange));
}

sub InternalDockerShuttle_Landing
{
	my ( $self, $dm, @rc) = @_;
	return unless $self-> {drag}; # only for interactive
	my $wi = $::application-> get_widget_from_point($::application-> pointerPos);
	return if !$wi || $wi == $dm;
	unless ( $wi-> can('dock')) {
		$wi = $wi-> owner;
		return if $wi == $dm;
	}
	return unless $wi-> can('dock') && $wi-> isa('Prima::DockManager::ToolbarDocker');
	$self-> clear_event;
}

package Prima::DockManager::S::SpeedButton;

sub class
{
	my ( $image, $action, %profile) = @_;
	$image =~ s/\:(\d+)$//;
	my $index = $1 || 0;
	my $i = Prima::Icon-> create;
	undef($i) unless $i-> load(Prima::Utils::find_image($image), index => $index);
	my $s = $::application ? $::application->uiScaling : 1;
	$i->ui_scale if $i;
	return $action, {
		class   => 'Prima::SpeedButton',
		profile => {
			size        => [ 24 * $s, 24 * $s],
			image       => $i,
			borderWidth => 1,
			onClick     => \&on_click,
			%profile,
		},
	},
}

sub on_click
{
	$_[0]-> owner-> instance-> notify(q(Command), $_[0]-> {CLSID});
}

1;

=pod

=head1 NAME

Prima::DockManager - advanced dockable widgets

=head1 DESCRIPTION

C<Prima::DockManager> contains classes that implement additional
functionality in the dockable widgets paradigm.

The module introduces two new dockable widget classes:
C<Prima::DockManager::Panelbar>, a general-purpose dockable container for
variable-sized widgets; and C<Prima::DockManager::Toolbar>, a dockable
container for fixed-size command widgets, mostly push buttons.  The command
widgets nested in a toolbar can also be docked.

The C<Prima::DockManager> class is application-oriented in a way that a
single of it is needed. It is derived from C<Prima::Component> and therefore is
never visualized.  The class instance is stored in the C<instance> property in all
module classes to serve as a docking hierarchy root. Through the document,
the I<instance> term means the C<Prima::DockManager> class instance.

The module by itself is not enough to make a docking-aware application work
effectively. The reader is urged to look at the F<examples/dock.pl>
example code, which demonstrates the usage and capabilities of
the module.

=head1 Prima::DockManager::Toolbar

A toolbar widget class. The toolbar has a dual nature; it can both dock itself
and accept dockable widgets. As a dock, toolbars can host command widgets, mostly
push buttons.

The toolbar consists of two widgets. The external dockable widget is
implemented in C<Prima::DockManager::Toolbar>, and the internal dock
in C<Prima::DockManager::ToolbarDocker> classes.

=head2 Properties

=over

=item autoClose BOOLEAN

Selects the behavior of the toolbar when all of its command widgets are
undocked. If 1 (default), the toolbar is automatically destroyed. If 0
it calls C<visible(0)>.

=item childDocker WIDGET

Pointer to the C<Prima::DockManager::ToolbarDocker> instance.

See also C<Prima::DockManager::ToolbarDocker::parentDocker>.

=item instance INSTANCE

C<Prima::DockManager> instance, the docking hierarchy root.

=back

=head1 Prima::DockManager::ToolbarDocker

An internal class, implements the dock widget for command widgets, and a client
in a dockable toolbar, a C<Prima::LinearDockerShuttle> descendant. When its
size is changed due to an eventual rearrangement of its docked widgets, also resizes
the toolbar.

=head2 Properties

=over

=item instance INSTANCE

The C<Prima::DockManager> instance, the docking hierarchy root.

=item parentDocker WIDGET

Pointer to a C<Prima::DockManager::Toolbar> instance. When in
the docked state, the C<parentDocker> value is always equal to C<owner>.

See also C<Prima::DockManager::Toolbar::childDocker>.

=back

=head2 Methods

=over

=item get_extent

Calculates the minimal rectangle that encloses all docked widgets
and returns its extensions.

=item update_size

Called when the size is changed to resize the owner widget. If the toolbar is
docked, the change might result in a change of its position or docking
state.

=back

=head1 Prima::DockManager::Panelbar

The class is derived from C<Prima::LinearDockerShuttle>, and
is different only in that the C<instance> property is introduced,
and the external shuttle can be resized interactively.

The class is to be used as a simple host to sizeable widgets.
The user can dispose of the panel bar by clicking the close button
on the external shuttle.

=head2 Properties

=over

=item instance INSTANCE

The C<Prima::DockManager> instance, the docking hierarchy root.

=back

=head1 Prima::DockManager

A binder class, contains a set of functions that groups toolbars, panels, and
command widgets together under the docking hierarchy.

The manager serves several purposes.  First, it is a command state holder: the
command widgets, mostly buttons, usually are in an enabled or disabled state in
different life stages of a program. The manager maintains the enabled/disabled
state by assigning each command a unique scalar value, or a I<CLSID>.
The toolbars can be created using a set of command widgets, using these
CLSIDs. The same is valid for the panels - although they do not host command
widgets, the widgets that they do host can also be created indirectly via CLSID
identifier.  In addition to CLSIDs, the commands can be grouped by sections.
Both CLSID and group descriptor scalars are defined by the programmer.

Second, the C<create_manager> method presents the standard launchpad interface
that allows the rearranging of normally non-dockable command widgets, by presenting
a full set of available commands to the user as icons.  Dragging the icons to
toolbars, dock widgets, or merely outside the configuration widget
automatically creates the corresponding command widget.  The notable moment
here is that the command widgets are not required to know anything about
dragging and docking; any C<Prima::Widget> descendant can be used as a command
widget.

Third, it helps maintain the toolbars' and panels' visibility
when the main window is hidden or restored. The C<windowState> method
hides or shows the toolbars and panels effectively.

Fourth, it serves as a docking hierarchy root. All docking sessions start their
protocol interactions at a C<Prima::DockManager> object, which although does
not provide docking capabilities itself ( it is a C<Prima::Component> descendant
), redirects the docking requests to the children dock widgets.

Finally, it provides several helper methods and notifications
and enforces the use of the C<fingerprint> property by all dockable widgets.
The module defines the following fingerprint C<dmfp::XXX> constants:

	fdmp::Tools      ( 0x0F000) - dock the command widgets
	fdmp::Toolbar    ( 0x10000) - dock the toolbars
	fdmp::LaunchPad  ( 0x20000) - allows widgets recycling

All this functionality is demonstrated in F<examples/dock.pl>
example.

=head2 Properties

=over

=item commands HASH

A hash of boolean values with keys of CLSID scalars, where if the value is 1,
the command is available and is disabled otherwise. Changes to this property
are reflected in the visible command widgets, which are enabled or disabled
immediately. Also, the C<CommandChange> notification is triggered.

=item fingerprint INTEGER

The property is read-only, and always returns C<0xFFFFFFFF>,
to allow landing to all dockable widgets. In case a finer
granulation is needed, the default C<fingerprint> values of
toolbars and panels can be reset.

=item interactiveDrag BOOLEAN

If 1, the command widgets can be interactively dragged, created, and destroyed.
This property is usually operated together with the C<create_manager> launchpad
widget. If 0, the command widgets cannot be dragged.

Default value: 0

=back

=head2 Methods

=over

=item activate

Brings to front all toolbars and panels. To be
used inside a callback code of a main window, that has
the toolbars and panels attached to:

	onActivate => sub { $dock_manager-> activate }

=item auto_toolbar_name

Returns a unique name for an automatically created
toolbar, like C<Toolbar1>, C<Toolbar2> etc.

=item commands_enable BOOLEAN, @CLSIDs

Enables or disables commands from CLSIDs array.  The changes are reflected in
the visible command widgets, which are enabled or disabled immediately.  Also,
the C<CommandChange> notification is triggered.

=item create_manager OWNER, %PROFILE

Inserts two widgets into OWNER with PROFILE parameters: a list box with command
section groups, displayed as items, that usually correspond to the predefined
toolbar names, and a notebook that displays the command icons. The notebook
pages can be interactively selected by the list box navigation.

The icons dragged from the notebook, behave as dockable widgets: they can be
landed on a toolbar, or any other dock widget, given it matches the
C<fingerprint> ( by default C<dmfp::LaunchPad|dmfp::Toolbar|dmfp::Tools>).
C<dmfp::LaunchPad> constant allows the recycling; if a widget is dragged back
onto the notebook, it is destroyed.

Returns the two widgets created, the list box and the notebook.

PROFILE recognizes the following keys:

=over

=item origin X, Y

Position where the widgets are to be inserted.
The default value is 0,0.

=item size X, Y

Size of the widget insertion area. By default,
the widgets occupy all OWNER interiors.

=item listboxProfile PROFILE

Custom parameters passed to the list box.

=item dockerProfile PROFILE

Custom parameters passed to the notebook.

=back

=item create_panel CLSID, %PROFILE

Spawns a dockable panel from a previously registered CLSID.
PROFILE recognizes the following keys:

=over

=item profile HASH

A hash of parameters passed to the C<new()> method of the panel widget class.
Before passing it is merged with the set of parameters registered
by C<register_panel>. The C<profile> hash takes precedence.

=item dockerProfile HASH

Contains extra options passed to the C<Prima::DockManager::Panelbar>
widget. Before passing it is merged with the set of parameters
registered by C<register_panel>.

Note: The C<dock> key contains a reference to the desired dock widget.
If C<dock> is set to C<undef>, the panel is created in the non-docked state.

=back

Example:

	$dock_manager-> create_panel( $CLSID,
		dockerProfile => { dock => $main_window }},
		profile       => { backColor => cl::Green });

=item create_tool OWNER, CLSID, X1, Y1, X2, Y2

Inserts a command widget, previously registered with a CLSID by
C<register_tool>, into OWNER widget at X1 - Y2 coordinates. For automatic
maintenance of enabled/disabled states of command widgets, OWNER is expected to
be a toolbar. If it is not, the maintenance must be performed separately, for
example, by reacting to the C<CommandChange> event.

=item create_toolbar %PROFILE

Creates a new toolbar of the C<Prima::DockManager::Toolbar> class.
The following PROFILE options are recognized:

=over

=item autoClose BOOLEAN

Manages the C<autoClose> property of the toolbar.

The default value is 1 if the C<name> option is set, and 0 otherwise.

=item dock DOCK

Contains a reference to the desired DOCK widget. If C<undef>,
the toolbar is created in the non-docked state.

=item dockerProfile HASH

Parameters passed to C<Prima::DockManager::Toolbar> as
creation properties.

Note: The C<dock> key contains a reference to the desired dock widget.
If C<dock> is set to C<undef>, the panel is created in the non-docked state.

=item rect X1, Y1, X2, Y2

Manages geometry of the C<Prima::DockManager::ToolbarDocker> instance in
the dock widget ( if docked ) or the screen ( if non-docked ) coordinates.

=item toolbarProfile HASH

Parameters passed to C<Prima::DockManager::ToolbarDocker> as
properties.

=item vertical BOOLEAN

Sets the C<vertical> property of the toolbar.

=item visible BOOLEAN

Selects the visibility state of the toolbar.

=back

=item get_class CLSID

Returns a class record hash, registered under a CLSID, or C<undef>
if the class is not registered. The hash format contains
the following keys:

=over

=item class STRING

Widget class

=item profile HASH

Creation parameters passed to C<new> when the corresponding widget is
instantiated.

=item tool BOOLEAN

If 1, the class belongs to a control widget. If 0,
the class represents a panel client widget.

=item lastUsedDock DOCK

Saved value of the last used dock widget

=item lastUsedRect X1, Y1, X2, Y2

Saved coordinates of the widget

=back

=item panel_by_id CLSID

Returns reference to the panel widget represented by CLSID scalar,
or C<undef> if none is found.

=item panel_menuitems CALLBACK

A helper function; maps all panel names into a structure, ready to
feed into the C<Prima::AbstractMenu::items> property ( see L<Prima::Menu> ).
The action member of the menu item record is set to the CALLBACK scalar.

=item panel_visible CLSID, BOOLEAN

Sets the visibility of a panel referred to by the CLSID scalar.
If VISIBLE is 0, the panel is destroyed; if 1, a new panel instance
is created.

=item panels

Returns all visible panel widgets in an array.

=item predefined_panels CLSID, DOCK, [ CLSID, DOCK, ... ]

Accepts pairs of scalars, where each first item is a panel CLSID
and the second is the default dock widget. Checks for the panel visibility
and creates the panels that are not visible.

The method is useful in a program startup, when some panels
have to be visible from the beginning.

=item predefined_toolbars @PROFILES

Accepts an array of hashes where each array item describes a toolbar and
a default set of command widgets. Checks for the toolbar visibility
and creates the toolbars that are not visible.

The method recognizes the following PROFILES options:

=over

=item dock DOCK

The default dock widget.

=item list ARRAY

An array of CLSIDs corresponding to the command widgets to be inserted
into the toolbar.

=item name STRING

Selects the toolbar name.

=item origin X, Y

Selects the toolbar position relative to the dock ( if C<dock> is specified )
or to the screen ( if C<dock> is not specified ).

=back

The method is useful in program startup, when some panels
have to be visible from the beginning.

=item register_panel CLSID, PROFILE

Registers a panel client class and set of parameters to be associated with
a CLSID scalar. PROFILE must contain the following keys:

=over

=item class STRING

Client widget class

=item text STRING

A string of text displayed in the panel title bar

=item dockerProfile HASH

A hash of parameters passed to C<Prima::DockManager::Panelbar>.

=item profile

A hash of parameters passed to the client widget.

=back

=item register_tool CLSID, PROFILE

Registers a control widget class and set of parameters to be associated with
a CLSID scalar. PROFILE must contain the following keys:

=over

=item class STRING

Client widget class

=item profile HASH

A hash of parameters passed to the control widget.

=back

=item toolbar_by_name NAME

Returns a reference to the toolbar of NAME, or C<undef> if none is found.

=item toolbar_menuitems CALLBACK

A helper function; maps all toolbar names into a structure, ready to
feed into the C<Prima::AbstractMenu::items> property ( see L<Prima::Menu> ).
The action member of the menu item record is set to the CALLBACK scalar.

=item toolbar_visible TOOLBAR, BOOLEAN

Sets the visibility of a TOOLBAR.
If VISIBLE is 0, the toolbar is hidden; if 1, it is shown.

=item toolbars

Returns all toolbar widgets in an array.

=item windowState INTEGER

Mimics interface of C<Prima::Window::windowState>, and maintains
visibility of toolbars and panels. If the parameter is C<ws::Minimized>,
the toolbars and panels are hidden. On any other parameter, these are shown.

To be used inside a callback code of a main window, that has the toolbars
and panels attached to:

	onWindowState => sub { $dock_manager-> windowState( $_[1] ) }

=back

=head2 Events

=over

=item Command CLSID

A generic event triggered by a command widget when the user activates
it. It can also be called by other means.

CLSID is the widget identifier.

=item CommandChange

Called when the C<commands> property changes or the C<commands_enable> method is invoked.

=item PanelChange

Triggered when a panel is created or destroyed by the user.

=item ToolbarChange

Triggered when a toolbar is created, shown, gets hidden, or destroyed by the user.

=back

=head1 Prima::DockManager::S::SpeedButton

The package simplifies the creation of C<Prima::SpeedButton> command widgets.

=head2 Methods

=over

=item class IMAGE, CLSID, %PROFILE

Builds a hash with parameters, ready to feed to
C<Prima::DockManager::register_tool> for registering a combination of the
C<Prima::SpeedButton> class and the PROFILE parameters.

IMAGE is the path to an image file, loaded and stored in the registration hash.
IMAGE provides an extended syntax for selecting the frame index if the image file
is multiframed: the frame index is appended to the path name with the C<:>
character prefix.

CLSID scalar is not used but is returned so the method result can
directly be passed into the C<register_tool> method.

Returns two scalars: CLSID and the registration hash.

Example:

	$dock_manager-> register_tool(
		Prima::DockManager::S::SpeedButton::class( "myicon.gif:2",
		q(CLSID::Logo), hint => 'Logo image' ));

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Docks>, F<examples/dock.pl>

=cut
