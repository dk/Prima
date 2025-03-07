use strict;
use warnings;
use Prima qw(Notebooks MsgBox ComboBox
Dialog::FontDialog Dialog::ColorDialog Dialog::FileDialog Dialog::ImageDialog
IniFile Utils Widget::RubberBand KeySelector Utils sys::GUIException);
use Prima::VB::VBLoader;
use Prima::VB::VBControls;
use Prima::VB::CfgMaint;

######### built-in setup variables  ###############

my $lite = 0;                          # set to 1 to use ::Lite packages. For debug only
$Prima::VB::CfgMaint::systemWide = 0;  # 0 - user config, 1 - root config to write
my $singleConfig                 = 0;  # set 1 to use only either user or root config
my $VBVersion                    = 0.2;

###################################################

my $classes = 'Prima::VB::Classes';
if ( $lite) {
	$classes = 'Prima::VB::Lite::Classes';
	$Prima::VB::CfgMaint::systemWide = 1;
	$Prima::VB::CfgMaint::rootCfg = 'Prima/VB/Lite/Config.pm';
}

eval "use $classes;";
die "$@" if $@;
use Prima::Application name => 'Form template builder';

package VB;
use strict;
use vars qw($inspector
	$main
	$editor
	$code
	$font_dialog
	$color_dialog
	$form
	$fastLoad
	$writeMode
	$dpi
);

$fastLoad = 1;
$writeMode = 0;
$dpi = 0;

my $openFileDlg;
my $saveFileDlg;
my $openImageDlg;
my $saveImageDlg;
my $fontDlg;


sub open_dialog
{
	my %profile = @_;
	$openFileDlg = Prima::Dialog::OpenDialog-> create(
		icon => $VB::ico,
		directory => $VB::main-> {ini}-> {OpenPath},
		system => 1,
	) unless $openFileDlg;
	$openFileDlg-> set( %profile);
	return $openFileDlg;
}

sub save_dialog
{
	my %profile = @_;
	$saveFileDlg = Prima::Dialog::SaveDialog-> create(
		icon => $VB::ico,
		directory => $VB::main-> {ini}-> {SavePath},
		system => 1,
	) unless $saveFileDlg;
	$saveFileDlg-> set( %profile);
	return $saveFileDlg;
}

sub image_open_dialog
{
	my %profile = @_;
	$openImageDlg = Prima::Dialog::ImageOpenDialog-> create(
		icon => $VB::ico,
		directory => $VB::main-> {ini}-> {OpenPath},
	) unless $openImageDlg;
	$openImageDlg-> set( %profile);
	return $openImageDlg;
}

sub image_save_dialog
{
	my %profile = @_;
	$saveImageDlg = Prima::Dialog::ImageSaveDialog-> create(
		icon => $VB::ico,
		directory => $VB::main-> {ini}-> {SavePath},
	) unless $saveImageDlg;
	$saveImageDlg-> set( %profile);
	return $saveImageDlg;
}

sub font_dialog
{
	my %profile = @_;
	$fontDlg = Prima::Dialog::FontDialog-> create(
		icon => $VB::ico,
		name => 'Select font',
	) unless $fontDlg;
	$fontDlg-> set( %profile);
	return $fontDlg;
}

sub accelItems
{
	return [
		['openitem' => '~Open' => 'Ctrl+O' => '^O' => sub { $VB::main-> open_file;}],
		['-saveitem1' => '~Save' => 'Ctrl+S' => '^S' => sub {$VB::main-> save;}],
		['Exit' => 'Ctrl+Q' => '^Q' => sub{ $VB::main-> close;}],
		['Object Inspector' => 'F11' => 'F11' => sub { $VB::main-> bring_inspector; }],
		['Code Editor' => 'F12' => 'F12' => sub { $VB::main-> bring_code_editor; }],
		['-runitem' => '~Run' => 'Ctrl+F9' => '^F9' => sub { $VB::main-> form_run}, ],
		['~Help' => 'F1' => 'F1' => sub { $::application-> open_help('VB/Help')}],
		['~Widget property' => 'Shift+F1' => '#F1' => sub { Prima::VB::ObjectInspector::help_lookup() }],
	];
}

package Prima::VB::OPropListViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::VB::PropListViewer);

sub on_click
{
	my $self = $_[0];
	my $index = $self-> focusedItem;
	my $current = $VB::inspector-> {current};
	return if $index < 0;
	my $id = $self-> {'id'}-> [$index];
	return if $id eq 'name' || $id eq 'owner';
	$self-> SUPER::on_click;
	for ( $current, grep { $current != $_ } $VB::form-> marked_widgets) {
		if ( $self-> {check}-> [$index]) {
			$_-> prf_set( $id => $_-> {default}-> {$id});
		} else {
			$_-> prf_delete( $id);
		}
	}
}

sub on_selectitem
{
	my ( $self, $lst, $state) = @_;
	return unless $state;
	$VB::inspector-> close_item;
	$VB::inspector-> open_item;
}


package Prima::VB::ObjectInspector;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text           => 'Object Inspector',
		width          => 280 * $::application-> uiScaling,
		height         => 350 * $::application-> uiScaling,
		left           => 6,
		sizeDontCare   => 0,
		originDontCare => 0,
		icon           => $VB::ico,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	$VB::main-> init_position( $self, 'ObjectInspectorRect');

	my @sz = $self-> size;

	my $fh = $self-> font-> height + 6;

	$self-> insert( ComboBox =>
		origin   => [ 0, $sz[1] - $fh],
		size     => [ $sz[0], $fh],
		growMode => gm::Ceiling,
		style    => cs::DropDown,
		name     => 'Selector',
		items    => [''],
		delegations => [qw(Change)],
	);

	my $step = $self-> font-> width * 25;
	$self-> {monger} = $self-> insert( Notebook =>
		origin  => [ 0, $fh],
		size    => [ $step,  $sz[1] - $fh * 2],
		growMode => gm::Client,
		pageCount => 2,
	);

	$self-> {mtabs} = $self-> insert( Button =>
		origin   => [ 0, 0],
		size     => [ $step, $fh],
		text     => '~Events',
		growMode => gm::Floor,
		name     => 'MTabs',
		delegations => [qw(Click)],
	);
	$self-> {mtabs}-> {mode} = 0;

	$self-> {plist} = $self-> {monger}-> insert_to_page( 0, 'Prima::VB::OPropListViewer' =>
		origin   => [ 0, 0],
		size     => [ $step, $sz[1] - $fh * 2],
		name       => 'PList',
		growMode   => gm::Client,
	);

	$self-> {elist} = $self-> {monger}-> insert_to_page( 1, 'Prima::VB::OPropListViewer' =>
		origin   => [ 0, 0],
		size     => [ $step, $sz[1] - $fh * 2],
		name       => 'EList',
		growMode   => gm::Client,
	);
	$self-> {currentList} = $self-> {'plist'};

	$self-> insert( 'Prima::VB::Divider' =>
		vertical => 1,
		origin => [ $step, 0],
		size   => [ 6, $sz[1] - $fh],
		min    => 50,
		max    => 50,
		name   => 'Div',
		delegations => [qw(Change)],
	);

	$self-> {panel} = $self-> insert( Notebook =>
		origin    => [ $step + 6, 0],
		size      => [ $sz[0] - $step - 6, $sz[1] - $fh],
		growMode  => gm::Right,
		name      => 'Panel',
		pageCount => 1,
	);
	$self-> {panel}-> {pages} = {};

	$self-> {current} = undef;
	return %profile;
}

sub on_size
{
	my ( $self, $ox, $oy, $x, $y) = @_;
	return if $x == $ox;
	my $d = $self-> bring('Div');
	return unless $d;
	$self-> {div_ratio} = $d-> left / ( $x || 1 ) unless $self-> {div_ratio};
	$d-> left( $x * $self-> {div_ratio});
	$self-> {lock_div_ratio} = 1;
	$self-> Div_Change( $d);
	delete $self-> {lock_div_ratio};
}

sub Div_Change
{
	my $self = $_[0];
	my ( $left, $right) = ( $self-> Div-> left, $self-> Div-> right);
	$self-> {monger}-> width( $left);
	$self-> {mtabs}-> width( $left);
	$self-> Panel-> set(
		width => $self-> width - $right,
		left  => $right,
	);
	$self-> {div_ratio} = $left / ( $self-> width || 1 )
		unless $self-> {lock_div_ratio};
}

sub set_monger_index
{
	my ( $self, $ix) = @_;
	my $mtabs = $self-> {mtabs};
	return if $ix == $mtabs-> {mode};
	$mtabs-> {mode} = $ix;
	$mtabs-> text( $ix ? '~Properties' : '~Events');
	$self-> {monger}-> pageIndex( $ix);
	$self-> {currentList} = $self-> { $ix ? 'elist' : 'plist' };
	$self-> close_item;
	$self-> open_item;
}

sub MTabs_Click
{
	my ( $self, $mtabs) = @_;
	$self-> set_monger_index( $mtabs-> {mode} ? 0 : 1);
}

sub Selector_Change
{
	my $self = $_[0];
	return if $self-> {selectorChanging};
	return unless $VB::form;
	my $t = $self-> Selector-> text;
	return unless length( $t);
	$self-> {selectorRetrieving} = 1;
	my $enter;
	if ( $t eq $VB::form-> text) {
		$VB::form-> focus;
		$enter = $VB::form;
	} else {
		my @f = $VB::form-> widgets;
		for ( @f) {
			$enter = $_, $_-> show, $_-> focus, last
				if $t eq $_-> name;
		}
	}
	if ( $enter) {
		$enter-> marked(1,1);
		Prima::VB::ObjectInspector::enter_widget( $enter);
	}
	$self-> {selectorRetrieving} = 0;
}


sub item_changed
{
	my  $self = $VB::inspector;
	return unless $self;
	return unless $self-> {opened};
	return if $self-> {sync};
	if ( $self-> {opened}-> valid) {
		if ( $self-> {opened}-> can( 'get')) {
			$self-> {sync} = 1;
			my $data = $self-> {opened}-> get;
			$self-> {opened}-> {widget}-> prf_set( $self-> {opened}-> {id} => $data);
			my $list = $self-> {currentList};
			my $ix = $list-> {index}-> {$self-> {opened}-> {id}};
			unless ( $list-> {check}-> [$ix]) {
				$list-> {check}-> [$ix] = 1;
				$list-> redraw_items( $ix);
			}

			my @w = $VB::form-> marked_widgets;
			for my $w ( @w) {
				next if $w == $self-> {current};
				$w-> prf_set($self-> {opened}-> {id} => $data);
			}
			$self-> {sync} = undef;
		}
	}
}

sub widget_changed
{
	my ( $how, $id) = @_;
	my $self = $VB::inspector;
	return unless $self;
	return if $self-> {currentList} != $self-> {plist};
	if ( $self-> {opened}) {
		if ( $id eq $self-> {opened}-> {id}) {
			return if $self-> {sync};
			$self-> {sync} = 1;
			my $data = $self-> {opened}-> {widget}-> prf( $id);
			$self-> {opened}-> set( $data);
			$self-> {sync} = undef;
		}
	}
	my $list = $self-> {currentList};
	my $ix = $list-> {index}-> {$id};
	if ( $list-> {check}-> [$ix] == $how) {
		$list-> {check}-> [$ix] = $how ? 0 : 1;
		$list-> redraw_items( $ix);
	}
}

sub close_item
{
	my $self = $_[0];
	return unless defined $self-> {opened};
	$self-> {lastOpenedId} = $self-> {opened}-> {id};
	$self-> {opened} = undef;
}

