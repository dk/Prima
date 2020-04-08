#
#  Created by:
#     Anton Berezin  <tobez@tobez.org>
#     Dmitry Karasik <dk@plab.ku.dk>
#  Modifications by:
#     David Scott <dscott@dgt.com>
#

use strict;
use warnings;
use Prima::Classes;
use Prima::Buttons;
use Prima::Lists;
use Prima::Label;
use Prima::InputLine;
use Prima::ComboBox;
use Prima::MsgBox;

package Prima::DirectoryListBox;
use vars qw(@ISA @images);

@ISA = qw(Prima::ListViewer);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		path           => '.',
		openedGlyphs   => 1,
		closedGlyphs   => 1,
		openedIcon     => undef,
		closedIcon     => undef,
		indent         => 12 * $::application->uiScaling,
		multiSelect    => 0,
		showDotDirs    => 1,
	}
}

sub init
{
	unless (@images) {
		my $i = 0;
		for ( sbmp::SFolderOpened, sbmp::SFolderClosed) {
			$images[ $i++] = Prima::StdBitmap::icon($_);
		}
	}
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	for ( qw( maxWidth oneSpaceWidth))            { $self-> {$_} = 0}
	for ( qw( files filesStat items))             { $self-> {$_} = []; }
	for ( qw( openedIcon closedIcon openedGlyphs closedGlyphs indent showDotDirs))
		{ $self-> {$_} = $profile{$_}}
	$self-> {openedIcon} = $images[0] unless $self-> {openedIcon};
	$self-> {closedIcon} = $images[1] unless $self-> {closedIcon};
	$self-> {fontHeight} = $self-> font-> height;
	$self-> recalc_icons;
	$self-> path( $profile{path});
	return %profile;
}

use constant ROOT      => 0;
use constant ROOT_ONLY => 1;
use constant LEAF      => 2;
use constant LAST_LEAF => 3;
use constant NODE      => 4;
use constant LAST_NODE => 5;

sub on_stringify
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> {items}-> [$index]-> {text};
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	my $item = $self-> {items}-> [$index];
	$$sref = $self-> get_text_width( $item-> {text}) +
			$self-> {oneSpaceWidth} +
			( $self-> {opened} ?
			( $self-> {openedIcon} ? $self-> {openedIcon}-> width : 0):
			( $self-> {closedIcon} ? $self-> {closedIcon}-> width : 0)
			) +
			4 + $self-> {indent} * $item-> {indent};
}

sub on_fontchanged
{
	my $self = shift;
	$self-> recalc_icons;
	$self-> {fontHeight} = $self-> font-> height;
	$self-> SUPER::on_fontchanged(@_);
}

sub on_click
{
	my $self = $_[0];
	my $items = $self-> {items};
	my $foc = $self-> focusedItem;
	return if $foc < 0;
	my $newP = '';
	my $ind = $items-> [$foc]-> {indent};
	for ( @{$items} ) {
		$newP .= $_-> {text}."/" if $_-> {indent} < $ind;
	}
	$newP .= $items-> [$foc]-> {text};
	$newP .= '/' unless $newP =~ m/[\/\\]$/;
	$self-> path( $newP);
}

sub on_drawitem
{
	my ($self, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focusedItem, $prelight) = @_;
	my $item  = $self-> {items}-> [$index];
	my $text  = $item-> {text};
	$text =~ s[^\s*][];
	# my $clrSave = $self-> color;
	# my $backColor = $hilite ? $self-> hiliteBackColor : $self-> backColor;
	# my $color     = $hilite ? $self-> hiliteColor     : $clrSave;
	# $canvas-> color( $backColor);
	# $canvas-> bar( $left, $bottom, $right, $top);
	my ( $c, $bc);
	if ( $hilite ) {
		$c = $self-> color;
		$bc = $self-> backColor;
		$canvas-> color($self-> hiliteColor);
		$canvas-> backColor($self-> hiliteBackColor);
	}

	$self-> draw_item_background( $canvas, $left, $bottom, $right, $top, $prelight);

	my $type = $item-> {type};
	# $canvas-> color($color);
	my ($x, $y, $x2);
	my $indent = $self-> {indent} * $item-> {indent};
	my $prevIndent = $indent - $self-> {indent};
	my ( $icon, $glyphs) = (
		$item-> {opened} ? $self-> {openedIcon}   : $self-> {closedIcon},
		$item-> {opened} ? $self-> {openedGlyphs} : $self-> {closedGlyphs},
	);

	my ( $iconWidth, $iconHeight) = $icon ?
		( $icon-> width/$glyphs, $icon-> height) : ( 0, 0);
	if ( $type == ROOT || $type == NODE) {
		$x = $left + 2 + $indent + $iconWidth / 2;
		$x = int( $x);
		$y = ($top + $bottom) / 2;
		$canvas-> line( $x, $bottom, $x, $y);
	}

	if ( $type != ROOT && $type != ROOT_ONLY) {
		$x = $left + 2 + $prevIndent + $iconWidth / 2;
		$x = int( $x);
		$x2 = $left + 2 + $indent + $iconWidth / 2;
		$x2 = int( $x2);
		$y = ($top + $bottom) / 2;
		$canvas-> line( $x, $y, $x2, $y);
		$canvas-> line( $x, $y, $x, $top);
		$canvas-> line( $x, $y, $x, $bottom) if $type == LEAF;
	}

	$canvas-> put_image_indirect ( $icon,
		$left + 2 + $indent,
		int(($top + $bottom - $iconHeight) / 2+0.5),
		0, 0,
		$iconWidth, $iconHeight,
		$iconWidth, $iconHeight, rop::CopyPut) if 1;

	$canvas-> shape_text_out( $text,
		$left + 2 + $indent + $self-> {oneSpaceWidth} + $iconWidth,
		int($top + $bottom - $self-> {fontHeight}) / 2+0.5);

	$canvas-> rect_focus( $left + $self-> {offset}, $bottom, $right, $top)
		if $focusedItem;

	if ( $hilite ) {
		$canvas-> color($c);
		$canvas-> backColor($bc);
	}
	# $canvas-> color($clrSave);
}


sub recalc_icons
{
	my $self = $_[0];
	my $hei = $self-> font-> height + 2;
	my ( $o, $c) = (
		$self-> {openedIcon} ? $self-> {openedIcon}-> height : 0,
		$self-> {closedIcon} ? $self-> {closedIcon}-> height : 0
	);
	$hei = $o if $hei < $o;
	$hei = $c if $hei < $c;
	$self-> itemHeight( $hei);
}

sub recalc_items
{
	my ($self, $items) = ($_[0], $_[0]-> {items});
	$self-> begin_paint_info;
	$self-> {oneSpaceWidth} = $self-> get_text_width(' ');
	my $maxWidth = 0;
	my @widths = (
		$self-> {openedIcon} ? ( $self-> {openedIcon}-> width / $self-> {openedGlyphs}) : 0,
		$self-> {closedIcon} ? ( $self-> {closedIcon}-> width / $self-> {closedGlyphs}) : 0,
	);
	for ( @$items) {
		my $width = $self-> get_text_width( $_-> {text}) +
			$self-> {oneSpaceWidth} +
			( $_-> {opened} ? $widths[0] : $widths[1]) +
			4 + $self-> {indent} * $_-> {indent};
		$maxWidth = $width if $maxWidth < $width;
	}
	$self-> end_paint_info;
	$self-> {maxWidth} = $maxWidth;
}

