=pod

=head1 NAME

examples/dock.pl - Docking widgets

=head1 FEATURES

A demo of the Prima::Dock and Prima::DockManager modules. The window is a
docking client that is able to accept toolbars and panels as docked widdgets,
and the toolbars in turn are able to accept buttons. Buttons are very
primitive; there are two panels, Edit and Banner, that are docked using
different mechanisms.  Note the following unevident features:

=over 4

=item popup on the border of the window (and the Customize command there)

=item dragging of buttons on the window and the extreior

=item dragging panels and toolbar outside the window

=item storing and loading of the geometry in the ~/.demo_dock file

=back

=cut

use strict;
use warnings;

use Prima;
use Prima::Application;
use Prima::Edit;
use Prima::Buttons;
use Prima::DockManager;
use Prima::Utils;

package dmfp;
use constant Edit       => 0x100000;
use constant Vertical   => 0x200000;
use constant Horizontal => 0x400000;

# This is the main window. it's responsible for
# command handling and bar visiblity;
# NB - bars are not owned by this window when undocked.

package Prima::Dock::BasicWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub profile_default
{
	my $def = $_[0]-> SUPER::profile_default;
	my %prf = (
		instance => undef,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my  $self = shift;
	my %profile = $self-> SUPER::init( @_);
	$self-> $_($profile{$_}) for qw(instance);
	$self-> {toolBarPopup} = $self-> insert( Popup =>
		autoPopup  => 0,
		items      => $self-> make_popupitems(),
	);
	$self-> {mainDock} = $self-> insert( FourPartDocker =>
		rect        => [ 0, 0, $self-> size],
		fingerprint => dmfp::Tools|dmfp::Toolbar|dmfp::Edit|dmfp::Horizontal|dmfp::Vertical,
		dockup      => $self-> instance,
		dockerCommonProfile => {
			hasPocket => 0,
			onPopup => sub { # all dockers would render this popup
				my ( $me, $btn, $x, $y) = @_;
				( $x, $y) = $self-> screen_to_client( $me-> client_to_screen($x, $y));
				$self-> {toolBarPopup}-> popup( $x, $y);
				$me-> clear_event;
			}
		},
		dockerProfileClient => { # allow docking only to Edit
			fingerprint => dmfp::Edit,
		},
		dockerProfileLeft   => { fingerprint => dmfp::Vertical|dmfp::Tools|dmfp::Toolbar },
		dockerProfileRight  => { fingerprint => dmfp::Vertical|dmfp::Tools|dmfp::Toolbar },
		dockerProfileTop    => { fingerprint => dmfp::Horizontal|dmfp::Tools|dmfp::Toolbar },
		dockerProfileBottom => { fingerprint => dmfp::Horizontal|dmfp::Tools|dmfp::Toolbar },
	);
	$self-> instance-> add_notification( 'ToolbarChange', \&on_toolbarchange, $self);
	$self-> instance-> add_notification( 'PanelChange',   \&on_toolbarchange, $self);
	$self-> instance-> add_notification( 'Command',   \&on_command, $self);
	return %profile;
}

sub make_popupitems
{
	my $items = $_[0]-> instance-> toolbar_menuitems( \&Menu_Check_Toolbars);
	# actually DockManager doesn't care if panel CLSID and toolbar name intermix.
	# this is the demonstration of resolving that clash
	$$_[0] .= ',toolbar' for @$items;
	push ( @$items, []);
	push ( @$items, @{$_[0]-> instance-> panel_menuitems( \&Menu_Check_Panels)});
	push ( @$items, []);
	push ( @$items, ['customize' => "~Customize..." => q(open_dockmanaging)]);
	return $items;
}


sub Menu_Check_Toolbars
{
	my ( $self, $var) = @_;
	my $toolname = $var;
	$toolname =~ s/\,toolbar$//;
	$self-> instance-> toolbar_visible(
		$self-> instance-> toolbar_by_name($toolname),
		$self-> {toolBarPopup}-> toggle( $var)
	);
}

sub Menu_Check_Panels
{
	my ( $self, $var) = @_;
	$self-> instance-> panel_visible(
		$var, $self-> {toolBarPopup}-> toggle( $var));
}

sub instance
{
	return $_[0]-> {instance} unless $#_;
	$_[0]-> {instance} = $_[1];
}


sub on_toolbarchange
{
	$_[0]-> {toolBarPopup}-> items( $_[0]-> make_popupitems());
}

sub on_command
{
	my ( $self, $instance, $command) = @_;
	$command =~ s/\://g;
	my $x = $self-> can( $command);
	return unless $x;
	$x-> ( $self);
}

# we'll take our actions we need to reflect the state.
sub open_dockmanaging
{
	my $self = $_[0];
	my $i = $self-> instance;
	return if $i-> interactiveDrag;
	my $wpanel = Prima::Window-> new(
		name => 'Customize tools',
		size => [ 400, 200],
		onClose => sub {
			$self-> {toolBarPopup}-> customize-> enabled(1);
			$i-> interactiveDrag(0);
		},
	);
	$i-> create_manager( $wpanel,  dockerProfile => {
		hint => 'Drag here unneeded buttons',
	});
	$i-> interactiveDrag(1);
	$self-> {toolBarPopup}-> customize-> enabled(0);
}

sub get_docks
{
	my $self = $_[0];
	my @docks = ( $self-> {mainDock});
	my $sid = $self-> {mainDock}-> open_session({
		self => $self-> {mainDock},
		sizes => [[0,0]],
		sizeable => [1,1],
	});
	if ( $sid) {
		while ( 1) {
			my $x = $self-> {mainDock}-> next_docker( $sid);
			last unless $x;
			next if $x-> isa(q(Prima::DockManager::LaunchPad));
			push ( @docks, $x);
		}
		$self-> {mainDock}-> close_session( $sid);
	}
	return @docks;
}

sub init_read
{
	my ( $self, $fd) = @_;
	my $last = undef;
	my @docks = $self-> get_docks;
	my $state;
	my %docks = map { my $x = $_-> name; $x =~ s/(\W)/\%sprintf("%02x",$1)/; $x => $_} @docks;

	while ( <$fd>) {
		$state = 1, last if m/^DOCK_STMT_START/;
	}
	return unless $state;
	my $i = $self-> instance;
	my %audocks;
	tie %audocks, 'Tie::RefHash';


	while ( <$fd>) {
		chomp;
		last if m/^DOCK_STMT_END/;
		if ( m/^MYSELF\[(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\]/) {
			$self-> rect( $1,$2,$3,$4);
			next;
		}
		if ( m/^TOOLBAR\:(\w*)\:(\d)\:(\d)\:\[(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\]\:(.*)$/) {
			my ( $dockID, $vertical, $visible, $x1, $y1, $x2, $y2, $name) =
				($1,$2,$3,$4,$5,$6,$7,$8);
			my $auto = $name =~ /^ToolBar/;

			my ( $x, $xcl) = $i-> create_toolbar(
				visible   => $visible,
				vertical  => $vertical,
				dock      => $docks{$dockID},
				rect      => [ $x1, $y1, $x2, $y2],
				name      => $name,
				autoClose => $auto,
			);
			$last = $xcl;
			$name =~ s/(\W)/\%sprintf("%02x",$1)/;
			$docks{$name} = $xcl;
			next;
		} elsif ( m/^TOOL\:([^\s]+)\s\[(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\]/) {
			my ( $CLSID, $x1, $y1, $x2, $y2) = ($1,$2,$3,$4,$5);
			next unless $last;
			my $ctrl = $i-> create_tool( $last, $CLSID, $x1, $y1, $x2, $y2);
			next unless $ctrl;
			push @{$audocks{$last}}, $ctrl;
			next;
		} elsif ( m/^PANEL\:(\w*)\:([^\s]+)\s\[(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\]/) {
			my ( $dockID, $CLSID, $x1, $y1, $x2, $y2) = ($1,$2,$3,$4,$5,$6);
			my ( $x, $xcl) = $i-> create_panel( $CLSID, dockerProfile => {
				dock => $docks{$dockID},
				origin => [$x1, $y1], # because original profile uses size
				size   => [$x2 - $x1, $y2 - $y1], # this is hack to override it
				rect   => [ $x1, $y1, $x2, $y2],
			});
			next;
		}
	}
	$_-> dock_bunch( @{$audocks{$_}}) for keys %audocks;
	$i-> notify(q(ToolbarChange));
}

sub init_write
{
	my ( $self, $fd) = @_;
	print $fd "DOCK_STMT_START\n";
	my @rc = $self-> rect;
	print $fd "MYSELF[@rc]\n";
	for ( $self-> instance-> toolbars) {
		my $p = $_;
		my $x = $_-> childDocker;
		my ( $e, $n);
		my @rect = $x-> rect;
		if ( $p-> dock) {
			$e = $p;
			$n = $p-> dock-> name;
			$n =~ s/(\W)/\%sprintf("%02x",$1)/g;
			@rect = $p-> dock-> screen_to_client( $p-> client_to_screen( @rect));
		} else {
			$n = '';
			$e = $p-> externalDocker;
			@rect = $x-> client_to_screen( @rect);
		}
		my $vis  = $e-> visible ? 1 : 0;
		my $ver  = $x-> vertical ? 1 : 0;
		print $fd "TOOLBAR:$n:$ver:$vis:[@rect]:".$p-> text."\n";
		for ( $x-> docklings) {
			@rect = $_-> rect;
			my $ena = $_-> enabled;
			my $CLSID = $_-> {CLSID};
			next unless defined $CLSID;
			print $fd "TOOL:$CLSID [@rect]:$ena\n";
		}
	}
	for ( $self-> instance-> panels) {
		my @r = $_-> dock() ? $_-> rect : $_-> externalDocker-> rect;
		my $n = '';
		if ( $_-> dock) {
			$n = $_-> dock-> name;
			$n =~ s/(\W)/\%sprintf("%02x",$1)/g;
		}
		my $CLSID = $_-> {CLSID};
		print $fd "PANEL:$n:$CLSID [@r]\n";
	}
	print $fd "DOCK_STMT_END\n";
}

sub FileOpen
{
	$_[0]-> open_dockmanaging;
}

sub FileClose
{
	$_[0]-> close;
}

package Banner;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub on_create
{
	my $self = $_[0];
	$self-> {offset} = 0;
	$self-> text( "Visit www.prima.eu.org");
	$self-> font-> size( 18);
	$self-> {maxOffset} = $self-> width;
	$self-> {textLen} = $self-> get_text_width( $self-> text);
	$self-> insert( Timer => timeout => 100 => onTick => sub {
		$self-> {offset} = $self-> {maxOffset}
			if ( $self-> {offset} -= 5) < -$self-> {textLen};
		$self-> repaint;
	})-> start;
}

sub on_size
{
	my ( $self, $ox, $oy, $x, $y) = @_;
	$self-> {maxOffset} = $x;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$canvas-> clear;
	my @sz = $self-> size;
	$canvas-> text_out( $self-> text,
		$self-> {offset}, ( $sz[1] - $canvas-> font-> height) / 2);
}

package X;

# createing the docking instance with predefined command state
my $i = Prima::DockManager-> new(
	commands  => {
		'Edit::OK' => 0,
		'Edit::Cancel' => 0,
	},
);

# registering buttons
sub reg
{
	my ( $id, $name, $hint, %profile) = @_;
	$i-> register_tool( Prima::DockManager::S::SpeedButton::class( "sysimage.gif:$id",
		$name, hint => $hint, %profile));
}

reg( sbmp::SFolderOpened, 'File::Open',  'Rearrange buttons');
reg( sbmp::SFolderClosed, 'File::Close', 'Close document');
reg( sbmp::GlyphOK,       'Edit::OK',    'OK', glyphs => 2);
reg( sbmp::GlyphCancel,   'Edit::Cancel','Cancel', glyphs => 2);
reg( sbmp::DriveFloppy,   'Drive::Floppy', 'Floppy disk');
reg( sbmp::DriveHDD,      'Drive::HDD'   , 'Hard disk');
reg( sbmp::DriveNetwork,  'Drive::Network','Network connection');
reg( sbmp::DriveCDROM,    'Drive::CDROM',  'CD-ROM device');
reg( sbmp::DriveMemory,   'Drive::Memory', 'Memory-mapped drive');
reg( sbmp::DriveUnknown,  'Drive::Unknown','FAT-64');

# registering panels
$i-> register_panel( 'Edit' => {
	class => 'Prima::Edit',
	text  => 'Edit window',
	dockerProfile => {
		fingerprint => dmfp::Edit,
		growMode    => gm::Client,
	},
	profile => {
		vScroll => 1,
		text    => '',
	},
});
$i-> register_panel( 'Banner' => {
	class => 'Banner',
	text  => 'Banner window',
	dockerProfile => {
		fingerprint => dmfp::Horizontal,
		size => [ 200, 30]
	},
});


my $resFile = Prima::Utils::path('demo_dock');

# after all that, creating window ( the window itself is of small importance...)

my $ww = Prima::Dock::BasicWindow -> new(
	instance => $i,
	onClose => sub {
		if ( open F, "> $resFile") {
			$_[0]-> init_write( *F);
			close F;
		} else {
			warn "Cannot open $resFile:$!\n";
		};
	},
	onDestroy => sub {
		$::application-> destroy;
	},
	size      => [ 400, 400],
	text       => 'Docking example',
	onActivate    => sub { $i-> activate; },
	onWindowState => sub { $i-> windowState( $_[1]); },
);


# opening predefined bars
if ( open F, $resFile) {
	$ww-> init_read(*F);
	close F;
} else {
	$i-> predefined_panels( "Edit" => $ww-> {mainDock}-> ClientDocker);
}

$i-> predefined_toolbars( {
	name => "File",
	list => ["File::Open", "File::Close"],
	dock => $ww-> {mainDock}-> TopDocker,
	origin => [ 0, 0],
}, {
	name => "Edit",
	list => [ "Edit::OK", "Edit::Cancel", ],
	dock => $ww-> {mainDock}-> TopDocker,
	origin => [ 0, 0],
});

#$ww-> open_dockmanaging; # uncomment this for Customize window popup immediately

run Prima;

1;