sub open_item
{
	my $self = $_[0];
	return if defined $self-> {opened};
	my $list = $self-> {currentList};
	my $f = $list-> focusedItem;

	if ( $f < 0) {
		$self-> {panel}-> pageIndex(0);
		return;
	}
	my $id   = $list-> {id}-> [$f];
	my $type = $VB::main-> get_typerec( $self-> {current}-> {types}-> {$id});
	my $p = $self-> {panel};
	my $pageset;
	if ( exists $p-> {pages}-> {$type}) {
		$self-> {opened} = $self-> {typeCache}-> {$type};
		$pageset = $p-> {pages}-> {$type};
		$self-> {opened}-> renew( $id, $self-> {current});
	} else {
		$p-> pageCount( $p-> pageCount + 1);
		$p-> pageIndex( $p-> pageCount - 1);
		$p-> {pages}-> {$type} = $p-> pageIndex;
		$self-> {opened} = $type-> new( $p, $id, $self-> {current});
		$self-> {typeCache}-> {$type} = $self-> {opened};
	}
	my $data = $self-> {current}-> prf( $id);
	$self-> {sync} = 1;
	$self-> {opened}-> set( $data);
	$self-> {sync} = undef;
	$p-> pageIndex( $pageset) if defined $pageset;
	$self-> {lastOpenedId} = undef;
}

sub enter_widget
{
	return unless $VB::inspector;
	my ( $self, $w) = ( $VB::inspector, $_[0]);
	if ( defined $w) {
		return if defined $self-> {current} and $self-> {current} == $w;
	} else {
		return unless defined $self-> {current};
	}
	my $oid = $self-> {opened}-> {id} if $self-> {opened};
	$oid = $self-> {lastOpenedId} unless defined $oid;
	$self-> {current} = $w;

	if ( $self-> {current}) {
		$self-> close_item;
		my %df = %{$_[0]-> {default}};
		my $pf = $_[0]-> {profile};
		my @ef = sort keys %{$self-> {current}-> {events}};
		my $ep = $self-> {elist};
		my $num = 0;
		my @check = ();
		my %ix = ();
		for ( @ef) {
			push( @check, exists $pf-> {$_} ? 1 : 0);
			$ix{$_} = $num++;
			delete $df{$_};
		}
		$ep-> reset_items( [@ef], [@check], {%ix});
		$ep-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

		my $lp = $self-> {plist};
		@ef = sort keys %df;
		%ix = ();
		@check = ();
		$num = 0;
		for ( @ef) {
			push( @check, exists $pf-> {$_} ? 1 : 0);
			$ix{$_} = $num++;
		}
		$lp-> reset_items( [@ef], [@check], {%ix});
		$lp-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

		$self-> Selector-> text( $self-> {current}-> name)
			unless $self-> {selectorRetrieving};
		$self-> open_item;
	} else {
		$self-> {panel}-> pageIndex(0);
		for ( qw( plist elist)) {
			my $p = $self-> {$_};
			$p-> {check} = [];
			$p-> {index} = {};
			$p-> set_count(0);
		}
		$self-> Selector-> text( '');
	}
}

sub update_markings
{
	return unless $VB::inspector;
	my @w = $VB::form-> marked_widgets;
	return enter_widget( $VB::form) if 0 == @w;
	return enter_widget( $w[0])     if 1 == @w;

	my $self = $VB::inspector;
	$self-> close_item;
	my $n1 = $self-> {current} = shift @w;
	my %el = %{$n1-> {events}};
	my %pl = %{$n1-> {default}};
	delete @pl{ keys %el };
	delete @pl{ qw(name) }; # won't set these properties
	for my $w ( @w) {
		delete @el{ grep { not exists $w->{events}->{$_} }  keys %el};
		delete @pl{ grep { not exists $w->{default}->{$_} } keys %pl};
	}

	my $oid = $self-> {lastOpenedId};

	my @ef = sort keys %el;
	my $ep = $self-> {elist};
	my $num = 0;
	my @check = ();
	my %ix = ();
	for my $e ( @ef) {
		push( @check, ( grep { $_-> {profile}->{$e} } $n1, @w ) ? 1 : 0);
		$ix{$e} = $num++;
	}
	$ep-> reset_items( [@ef], [@check], {%ix});
	$ep-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

	my $lp = $self-> {plist};
	@ef = sort keys %pl;
	%ix = ();
	@check = ();
	$num = 0;
	for my $e ( @ef) {
		push( @check, ( grep { $_-> {profile}->{$e} } $n1, @w ) ? 1 : 0);
		$ix{$e} = $num++;
	}
	$lp-> reset_items( [@ef], [@check], {%ix});
	$lp-> focusedItem( $ix{$oid}) if defined $oid and defined $ix{$oid};

	$self-> Selector-> text( join(',', map { $_-> name } $n1, @w));
	$self-> open_item;
}

sub renew_widgets
{
	return unless $VB::inspector;
	return if $VB::inspector-> {selectorChanging};
	$VB::inspector-> {selectorChanging} = 1;
	$VB::inspector-> close_item;
	if ( $VB::form) {
		my @f = ( $VB::form, $VB::form-> widgets);
		$_ = $_-> name for @f;
		@f = sort @f;
		$VB::inspector-> Selector-> items( \@f);
		my $fx = $VB::form-> focused ? $VB::form : $VB::form-> selectedWidget;
		$fx = $VB::form unless $fx;
		enter_widget( $fx);
	} else {
		$VB::inspector-> Selector-> items( ['']);
		$VB::inspector-> Selector-> text( '');
		enter_widget(undef);
	}
	$VB::inspector-> {selectorChanging} = undef;
}

sub preload
{
	my $self = $VB::inspector;
	my $l = $self-> {plist};
	my $cnt = $l-> count;
	$self-> {panel}-> hide;
	$l-> hide;
	$l-> focusedItem( $cnt) while $cnt-- > 0;
	$l-> show;
	$self-> {panel}-> show;
}

sub on_destroy
{
	$VB::inspector = undef;
	$VB::main-> {ini}-> {ObjectInspectorRect} = join( ' ', $_[0]-> rect);
}

sub help_lookup
{
	return unless $VB::inspector;
	my $self = $VB::inspector;
	my $list = $self-> {currentList};
	my $f    = $list-> focusedItem;
	return if $f < 0;
	my $event = $self-> {mtabs}-> {mode};
	my $id    = $list-> {id}-> [$f];
	$id =~ s/^on// if $event;
	$::application-> open_help("Prima::Widget/$id");
}


package Prima::VB::Form;
use strict;
use vars qw(@ISA);
@ISA = qw( Prima::Window Prima::VB::Window);