sub new_directory
{
	my $self = shift;
	my $p = $self-> path;
	my @fs = Prima::Utils::getdir( $p);
	unless ( scalar @fs) {
		$self-> path('.'), return unless $p =~ tr{/\\}{} > 1;
		$self-> {path} =~ s{[/\\][^/\\]+[/\\]?$}{/};
		$self-> path('.'), return if $p eq $self-> {path};
		$self-> path($self-> {path});
		return;
	}

	my $oldPointer = $::application-> pointer;
	$::application-> pointer( cr::Wait);
	my $i;
	my @fs1;
	my @fs2;
	for ( $i = 0; $i < scalar @fs; $i += 2) {
		next if !$self-> {showDotDirs} && $fs[$i] =~ /^\./;
		push( @fs1, $fs[ $i]);
		if ( $fs[ $i + 1] eq 'lnk') {
			if ( -f $p.$fs[$i]) {
				$fs[ $i + 1] = 'reg';
			} elsif ( -d _) {
				$fs[ $i + 1] = 'dir';
			}
		}
		push( @fs2, $fs[ $i + 1]);
	}

	$self-> {files}     = \@fs1;
	$self-> {filesStat} = \@fs2;
	my @d   = sort grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
	my $ind = 0;
	my @ups = split /[\/\\]/, $p;
	my @lb;
	my $wasRoot = 0;
	$ups[0] = '/' if $p =~ /^\//;

	for ( @ups) {
		push @lb, {
			text          => $_,
			opened        => 1,
			type          => $wasRoot ? NODE : ROOT,
			indent        => $ind++,
		};
		$wasRoot = 1;
	}
	$lb[-1]-> {type} = LAST_LEAF unless scalar @d;
	$lb[-1]-> {type} = ROOT_ONLY if $#ups == 0 && scalar @d == 0;
	my $foc = $#ups;
	for (@d) {
		push @lb, {
			text   => $_,
			opened => 0,
			type   => LEAF,
			indent => $ind,
		};
	}
	$lb[-1]-> {type} = LAST_LEAF if scalar @d;
	$self-> items([@lb]);
	$self-> focusedItem( $foc);
	$self-> notify( q(Change));
	$::application-> pointer( $oldPointer);
}

sub safe_abs_path
{
	my $p = $_[0];
	my $warn;
	local $SIG{__WARN__} = sub {
		$warn = "@_";
	};
	$p = eval { Cwd::abs_path($p) };
	$@ .= $warn if defined $warn;
	return $p;
}

sub path
{
	return $_[0]-> {path} unless $#_;
	my $p = $_[1];
	$p =~ s{^([^\\\/]*[\\\/][^\\\/]*)[\\\/]$}{$1};
	$p .= '/' unless $p =~ m/[\/\\]$/;
	$p =~ s/^\/\//\//; # cygwin barfs on // paths
	unless( scalar( stat $p)) {
		$p = "";
	} else {
		$p = safe_abs_path($p);
		$p = "." if $@ || !defined $p;
		$p = "" unless -d $p;
		$p .= '/' unless $p =~ m/[\/\\]$/;
	}
	$_[0]-> {path} = $p;
	return if defined $_[0]-> {recursivePathCall} && $_[0]-> {recursivePathCall} > 2;
	$_[0]-> {recursivePathCall}++;
	$_[0]-> new_directory;
	$_[0]-> {recursivePathCall}--;
}

sub files {
	my ( $fn, $fs) = ( $_[0]-> {files}, $_[0]-> {filesStat});
	return wantarray ? @$fn : $fn unless ($#_);
	my @f;
	for ( my $i = 0; $i < scalar @$fn; $i++) {
		push ( @f, $$fn[$i]) if $$fs[$i] eq $_[1];
	}
	return wantarray ? @f : \@f;
}

sub openedIcon
{
	return $_[0]-> {openedIcon} unless $#_;
	$_[0]-> {openedIcon} = $_[1];
	$_[0]-> recalc_icons;
	$_[0]-> calibrate;
}

sub closedIcon
{
	return $_[0]-> {closedIcon} unless $#_;
	$_[0]-> {closedIcon} = $_[1];
	$_[0]-> recalc_icons;
	$_[0]-> calibrate;
}

sub openedGlyphs
{
	return $_[0]-> {openedGlyphs} unless $#_;
	$_[1] = 1 if $_[1] < 1;
	$_[0]-> {openedGlyphs} = $_[1];
	$_[0]-> recalc_icons;
	$_[0]-> calibrate;
}

sub closedGlyphs
{
	return $_[0]-> {closedGlyphs} unless $#_;
	$_[1] = 1 if $_[1] < 1;
	$_[0]-> {closedGlyphs} = $_[1];
	$_[0]-> recalc_icons;
	$_[0]-> calibrate;
}

sub indent
{
	return $_[0]-> {indent} unless $#_;
	$_[1] = 0 if $_[1] < 0;
	return if $_[0]-> {indent} == $_[1];
	$_[0]-> calibrate;
}

sub showDotDirs
{
	return $_[0]-> {showDotDirs} unless $#_;
	my ( $self, $show) = @_;
	$show =  ( $show ? 1 : 0);
	return if $show == $self-> {showDotDirs};
	$self-> {showDotDirs} = $show;
	$self-> new_directory;
}

package Prima::DriveComboBox::InputLine;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		ownerBackColor   => 1,
		selectable       => 1,
		selectingButtons => 0,
	}
}

sub text
{
	return $_[0]-> SUPER::text unless $#_;
	my ($self,$cap) = @_;
	$self-> SUPER::text( $cap);
	$self-> notify(q(Change));
	$self-> repaint;
}

sub borderWidth { return 1}
sub selection   { return 0, 0; }

sub on_paint
{
	my ( $self, $canvas, $combo, $W, $H, $focused) =
		($_[0],$_[1],$_[0]-> owner,$_[1]-> size,$_[0]-> focused || $_[0]-> owner-> listVisible);

	my $back = $focused ? $self-> hiliteBackColor : $self-> backColor;
	my $fore = $focused ? $self-> hiliteColor : $self-> color;
	$canvas-> rect3d( 0, 0, $W-1, $H-1, 1, $self-> dark3DColor, $self-> light3DColor, $back);
	my $icon = $combo-> {icons}[$combo-> focusedItem];
	my $adj = 3;

	if ( $icon) {
		my ($h, $w) = ($icon-> height, $icon-> width);
		$canvas-> put_image( 3, ($H - $h)/2, $icon);
		$adj += $w + 3;
	}

	$canvas-> color( $fore);
	$canvas-> shape_text_out( $self-> text, $adj, ($H - $canvas-> font-> height) / 2);
}

sub on_mousedown
{
	# this code ( instead of listVisible(!listVisible)) is formed so because
	# ::InputLine is selectable, and unwilling focus() could easily hide
	# listBox automatically. Manual focus is also supported by
	# selectingButtons == 0.
	my $self = $_[0];
	my $lv = $self-> owner-> listVisible;
	$self-> owner-> listVisible(!$lv);
	$self-> focus if $lv;
	$self-> clear_event;
}

sub on_enter { $_[0]-> repaint; }
sub on_leave { $_[0]-> repaint; }

sub on_mouseclick
{
	$_[0]-> clear_event;
	return unless $_[5];
	shift-> notify(q(MouseDown), @_);
}


package Prima::DriveComboBox;
use vars qw(@ISA @images);
@ISA = qw(Prima::ComboBox);

sub profile_default
{
	my %sup = %{$_[ 0]-> SUPER::profile_default};
	return {
		%sup,
		style            => cs::DropDownList,
		height           => $sup{ editHeight},
		firstDrive       => 'A:',
		drive            => 'C:',
		editClass        => 'Prima::DriveComboBox::InputLine',
		listClass        => 'Prima::ListViewer',
		editProfile      => {},
	};
}

Prima::Application::add_startup_notification( sub {
	my $i = 0;
	for (
		sbmp::DriveFloppy, sbmp::DriveHDD,    sbmp::DriveNetwork,
		sbmp::DriveCDROM,  sbmp::DriveMemory, sbmp::DriveUnknown
	) {
		$images[ $i++] = Prima::StdBitmap::icon($_);
	}
});

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { style} = cs::DropDownList;
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self    = shift;
	my %profile = @_;
	$self-> {driveTransaction} = 0;
	$self-> {firstDrive} = $profile{firstDrive};
	$self-> {drives} = [split ' ', Prima::Utils::query_drives_map( $profile{firstDrive})];
	$self-> {icons} = [];
	my $maxhei = $profile{itemHeight};
	for ( @{$self-> {drives}}) {
		my $dt = Prima::Utils::query_drive_type($_) - dt::Floppy;
		$dt = -1 if $dt < 0;
		my $ic = $images[ $dt];
		push( @{$self-> {icons}}, $ic);
		$maxhei = $ic-> height if $ic && $ic-> height > $maxhei;
	}
	$profile{text} = $profile{drive};
	$profile{items} = [@{$self-> {drives}}];
	push (@{$profile{editDelegations}}, qw(KeyDown MouseEnter MouseLeave));
	push (@{$profile{listDelegations}}, qw(DrawItem FontChanged MeasureItem Stringify));
	%profile = $self-> SUPER::init(%profile);
	$self-> {drive} = $self-> text;
	$self-> {list}-> itemHeight( $maxhei);
	$self-> drive( $self-> {drive});
	return %profile;
}

sub on_change
{
	my $self = shift;
	return unless scalar @{$self-> {drives}};
	$self-> {driveTransaction} = 1;
	$self-> drive( $self-> {drives}-> [$self-> List-> focusedItem]);
	$self-> {driveTransaction} = undef;
}

sub drive
{
	return $_[0]-> {drive} unless $#_;
	return if $_[0]-> {drive} eq $_[1];
	if ( $_[0]-> {driveTransaction}) {
		$_[0]-> {drive} = $_[1];
		return;
	}
	$_[0]-> {driveTransaction} = 1;
	$_[0]-> text( $_[1]);
	my $d = $_[0]-> {drive};
	$_[0]-> {drive} = $_[0]-> text;
	$_[0]-> notify( q(Change)) if $d ne $_[0]-> text;
	$_[0]-> {driveTransaction} = 0;
}

sub InputLine_KeyDown
{
	my ( $combo, $self, $code, $key) = @_;
	$combo-> listVisible(1), $self-> clear_event if $key == kb::Down;
	return if $key != kb::NoKey;
	$code = uc chr($code) .':';
	($_[0]-> text( $code),
	$_[0]-> notify( q(Change)))
		if (scalar grep { $code eq $_ } @{$combo-> {drives}}) && ($code ne $_[0]-> text);
	$self-> clear_event;
}

sub InputLine_MouseEnter
{
	my $self = $_[1];
	$self->{colors} = [
		backColor       => $self-> backColor,
		hiliteBackColor => $self->hiliteBackColor,
		ownerBackColor  => $self->ownerBackColor,
	];
	$self->set(
		backColor       => $self-> prelight_color($self->backColor),
		hiliteBackColor => $self-> prelight_color($self->hiliteBackColor),
	);
}