{
my %RNT = (
	%{Prima::Window-> notification_types()},
	Load => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		sizeDontCare   => 0,
		originDontCare => 0,
		width          => 300,
		height         => 200,
		centered       => 1,
		class          => 'Prima::Window',
		module         => 'Prima::Classes',
		selectable     => 1,
		mainEvent      => 'onMouseClick',
		popupItems     => $VB::main-> menu-> get_items( 'edit'),
		ownerIcon      => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$profile{profile}-> {name} = $self-> name;
	for ( qw( marked sizeable)) {
		$self-> {$_}=0;
	};
	$self-> {dragMode} = 3;
	$self-> init_profiler( \%profile);
	$self-> {guidelineX} = $self-> width  / 2;
	$self-> {guidelineY} = $self-> height / 2;
	unless ( $self-> {syncRecting}) {
		$self-> {syncRecting} = $self;
		$self-> prf_set(
			origin => [$self-> origin],
			size   => [$self-> size],
			originDontCare => 0,
			sizeDontCare   => 0,
		);
		$self-> {syncRecting} = undef;
	}
	return %profile;
}

sub set_default_props
{
	my $self = shift;
	$self->prf_set( designScale => [ $self->font->width, $self->font->height ] );
}

sub insert_new_control
{
	my ( $self, $x, $y, $where, %profile) = @_;
	my $class = exists($profile{class}) ? $profile{class} : $VB::main-> {currentClass};
	my $xclass = $VB::main-> {classes}-> {$class}-> {class};
	return unless defined $xclass;
	my $creatOrder = 0;
	my $wn = $where-> name;
	for ( $self-> widgets) {
		my $po = $_-> prf('owner');
		$po = $VB::form-> name unless length $po;
		next unless $po eq $wn;
		my $co = $_-> {creationOrder};
		$creatOrder++;
		next if $co < $creatOrder;
		$creatOrder = $co + 1;
	}
	my $j;
	my %prf = exists($profile{profile}) ? ( %{$profile{profile}}) : ();
	eval {
		eval "use $VB::main->{classes}->{$class}->{RTModule};";
		die "$@" if $@;
		$j = $self-> insert( $xclass,
			profile => {
				%prf,
				owner  => $wn,
				origin => [$x-4,$y-4],
			},
			class         => $class,
			module        => $VB::main-> {classes}-> {$class}-> {RTModule},
			creationOrder => $creatOrder,
		);
		$j-> {realClass} = $profile{realClass} if exists $profile{realClass};
	};
	$VB::main-> {currentClass} = undef unless exists $profile{class};
	if ( $@) {
		Prima::MsgBox::message( "Error:$@");
		return;
	}

	$self-> {modified} = 1;

	unless ( $profile{manualSelect}) {
		$j-> select;
		$j-> marked(1,1);
		Prima::VB::ObjectInspector::enter_widget( $j);
	}
	return $j;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$canvas-> backColor( $self-> backColor);
	$canvas-> color( cl::Blue);
	$canvas-> rop2(rop::CopyPut);
	$canvas-> fillPattern([0,0,0,0,4,0,0,0]);
	my @sz = $canvas-> size;
	$canvas-> bar(0,0,@sz);
	$canvas-> rop2(rop::NoOper);
	$canvas-> fillPattern( fp::Solid);
	$canvas-> linePattern( lp::Dash);
	$canvas-> line( $self-> {guidelineX}, 0, $self-> {guidelineX}, $sz[1]);
	$canvas-> line( 0, $self-> {guidelineY}, $sz[0], $self-> {guidelineY});
}

sub on_move
{
	my $self = shift;
	$self-> SUPER::on_move(@_);
	$self-> prf_set( originDontCare => 0);
}

sub on_size
{
	my $self = shift;
	$self-> SUPER::on_size(@_);
	$self-> prf_set( sizeDontCare => 0);
}

sub on_close
{
	my $self = $_[0];
	if ( $self-> {modified} || ( $VB::editor && $VB::editor-> {modified})) {
		my $name = defined ( $VB::main-> {fmName}) ? $VB::main-> {fmName} : 'Untitled';
		my $r = Prima::MsgBox::message(
			"Save changes to $name?",
			mb::YesNoCancel|mb::Warning
		);
		if ( $r == mb::Yes) {
			$self-> clear_event, return unless $VB::main-> save;
			$self-> {modified} = undef;
		} elsif ( $r == mb::Cancel) {
			$self-> clear_event;
		} else {
			$self-> {modified} = undef;
		}
	}
}

sub on_destroy
{
	if ( $VB::form && ( $VB::form == $_[0])) {
		$VB::form = undef;
		if ( defined $VB::main) {
			$VB::main-> {fmName} = undef;
			$VB::main-> update_menu();
			$VB::main-> update_markings();
		}
	}
	Prima::VB::CodeEditor::flush;
	Prima::VB::ObjectInspector::renew_widgets;
}

sub veil
{
	my ($self, $draw) = @_;
	my @r = ( @{$self-> {anchor}}, @{$self-> {dim}});
	( $r[0], $r[2]) = ( $r[2], $r[0]) if $r[2] < $r[0];
	( $r[1], $r[3]) = ( $r[3], $r[1]) if $r[3] < $r[1];
	@r = $self-> client_to_screen( @r);
	$::application-> rubberband(
		clipRect => [ $self-> client_to_screen( 0,0,$self->size ) ],
		$draw ?
			( rect => \@r ) :
			( destroy => 1 )
	);
}


sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {transaction};
	if ( $btn == mb::Left) {
		unless ( $self-> {transaction}) {
			if ( abs( $self-> {guidelineX} - $x) < 3) {
				$self-> capture(1, $self);
				$self-> {saveHdr} = $self-> text;
				$self-> {transaction} = ( abs( $self-> {guidelineY} - $y) < 3) ? 4 : 2;
				return;
			} elsif ( abs( $self-> {guidelineY} - $y) < 3) {
				$self-> {transaction} = 3;
				$self-> capture(1, $self);
				$self-> {saveHdr} = $self-> text;
				return;
			}
		}

		if ( defined $VB::main-> {currentClass}) {
			$self-> insert_new_control( $x, $y, $self);
		} else {
			$self-> focus;
			$self-> marked(0,1);
			$self-> update_view;
			$self-> {transaction} = 1;
			$self-> capture(1);
			$self-> {anchor} = [ $x, $y];
			$self-> {dim}    = [ $x, $y];
			$self-> veil(1);
			Prima::VB::ObjectInspector::enter_widget( $self);
		}
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	unless ( $self-> {transaction}) {
		if ( abs( $self-> {guidelineX} - $x) < 3) {
			$self-> pointer(( abs( $self-> {guidelineY} - $y) < 3) ?
				cr::Move :
				cr::SizeWE);
		} elsif ( abs( $self-> {guidelineY} - $y) < 3) {
			$self-> pointer( cr::SizeNS);
		} else {
			$self-> pointer( cr::Arrow);
		}
		return;
	}
	if ( $self-> {transaction} == 1) {
		return if $self-> {dim}-> [0] == $x && $self-> {dim}-> [1] == $y;
		$self-> {dim} = [ $x, $y];
		$self-> veil(1);
		return;
	}

	if ( $VB::main-> {ini}-> {SnapToGrid}) {
		$x -= $x % 4;
		$y -= $y % 4;
	}

	if ( $self-> {transaction} == 2) {
		$self-> {guidelineX} = $x;
		$self-> text( $x);
		$_-> repaint for grep { $_-> {locked} } $self-> widgets;
		$self-> repaint;
		return;
	}

	if ( $self-> {transaction} == 3) {
		$self-> {guidelineY} = $y;
		$self-> text( $y);
		$_-> repaint for grep { $_-> {locked} } $self-> widgets;
		$self-> repaint;
		return;
	}

	if ( $self-> {transaction} == 4) {
		$self-> {guidelineY} = $y;
		$self-> {guidelineX} = $x;
		$self-> text( "$x:$y");
		$_-> repaint for grep { $_-> {locked} } $self-> widgets;
		$self-> repaint;
		return;
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {transaction};
	return unless $btn == mb::Left;

	$self-> capture(0);
	if ( $self-> {transaction} == 1) {
		$self-> veil(0);
		my @r = ( @{$self-> {anchor}}, @{$self-> {dim}});
		( $r[0], $r[2]) = ( $r[2], $r[0]) if $r[2] < $r[0];
		( $r[1], $r[3]) = ( $r[3], $r[1]) if $r[3] < $r[1];
		$self-> {transaction} = $self-> {anchor} = $self-> {dim} = undef;

		for ( $self-> widgets) {
			my @x = $_-> rect;
			next if $x[0] < $r[0] || $x[1] < $r[1] || $x[2] > $r[2] || $x[3] > $r[3];
			next if $_-> {locked};
			$_-> marked(1);
		}
		Prima::VB::ObjectInspector::update_markings();
		return;
	}

	if ( $self-> {transaction} > 1 && $self-> {transaction} < 5) {
		$self-> {transaction} = undef;
		$self-> text( $self-> {saveHdr});
		return;
	}
}

sub on_leave
{
	$_[0]-> notify(q(MouseUp), mb::Left, 0, 0, 0) if $_[0]-> {transaction};
}

sub dragMode
{
	if ( $#_) {
		my ( $self, $dm) = @_;
		return if $dm == $self-> {dragMode};
		$dm = 1 if $dm > 3 || $dm < 1;
		$self-> {dragMode} = $dm;
	} else {
		return $_[0]-> {dragMode};
	}
}

sub dm_init
{
	my ( $self, $widget) = @_;
	my $cr;
	if ( $self-> {dragMode} == 1) {
		$cr = cr::SizeWE;
	} elsif ( $self-> {dragMode} == 2) {
		$cr = cr::SizeNS;
	} else {
		$cr = cr::Move;
	}
	$widget-> pointer( $cr);
}

sub dm_next
{
	my ( $self, $widget) = @_;
	$self-> dragMode( $self-> dragMode + 1);
	$self-> dm_init( $widget);
}

sub fm_resetguidelines
{
	my $self = $VB::form;
	return unless $self;
	$self-> {guidelineX} = $self-> width  / 2;
	$self-> {guidelineY} = $self-> height / 2;
	$self-> repaint;
}

sub fm_reclass
{
	my $self = $VB::form;
	return unless $self;
	my @wijs = $VB::form-> marked_widgets;
	$self = $wijs[0] if @wijs;
	my $lab_text = 'Class name';
	$lab_text =  "Temporary class for ".$self-> {realClass} if defined $self-> {realClass};
	my $dlg = Prima::Dialog-> create(
		icon => $VB::ico,
		size => [ 429, 55],
		centered => 1,
		text => 'Change class',
	);
	my ( $i, $b, $l) = $dlg-> insert([ InputLine =>
		origin => [ 5, 5],
		size => [ 317, 20],
		text => $self-> {class},
	], [ Button =>
		origin => [ 328, 4],
		size => [ 96, 36],
		text => '~Change',
		onClick => sub { $dlg-> ok },
		default => 1,
	], [ Label =>
		origin => [ 5, 28],
		autoWidth => 0,
		size => [ 317, 20],
		text => $lab_text,
	]);
	if ( $dlg-> execute == mb::OK) {
		$self-> {class} = $i-> text;
		delete $self-> {realClass};
	}
	$dlg->  destroy;
}

sub marked_widgets
{
	my $self = $_[0];
	my @ret = ();
	for ( $self-> widgets) {
		push ( @ret, $_) if $_-> marked;
	}
	return @ret;
}

sub fm_subalign
{
	my $self = $VB::form;
	return unless $self;
	my ( $forward) = @_;
	my @marked_widgets = $VB::form-> marked_widgets;
	return unless scalar @marked_widgets;
	my @sorted_widgets = sort { $a-> {creationOrder} <=> $b-> {creationOrder}}
		$VB::form-> widgets;
	my %marked = map { $_-> {creationOrder}  => 1 } @marked_widgets;
	return if @sorted_widgets == scalar keys %marked;
	@marked_widgets = grep { exists $marked{$_-> {creationOrder}}} @sorted_widgets;
	my @new_indexes;
	push @new_indexes, grep { ! exists $marked{$_}} (0..$#sorted_widgets) if $forward;
	push @new_indexes, grep {   exists $marked{$_}} (0..$#sorted_widgets);
	push @new_indexes, grep { ! exists $marked{$_}} (0..$#sorted_widgets) unless $forward;
	for ( my $i = 0; $i < @sorted_widgets; $i++) {
		$sorted_widgets[$new_indexes[$i]]-> {creationOrder} = $i;
		$sorted_widgets[$new_indexes[$i]]-> bring_to_front;
	}
}

sub fm_stepalign
{
	my $self = $VB::form;
	return unless $self;
	my ($forward) = @_;
	my @marked_widgets = $VB::form-> marked_widgets;
	return unless scalar @marked_widgets;
	my @sorted_widgets = sort { $a-> {creationOrder} <=> $b-> {creationOrder}}
		$VB::form-> widgets;
	my %marked = map { $_-> {creationOrder}  => 1 } @marked_widgets;
	my @marked_indexes = grep { exists $marked{$_}} (0..$#sorted_widgets);
	@marked_widgets = grep { exists $marked{$_-> {creationOrder}}} @sorted_widgets;
	return if @marked_indexes == @sorted_widgets;
	my @new_indexes;
	my $anchor;
	if ( $forward) {
		push @new_indexes, grep { ! exists $marked{$_}} (0..$marked_indexes[-1]);
		push @new_indexes, ($marked_indexes[-1]+1)
			if $marked_indexes[-1] < $#sorted_widgets;
		push @new_indexes, @marked_indexes;
		if ( $marked_indexes[-1] < $#sorted_widgets - 1) {
			push @new_indexes, ($marked_indexes[-1]+2..$#sorted_widgets);
		}
	} else {
		push @new_indexes, (0..$marked_indexes[0]-2)
			if $marked_indexes[0] > 1;
		push @new_indexes, @marked_indexes;
		my $anchor = @new_indexes;
		push @new_indexes, ($marked_indexes[0]-1)
			if $marked_indexes[0] > 0;
		push @new_indexes, grep { ! exists $marked{$_}} ($marked_indexes[0]..$#sorted_widgets);
	}
	for ( my $i = 0; $i < @sorted_widgets; $i++) {
		$sorted_widgets[$new_indexes[$i]]-> {creationOrder} = $i;
		$sorted_widgets[$new_indexes[$i]]-> bring_to_front;
	}
}

sub fm_realign
{
	my $self = $VB::form;
	return unless $self;
	$_-> bring_to_front for
		sort { $a-> {creationOrder} <=> $b-> {creationOrder}}
		$VB::form-> widgets;
}

sub fm_duplicate
{
	my $self = $VB::form;
	return unless $self;
	my @r = ();
	my %wjs  = map { $_-> prf('name') => $_} ( $self, $self-> widgets);
	my @marked = $self-> marked_widgets;
	$self-> marked(0,1);

	for ( @marked) {
		my %prf = (
			class        => $_-> {class},
			manualSelect => 1,
			profile      => $_-> {profile},
		);
		$prf{realClass} = $_-> {realClass} if defined $_-> {realClass};
		my $j = $self-> insert_new_control( $_-> left + 14, $_-> bottom + 14,
			$wjs{$_-> prf('owner')}, %prf);
		next unless $j;
		$j-> focus unless scalar @r;
		push ( @r, $j);
		$j-> marked(1,0);
	}
	Prima::VB::ObjectInspector::update_markings();
}

sub fm_selectall
{
	return unless $VB::form;
	$_-> marked(1) for $VB::form-> widgets;
	Prima::VB::ObjectInspector::update_markings();
}

sub fm_delete
{
	return unless $VB::form;
	$_-> destroy for $VB::form-> marked_widgets;
	Prima::VB::ObjectInspector::renew_widgets();
}

sub fm_copy
{
	return if !$VB::form || ! scalar $VB::form-> marked_widgets;
	my $c = $VB::main-> write_form(1);
	$::application-> Clipboard-> text( $c);
}

sub fm_paste
{
	return unless $VB::form;
	my @seq = $VB::main-> inspect_load_data( $::application-> Clipboard-> text, 0);
	return unless @seq;

	$VB::main-> wait;
	my $i;
	my %names  = map { $_-> prf('name') => 1 } ( $VB::form, $VB::form-> widgets);
	my $main   = $VB::form-> prf('name');
	my %keymap;
	my $classes = $VB::main-> {classes};

	for ( $i = 0; $i < scalar @seq; $i+= 2) {
		my ( $key, $hash) = ( $seq[$i], $seq[$i + 1]);

		# handling name clashes
		my $j = 0;
		$key = $seq[$i] . "_$j", $j++ while exists $names{$key};
		$keymap{$seq[$i]} = $key;
		$hash-> {profile}-> {name} = $key;

		unless ( $classes-> {$hash-> {class}}) {
			$hash-> {realClass} = $hash-> {class};
			$hash-> {class}     = 'Prima::Widget';
		}

		my $wclass = $classes-> {$hash-> {class}}-> {class};
		my %handleTypes = map { $_ => 1} @{$wclass-> prf_types-> {Handle}};
		for ( keys %{$hash-> {profile}}) {
			next unless exists $handleTypes{ $_};
			my $mapv = $keymap{$hash-> {profile}-> {$_}};
			$mapv = $main unless defined $mapv;
			$hash-> {profile}-> {$_} = defined($mapv) ? $mapv : $main;
		}
		$seq[$i] = $key;
	}

	$VB::form-> marked(0,1);
	@seq = $VB::main-> push_widgets( @seq);
	Prima::VB::ObjectInspector::renew_widgets;
	$_-> notify(q(Load)) for @seq;
	$_-> marked( 1, 0) for @seq;
	Prima::VB::ObjectInspector::update_markings();
}


sub fm_creationorder
{
	my $self = $VB::form;
	return unless $self;

	my %cos;
	my @names = ();
	$cos{$_-> {creationOrder}} = $_-> name for $self-> widgets;
	push( @names, $cos{$_}) for sort {$a <=> $b} keys %cos;

	return unless scalar @names;

	my $d = Prima::Dialog-> create(
		icon => $VB::ico,
		origin => [ 358, 396],
		size => [ 243, 325],
		text => 'Creation order',
	);
	$d-> insert( [ Button =>
		origin => [ 5, 5],
		size => [ 96, 36],
		text => '~OK',
		default => 1,
		modalResult => mb::OK,
	], [ Button =>
		origin => [ 109, 5],
		size => [ 96, 36],
		text => 'Cancel',
		modalResult => mb::Cancel,
	], [ ListBox =>
		origin => [ 5, 48],
		name => 'Items',
		size => [ 199, 269],
		items => \@names,
		focusedItem => 0,
	], [ SpeedButton =>
		origin => [ 209, 188],
		image => Prima::Icon-> create( width=>13, height=>13, type => im::bpp1,
palette => [ 0,0,0,0,0,0],
data =>
"\xff\xf8\x00\x00\x7f\xf0\x00\x00\x7f\xf0\x00\x00\?\xe0\x00\x00\?\xe0\x00\x00".
"\x1f\xc0\x00\x00\x1f\xc0\x00\x00\x0f\x80\x00\x00\x0f\x80\x00\x00\x07\x00\x00\x00".
"\x07\x00\x00\x00\x02\x00\x00\x00\x02\x00\x00\x00".
''),
		size => [ 29, 130],
		flat    => 1,
		onClick => sub {
			my $i  = $d-> Items;
			my $fi = $i-> focusedItem;
			return if $fi < 1;
			my @i = @{$i-> items};
			@i[ $fi - 1, $fi] = @i[ $fi, $fi - 1];
			$i-> items( \@i);
			$i-> focusedItem( $fi - 1);
		},
	], [ SpeedButton =>
		origin => [ 209, 48],
		image => Prima::Icon-> create( width=>13, height=>13, type => im::bpp1,
palette => [ 0,0,0,0,0,0],
data =>
"\x02\x00\x00\x00\x02\x00\x00\x00\x07\x00\x00\x00\x07\x00\x00\x00\x0f\x80\x00\x00".
"\x0f\x80\x00\x00\x1f\xc0\x00\x00\x1f\xc0\x00\x00\?\xe0\x00\x00\?\xe0\x00\x00".
"\x7f\xf0\x00\x00\x7f\xf0\x00\x00\xff\xf8\x00\x00".
''),
		size => [ 29, 130],
		flat => 1,
		onClick => sub {
			my $i   = $d-> Items;
			my $fi  = $i-> focusedItem;
			my $c   = $i-> count;
			return if $fi >= $c - 1;
			my @i = @{$i-> items};
			@i[ $fi + 1, $fi] = @i[ $fi, $fi + 1];
			$i-> items( \@i);
			$i-> focusedItem( $fi + 1);
		},
	]);
	if ( $d-> execute != mb::Cancel) {
		my $cord = 1;
		$self-> bring( $_)-> {creationOrder} = $cord++ for @{$d-> Items-> items};
	}
	$d-> destroy;
}

sub fm_toggle_lock
{
	my $self = $VB::form;
	return unless $self;

	my @w      = $self-> marked_widgets;
	my $unlock = not grep { $_-> {locked} } @w;
	$_-> {locked} = $unlock for @w;
	$self-> marked(0,1) if $unlock;
	$_-> repaint for @w;
}

sub prf_icon
{
	$_[0]-> icon( $_[1]);
}

sub prf_menuItems
{
	local $_[0]-> {syncRecting};
	$_[0]-> {syncRecting} = 'height';
	$_[0]-> menuItems( $_[1]);
}

package Prima::VB::MainPanel;
use strict;
use vars qw(@ISA *do_layer);
@ISA = qw(Prima::Window);
use Prima::sys::FS;

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text           => $::application-> name,
		width          => $::application-> width - 12,
		left           => 6,
		bottom         => $::application-> height - $::application->uiScaling * 106 -
			$::application-> get_system_value(sv::YTitleBar) -
			$::application-> get_system_value(sv::YMenu),
		sizeDontCare   => 0,
		originDontCare => 0,
		height         => 100 * $::application->uiScaling,
		icon           => $VB::ico,
		menuItems      => [
			['~File' => [
				['newitem' => '~New' => 'Ctrl+N' => '^N' =>     sub {$_[0]-> new;}],
				['openitem' => '~Open' => 'Ctrl+O' => '^O' =>   sub {$_[0]-> open_file;}],
				['-saveitem1' => '~Save' => 'Ctrl+S' => '^S' => sub {$_[0]-> save;}],
				['-saveitem2' =>'Save ~as...' =>                sub {$_[0]-> saveas;}],
				['closeitem' =>'~Close' => 'Ctrl+W' => '^W' =>  sub { $VB::form-> close if $VB::form}],
				[],
				[exit => 'E~xit' => 'Ctrl+Q' => '^Q' => sub{$_[0]-> close;}],
			]],
			['edit' => '~Edit' => [
				[copy => 'Cop~y' => 'Ctrl+C' => '^C' =>       sub { Prima::VB::Form::fm_copy(); }],
				[paste => '~Paste' => 'Ctrl+V' => '^V' =>      sub { Prima::VB::Form::fm_paste(); }],
				[delete => '~Delete' => 'Ctrl-X' => '^X' =>     sub { Prima::VB::Form::fm_delete(); } ],
				[selectall => '~Select all' => 'Ctrl+A' => '^A' => sub { Prima::VB::Form::fm_selectall(); }],
				[dup => 'D~uplicate'  => 'Ctrl+D' => '^D' => sub { Prima::VB::Form::fm_duplicate(); }],
				[],
				['~Align' => [
					[align0 => '~Bring to front' => 'Shift+PgUp' => km::Shift|kb::PgUp, sub { Prima::VB::Form::fm_subalign(1);}],
					[align1 => '~Send to back'   => 'Shift+PgDn' => km::Shift|kb::PgDn, sub { Prima::VB::Form::fm_subalign(0);}],
					[align2 => 'Step ~forward'   => 'Ctrl+PgUp' => km::Ctrl|kb::PgUp, sub { Prima::VB::Form::fm_stepalign(1);}],
					[align3 => 'Step bac~k'      => 'Ctrl+PgDn' => km::Ctrl|kb::PgDn, sub { Prima::VB::Form::fm_stepalign(0);}],
					[align_restore => '~Restore order'   => 'Shift+Ctrl+PgDn' => km::Shift|km::Ctrl|kb::PgDn, sub { Prima::VB::Form::fm_realign;}],
				]],
				[setclass => '~Change class...' => sub { Prima::VB::Form::fm_reclass();}],
				[setorder => 'Creation ~order' => sub { Prima::VB::Form::fm_creationorder(); } ],
				[togglelock => 'To~ggle lock' => 'Ctrl+G' => '^G' => sub { Prima::VB::Form::fm_toggle_lock(); }],
				[shortcuts => 'Edit shortcuts...' => 'edit_menu' ],
			]],
			['~View' => [
				[objects => '~Object Inspector' => 'F11' => 'F11' => sub { $_[0]-> bring_inspector; }],
				[editor => '~Code editor'      => 'F12' => 'F12' => sub { $_[0]-> bring_code_editor; }],
				[colors => 'Co~lor dialog'  => q(bring_color_dialog) ],
				[fonts => '~Font dialog'  => q(bring_font_dialog) ],
				[widgets => '~Add widgets...' => q(add_widgets)],
				[],
				[resetlines => 'Reset ~guidelines' => sub { Prima::VB::Form::fm_resetguidelines(); } ],
				['@*gsnap' => 'Snap to guid~elines' => sub { $VB::main-> {ini}-> {SnapToGuidelines} = $_[2]; } ],
				['@*dsnap' => 'Snap to gri~d'       => sub { $VB::main-> {ini}-> {SnapToGrid}       = $_[2] } ],
				[],
				['-runitem' => '~Run' => 'Ctrl+F9' => '^F9' => \&form_run ],
				['-breakitem' => '~Break' => \&form_cancel ],
				['~Emulate resolution' => [
					["(*dpi0" => '~Native' => \&dpi_toggle ],
					['('],
					map {
						["dpi$_" => "$_ PPI", \&dpi_toggle ]
					} (48, 72, 96, 120, 144, 168, 192, 216, 240, 264, 288, 312)
				]],
			]],
			[],
			['~Help' => [
				['~About' => sub { Prima::MsgBox::message("Visual Builder for Prima toolkit, version $VBVersion")}],
				[help => '~Help' => 'F1' => 'F1' => sub { $::application-> open_help('VB/Help')}],
				[contexthelp => '~Widget property' => 'Shift+F1' => '#F1' => sub { Prima::VB::ObjectInspector::help_lookup() }],
			]],
		],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	my @r = ( $lite || $singleConfig) ?
		Prima::VB::CfgMaint::open_cfg :
		Prima::VB::CfgMaint::read_cfg;
	die "Error:$r[1]\n" unless $r[0];

	my %classes = %Prima::VB::CfgMaint::classes;
	my @pages   = @Prima::VB::CfgMaint::pages;

	my $s = $::application->uiScaling;
	$self-> set(
		sizeMin => [ $s*350, $self-> height],
		sizeMax => [ 16384, $self-> height],
	);

	my ( $dx, $dy ) = map { $s * $_ } (26,30);

	my @images = map {
		my $i = Prima::Icon->new;
		if ( my $f = Prima::Utils::find_image( 'VB::VB.gif')) {
			if ( $i->load($f, index => $_)) {
				$i-> ui_scale;
			}
		} else {
			$i = undef;
		}
		$i
	} 1..4;

	$self-> {newbutton} = $self-> insert( SpeedButton =>
		origin    => [ 4, $self-> height - $dy],
		size      => [ $dx, $dx],
		hint      => 'New',
		image     => $images[0],
		glyphs    => 2,
		flat      => 1,
		onClick   => sub { $VB::main-> new; } ,
	);

	$self-> {openbutton} = $self-> insert( SpeedButton =>
		origin    => [ 4 + $dx, $self-> height - $dy],
		size      => [ $dx, $dx],
		hint      => 'Open',
		image     => $images[1],
		glyphs    => 2,
		flat      => 1,
		onClick   => sub { $VB::main-> open_file; } ,
	);

	$self-> {savebutton} = $self-> insert( SpeedButton =>
		origin    => [ 4 + $dx*2, $self-> height - $dy],
		size      => [ $dx, $dx],
		hint      => 'Save',
		image     => $images[2],
		glyphs    => 2,
		flat      => 1,
		onClick   => sub { $VB::main-> save; } ,
	);

	$self-> {runbutton} = $self-> insert( SpeedButton =>
		origin    => [ 4 + $dx*3, $self-> height - $dy],
		size      => [ $dx, $dx],
		hint      => 'Run',
		image     => $images[3],
		glyphs    => 2,
		flat      => 1,
		onClick   => sub { $VB::main-> form_run} ,
	);

	my $ts_x = 8 + $dx * 5;
	$self-> {tabset} = $self-> insert( TabSet =>
		left   => $ts_x,
		name   => 'TabSet',
		top    => $self-> height,
		width  => $self-> width - $ts_x,
		growMode => gm::Ceiling,
		topMost  => 1,
		tabs     => [ @pages],
		buffered => 1,
		delegations => [qw(Change)],
	);

	$self-> {nb} = $self-> insert( Widget =>
		origin => [ $ts_x, 0],
		size   => [$self-> width - $ts_x, $self-> height - $self-> {tabset}-> height],
		growMode => gm::Client,
		name    => 'TabbedNotebook',
		onPaint => sub {
			my ( $self, $canvas) = @_;
			my @sz = $self-> size;
			$canvas-> rect3d(0,0,$sz[0]-1,$sz[1],1,
				$self-> light3DColor,$self-> dark3DColor,$self-> backColor);
		},
	);

	$self-> {nbpanel} = $self-> {nb}-> insert( Notebook =>
		origin     => [$s*12,1],
		size       => [$self-> {nb}-> width-$s*24,36*$s+8],
		growMode   => gm::Floor,
		name       => 'NBPanel',
	);

	$self-> {leftScroll} = $self-> {nb}-> insert( SpeedButton =>
		origin  => [1,5],
		size    => [map { $s * $_ } 11,36],
		name    => 'LeftScroll',
		autoRepeat => 1,
		flat    => 1,
		onPaint => sub {
			$_[0]-> on_paint( $_[1]);
			$_[1]-> color( $_[0]-> enabled ? cl::Black : cl::Gray);
			$_[1]-> fillpoly([map { $s * $_ } 7,4,7,32,3,17]);
		}, 
		delegations => [ $self, qw(Click)],
	);

	$self-> {rightScroll} = $self-> {nb}-> insert( SpeedButton =>
		origin  => [$self-> {nb}-> width-11*$s,5],
		size    => [map { $s * $_ } 11,36],
		name    => 'RightScroll',
		growMode => gm::Right,
		autoRepeat => 1,
		flat    => 1,
		onPaint => sub {
			$_[0]-> on_paint( $_[1]);
			$_[1]-> color( $_[0]-> enabled ? cl::Black : cl::Gray);
			$_[1]-> fillpoly([map { $s * $_ } 3,4,3,32,7,17]);
		},
		delegations => [ $self, qw(Click)],
	);

	$self-> {classes} = \%classes;
	$self-> {pages}   = \@pages;
	$self-> {gridAlert} = 5;
	my $font = $self-> font;

	$self-> {iniFile} = Prima::IniFile-> create(
		file    => Prima::Utils::path('VisualBuilder'),
		default => [
			'View' => [
				'SnapToGrid' => 1,
				'SnapToGuidelines' => 1,
				'ObjectInspectorVisible' => 1,
				'ObjectInspectorRect' => '-1 -1 -1 -1',
				'CodeEditorVisible' => 0,
				'CodeEditorRect' => '-1 -1 -1 -1',
				'MainPanelRect' => '-1 -1 -1 -1',
				'OpenPath' => '.',
				'SavePath' => '.',
			],
			'Editor' => [
				'syntaxHilite'    => 1,
				'autoIndent'      => 1,
				'persistentBlock' => 0,
				'blockType'       => 0,
				'FontName'        => $font-> name,
				'FontSize'        => $font-> size,
				'FontStyle'       => $font-> style,
				'FontEncoding'    => $font-> encoding,
			],
			'Shortcuts' => [],
		],
	);
	my $i = $self-> {ini} = $self-> {iniFile}-> section( 'View' );
	$self-> menu-> dsnap-> checked( $i-> {SnapToGrid});
	$self-> menu-> gsnap-> checked( $i-> {SnapToGuidelines});
	$self-> init_position( $self, 'MainPanelRect');
	$self-> menu-> keys_load( $self->{iniFile}->section('Shortcuts'));
	return %profile;
}

sub on_create
{
	$_[0]-> reset_tabs;
}

sub on_close
{
	return unless $VB::form;
	my $self = shift;
	$self-> clear_event, return if !$VB::form-> close;
	$self-> menu-> keys_save($self->{iniFile}->section('Shortcuts'))
}

sub on_destroy
{
	$_[0]-> {ini}-> {MainPanelRect} = join( ' ', $_[0]-> rect);
	my @rx = ( $_[0]-> {ini}-> {ObjectInspectorVisible} = ( $VB::inspector ? 1 : 0))
		? $VB::inspector-> rect : ((-1)x4);
	$_[0]-> {ini}-> {ObjectInspectorRect} = join( ' ', @rx);
	@rx = ( $_[0]-> {ini}-> {CodeEditorVisible} = ( $VB::editor ? 1 : 0))
		? $VB::editor-> rect : ((-1)x4);
	$_[0]-> {ini}-> {CodeEditorRect} = join( ' ', @rx);
	$_[0]-> {ini}-> {OpenPath} = $openFileDlg-> directory if $openFileDlg;
	$_[0]-> {ini}-> {SavePath} = $saveFileDlg-> directory if $saveFileDlg;
	$VB::editor-> close if $VB::editor;
	$VB::main = undef;
	$::application-> close;
}


sub reset_tabs
{
	my $self = $_[0];
	my $nb = $self-> {nbpanel};
	$nb-> lock;
	$_-> destroy for $nb-> widgets;
	$self-> {tabset}-> tabs( $self-> {pages});
	$self-> {tabset}-> tabIndex(0);
	$nb-> pageCount( scalar @{$self-> {pages}});
	my %offsets = ();
	my %pagofs  = ();
	my %modules = ();
	my $i=0;
	$pagofs{$_} = $i++ for @{$self-> {pages}};

	$modules{$self-> {classes}-> {$_}-> {module}}=1 for keys %{$self-> {classes}};

	for ( keys %modules) {
		my $c = $_;
		eval("use $c;");
		if ( $@) {
			Prima::MsgBox::message( "Error loading module $_:$@");
			$modules{$_} = 0;
		}
	}

	my %iconfails = ();
	my %icongtx   = ();
	my $s = $::application->uiScaling;
	for ( sort keys %{$self-> {classes}}) {
		my ( $class, %info) = ( $_, %{$self-> {classes}-> {$_}});
		$offsets{$info{page}} = 4 unless exists $offsets{$info{page}};
		next unless $modules{$info{module}};
		my $i = undef;
		if ( $info{icon}) {
			$info{icon} =~ s/\:(\d+)$//;
			my @parms = ( Prima::Utils::find_image( $info{icon}));
			push( @parms, 'index', $1) if defined $1;
			$i = Prima::Icon-> create;
			unless ( defined $parms[0] && $i-> load( @parms)) {
				$iconfails{$info{icon}} = 1;
				$i = undef;
			}
			$i-> ui_scale if $i;
		};
		my $j = $nb-> insert_to_page( $pagofs{$info{page}}, SpeedButton =>
			hint   => $class,
			name   => 'ClassSelector',
			image  => $i,
			flat   => 1,
			origin => [ $offsets{$info{page}}, 4],
			size   => [ map { $s * $_ } 36, 36],
			delegations => [$self, qw(Click)],
		);
		$j-> {orgLeft}   = $offsets{$info{page}};
		$j-> {className} = $class;
		$offsets{$info{page}} += 36 * $s + 4;
	}
	$self-> {nbIndex} = 0;
	$nb-> unlock;
	$self-> {currentClass} = undef;

	if ( scalar keys %iconfails) {
		my @x = keys %iconfails;
		Prima::MsgBox::message( "Error loading images: @x");
	}
}

sub add_widgets
{
	my $self = $_[0];
	my $d = VB::open_dialog(
		filter => [['Packages' => '*.pm'], [ 'All files' => '*']],
	);
	return unless $d-> execute;
	my @r = Prima::VB::CfgMaint::add_module( $d-> fileName);
	Prima::MsgBox::message( "Error:$r[1]"), return unless $r[0];
	$self-> {classes} = {%Prima::VB::CfgMaint::classes};
	$self-> {pages}   = [@Prima::VB::CfgMaint::pages];
	$self-> reset_tabs;
	@r = Prima::VB::CfgMaint::write_cfg;
	Prima::MsgBox::message( "Error: $r[1]"), return unless $r[0];
}

sub get_nbpanel_count
{
	return $_[0]-> {nbpanel}-> widgets_from_page($_[0]-> {nbpanel}-> pageIndex);
}

sub set_nbpanel_index
{
	my ( $self, $idx) = @_;
	return if $idx < 0;
	my $nb  = $self-> {nbpanel};
	my @wp  = $nb-> widgets_from_page( $nb-> pageIndex);
	return if $idx >= scalar @wp;
	for ( @wp) {
		$_-> left( $_-> {orgLeft} - $idx * 40);
	}
	$self-> {nbIndex} = $idx;
}

sub LeftScroll_Click
{
	$_[0]-> set_nbpanel_index( $_[0]-> {nbIndex} - 1);
}

sub RightScroll_Click
{
	$_[0]-> set_nbpanel_index( $_[0]-> {nbIndex} + 1);
}

sub ClassSelector_Click
{
	$_[0]-> {currentClass} = $_[1]-> {className};
}


sub TabSet_Change
{
	my $self = $_[0];
	my $nb = $self-> {nbpanel};
	$nb-> pageIndex( $_[1]-> tabIndex);
	$self-> set_nbpanel_index(0);
}


sub get_typerec
{
	my ( $self, $type, $valPtr) = @_;
	unless ( defined $type) {
		my $rwx = 'fallback';
		if ( defined $valPtr && defined $$valPtr) {
			if ( ref($$valPtr)) {
				if (( ref($$valPtr) eq 'ARRAY') || ( ref($$valPtr) eq 'HASH')) {
				} elsif ( $$valPtr-> isa('Prima::Icon')) {
					$rwx = 'icon';
				} elsif ( $$valPtr-> isa('Prima::Image')) {
					$rwx = 'image';
				}
			}
		}
		return "Prima::VB::Types::$rwx";
	}
	$self-> {typerecs} = () unless exists $self-> {typerecs};
	my $t = $self-> {typerecs};
	return $t-> {$type} if exists $t-> {type};
	my $ns = \%Prima::VB::Types::;
	my $rwx = exists $ns-> {$type.'::'} ? $type : 'fallback';
	$rwx = 'Prima::VB::Types::'.$rwx;
	$t-> {$type} = $rwx;
	return $rwx;
}

sub new
{
	my $self = $_[0];
	return if $VB::form and !$VB::form-> close;
	$VB::form = Prima::VB::Form-> create;
	$VB::form-> set_default_props;
	$VB::main-> {fmName} = undef;
	Prima::VB::ObjectInspector::renew_widgets;
	update_menu();
}

sub inspect_load_data
{
	my ($self, $data, $asFile) = @_;
	my @preload_modules;

	my $fn = ( $asFile ? $data : "input data");
	if ( $asFile) {
		unless (open F, "<", $data) {
			Prima::MsgBox::message( "Error loading " . $data);
			return;
		}
		local $/;
		$data = <F>;
		close F;
	}

	my @d = split( "\n", $data);
	undef $data;


	if ( !defined($d[0]) || !($d[0] =~ /^# VBForm/ )) {
INV:
		Prima::MsgBox::message("Invalid format of $fn");
		return;
	}

	my @fvc = Prima::VB::VBLoader::check_version( $d[0]);
	Prima::MsgBox::message("Incompatible file format version ($fvc[1]) of $fn.\nBugs possible!",
		mb::Warning|mb::OK) unless $fvc[0];

	shift @d;
	my $i;
	for ( $i = 0; $i < scalar @d; $i++) {
		last unless $d[$i] =~ /^#/;
		next unless $d[$i] =~ /^#\s*\[([^\]]+)\](.*)$/;
		if ( $1 eq 'preload') {
			push( @preload_modules, split( ' ', $2));
		}
	}
	goto INV if $i >= scalar @d;

	for ( @preload_modules) {
		eval "use $_;";
		next unless $@;
		Prima::MsgBox::message( "Error loading module $_:$@");
		return;
	}
	$Prima::VB::VBLoader::builderActive = 1;
	my $sub = eval( join( "\n", @d[$i..$#d]));
	$Prima::VB::VBLoader::builderActive = 0;

	if ( $@ || ref($sub) ne 'CODE') {
		Prima::MsgBox::message("Error loading $fn:$@");
		return;
	}

	$Prima::VB::VBLoader::builderActive = 1;
	my @ret = $sub-> ();
	$Prima::VB::VBLoader::builderActive = 0;
	return @ret;
}

sub preload_modules
{
	my $self = shift;


}

sub push_widgets
{
	my $self     = shift;
	my $classes = $self-> {classes};
	my $callback = shift if $_[0] && ref($_[0]) eq 'CODE';

	my @seq = @_;
	my $main = $VB::form-> prf('name');
	my %owners  = ( $main => '');
	my %widgets = ( $main => $VB::form);
	my @return;
	my %dep;
	my $i;

	for ( $i = 0; $i < scalar @seq; $i+= 2) {
		$dep{$seq[$i]} = $seq[$i + 1];
	}

	for ( keys %dep) {
		$owners{$_} = exists $dep{$_}-> {profile}-> {owner} ?
			$dep{$_}-> {profile}-> {owner} :
			$main;
	}

	local *do_layer = sub
	{
		my $id = $_[0];
		my $i;
		for ( $i = 0; $i < scalar @seq; $i += 2) {
			$_ = $seq[$i];
			next unless $owners{$_} eq $id;
			eval "use $dep{$_}->{module};";
			Prima::message("Error loading $dep{$_}->{module}:$@"), next if "$@";
			my $c = $VB::form-> insert(
				$classes-> {$dep{$_}-> {class}}-> {class},
				realClass     => $dep{$_}-> {realClass},
				class         => $dep{$_}-> {class},
				module        => $dep{$_}-> {module},
				extras        => $dep{$_}-> {extras},
				creationOrder => $i / 2,
			);
			push ( @return, $c);
			if ( exists $dep{$_}-> {profile}-> {origin}) {
				my @norg = @{$dep{$_}-> {profile}-> {origin}};
				unless ( exists $widgets{$owners{$_}}) {
					# validating owner entry
					$owners{$_} = $main;
					$dep{$_}-> {profile}-> {owner} = $main;
				}
				my @ndelta = $owners{$_} eq $main ? (0,0) : (
					$widgets{$owners{$_}}-> left,
					$widgets{$owners{$_}}-> bottom
				);
				my @aperture = $owners{$_} eq $main ? (0,0) : (
					$widgets{$owners{$_}}-> o_delta_aperture
				);
				$norg[$_] += $ndelta[$_] + $aperture[$_] for 0..1;
				$dep{$_}-> {profile}-> {origin} = \@norg;
			}
			$c-> prf_set( %{$dep{$_}-> {profile}});
			$widgets{$_} = $c;
			$callback-> ( $self) if $callback;
			&do_layer( $_);
		}
	};
	&do_layer( $main, \%owners);
	return @return;
}

sub load_file
{
	my ($self,$fileName) = @_;

	$VB::form-> destroy if $VB::form;
	$VB::form = undef;
	update_menu();

	my @seq = $self-> inspect_load_data( $fileName, 1);
	return unless @seq;

	$self-> {fmName} = $fileName;

	$VB::main-> wait;
	my ( $i, $mf, $main, $fmindex);
	my $classes = $self-> {classes};

	for ( $i = 0; $i < scalar @seq; $i+= 2) {
		my $hash = $seq[$i + 1];
		if ( $hash-> {parent}) {
			$main = $seq[$i];
			$mf   = $seq[$i+1];
			$fmindex = $i;
		}
		unless ( $classes-> {$hash-> {class}}) {
			$hash-> {realClass} = $hash-> {class};
			$hash-> {class}     = $hash-> {parent} ? 'Prima::Window' : 'Prima::Widget';
		}
	}

	defined($main) ?
		splice( @seq, $fmindex, 2) :
		( $main = 'Form1', $mf = {});

	my $oldtxt = $self-> text;
	my $maxwij = scalar(@seq) / 2;
	$self-> text( "Loading...");

	$VB::form = Prima::VB::Form-> create(
		realClass   => $mf-> {realClass},
		class       => $mf-> {class},
		module      => $mf-> {module},
		extras      => $mf-> {extras},
		creationOrder => 0,
		visible     => 0,
	);
	if ( exists $mf-> {code}) {
		if ( $@) {
			Prima::MsgBox::message("Error loading $fileName: $@");
		} else {
			$VB::code = $mf-> {code};
			if ( $VB::editor) {
				$VB::editor-> Editor-> textRef( \$VB::code );
				$VB::editor-> {modified} = 0;
			}
		}
	}
	$VB::form-> prf_set( %{$mf-> {profile}});
	$VB::inspector-> {selectorChanging} = 1 if $VB::inspector;
	my $loaded;
	$self-> push_widgets( sub {
		$self-> text( sprintf( "Loaded %d%%", ($loaded++ / $maxwij) * 100));
	}, @seq);
	$VB::form-> show;
	$VB::inspector-> {selectorChanging}-- if $VB::inspector;
	Prima::VB::ObjectInspector::renew_widgets;
	update_menu();
	$self-> text( $oldtxt);
	$VB::form-> notify(q(Load));
	$_-> notify(q(Load)) for $VB::form-> widgets;
}

sub open_file
{
	my $self = $_[0];

	return if $VB::form and !$VB::form-> can_close;

	my $d = VB::open_dialog(
		filter => [['Form files' => '*.fm'], [ 'All files' => '*']],
	);
	return unless $d-> execute;
	$self-> load_file( $d-> fileName);
}

sub write_form
{
	my ( $self, $partialExport) = @_;
	$VB::writeMode = 0;

	my @cmp = $partialExport ?
		$VB::form-> marked_widgets : $VB::form-> widgets;
	my %preload_modules;

	my $header = <<PREHEAD;
# VBForm version file=$Prima::VB::VBLoader::fileVersion builder=$VBVersion
PREHEAD

	my $c = <<STARTSUB;
sub
{
\treturn (
STARTSUB

	my $main = $VB::form-> prf( 'name');
	push( @cmp, $VB::form) unless $partialExport;
	@cmp = sort { $a-> {creationOrder} <=> $b-> {creationOrder}} @cmp;

	for ( @cmp) {
		my ( $class, $module) = @{$_}{'class','module'};
		$class = $_-> {realClass} if defined $_-> {realClass};
		my $types = $_-> {types};
		my $name = $_-> prf( 'name');
		$Prima::VB::VBLoader::eventContext[0] = $name;
		$c .= <<MEDI;
\t'$name' => {
\t\tclass   => '$class',
\t\tmodule  => '$module',
MEDI
		if ( $_ == $VB::form) {
			Prima::VB::CodeEditor::sync_code;
			$c .= "\t\tparent => 1,\n";
			$c .= "\t\tcode => Prima::VB::VBLoader::GO_SUB(\'".
				Prima::VB::Types::generic::quotable($VB::code). "'),\n";
		}
		my %extras    = $_-> ext_profile;
		if ( scalar keys %extras) {
			$c .= "\t\textras => {\n";
			for ( sort keys %extras) {
				my $val  = $extras{$_};
				my $type = $self-> get_typerec( $types-> {$_}, \$val);
				$val = defined($val) ? $type-> write( $_, $val) : 'undef';
				$c .= "\t\t\t$_ => $val,\n";
			}
			$c .= "\t\t},\n";
		}
		%extras    = $_-> act_profile;
		if ( scalar keys %extras) {
			$c .= "\t\tactions => {\n";
			for ( sort keys %extras) {
				my $val  = $extras{$_};
				my $type = $self-> get_typerec( $types-> {$_}, \$val);
				$val = defined($val) ? $type-> write( $_, $val) : 'undef';
				$c .= "\t\t\t$_ => $val,\n";
			}
			$c .= "\t\t},\n";
		}
		my %Handle_props = map { $_ => 1 } $_-> {prf_types}-> {Handle} ? @{$_-> {prf_types}-> {Handle}} : ();
		delete $Handle_props{owner};
		if ( scalar keys %Handle_props) {
			$c .= "\t\tsiblings => [qw(" . join(' ', sort keys %Handle_props) . ")],\n";
		}
		$c .= "\t\tprofile => {\n";
		my ( $x,$prf) = ($_, $_-> {profile});
		my @o = $_-> get_o_delta;
		for( sort keys %{$prf}) {
			my $val = $prf-> {$_};
			if ( $_ eq 'origin' && defined $val) {
				my @nval = (
					$$val[0] - $o[0],
					$$val[1] - $o[1],
				);
				$val = \@nval;
			}
			my $type = $self-> get_typerec( $types-> {$_}, \$val);
			$val = defined($val) ? $type-> write( $_, $val) : 'undef';
			$preload_modules{$_} = 1 for $type-> preload_modules();
			$c .= "\t\t\t$_ => $val,\n";
		}
		$c .= "\t}},\n";
	}

$c .= <<POSTHEAD;
\t);
}
POSTHEAD
	$header .= '# [preload] ' . join( ' ', sort keys %preload_modules) . "\n";
	return $header . $c;
}

sub write_PL
{
	my $self = $_[0];
	my $main = $VB::form-> prf( 'name');
	my @cmp = $VB::form-> widgets;
	$VB::writeMode = 1;

	my $header = <<PREPREHEAD;
package ${main}Window;

use Prima;
use Prima::Classes;
use vars qw(\@ISA);
\@ISA = qw(Prima::MainWindow);

PREPREHEAD

	my %modules = map { $_-> {module} => 1 } @cmp;

	Prima::VB::CodeEditor::sync_code;

	my $c = <<PREHEAD;

$VB::code

sub profile_default
{
	my \$def = \$_[ 0]-> SUPER::profile_default;
	my \%prf = (
PREHEAD
	my $prf   = $VB::form-> {profile};
	my $types = $VB::form-> {types};
	for ( sort keys %$prf) {
		my $val = $prf-> {$_};
		my $type = $self-> get_typerec( $types-> {$_}, \$val);
		$val = defined($val) ? $type-> write( $_, $val) : 'undef';
		$c .= "\t\t$_ => $val,\n";
	}

	# size/origin have lower priority than width/left etc
	if ( not($prf-> {sizeDontCare}) and exists $prf->{size}) {
		my @s = @{$prf->{size}};
		$c .= "\t\twidth => $s[0],\n\t\theight => $s[1],\n";
	}
	if ( not($prf-> {originDontCare}) and exists $prf->{origin}) {
		my @o = @{$prf->{origin}};
		$c .= "\t\tleft => $o[0],\n\t\tbottom => $o[1],\n";
	}

	my @ds = ( $::application-> font-> width, $::application-> font-> height);
	$c .= "\t\tdesignScale => [ $ds[0], $ds[1]],\n";
	$c .= <<HEAD2;
	);
	\@\$def{keys \%prf} = values \%prf;
	return \$def;
}

HEAD2

	my @actNames  = qw( onBegin onFormCreate onCreate onChild onChildCreate onEnd);
	my %actions   = map { $_ => {}} @actNames;
	my @initInstances;
	for ( @cmp, $VB::form) {
		my $key = $_-> prf('name');
		my %act = $_-> act_profile;
		next unless scalar keys %act;
		push ( @initInstances, $key);
		for ( @actNames) {
			next unless exists $act{$_};
			my $aname = "${_}_$key";
			$actions{$_}-> {$key} = $aname;
			my $asub = join( "\n\t", split( "\n", $act{$_}));
			$c .= "sub $aname {\n\t$asub\n}\n\n";
		}
	}
	$c .= <<HEAD3;
sub init
{
	my \$self = shift;
	my \%instances = map {\$_ => {}} qw(@initInstances);
HEAD3
	for ( @initInstances) {
		my $obj = ( $_ eq $main) ? $VB::form : $VB::form-> bring($_);
		my %extras = $obj-> ext_profile;
		next unless scalar keys %extras;
		$c .= "\t\$instances{$_}->{extras} = {\n";
		for ( sort keys %extras) {
			my $val  = $extras{$_};
			my $type = $self-> get_typerec( $types-> {$_}, \$val);
			$val = defined($val) ? $type-> write( $_, $val) : 'undef';
			$c .= "\t\t$_ => $val,\n";
		}
		$c .= "\t};\n";
	}

	$c .= "\t".$actions{onBegin}->{$_}."(q($_), \$instances{$_});\n"
		for sort keys %{$actions{onBegin}};
	$c .= <<HEAD4;
	my \%profile = \$self-> SUPER::init(\@_);
	my \%names   = ( q($main) => \$self);
	\$self-> lock;
HEAD4
	@cmp = sort { $a-> {creationOrder} <=> $b-> {creationOrder}} @cmp;
	my %names = (
		$main => 1
	);
	my @re_cmp = ();

	$c .= "\t".$actions{onFormCreate}->{$_}."(q($_), \$instances{$_}, \$self);\n"
		for sort keys %{$actions{onFormCreate}};
	$c .= "\t".$actions{onCreate}->{$main}."(q($main), \$instances{$main}, \$self);\n"
		if $actions{onCreate}->{$main};
AGAIN:
	for ( @cmp) {
		my $owner = $_-> prf('owner');
		unless ( $names{$owner}) {
			push @re_cmp, $_;
			next;
		}
		my ( $class, $module) = @{$_}{'class','module'};
		$class = $_-> {realClass} if defined $_-> {realClass};
		my $types = $_-> {types};
		my $name = $_-> prf( 'name');
		$names{$name} = 1;

		$c .= "\t".$actions{onChild}-> {$owner}.
			"(q($owner), \$instances{$owner}, \$names{$owner}, q($name));\n"
				if $actions{onChild}-> {$owner};

		$c .= "\t\$names{$name} = \$names{$owner}-> insert( qq($class) => \n";
		my ( $x,$prf) = ($_, $_-> {profile});
		my @o = $_-> get_o_delta;
		for ( sort keys %{$prf}) {
			my $val = $prf-> {$_};
			if ( $_ eq 'origin' && defined $val) {
				my @nval = (
					$$val[0] - $o[0],
					$$val[1] - $o[1],
				);
				$val = \@nval;
			}
			next if $_ eq 'owner';
			my $type = $self-> get_typerec( $types-> {$_}, \$val);
			$val = defined($val) ? $type-> write( $_, $val) : 'undef';
			$modules{$_} = 1 for $type-> preload_modules();
			$c .= "\t\t$_ => $val,\n";
		}
		$c .= "\t);\n";

		$c .= "\t".$actions{onCreate}-> {$name}.
			"(q($name), \$instances{$name}, \$names{$name});\n"
				if $actions{onCreate}-> {$name};
		$c .= "\t".$actions{onChildCreate}-> {$owner}.
			"(q($owner), \$instances{$owner}, \$names{$owner}, \$names{$name});\n"
				if $actions{onChildCreate}-> {$owner};
	}
	if ( scalar @re_cmp) {
		@cmp = @re_cmp;
		@re_cmp = ();
		goto AGAIN;
	}

	$c .= "\t".$actions{onEnd}-> {$_}."(q($_), \$instances{$_}, \$names{$_});\n"
		for sort keys %{$actions{onEnd}};
$c .= <<POSTHEAD;
	\$self-> unlock;
	return \%profile;
}

package ${main}Auto;

use Prima::Application;
${main}Window-> create;
Prima->run;

POSTHEAD

	$header .= "use $_;\n" for sort keys %modules;
	$VB::writeMode = 0;
	return $header.$c;
}


sub save
{
	my ( $self, $asPL) = @_;

	return 0 unless $VB::form;

	return $self-> saveas unless defined $self-> {fmName};

	if ( open F, ">", $self-> {fmName}) {
		local $/;
		$VB::main-> wait;
		my $c = $asPL ? $self-> write_PL : $self-> write_form;
		print F $c;
	} else {
		Prima::MsgBox::message("Error saving ".$self-> {fmName});
		return 0;
	}
	close F;

	$VB::form-> {modified} = undef unless $asPL;
	$VB::editor-> {modified} = 0 if $VB::editor && !$asPL;

	return 1;
}

sub saveas
{
	my $self = $_[0];
	return 0 unless $VB::form;
	my $d = VB::save_dialog(
		filter => [
			['Form files' => '*.fm'],
			['Program scratch' => '*.pl'],
			[ 'All files' => '*']
		],
	);
	$self-> {saveFileDlg} = $d;
	return 0 unless $d-> execute;
	my $fn = $d-> fileName;
	my $asPL = ($d-> filterIndex == 1);
	my $ofn = $self-> {fmName};
	$self-> {fmName} = $fn;
	$self-> save( $asPL);
	$self-> {fmName} = $ofn if $asPL;
	return 1;
}

sub update_menu
{
	return unless $VB::main;
	my $m = $VB::main-> menu;
	my $a = $::application-> accelTable;
	my $f = (defined $VB::form) ? 1 : 0;
	my $r = (defined $VB::main-> {running}) ? 1 : 0;
	$m-> enabled( 'runitem', $f && !$r);
	$a-> enabled( 'runitem', $f && !$r);
	$m-> enabled( 'newitem', !$r);
	$m-> enabled( 'openitem', !$r);
	$a-> enabled( 'openitem', !$r);
	$m-> enabled( 'saveitem1', $f);
	$a-> enabled( 'saveitem1', $f);
	$m-> enabled( 'saveitem2', $f);
	$m-> enabled( 'closeitem', $f);
	$m-> enabled( 'breakitem', $f && $r);
	$VB::main-> {runbutton}-> enabled( $f && !$r);
	$VB::main-> {openbutton}-> enabled( !$r);
	$VB::main-> {newbutton}-> enabled( !$r);
	$VB::main-> {savebutton}-> enabled( $f);
}

sub update_markings
{
}

sub form_cancel
{
	if ( $VB::main) {
		if ( $VB::main-> {topLevel}) {
			for ( $::application-> get_components) {
				next if $VB::main-> {topLevel}-> {"$_"};
				eval { $_-> destroy; };
			}
			$VB::main-> {topLevel} = undef;
		}
		return unless $VB::main-> {running};
		$VB::main-> {running}-> destroy;
		$VB::main-> {running} = undef;
		update_menu();
	}
	$VB::form-> show if $VB::form;
	$VB::inspector-> show if $VB::inspector;
	$VB::editor-> show if $VB::editor;
}


sub form_run
{
	my $self = $_[0];
	return unless $VB::form;
	if ( $VB::main-> {running}) {
		$VB::main-> {running}-> destroy;
		$VB::main-> {running} = undef;
	}
	$VB::main-> wait;
	my $c = $self-> write_form;
	my $okCreate = 0;
	$VB::main-> {topLevel} = { map { ("$_" => 1) } $::application-> get_components };
	@Prima::VB::VBLoader::eventContext = ('', '');
	my $uiScaling = $::application->uiScaling;
	my @saved_font = @Prima::Widget::default_font_box;
	eval{
		local $SIG{__WARN__} = sub {
			return if $_[0] =~ /^Subroutine.*redefined/;
			die $_[0]
		};
		my $n = $VB::form-> prf('name');
		my %param;
		if ( $VB::dpi > 0 ) {
			my $sc = $VB::dpi / 96.0;
			$::application->uiScaling($sc);
			my %emulated_font;
			%emulated_font = %{ $::application->get_default_font };
			$emulated_font{$_} *= $sc for qw(size height width);
			$param{$n}->{font} = \%emulated_font;
			unless ( @Prima::Widget::default_font_box ) {
				my $f = $::application-> get_default_font;
				@Prima::Widget::default_font_box = ( $f-> { width}, $f-> { height});
			}
			$Prima::Widget::default_font_box[$_] *= $sc for 0,1;
		}
		my $sub = eval("$c");
		die "Error loading module $@" if $@;
		my @d = $sub-> ();
		my %r = Prima::VB::VBLoader::AUTOFORM_REALIZE( \@d, \%param );
		my $f = $r{$n};
		$okCreate = 1;
		if ( $f) {
			$f-> set( onClose => \&form_cancel );
			$VB::main-> {running} = $f;
			update_menu();
			$f-> select;
			$VB::form-> hide;
			$VB::inspector-> hide if $VB::inspector;
			$VB::editor-> hide if $VB::editor;
		};
	};
	my $error = $@;
	if ( $VB::dpi > 0 ) {
		$::application->uiScaling($uiScaling);
		@Prima::Widget::default_font_box = @saved_font;
	}
	if ( $error ) {
		my $msg = "$error";
		$msg =~ s/ \(eval \d+\)//g;
		if ( defined( $Prima::VB::VBLoader::eventContext[0]) &&
			length ($Prima::VB::VBLoader::eventContext[0])) {
			$VB::main-> bring_inspector;
			$VB::main-> {topLevel}-> { "$VB::inspector" } = 1;
			$VB::inspector-> Selector-> text( $Prima::VB::VBLoader::eventContext[0]);
			if ( $Prima::VB::VBLoader::eventContext[0] eq $VB::inspector-> Selector-> text &&
					length($Prima::VB::VBLoader::eventContext[1])) {
				$VB::inspector-> set_monger_index(1);
				my $list = $VB::inspector-> {currentList};
				my $ix = $list-> {index}-> {$Prima::VB::VBLoader::eventContext[1]};
				if ( defined $ix) {
					$list-> focusedItem( $ix);
					$VB::inspector-> {panel}-> select;
				}
			}
		}
		Prima::MsgBox::message( $msg);
		for ( $::application-> get_components) {
			next if $VB::main-> {topLevel}-> {"$_"};
			eval { $_-> destroy; };
		}
		$VB::main-> {topLevel} = undef;
	}
}

sub dpi_toggle
{
	my ( $self, $id ) = @_;
	$id =~ s/^dpi//;
	$VB::dpi = $id;
}

sub wait
{
	my $t = $_[0]-> insert( Timer => timeout => 10, onTick => sub {
		$::application-> pointer( $_[0]-> {savePtr});
		$_[0]-> destroy;
	});
	$t-> {savePtr} = $::application-> pointer;
	$::application-> pointer( cr::Wait);
	$t-> start;
}

sub bring_inspector
{
	if ( $VB::inspector) {
		$VB::inspector-> restore if $VB::inspector-> windowState == ws::Minimized;
		$VB::inspector-> bring_to_front;
		$VB::inspector-> select;
	} else {
		$VB::inspector = Prima::VB::ObjectInspector-> create;
		Prima::VB::ObjectInspector::renew_widgets;
	}
}

sub bring_code_editor
{
	if ( $VB::editor) {
		$VB::editor-> restore if $VB::editor-> windowState == ws::Minimized;
		$VB::editor-> bring_to_front;
		$VB::editor-> select;
	} else {
		$VB::editor = Prima::VB::CodeEditor-> create;
	}
}

sub bring_font_dialog
{
	if ( $VB::font_dialog) {
		$VB::font_dialog-> restore if $VB::font_dialog-> windowState == ws::Minimized;
	} else {
		$VB::font_dialog = Prima::Dialog::FontDialog-> new(
			borderIcons => bi::All & ~bi::Maximize,
			taskListed  => 1,
			onDestroy   => sub { $VB::font_dialog = undef },
		);
		my $y;
		for ( $VB::font_dialog-> widgets) {
			next unless $_-> isa("Prima::Button") and $_-> text =~ /ok|cancel/i;
			$y = $_-> right;
			$_-> set( visible => 0, enabled => 0);
		}
		$VB::font_dialog-> Size-> set(
			left  => $VB::font_dialog-> Size-> left,
			right => $y
		);
		$VB::font_dialog-> visible(1);
	}
	$VB::font_dialog-> bring_to_front;
	$VB::font_dialog-> select;
}

sub bring_color_dialog
{
	if ( $VB::color_dialog) {
		$VB::color_dialog-> restore if $VB::color_dialog-> windowState == ws::Minimized;
	} else {
		$VB::color_dialog = Prima::Dialog::ColorDialog-> new(
			borderIcons => bi::All & ~bi::Maximize,
			taskListed  => 1,
			onDestroy => sub { $VB::color_dialog = undef },
		);
		my $y;
		for ( $VB::color_dialog-> widgets) {
			next unless $_-> isa("Prima::Button") and $_-> text =~ /ok|cancel/i;
			$y = $_-> top;
			$_-> set( visible => 0, enabled => 0);
		}
		$_-> bottom( $_-> bottom - $y) for $VB::color_dialog-> widgets;
		$VB::color_dialog-> height( $VB::color_dialog-> height - $y);
		$VB::color_dialog-> visible(1);
	}
	$VB::color_dialog-> bring_to_front;
	$VB::color_dialog-> select;
}

sub init_position
{
	my ( $self, $window, $name) = @_;

	my @sz = $::application-> size;
	my @rx = split( ' ', $self-> {ini}-> {$name});
	return unless grep { $_ != -1 } @rx;

	$rx[0] = 0 if $rx[0] < 0 or $rx[0] > $sz[0] + 100;
	$rx[1] = 0 if $rx[1] < 0 or $rx[1] > $sz[1] + 100;
	$rx[2] = $sz[0] if $rx[2] > $sz[0];
	$rx[3] = $sz[1] if $rx[3] > $sz[1];

	$window-> rect( @rx);
}

my $menuDlg;

sub edit_menu
{
	my $self = shift;
	$menuDlg //= Prima::KeySelector::Dialog-> new(menuTree => $self-> menu);
	$menuDlg->execute;
}

package Prima::VB::VisualBuilder;
use strict;

$::application-> icon( Prima::Image-> load( Prima::Utils::find_image( 'VB::VB.gif'), index => 6));
$::application-> accelItems( VB::accelItems);
$VB::main = Prima::VB::MainPanel-> create;
$VB::inspector = Prima::VB::ObjectInspector-> create(
	top => $VB::main-> bottom - 12 - $::application-> get_system_value(sv::YTitleBar)
) if $VB::main-> {ini}-> {ObjectInspectorVisible};
$VB::code = '';
$VB::editor = Prima::VB::CodeEditor-> create() if $VB::main-> {ini}-> {CodeEditorVisible};
$VB::form = Prima::VB::Form-> create;
$VB::form-> set_default_props;
Prima::VB::ObjectInspector::renew_widgets;
Prima::VB::ObjectInspector::preload() unless $VB::fastLoad;
$VB::main-> update_menu();

$VB::main-> load_file( $ARGV[0]) if @ARGV && -f $ARGV[0] && -r _;

Prima->run;

1;

__END__

=pod

=head1 NAME

VB - Visual Builder for the Prima toolkit

=for podview <img src="vb-large.png">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/vb-large.png">

=head1 SYNOPSIS

Run the C<VB> command in your terminal

=head1 DESCRIPTION

Visual Builder is a RAD-style suite for designing forms using the Prima
toolkit. It provides a rich set of perl-based widgets which can be inserted into
a window-based form by simple actions. The form can be stored in a file and
loaded by either a user program or a simple wrapper, C<utils/prima-fmview.pl>;
the form can be also stored as a valid perl program.

A form file has the I<.fm> extension and can be loaded fairly simply by using
the L<Prima::VB::VBLoader> module. The following code is the only content of
the C<prima-fmview.pl> program:

	use Prima qw(Application VB::VBLoader);
	my $ret = Prima::VBLoad( $ARGV[0] );
	die "$@\n" unless $ret;
	$ret-> execute;

Such code is usually sufficient for executing a form file.

=head1 Help

The builder provides three main windows, that are used for interactive design.
These are called I<main panel>, I<object inspector>, and I<form window>. When
the builder is started, the form window is empty.

The main panel consists of the menu bar, speed buttons, and the widget buttons.
If the user presses a widget button and then clicks the mouse on the form
window, the selected widget is inserted into the form and becomes a child of
the form window.  If the click was made on a visible widget in the form window,
the newly inserted widget becomes a child of that widget. After the widget is
inserted, its properties are accessible in the object inspector window.

The menu bar contains the following commands:

=over

=item File

=over

=item New

Closes the current form and opens a new empty form.
If the old form was not saved, the user is asked if the changes made
are to be saved.

This command is an alias to the 'new file' icon on the panel.

=item Open

Invokes the file open dialog so a I<.fm> form file can be opened.
After a successful file load, all form widgets are visible and
available for editing.

This command is an alias to the 'open folder' icon on the panel.

=item Save

Stores the form into a file. The user here can select a type
of the file to be saved. If the form is saved as a I<.fm> form
file then it can be re-loaded either in the builder or a
user program ( see L<Prima::VB::VBLoader> for details ).
If the form is saved as a I<.pl> program, then it
can not be loaded; instead, the program can be run immediately
without the builder or any supplementary code.

This command is an alias to the 'save on disk' icon on the panel.

=item Save as

Same as L<Save>, except that a new name or type of file is
asked every time the command is invoked.

=item Close

Closes the form and removes the form window. If the form window
was changed, the user is asked if the changes made
are to be saved.

=back

=item Edit

=over

=item Copy

Copies the selected widgets into the clipboard so they can be inserted later by
using the L<Paste> command.  The form window itself can not be copied, only the
widgets it contains.

=item Paste

Reads the information put by the builder L<Copy> command into the clipboard and
inserts the widgets into the form window. The child-parent relation is kept by
the names of the widgets; if the widget with the name of the parent of the
clipboard-read widgets is not found, the widgets are inserted into the form
window.  The form window is not affected by this command.

=item Delete

Deletes the selected widgets.
The form window itself can not be deleted.

=item Select all

Selects all of the widgets inserted in the form window except the
form window itself.

=item Duplicate

Duplicates the selected widgets.
The form window is not affected by this command.

=item Align

This menu item contains z-ordering actions that
are performed on selected widgets.
These are:

   Bring to front
   Send to back
   Step forward
   Step backward
   Restore order

=back

=item Change class

Changes the class of the selected widget. This is an advanced option and can
lead to confusion or errors if the default widget class and the supplied class
differ too much. It is used when the widget that has to be inserted is not
present in the builder installation. Also, it is called implicitly when the
loaded form does not contain a valid widget class; in such case the
I<Prima::Widget> class is assigned.

=item Creation order

Opens the dialog that manages the creation order of the widgets.
It is not that important for the widget child-parent relation, since
the builder tracks these, and does not allow a child to be created
before its parent. However, the explicit order might be helpful
in a case when, for example, the C<tabOrder> property is left to its default
value, so it is assigned according to the order of widget creation.

=item Toggle lock

Changes the lock status for selected widgets. The lock, if set, prevents
widgets from being selected by the mouse to avoid occasional positional changes.
This is useful when a widget is used as an owner for many sub-widgets.

The ctrl+mouse combination click locks and unlocks widgets.

=back

=over

=item View

=over

=item Object inspector

Brings the object inspector window, if it was hidden or closed.

=item Add widgets

Opens the file dialog to install additional VB widgets.
The modules are used for providing custom widgets and properties
for the builder. As an example, the F<Prima/VB/examples/Widgety.pm>
module is provided with the builder and the toolkit. Look inside this
file for the implementation details.

=item Reset guidelines

Resets the guidelines on the form window into the center.

=item Snap to guidelines

Specifies if the moving and resizing widget actions must treat
the form window guidelines as snapping areas.

=item Snap to grid

Specifies if the moving and resizing widget actions must use
the form window grid granularity instead of the pixel granularity.

=item Run

This command hides the form and object inspector windows and
'executes' the form as if it would be run by C<prima-fmview.pl>.
The execution session ends either by closing the form window
or by calling the L<Break> command.

This command is an alias to the 'run' icon on the panel.

=item Break

Explicitly terminates the execution session initiated by the L<Run>
command.

=back

=back

=over

=item Help

=over

=item About

Displays the information about the visual builder.

=item Help

Displays the information about the usage of the visual builder.

=item Widget property

Invokes the help viewer on the L<Prima::Widget> manpage and tries to open the
topic corresponding to the current selection of the object inspector property
or event. While the manpage covers far not all ( but still many ) properties
and events, it is still a little bit more convenient than nothing.

=back

=back

=head2 Form window

The form widget is the common parent for all widgets created by the
builder. The form window provides the following basic navigation.

=over

=item Guidelines

The form window contains two guidelines, the horizontal and the vertical, drawn
as blue dashed lines. Dragging with the mouse can move these lines.  If the
menu option L<Snap to guidelines> is checked, the widgets' moving and sizing
operations treat the guidelines as snapping areas.

=item Selection

A widget can be selected by clicking with the mouse on it. There can be more
than one selected widget at a time, or none at all.  To explicitly select a
widget in addition to the already selected ones, hold the C<shift> key while
clicking on a widget. This combination also deselects the widget. To select all
widgets on the form window, call the L<Select all> command from the menu. To
prevent widgets from being occasionally selected, lock them with the "Edit/Toggle
lock" command or Ctrl+mouse click.

=item Moving

Dragging the mouse can move the selected widgets. Widgets can be snapped to the
grid or the guidelines during the move. If one of the moving widgets is
selected in the object inspector window, the coordinate changes are reflected
in the C<origin> property.

If the C<Tab> key is pressed during the move, the mouse pointer is changed
between three states, each reflecting the currently accessible coordinates for
dragging. The default accessible coordinates are both the horizontal and
the vertical; the other two are the horizontal only and the vertical only.

=item Sizing

The sizeable widgets can be dynamically resized. Only one widget at a time can
be resized.  If the resized widget is selected in the object inspector window,
the size changes are reflected in the C<size> property.

=item Context menus

The right-click (or the other system-defined pop-up menu invocation command)
provides the menu, identical to the main panel's L<Edit> submenu.

The alternative context menus can be provided with some widgets ( for
example, C<TabbedNotebook> ), and are accessible with the C<control + right click>
combination.

=back

=head2 Object inspector window

The inspector window reflects the events and properties of a widget.  To
explicitly select a widget, it must be either clicked by the mouse on the form
window or selected in the widget combo box. Depending on whether properties or
events are selected, the left panel of the inspector provides a list of
properties or events, and the right panel - a value of the currently selected
property or event. To toggle between the properties and the events use the
button below the list.

The adjustable properties of a widget include an incomplete set of the
properties returned by the class method C<profile_default> ( for a detailed
explanation see L<Prima::Object>). Among these are such basic properties as
C<origin>, C<size>, C<name>, C<color>, C<font>, C<visible>, C<enabled>,
C<owner>, and many others.  Each property can be selected by the property
selector; in such case the name of a property is highlighted in the list -
that means, that the property is initialized. To remove a property from the
initialization list, double-click on it, so it is grayed again. Some very basic
properties as C<name> can not be deselected. This is because no widgets of the same
name can coexist simultaneously in the builder.

The events, much like the properties, are accessible for direct change.
All the events provide a small editor, so the custom code can be supplied.
This code is executed when the form is run or loaded via the C<Prima::VB::VBLoader>
interface.

The full explanation of properties and events is not provided here. It is not
even the goal of this document because the builder can work with the widgets
irrespective of their property or event capabilities; this information is
provided by the toolkit. To read what each property or event means, use the
documentation on the class of interest; L<Prima::Widget> is a good start
because it encompasses the basic C<Prima::Widget> functionality.  The other
widgets are documented in their respective modules, for example, the
C<Prima::ScrollBar> documentation can be found in L<Prima::ScrollBar>.

=head1 SEE ALSO

L<Prima>, L<Prima::VB::VBLoader>

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 COPYRIGHT

This program is distributed under the BSD License.

=cut