sub InputLine_MouseLeave { $_[1]->set( @{ delete($_[1]->{colors}) // [] } ) }

sub List_DrawItem
{
	my ($combo, $me, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focused, $prelight) = @_;

	my ( $c, $bc);
	if ( $hilite || $prelight) {
		$c = $canvas-> color;
		$bc = $canvas-> backColor;
		$canvas-> color($hilite ? $me-> hiliteColor : $me->color);
		my $bk = $hilite ? $me-> hiliteBackColor : $me->backColor;
		$bk = $me->prelight_color($bk) if $prelight;
		$canvas-> backColor($bk);
	}
	$canvas-> clear( $left, $bottom, $right, $top);

	my $text = ${$combo-> {drives}}[$index];
	my $icon = ${$combo-> {icons}}[$index];
	my $font = $canvas-> font;
	my $x = $left + 2;
	my ($h, $w);

	if ( $icon) {
		($h, $w) = ($icon-> height, $icon-> width);
		$canvas-> put_image( $x, ($top + $bottom - $h) / 2, $icon);
		$x += $w + 4;
	}

	($h,$w) = ($font-> height, $canvas-> get_text_width( $text));
	$canvas-> shape_text_out( $text, $x, ($top + $bottom - $h) / 2);

	if ( $hilite || $prelight) {
		$canvas-> color( $c);
		$canvas-> backColor( $bc);
	}
}

sub List_FontChanged
{
	my ( $combo, $self) = @_;
	return unless $self-> {autoHeight};

	my $maxHei = $self-> font-> height;

	for ( @{$combo-> {icons}}) {
		next unless defined $_;
		$maxHei = $_-> height if $maxHei < $_-> height;
	}

	$self-> itemHeight( $maxHei);
	$self-> autoHeight(1);
}

sub List_MeasureItem
{
	my ( $combo, $self, $index, $sref) = @_;

	my $iw = ( $combo-> {icons}-> [$index] ? $combo-> {icons}-> [$index]-> width : 0);
	$$sref = $self-> get_text_width($combo-> {drives}[$index]) + $iw;
	$self-> clear_event;
}

sub List_Stringify
{
	my ( $combo, $self, $index, $sref) = @_;

	$$sref = $combo-> {drives}[$index];
	$self-> clear_event;
}


sub set_style { $_[0]-> raise_ro('set_style')}


package Prima::FileDialog;
use Prima::MsgBox;
use Cwd;
use vars qw( @ISA);
@ISA = qw( Prima::Dialog);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		width       => 635,
		height      => 410,
		sizeMin     => [380, 280],
		centered    => 1,
		visible     => 0,
		borderStyle => bs::Sizeable,

		defaultExt  => '',
		fileName    => '',
		filter      => [[ 'All files' => '*']],
		filterIndex => 0,
		directory   => '.',
		designScale => [ 8, 20],

		createPrompt       => 0,
		multiSelect        => 0,
		noReadOnly         => 0,
		noTestFileCreate   => 0,
		overwritePrompt    => 1,
		pathMustExist      => 1,
		fileMustExist      => 1,
		showHelp           => 0,
		sorted             => 1,
		showDotFiles       => 0,

		openMode           => 1,
		system             => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	unless ( $p-> {sizeMin}) {
		$p-> {sizeMin}-> [0] = $default-> {sizeMin}-> [0] * $p-> {width} / $default-> {width};
		$p-> {sizeMin}-> [1] = $default-> {sizeMin}-> [1] * $p-> {height} / $default-> {height};
	}
}

my $unix  = ($^O =~ /cygwin/) || (Prima::Application-> get_system_info-> {apc} == apc::Unix);
my $win32 = (Prima::Application-> get_system_info-> {apc} == apc::Win32);
my $gtk   = (Prima::Utils::get_gui == gui::GTK);

sub create
{
	my ( $class, %params) = @_;
	if ( $params{system} && ( $win32 || $gtk))  {
		my $sys = $win32 ? 'win32' : 'gtk';
		if ( $class =~ /^Prima::(Open|Save|File)Dialog$/) {
			undef $@;
			eval "use Prima::sys::${sys}::FileDialog";
			die $@ if $@;
			$class =~ s/(Prima)/$1::sys::$sys/;
			return $class-> create(%params);
		}
	}
	return $class-> SUPER::create(%params);
}

sub canonize_mask
{
	my $self = shift;
	my @ary = split ';', $self-> { mask};
	for (@ary)
	{
		s{^.*[:/\\]([^:\\/]*)$}{$1};
		s{([^\w*?])}{\\$1}g;
		s{\*}{.*}g;
		s{\?}{.?}g;
	}
	$self-> { mask} = "^(${\(join( '|', @ary))})\$";
}

sub canon_path
{
	my $p = shift;
	return Prima::DirectoryListBox::safe_abs_path($p) if -d $p;
	my $dir = $p;
	my $fn;
	if ($dir =~ s{[/\\]([^\\/]+)$}{}) {
		$fn = $1;
	} else {
		$fn = $p;
		$dir = '.';
	}
	unless ( scalar(stat($dir . (( !$unix && $dir =~ /:$/) ? '/' : '')))) {
		$dir = "";
	} else {
		$dir = Prima::DirectoryListBox::safe_abs_path($dir);
		$dir = "." if $@;
		$dir = "" unless -d $dir;
		$dir =~ s/(\\|\/)$//;
	}
	return "$dir/$fn";
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	my $drives = length( Prima::Utils::query_drives_map);
	$self-> {hasDrives} = $drives;

	for ( qw(
		defaultExt filter directory filterIndex showDotFiles
		createPrompt fileMustExist noReadOnly noTestFileCreate
		overwritePrompt pathMustExist showHelp openMode sorted
	)) {
		$self-> {$_} = $profile{$_}
	}

	@{$self-> {filter}}  = [[ '' => '*']] unless scalar @{$self-> {filter}};
	my @exts;
	my @mdts;
	for ( @{$self-> {filter}}) {
		push @exts, $$_[0];
		push @mdts, $$_[1];
	}
	$self-> { filterIndex} = scalar @exts - 1 if $self-> { filterIndex} >= scalar @exts;

	$self-> { mask} = $mdts[ $self-> { filterIndex}];
	$self-> { mask} = $profile{fileName} if $profile{fileName} =~ /[*?]/;
	$self-> canonize_mask;

	$self-> insert( InputLine =>
		name      => 'Name',
		origin    => [ 14, 343],
		size      => [ 245, 25],
		text      => $profile{fileName},
		maxLen    => 32768,
		delegations => [qw(KeyDown)],
		growMode    => gm::GrowLoY,
	);
	$self-> insert( Label=>
		origin    => [ 14 , 375],
		size      => [ 245, 25],
		focusLink => $self-> Name,
		text      => '~Filename',
		growMode  => gm::GrowLoY,
		name      => 'NameLabel',
	);

	$self-> insert( ListBox  =>
		name        => 'Files',
		origin      => [ 14,  85 ],
		size        => [ 245, 243],
		multiSelect => $profile{ multiSelect},
		delegations => [qw(KeyDown SelectItem Click)],
		growMode    => gm::GrowHiY,
	);

	$self-> insert( ComboBox =>
		name    => 'Ext',
		origin  => [ 14 , 25],
		size    => [ 245, 25],
		style   => cs::DropDownList,
		items   => [ @exts],
		text    => $exts[ $self-> { filterIndex}],
		delegations => [qw(Change)],
	);

	$self-> insert( Label=>
		origin    => [ 14, 55],
		size      => [ 245, 25],
		focusLink => $self-> Ext,
		text   => '~Extensions',
		name   => 'ExtensionsLabel',
	);

	$self-> insert( Label =>
		name      => 'Directory',
		origin    => [ 275, 343],
		size      => [ 235, 25],
		autoWidth => 0,
		text      => $profile{ directory},
		delegations => [qw(FontChanged)],
		growMode    => gm::GrowLoY,
	);

	$self-> insert( DirectoryListBox =>
		name       => 'Dir',
		origin     => [ 275, $drives ? 85 : 25],
		size       => [ 235, $drives ? 243 : 303],
		path       => $self-> { directory},
		delegations=> [qw(Change)],
		showDotDirs=> $self-> {showDotFiles},
		growMode   => gm::GrowHiY,
	);

	$self-> insert( DriveComboBox =>
		origin     => [ 275, 25],
		size       => [ 235, 25],
		name       => 'Drive',
		drive      => $self-> Dir-> path,
		delegations=> [qw(Change)],
	) if $drives;

	$self-> insert( Label=>
		origin     => [ 275, 375],
		size       => [ 235, 25],
		text       => 'Di~rectory',
		focusLink  => $self-> Dir,
		growMode   => gm::GrowLoY,
		name       => 'DirectoryLabel',
	);

	$self-> insert( Label =>
		origin    => [ 275, 55],
		size      => [ 235, 25],
		text      => '~Drives',
		focusLink => $self-> Drive,
		name      => 'DriveLabel',
	) if $drives;

	my $button = $self-> insert( Button=>
		origin  => [ 524, 350],
		size    => [ 96, 36],
		text => $self-> {openMode} ? '~Open' : '~Save',
		name    => 'Open',
		default => 1,
		delegations => [qw(Click)],
		growMode    => gm::GrowLoX | gm::GrowLoY,
	);
	$self-> {right_margin}  = $self-> width - $button-> left;

	$self-> insert( Button=>
		origin      => [ 524, 294],
		name    => 'Cancel',
		text    => 'Cancel',
		size        => [ 96, 36],
		modalResult => mb::Cancel,
		growMode    => gm::GrowLoX | gm::GrowLoY,
	);
	$self-> insert( Button=>
		origin      => [ 524, 224],
		name        => 'Help',
		text        => '~Help',
		size        => [ 96, 36],
		growMode    => gm::GrowLoX | gm::GrowLoY,
	) if $self-> {showHelp};

	$self-> Name-> current(1);
	$self-> Name-> select_all;
	$self-> {curpaths} = {};

	if ( $drives) {
		for ( @{$self-> Drive-> items}) { $self-> {curpaths}-> {lc $_} = $_}
		$self-> {curpaths}-> {lc $self-> Drive-> drive} = $self-> Dir-> path;
		$self-> Drive-> {lastDrive} = $self-> Drive-> drive;
	}
	return %profile;
}

sub on_create
{
	my $self = $_[0];
	$self-> Dir_Change( $self-> Dir);
}

sub on_size
{
	my ( $self, $ox, $oy, $x, $y) = @_;

	my ( $w, $dx, @left, @right);

	$dx = $self-> Files-> left;
	$x -= $self-> {right_margin};
	$w = ( $x - 3 * $dx ) / 2;
	$_-> width( $w) for
		grep { defined } map { $self-> bring($_) }
		qw(Files Name NameLabel Ext ExtensionsLabel CompletionList);
	$x = 2 * $dx + $w;
	$_-> set( left => $x, width => $w) for
		grep { defined } map { $self-> bring($_) }
		qw(Directory DirectoryLabel Dir Drive DriveLabel);
}

sub on_show
{
	my $self = $_[0];
	$self-> Dir_Change( $self-> Dir);
}

sub on_endmodal
{
	$_[0]-> hide_completions;
}

sub execute
{
	return ($_[0]-> SUPER::execute != mb::Cancel) ?
		$_[0]-> fileName :
		( wantarray ? () : undef);
}

sub hide_completions
{
	if ( $_[0]-> {completionList}) {
		$_[0]-> {completionList}-> destroy;
		delete $_[0]-> {completionList};
	}
}

sub Name_KeyDown
{
	my ( $dlg, $self, $code, $key, $mod) = @_;

	if (($key == kb::Tab) && !($mod & km::Ctrl)) {
		$self-> clear_event;
		my $f = $self-> text;
		substr( $f, $self-> selStart) = ''
			if $self-> selStart == $self-> charOffset &&
				$self-> selEnd == length $f;
		$f =~ s/^\s*//;
		$f =~ s/\\\s/ /g;
		$f =~ s/^~/$ENV{HOME}/ if $f =~ m/^~/ && defined $ENV{HOME};
		my $relative;
		$f = $dlg-> Dir-> path .  $f, $relative = 1 if
			($unix && $f !~ /^\//) ||
			(!$unix && $f !~ /^([a-z]\:|\/)/i);
		$f =~ s/\\/\//g;
		my $path = $f;
		my $rel_path = $relative ? substr($path, length($dlg-> Dir-> path)) : $path;
		$path =~ s/(^|\/)[^\/]*$/$1/;
		$rel_path =~ s/(^|\/)[^\/]*$/$1/;
		my $residue = substr( $f, length $path);
		if ( -d $path) {
			my $i;
			my @fs = Prima::Utils::getdir( $path);
			my @completions;
			my $mask = $dlg-> {mask};
			for ( $i = 0; $i < scalar @fs; $i += 2) {
				next if !$dlg-> {showDotFiles} && $fs[$i] =~ /^\./;
				next if substr( $fs[$i], 0, length $residue) ne $residue;
				$fs[ $i + 1] = 'dir' if $fs[ $i + 1] eq 'lnk' && -d $path.$fs[$i];
				next if $fs[ $i + 1] ne 'dir' && $fs[$i] !~ /$mask/i;
				push @completions, $fs[$i] . (( $fs[ $i + 1] eq 'dir') ? '/' : '');
			}
			s/\s/\\ /g for @completions;
			if ( 1 == scalar @completions) {
				$self-> text( $rel_path . $completions[0]);
				$i = length( $rel_path) + length( $residue );
				$self-> selection( $i, length($rel_path) + length($completions[0]));
				$self-> charOffset( $i);
			} elsif ( 1 < scalar @completions) {
				unless ( $dlg-> {completionList}) {
					$dlg-> {completionList} = Prima::ListBox-> create(
						owner       => $dlg,
						width       => $self-> width,
						bottom      => $dlg-> Files-> bottom,
						top         => $self-> bottom - 1,
						left        => $self-> left,
						designScale => undef,
						name        => 'CompletionList',
						delegations => [qw(SelectItem KeyDown Click)],
						growMode    => gm::GrowHiY,
					);
					$dlg-> {completionMatch} = '';
					$dlg-> {completionListIndex} = 0;
				}
				if (
					$dlg-> {completionMatch} eq $rel_path &&
					defined $completions[$dlg-> {completionListIndex}] &&
					defined $dlg-> {completionList}-> get_items($dlg-> {completionListIndex}) &&
					$dlg-> {completionList}-> get_items($dlg-> {completionListIndex}) eq
						$completions[$dlg-> {completionListIndex}]
				) {
					$dlg-> {completionList}-> focusedItem($dlg-> {completionListIndex});
					$f = $rel_path . $completions[$dlg-> {completionListIndex}];
					$self-> text( $f);
					$i = length( $rel_path) + length( $residue);
					$self-> selection( $i , length $f);
					$self-> charOffset( $i);
					$dlg-> {completionListIndex} = 0 if ++$dlg-> {completionListIndex} >= @completions;
				} else {
					$dlg-> {completionListIndex} = 0;
				}
				$dlg-> {completionList}-> items( \@completions);
				$dlg-> {completionList}-> bring_to_front;
			} elsif ($dlg-> {completionList}) {
				$dlg-> {completionList}-> items([]);
				$dlg-> {completionListIndex} = 0;
			}
			$dlg-> {completionMatch} = $rel_path;
			$dlg-> {completionPath}  = $path;
		}
	} elsif ( $key == kb::Esc && $dlg-> {completionList}) {
		$dlg-> {completionList}-> destroy;
		delete $dlg-> {completionList};
		$self-> clear_event;
		my $f = $self-> text;
		if ( $self-> selStart == $self-> charOffset && $self-> selEnd == length $f) {
			substr( $f, $self-> selStart) = '';
			$self-> text( $f);
		}
	}
}

sub CompletionList_Click
{
	my ( $self, $lst) = @_;
	$self-> Name_text( $self-> {completionMatch} . $lst-> get_items($lst-> focusedItem));
	$self-> hide_completions;
	$self-> Name-> select;
}

sub CompletionList_SelectItem
{
	my ( $self, $lst) = @_;
	my $text = $lst-> get_items($lst-> focusedItem);
	$self-> Name_text( $self-> {completionMatch} . $text);
	if ( $self-> {completionPath} eq $self-> Dir-> path) { # simulate Files walk
		my $f = $self-> Files;
		my $c = $f-> count;
		for ( my $i = 0; $i < $c; $i++) {
			next unless $f-> get_item_text($i) eq $text;
			$f-> focusedItem( $i);
			last;
		}
	}
}

sub CompletionList_KeyDown
{
	my ( $dlg, $self, $code, $key, $mod) = @_;
	if ( $key == kb::Esc) {
		$self-> clear_event;
		$dlg-> hide_completions;
		$dlg-> Name-> select;
	} elsif ( $key == kb::Enter) {
		$dlg-> Name_text( $dlg-> {completionMatch} . $self-> get_items($self-> focusedItem));
		$self-> clear_event;
		$dlg-> hide_completions;
		$dlg-> Name-> select;
	}
}

sub Files_KeyDown
{
	my ( $dlg, $self, $code, $key, $mod) = @_;
	if ( $code == ord("\cR"))
	{
		$dlg-> Dir-> path( $dlg-> Dir-> path);
		$self-> clear_event;
	}
}

sub Directory_FontChanged
{
	my ( $self, $dc) = @_;
	my ( $w, $path) = ( $dc-> width, $self-> Dir-> path);
	if ( $w < $dc-> get_text_width( $path)) {
		$path =~ s{(./)}{$1...};
		while ( $w < $dc-> get_text_width( $path)) { $path =~ s{(./\.\.\.).}{$1}};
	}
	$dc-> text( $path);
}

sub Dir_Change
{
	my ( $self, $dir) = @_;
	my $mask = $self-> {mask};
	my @a = grep { /$mask/i; } $dir-> files( 'reg');
	@a = grep { !/^\./ } @a unless $self-> {showDotFiles};
	@a = sort {uc($a) cmp uc($b)} @a if $self-> {sorted};
	$self-> Files-> topItem(0);
	$self-> Files-> focusedItem(-1);
	$self-> Files-> items([@a]);
	$self-> Directory_FontChanged( $self-> Directory);
}

sub Drive_Change
{
	my ( $self, $drive) = @_;
	my $newDisk = $drive-> text . "/";
	until (-d $newDisk) {
		last if Prima::MsgBox::message_box( $self-> text, "Drive $newDisk is not ready!",
						mb::Cancel | mb::Retry | mb::Warning) != mb::Retry;
	}
	unless (-d $newDisk) {
		$drive-> drive($drive-> {lastDrive});
		$drive-> clear_event;
		return;
	}
	my $path = $self-> Dir-> path;
	my $drv  = lc substr($path,0,2);
	$self-> {curpaths}-> {$drv} = $path;
	$self-> Dir-> path( $self-> {curpaths}-> {lc $drive-> text});
	if ( lc $drive-> text ne lc substr($self-> Dir-> path,0,2)) {
		$drive-> drive( $self-> Dir-> path);
	}
	$drive-> {lastDrive} = $drive-> drive;
}

sub Ext_Change
{
	my ( $self, $ext) = @_;
	my %cont;
	for ( @{$self-> {filter}}) { $cont{$$_[0]} = $$_[1]; };
	$self-> {mask} = $cont{ $ext-> text};
	$self-> canonize_mask;
	$self-> Dir_Change( $self-> Dir);
	$self-> {filterIndex} = $ext-> List-> focusedItem;
}

sub Files_SelectItem
{
	my ( $self, $lst) = @_;
	my @items = $lst-> get_items($lst-> selectedItems);
	$self-> Name_text( scalar(@items) ? @items : '');
}

sub Files_Click
{
	my $self = shift;
	$self-> Files_SelectItem( @_);
	$self-> Open_Click( $self-> Open);
}

sub quoted_split
{
	my @ret;
	$_ = $_[0];
	s/(\\[^\\\s])/\\$1/g;
	study;
	{
		/\G\s+/gc && redo;
		/\G((?:[^\\\s]|\\.)+)\s*/gc && do {
			my $z = $1;
			$z =~ s/\\(.)/$1/g;
			push(@ret, $z);
			redo;
		};
		/\G(\\)$/gc && do { push(@ret, $1); redo; };
	}
	return @ret;
}

sub Name_text
{
	my $text = '';
	my $self;
	for ( @_) {
		$self = $_, next unless defined $self;
		my $x = $_;
		$x =~ s/(\s|\\)/\\$1/g;
		$text .= $x;
		$text .= ' ';
	}
	chop $text;
	$self-> Name-> text( $text);
}


sub Open_Click
{
	my $self = shift;
	$self-> hide_completions;
	$_ = $self-> Name-> text;
	my @files;
	if ( $self-> multiSelect) {
		@files = quoted_split( $_);
	} else {
		s/\\([\\\s])/$1/g;
		@files = ($_);
	}
	return unless scalar @files;
	@files = ($files[ 0]) if ( !$self-> multiSelect and scalar @files > 1);
	(@files = grep {/[*?]/} @files), @files = ($files[ 0]) if /[*?]/;
	my %uniq; @files = grep { !$uniq{$_}++ } @files;

	# validating names
	for ( @files) {
		s{\\}{/}g;
		s/^~/$ENV{HOME}/ if m/^~/ && defined $ENV{HOME};
		if ( $unix) {
			$_ = $self-> directory . $_ unless m{^/};
		} else {
			$_ = $self-> directory . $_ unless m{^/|[A-Za-z]:};
			$_ .= '/' if !$unix && m/^[A-Za-z]:$/;
		}
		my $pwd = cwd; chdir $self-> directory;
		$_ = canon_path($_);
		chdir $pwd;
	}

	# testing for indirect directory/mask use
	if ( scalar @files == 1) {
		# have single directory
		if ( -d $files[ 0]) {
			my %cont;
			for ( @{$self-> {filter}}) { $cont{$$_[0]} = $$_[1]};
			$self-> directory( $files[ 0]);
			$self-> Name-> text('');
			$self-> Name-> focus;
			return;
		}
		my ( $dirTo, $fileTo) = ( $files[ 0] =~ m{^(.*[:/\\])([^:\\/]*)$});
		$dirTo  or $dirTo = '';
		$fileTo = $files[ 0] unless $fileTo || $dirTo;
		# $fileTo =~ s/^\s*(.*)\s*$/$1/;
		# $dirTo =~ s/^\s*(.*)\s*$/$1/;

		# have directory with mask
		if ( $fileTo =~ /[*?]/)
		{
			my @masked = grep {
				/[*?]/
			} map {
				m{([^/\\]*)$} ? $1 : $_
			} grep {
				/[*?]/
			} @files;
			$self-> Name_text( @masked);
			$self-> {mask} = join( ';', @masked);
			$self-> canonize_mask;
			$self-> directory( $dirTo);
			$self-> Name-> focus;
			return;
		}
		if ( $dirTo =~ /[*?]/) {
			Prima::MsgBox::message_box(
				$self-> text,
				"Invalid path name " . $self-> Name-> text,
				mb::OK | mb::Error
			);
			$self-> Name-> select_all;
			$self-> Name-> focus;
			return;
		}
	}

	if (( 1 == scalar(@files)) && !($files[0] =~ m/\./)) {
# check if can authomatically add an extension
		for ( split(';', $self-> {filter}-> [$self-> {filterIndex}]-> [1])) {
			next unless m/^[\*\.]*([^;\.\*]+)/;
			my $f = $files[0] . '.' . $1;
			$files[0] = $f, last if !$self-> {openMode} || -f $f;
		}
	}

# possible commands recognized, treating names as files
	for ( @files) {
		$_ .= $self-> {defaultExt} if $self-> {openMode} && !m{\.[^/]*$};
		if ( -f $_) {
			if ( !$self-> {openMode} && $self-> {noReadOnly} && !(-w $_)) {
				Prima::MsgBox::message_box(
					$self-> text,
					"File $_ is read only",
					mb::OK | mb::Error
				);
				$self-> Name-> select_all;
				$self-> Name-> focus;
				return;
			}
			return if !$self-> {openMode} && $self-> {overwritePrompt} && (
					Prima::MsgBox::message_box( $self-> text,
					"File $_ already exists. Overwrite?",
					mb::OKCancel|mb::Warning) != mb::OK);

		} else {
			my ( $dirTo, $fileTo) = ( m{^(.*[:/\\])([^:\\/]*)$});
			$dirTo = '.', $fileTo = $_ unless defined $dirTo;
			if ( $self-> {openMode} && $self-> {createPrompt}) {
				return if ( Prima::MsgBox::message_box( $self-> text,
					"File $_ does not exists. Create?",
					mb::OKCancel|mb::Information
				) != mb::OK);
				if ( open FILE, ">$_") {
					close FILE;
				} else {
					Prima::MsgBox::message_box( $self-> text,
						"Cannot create file $_: $!",
						mb::OK | mb::Error
					);
					$self-> Name-> select_all;
					$self-> Name-> focus;
					return;
				}
			}
			if ( $self-> {pathMustExist} and !( -d $dirTo)) {
				Prima::MsgBox::message_box( $self-> text, "Directory $dirTo does not exist", mb::OK | mb::Error);
				$self-> Name-> select_all;
				$self-> Name-> focus;
				return;
			}
			if ( $self-> {fileMustExist} and !( -f $_)) {
				Prima::MsgBox::message_box( $self-> text, "File $_ does not exist", mb::OK | mb::Error);
				$self-> Name-> select_all;
				$self-> Name-> focus;
				return;
			}
		}
		if ( !$self-> {openMode} && !$self-> {noTestFileCreate}) {
			if ( open FILE, ">>$_") {
				close FILE;
			} else {
				Prima::MsgBox::message_box( $self-> text, "Cannot create file $_: $!", mb::OK | mb::Error);
				$self-> Name-> select_all;
				$self-> Name-> focus;
				return;
			}
		}
	};
# flags & files processed, ending process
	$self-> Name_text( @files);
	$self-> ok;
}

sub filter
{
	if ( $#_) {
		my $self   = $_[0];

		my @filter = @{$_[1]};
		@filter = [[ '' => '*']] unless scalar @filter;
		my @exts;
		my @mdts;
		for ( @filter) {
			push @exts, $$_[0];
			push @mdts, $$_[1];
		}
		$self-> { filterIndex} = scalar @exts - 1 if $self-> { filterIndex} >= scalar @exts;
		$self-> {filter} = \@filter;

		$self-> { mask} = $mdts[ $self-> { filterIndex}];
		$self-> { mask} = '*' unless defined $self-> { mask};
		$self-> canonize_mask;

		$self-> Ext-> items( \@exts);
		$self-> Ext-> text( $exts[$self-> { filterIndex}]);
	} else {
		return @{$_[0]-> {filter}};
	}
}

sub filterIndex
{
	if ( $#_) {
		return if $_[1] == $_[0]-> Ext-> focusedItem;
		$_[0]-> Ext-> focusedItem( $_[1]);
		$_[0]-> Ext-> notify(q(Change));
	} else {
		return $_[0]-> {filterIndex};
	}
}

sub directory
{
	return $_[0]-> Dir-> path unless $#_;
	$_[0]-> Dir-> path($_[1]);
	$_[0]-> Drive-> text( $_[0]-> Dir-> path) if $_[0]-> {hasDrives};
}

sub fileName
{
	$_[0]-> Name_text($_[1]), return if ($#_);
	my @s = quoted_split( $_[0]-> Name-> text);
	return $s[0] unless wantarray;
	return @s;
}

sub sorted
{
	return $_[0]-> {sorted} unless $#_;
	return if $_[0]-> {sorted} == $_[1];
	$_[0]-> {sorted} = $_[1];
	$_[0]-> Dir_Change( $_[0]-> Dir);
}

sub reread
{
	$_[0]-> Dir_Change( $_[0]-> Dir);
}

sub showDotFiles
{
	return $_[0]-> {showDotFiles} unless $#_;
	my ( $self, $show) = @_;
	$show =  ( $show ? 1 : 0);
	return if $show == $self-> {showDotFiles};
	$self-> {showDotFiles} = $show;
	$self-> Dir-> showDotDirs($show);
	$self-> reread;
}

sub multiSelect      { ($#_)? $_[0]-> Files-> multiSelect($_[1]) : return $_[0]-> Files-> multiSelect };
sub createPrompt     { ($#_)? $_[0]-> {createPrompt} = ($_[1])  : return $_[0]-> {createPrompt} };
sub noReadOnly       { ($#_)? $_[0]-> {noReadOnly}   = ($_[1])  : return $_[0]-> {noReadOnly} };
sub noTestFileCreate { ($#_)? $_[0]-> {noTestFileCreate}   = ($_[1])  : return $_[0]-> {noTestFileCreate} };
sub overwritePrompt  { ($#_)? $_[0]-> {overwritePrompt}   = ($_[1])  : return $_[0]-> {overwritePrompt} };
sub pathMustExist    { ($#_)? $_[0]-> {pathMustExist}     = ($_[1])  : return $_[0]-> {pathMustExist} };
sub fileMustExist    { ($#_)? $_[0]-> {fileMustExist}   = ($_[1])  : return $_[0]-> {fileMustExist} };
sub defaultExt       { ($#_)? $_[0]-> {defaultExt}   = ($_[1])  : return $_[0]-> {defaultExt} };
sub showHelp         { ($#_)? shift-> raise_ro('showHelp')  : return $_[0]-> {showHelp} };
sub openMode         { $_[0]-> {openMode} }

package Prima::OpenDialog;
use vars qw( @ISA);
@ISA = qw( Prima::FileDialog);

sub profile_default {
	return { %{$_[ 0]-> SUPER::profile_default},
		text  => 'Open file',
		openMode => 1,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { openMode} = 1;
	$self-> SUPER::profile_check_in( $p, $default);
}

package Prima::SaveDialog;
use vars qw( @ISA);
@ISA = qw( Prima::FileDialog);

sub profile_default  {
	return { %{$_[ 0]-> SUPER::profile_default},
		text         => 'Save file',
		openMode        => 0,
		fileMustExist   => 0,
	}
}
sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { openMode} = 0;
	$self-> SUPER::profile_check_in( $p, $default);
}

package Prima::ChDirDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		width       => 500,
		height      => 236,
		centered    => 1,
		visible     => 0,
		text        => 'Change directory',

		directory   => '',
		designScale => [7, 16],
		showHelp    => 0,
		showDotDirs => 0,

		borderStyle => bs::Sizeable,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $j;
	my $drives = length( Prima::Utils::query_drives_map);
	$self-> {hasDrives} = $drives;

	for ( qw( showHelp directory showDotDirs)) { $self-> {$_} = $profile{$_} }

	$self-> insert( DirectoryListBox =>
		origin   => [ 10, 40],
		width    => 480,
		growMode => gm::Client,
		height   => 160,
		name     => 'Dir',
		current  => 1,
		path     => $self-> { directory},
		delegations => [qw(KeyDown)],
		showDotDirs => $self-> {showDotDirs},
	);

	$self-> insert( Label =>
		name      => 'Directory',
		origin    => [ 10, 202],
		growMode => gm::GrowLoY,
		autoWidth => 1,
		autoHeight => 1,
		text      => '~Directory',
		focusLink => $self-> Dir,
	);

	$self-> insert( DriveComboBox =>
		origin => [ 10, 10],
		width  => 150,
		name   => 'Drive',
		drive  => $self-> Dir-> path,
		delegations => [qw(Change)],
	) if $drives;

	$self-> insert( Button =>
		origin  => [ 200, 3],
		size        => [ 80, 30],
		text    => '~OK',
		name    => 'OK',
		default => 1,
		delegations => [qw(Click)],
	);

	$self-> insert( Button=>
		origin      => [ 300, 3],
		name        => 'Cancel',
		text        => 'Cancel',
		size        => [ 80, 30],
		modalResult => mb::Cancel,
	);

	$self-> insert( Button=>
		origin      => [ 400, 3],
		name        => 'Help',
		text        => '~Help',
		size        => [ 80, 30],
	) if $self-> {showHelp};

	$self-> {curpaths} = {};
	if ( $drives) {
		for ( @{$self-> Drive-> items}) { $self-> {curpaths}-> {lc $_} = $_}
		$self-> {curpaths}-> {lc $self-> Drive-> drive} = $self-> Dir-> path;
		$self-> Drive-> {lastDrive} = $self-> Drive-> drive;
	}

	return %profile;
}



sub Dir_KeyDown
{
	my ( $dlg, $self, $code, $key, $mod) = @_;
	if ( $code == ord("\cR")) {
		$dlg-> Dir-> path( $dlg-> Dir-> path);
		$self-> clear_event;
	}
}

sub Drive_Change
{
	my ( $self, $drive) = @_;
	my $newDisk = $drive-> text . "/";
	until (-d $newDisk) {
		last if Prima::MsgBox::message_box(
			$self-> text,
			"Drive $newDisk is not ready!",
			mb::Cancel | mb::Retry | mb::Warning
		) != mb::Retry;
	}
	unless (-d $newDisk) {
		$drive-> drive($drive-> {lastDrive});
		$drive-> clear_event;
		return;
	}
	my $path = $self-> Dir-> path;
	my $drv  = lc substr($path,0,2);
	$self-> {curpaths}-> {$drv} = $path;
	$self-> Dir-> path( $self-> {curpaths}-> {lc $drive-> text});
	if ( lc $drive-> text ne lc substr($self-> Dir-> path,0,2)) {
		$drive-> drive( $self-> Dir-> path);
	}
	$drive-> {lastDrive} = $drive-> drive;
}

sub OK_Click
{
	my $self = $_[0];
	$self-> ok;
}

sub directory
{
	return $_[0]-> Dir-> path unless $#_;
	$_[0]-> Dir-> path($_[1]);
	$_[0]-> Drive-> text( $_[0]-> Dir-> path) if $_[0]-> {hasDrives};
}

sub showHelp         { ($#_)? shift-> raise_ro('showHelp')  : return $_[0]-> {showHelp} };

sub showDotDirs
{
	return $_[0]-> {showDotDirs} unless $#_;
	my ( $self, $show) = @_;
	$show =  ( $show ? 1 : 0);
	return if $show == $self-> {showDotDirs};
	$self-> {showDotDirs} = $show;
	$self-> Dir-> showDotDirs($show);
}

1;

=head1 NAME

Prima::FileDialog - File system related widgets and dialogs.

=head1 SYNOPSIS

# open a file
	use Prima qw(Application StdDlg);

	my $open = Prima::OpenDialog-> new(
		filter => [
			['Perl modules' => '*.pm'],
			['All' => '*']
		]
	);
	print $open-> fileName, " is to be opened\n" if $open-> execute;

	# save a file
	my $save = Prima::SaveDialog-> new(
		fileName => $open-> fileName,
	);
	print $save-> fileName, " is to be saved\n" if $save-> execute;

	# open several files
	$open-> multiSelect(1);
	print $open-> fileName, " are to be opened\n" if $open-> execute;

=for podview <img src="filedlg.gif" cut=1>

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/filedlg.gif">

=head1 DESCRIPTION

The module contains widgets for file and drive selection,
and also standard open file, save file, and change directory
dialogs.

=head1 Prima::DirectoryListBox

A directory listing list box. Shows the list of
subdirectories and upper directories, hierarchy-mapped,
with the folder images and outlines.

=head2 Properties

=over

=item closedGlyphs INTEGER

Number of horizontal equal-width images, contained in L<closedIcon>
property.

Default value: 1

=item closedIcon ICON

Provides an icon representation
for the directories, contained in the current directory.

=item indent INTEGER

A positive integer number of pixels, used for offset of
the hierarchy outline.

Default value: 12

=item openedGlyphs INTEGER

Number of horizontal equal-width images, contained in L<openedIcon>
property.

Default value: 1

=item openedIcon OBJECT

Provides an icon representation
for the directories, contained in the directories above the current
directory.

=item path STRING

Runtime-only property. Selects a file system path.

=item showDotDirs BOOLEAN

Selects if the directories with the first dot character
are shown the view. The treatment of the dot-prefixed names
as hidden is traditional to unix, and is of doubtful use under
win32.

Default value: 1

=back

=head2 Methods

=over

=item files [ FILE_TYPE ]

If FILE_TYPE value is not specified, the list of all files in the
current directory is returned. If FILE_TYPE is given, only the files
of the types are returned. The FILE_TYPE is a string, one of those
returned by C<Prima::Utils::getdir> ( see L<Prima::Utils/getdir>.

=back

=head1 Prima::DriveComboBox

Provides drive selection combo-box for non-unix systems.

=head2 Properties

=over

=item firstDrive DRIVE_LETTER

Create-only property.

Default value: 'A:'

DRIVE_LETTER can be set to other value to start the drive enumeration from.
Some OSes can probe eventual diskette drives inside the drive enumeration
routines, so it might be reasonable to set DRIVE_LETTER to C<C:> string
for responsiveness increase.

=item drive DRIVE_LETTER

Selects the drive letter.

Default value: 'C:'

=back

=head1 Prima::FileDialog

Provides a standard file dialog, allowing to navigate by the
file system and select one or many files. The class can
operate in two modes - 'open' and 'save'; these modes are
set by L<Prima::OpenDialog> and L<Prima::SaveDialog>.
Some properties behave differently depending on the mode,
which is stored in L<openMode> property.

=head2 Properties

=over

=item createPrompt BOOLEAN

If 1, and a file selected is nonexistent, asks the user
if the file is to be created.

Only actual when L<openMode> is 1.

Default value: 0

=item defaultExt STRING

Selects the file extension, appended to the
file name typed by the user, if the extension is not given.

Default value: ''

=item directory STRING

Selects the currently selected directory.

=item fileMustExist BOOLEAN

If 1, ensures that the file typed by the user exists before
closing the dialog.

Default value: 1

=item fileName STRING, ...

For single-file selection, assigns the selected file name,
For multiple-file selection, on get-call returns list of the selected
files; on set-call, accepts a single string, where the file names
are separated by the space character. The eventual space characters
must be quoted.

=item filter ARRAY

Contains array of arrays of string pairs, where each pair describes
a file type. The first scalar in the pair is the description of
the type; the second is a file mask.

Default value: [[ 'All files' => '*']]

=item filterIndex INTEGER

Selects the index in L<filter> array of the currently selected file type.

=item multiSelect BOOLEAN

Selects whether the user can select several ( 1 ) or one ( 0 ) file.

See also: L<fileName>.

=item noReadOnly BOOLEAN

If 1, fails to open a file when it is read-only.

Default value: 0

Only actual when L<openMode> is 0.

=item noTestFileCreate BOOLEAN

If 0, tests if a file selected can be created.

Default value: 0

Only actual when L<openMode> is 0.

=item overwritePrompt BOOLEAN

If 1, asks the user if the file selected is to be overwrittten.

Default value: 1

Only actual when L<openMode> is 0.

=item openMode BOOLEAN

Create-only property.

Selects whether the dialog operates in 'open' ( 1 ) mode or 'save' ( 0 )
mode.

=item pathMustExist BOOLEAN

If 1, ensures that the path, types by the user, exists before
closing the dialog.

Default value: 1

=item showDotFiles BOOLEAN

Selects if the directories with the first dot character
are shown the files view.

Default value: 0

=item showHelp BOOLEAN

Create-only property. If 1, 'Help' button is inserted in the dialog.

Default value: 1

=item sorted BOOLEAN

Selects whether the file list appears sorted by name ( 1 ) or not ( 0 ).

Default value : 1

=item system BOOLEAN

Create-only property. If set to 1, C<Prima::FileDialog> returns
instance of C<Prima::sys::XXX::FileDialog> system-specific file dialog,
if available for the I<XXX> platform.

C<system> knows only how to map C<FileDialog>, C<OpenDialog>, and C<SaveDialog>
classes onto the system-specific file dialog classes; the inherited classes
are not affected.

=back

=head2 Methods

=over

=item reread

Re-reads the currently selected directory.

=back

=head1 Prima::OpenDialog

Descendant of L<Prima::FileDialog>, tuned for open-dialog functionality.

=head1 Prima::SaveDialog

Descendant of L<Prima::FileDialog>, tuned for save-dialog functionality.

=head1 Prima::ChDirDialog

Provides standard dialog with interactive directory selection.

=head2 Properties

=over

=item directory STRING

Selects the directory

=item showDotDirs

Selects if the directories with the first dot character
are shown the view.

Default value: 0

=item showHelp

Create-only property. If 1, 'Help' button is inserted in the dialog.

Default value: 1

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Window>, L<Prima::Lists>,
F<examples/drivecombo.pl>, F<examples/launch.pl>.

=cut

