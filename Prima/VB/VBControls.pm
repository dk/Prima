package Prima::VB::VBControls;

use strict;
use warnings;
use Prima::Classes;
use Prima::Lists;

package Prima::VB::Divider;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		vertical       => 0,
		growMode       => gm::GrowHiX,
		pointerType    => cr::SizeNS,
		min            => 0,
		max            => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}


sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	my $vertical = exists $p-> {vertical} ? $p-> {vertical} : $default-> { vertical};
	if ( $vertical)
	{
		$p-> { growMode} = gm::GrowLoX | gm::GrowHiY if !exists $p-> { growMode};
		$p-> { pointerType } = cr::SizeWE if !exists $p-> { pointerType};
	}
}


sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	for ( qw( vertical min max))
		{ $self-> {$_} = 0; }
	for ( qw( vertical min max))
		{ $self-> $_( $profile{ $_}); }
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @sz = $canvas-> size;
	$self-> rect3d( 0,0,$sz[0]-1,$sz[1]-1,1,
		$self-> light3DColor,$self-> dark3DColor,$self-> backColor);
	my $v = $self-> {vertical};
	return if $sz[ $v ? 0 : 1] < 4;
	$self-> color($self-> light3DColor);
	my $d = int($sz[ $v ? 0 : 1] / 2);
	$v ?
		$self-> line($d, 2, $d, $sz[1]-3) :
		$self-> line(2, $d, $sz[0]-3, $d);
	$self-> color($self-> dark3DColor);
	$d++;
	$v ?
		$self-> line($d, 2, $d, $sz[1]-3) :
		$self-> line(2, $d, $sz[0]-3, $d);
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if ( $btn == mb::Left && !$self-> {drag}) {
		$self-> capture( 1, $self-> owner);
		$self-> {drag} = 1;
		$self-> {pos}  = $self-> {vertical} ? $x : $y;
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	if ( $btn == mb::Left && $self-> {drag}) {
		$self-> capture(0);
		$self-> {drag} = 0;
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	if ( $self-> {drag}) {
		if ( $self-> {vertical}) {
			my $np = $self-> left - $self-> {pos} + $x;
			my $w = $self-> owner-> width;
			my $ww = $self-> width;
			$np = $self-> {min} if $np < $self-> {min};
			$np = $w - $self-> {max} - $ww if $np > $w - $self-> {max} - $ww;
			$self-> left( $np);
			$self-> notify( q(Change));
		} else {
			my $np = $self-> bottom - $self-> {pos} + $y;
			my $w  = $self-> owner-> height;
			my $ww = $self-> height;
			$np = $self-> {min} if $np < $self-> {min};
			$np = $w - $self-> {max} - $ww if $np > $w - $self-> {max} - $ww;
			$self-> bottom( $np);
			$self-> notify( q(Change));
		}
	}
}

sub vertical
{
	if ( $#_) {
		return if $_[1] == $_[0]-> {vertical};
		$_[0]-> {vertical} = $_[1];
		$_[0]-> repaint;
	} else {
		return $_[0]-> {vertical};
	}
}

sub min
{
	if ( $#_) {
		my $mp = $_[1];
		return if $_[0]-> {min} == $mp;
		$_[0]-> {min} = $mp;
	} else {
		return $_[0]-> {min};
	}
}

sub max
{
	if ( $#_) {
		my $mp = $_[1];
		return if $_[0]-> {max} == $mp;
		$_[0]-> {max} = $mp;
	} else {
		return $_[0]-> {max};
	}
}

package Prima::VB::PropListViewer;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);


sub reset_items
{
	my ( $self, $id, $check, $ix) = @_;
	$self-> {id}    = $id;
	$self-> {items} = $id;
	$self-> {check} = $check;
	$self-> {fHeight} = $self-> font-> height;
	$self-> {index}  = $ix;
	$self-> set_count( scalar @{$self-> {id}});
	$self-> calibrate;
}

sub on_stringify
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> {id}-> [$index];
	$self-> clear_event;
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> get_text_width($self-> {id}-> [$index]) + 2;
	$self-> clear_event;
}

sub on_drawitem
{
	my ( $me, $canvas, $index, $left, $bottom, $right, $top, $hilite, $focused, $prelight) = @_;
	my $ena = $me-> {check}-> [$index];
	unless ( defined $me-> {hBenchColor}) {
		my ( $i1, $i2) = ( $me-> map_color( $me-> hiliteBackColor), $me-> map_color( $me-> hiliteColor));
		my ( $r1, $g1, $b1, $r2, $g2, $b2) = (cl::to_rgb($i1), cl::to_rgb($i2));
		$r1 = int(( $r1 + $r2) / 2);
		$g1 = int(( $g1 + $g2) / 2);
		$b1 = int(( $b1 + $b2) / 2);
		$me-> {hBenchColor} = cl::from_rgb( $r1, $g1, $b1);
		$me-> {hBenchColor} = $i1 if $me-> {hBenchColor} == $i2;
	}
	my ( $c, $bc);
	if ( $hilite || $prelight) {
		$bc = $canvas-> backColor;
		my $bk = $hilite ? ($ena ? $me-> hiliteBackColor : $me-> {hBenchColor}) : $bc;
		$bk = $me->prelight_color($bk) if $prelight;
		$canvas-> backColor( $bk );
	}
	if ( $hilite || !$ena) {
		$c = $canvas-> color;
		$canvas-> color( $ena ? $me-> hiliteColor : ( $hilite ? cl::White : cl::Gray));
	}

	$canvas-> clear( $left, $bottom, $right, $top);
	my $text = $me-> {id}-> [$index];
	my $x = $left + 2;
	$canvas-> text_shape_out( $text, $x, ($top + $bottom + 1 - $me-> {fHeight}) / 2);
	$canvas-> backColor( $bc) if $hilite || $prelight;
	$canvas-> color( $c) if $hilite || !$ena
}


sub on_click
{
	my $self = $_[0];
	my $index = $self-> focusedItem;
	return if $index < 0;
	my $id = $self-> {'id'}-> [$index];
	$self-> {check}-> [$index] = !$self-> {check}-> [$index];
	$self-> redraw_items( $index);
}


package Prima::VB::Editor;
use strict;
use vars qw(@ISA);
@ISA = qw(Prima::Edit);

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	my @accelItems = @{$def{accelItems}};
	my @acc = (
		[ PushMark  => 0, 0, km::Alt|kb::Down, q(push_mark)],
		[ PopMark   => 0, 0, km::Alt|kb::Up,   q(pop_mark)],
	);
	splice( @accelItems, -1, 0, @acc);
	return {
		%def,
		accelItems   => \@accelItems,
		syntaxHilite => 1,
		wordWrap     => 0,
		text         => '',
		ownerFont    => 0,
	}
}

sub set_cursor
{
	my $self = shift;
	my @c = $self-> cursor;
	$self-> SUPER::set_cursor(@_);
	return if $c[0] == $_[0] && $c[1] == $_[1];
	$self-> owner-> Indicator-> repaint;
}

sub push_mark
{
	my $self = $_[0];
	$self-> add_marker( $self-> cursor);
}

sub pop_mark
{
	my $self = $_[0];
	my $m = $self-> markers;
	return if scalar @{$m} == 0;
	$self-> cursor( @{$$m[-1]});
	$self-> delete_marker( -1);
}

sub on_change
{
	my $o = $_[0]-> owner;
	$o-> {modified} = 1;
	$o-> Indicator-> repaint;
}

package Prima::VB::CodeEditor;
use strict;
use vars qw(@ISA @editors);
@ISA = qw(Prima::Window);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text           => 'Code editor',
		icon           => $VB::ico,
		menuItems      => [
			['~File' => [
				['~Load' => 'Ctrl+F3' => '^F3' => q(load_code)],
				['~Save' => 'Ctrl+F2' => '^F2' => q(save_code)],
				[],
				['~Close' => 'Ctrl+F4' => '^F4' => sub { $_[0]-> close } ],
			]],
			['~Edit' => [
				['~Cut' => 'Shift+Del' => km::Shift|kb::Delete, sub { $_[0]-> Editor-> cut} ],
				['Cop~y' => 'Ctrl+Ins' => km::Ctrl|kb::Insert, sub { $_[0]-> Editor-> copy} ],
				['~Paste' => 'Shift+Ins' => km::Shift|kb::Insert, sub { $_[0]-> Editor-> paste} ],
				['~Delete' => 'Delete' => kb::Delete, sub { $_[0]-> Editor-> delete_block} ],
				['~Select all' => 'Ctrl+A' => '^A' => sub { $_[0]-> Editor-> select_all }],
				[],
				['~Find...' => 'Esc'      => kb::Esc   , q(find)],
				['~Replace...'=> 'Ctrl+S' => '^S'      , q(replace)],
				['Find ~next' => 'Ctrl+L' => '^L'      , q(find_next)],
				[],
				['~Undo' => 'Alt+Backspace' => km::Alt|kb::Backspace => sub { $_[0]-> Editor-> undo }],
				['Redo' => 'Ctrl+R' => '^R' => sub { $_[0]-> Editor-> redo }],
			]],
			['~Selection' => [
				[ '(bt-0', '~Normal'     => q(blockType)],
				[ 'bt-1', '~Vertical'   => q(blockType)],
				[ ')bt-2', '~Horizontal' => q(blockType)],
				[],
				[ '~Remove'        => 'Alt+U' => '@U' => sub { $_[0]-> Editor-> cancel_block }],
				[ '~Mark vertical' => 'Alt+B' => '@B' => sub {
					$_[0]-> Editor-> mark_vertical;
					$_[0]-> blockType(1);
				}],
				[ 'Mark horizontal' => 'Alt+L' => '@L' => sub {
					$_[0]-> Editor-> mark_horizontal;
					$_[0]-> blockType(2);
				}],
				['Cop~y block' => 'Alt+C' => '@C' => sub { $_[0]-> Editor-> copy_block }],
				['Over~write block' => 'Alt+O' => '@O' => sub { $_[0]-> Editor-> overtype_block }],
			]],
			['~Options' => [
				[ '@*syntaxHilite'   => '~Syntax hilite'     => q(bool_set)],
				[ '@*autoIndent'     => '~Auto indent'       => q(bool_set)],
				[ '@persistentBlock' => '~Presistent blocks' => q(bool_set)],
				[],
				[ 'Set ~font' => q(setfont)],
			]],
		],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub sync_code
{
	$VB::code = $VB::editor-> Editor-> text if $VB::editor;
}

sub load_code
{
	if ( $_[0]-> {modified}) {
		my $r = Prima::MsgBox::message( "Save changes?",
			mb::YesNoCancel|mb::Warning);
		if ( $r == mb::Yes) {
			return unless save_code($_[0]);
		} elsif ( $r == mb::Cancel) {
			return;
		}
	}
	my $d = VB::open_dialog( filter => [[ 'All files' => '*']]);
	return unless $d-> execute;
	unless ( open F, "< " . $d-> fileName) {
		Prima::MsgBox::message("Cannot open " . $d-> fileName . ":$!");
		return;
	}
	local $/;
	$_[0]-> Editor-> text(<F>);
	close F;
}

sub save_code
{
	my $d = VB::save_dialog( filter => [[ 'All files' => '*']]);
	return unless $d-> execute;
	unless ( open F, "> " . $d-> fileName) {
		Prima::MsgBox::message("Cannot open " . $d-> fileName . ":$!");
		return;
	}
	local $/;
	print F $_[0]-> Editor-> text;
	close F;
}

sub setfont
{
	my $self = $_[0];
	my $d = VB::font_dialog( logFont => $self-> Editor-> font);
	return unless $d-> execute == mb::OK;
	$_-> font( $d-> logFont) for @editors;
	my $f = $d-> logFont;
	my $i = $VB::main-> {iniFile}-> section('Editor');
	$i-> {'Font' . ucfirst($_)} = $f-> {$_} for qw(name size style encoding);
}

sub bool_set
{
	my ( $self, $var, $val) = @_;
	$self-> Editor-> set($var, $val);
}

sub blockType
{
	my ( $self, $var) = @_;
	return unless $var =~ /^bt-(\d)$/;
	$self-> Editor-> blockType($1);
}


my $findDialog;

sub find_dialog
{
	my ( $self, $findStyle) = @_;
	my %prf;
	%{$self-> {findData}} = (
		replaceText  => '',
		findText     => '',
		replaceItems => [],
		findItems    => [],
		options      => 0,
		scope        => fds::Cursor,
	) unless defined $self-> {findData};

	my $fd = $self-> {findData};
	my @props = qw(findText options scope);
	push( @props, q(replaceText)) unless $findStyle;
	if ( $fd) { for( @props) { $prf{$_} = $fd-> {$_}}}
	$findDialog = Prima::FindDialog-> create unless $findDialog;
	$findDialog-> set( %prf, findStyle => $findStyle, text => ($findStyle ? 'Find' : 'Replace'));
	$findDialog-> Find-> items($fd-> {findItems});
	$findDialog-> Replace-> items($fd-> {replaceItems}) unless $findStyle;
	my $ret = 0;
	my $rf  = $findDialog-> execute;
	if ( $rf != mb::Cancel) {
		$self-> {findData}-> {$_} = $findDialog-> $_() for @props;
		$self-> {findData}-> {result} = $rf;
		$self-> {findData}-> {asFind} = $findStyle;
		@{$self-> {findData}-> {findItems}} = @{$findDialog-> Find-> items};
		@{$self-> {findData}-> {replaceItems}} = @{$findDialog-> Replace-> items}
			unless $findStyle;
		$ret = 1;
	}
	return $ret;
}

sub do_find
{
	my $self = $_[0];
	my $e = $self-> Editor;
	my $p = $self-> {findData};
	my @scope;
	my $replaced;
	my $text = $self-> text;
	FIND:{
		if ( $$p{scope} != fds::Cursor) {
			if ( $e-> has_selection) {
				my @sel = $e-> selection;
				@scope = ($$p{scope} == fds::Top) ?
					($sel[0],$sel[1]) :
					($sel[2], $sel[3]);
			} else {
				@scope = ($$p{scope} == fds::Top) ?
					(0,0) : (-1,-1);
			}
		} else {
			@scope = $e-> cursor;
		}
		my @n = $e-> find( $$p{findText}, @scope, $$p{replaceText}, $$p{options});
		if ( !defined $n[0]) {
			Prima::MsgBox::message("No matches found");
			goto EXIT;
		}
		$e-> cursor(($$p{options} & fdo::BackwardSearch) ? $n[0] : $n[0] + $n[2], $n[1]);
		$e-> selection( $n[0], $n[1], $n[0] + $n[2], $n[1]);
		unless ( $$p{asFind}) {
			if ( $$p{options} & fdo::ReplacePrompt) {
				my $r = Prima::MsgBox::message_box( $self-> text,
				"Replace this text?",
				mb::YesNoCancel|mb::Information|mb::NoSound);
				redo FIND if ($r == mb::No) && ($$p{result} == mb::ChangeAll);
				last FIND if $r == mb::Cancel;
			} else {
				$self-> text( ++$replaced . " occurences replaced");
			}
			$e-> set_line( $n[1], $n[3]);
			redo FIND if $$p{result} == mb::ChangeAll;
		}
	}
EXIT:
	$self-> text( $text);
}

sub find
{
	my $self = $_[0];
	return unless $self-> find_dialog(1);
	$self-> do_find;
}

sub replace
{
	my $self = $_[0];
	return unless $self-> find_dialog(0);
	$self-> do_find;
}

sub find_next
{
	my $self = $_[0];
	return unless $self-> {findData};
	$self-> do_find;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	$VB::main-> init_position( $self, 'CodeEditorRect');

	my $i = $VB::main-> {iniFile}-> section('Editor');

	my @sz = $self-> size;
	my $fh = $self-> font-> height + 6;

	my $indicator;
	my $editor = $self-> insert( 'Prima::VB::Editor' =>
		origin   => [ 0, $fh],
		size     => [ $sz[0], $sz[1] - $fh],
		growMode => gm::Client,
		name     => 'Editor',
		font => { map { $_,  $i-> {'Font' . ucfirst($_)}} qw(name size style encoding) },
	);

	$indicator = $self-> insert( Widget =>
		origin  => [ 0, 0],
		size    => [ $sz[0], $fh],
		growMode => gm::Floor,
		name     => 'Indicator',
		onPaint  => sub {
			my ( $me, $canvas) = @_;
			$canvas-> rect3d( 0, 0, $me-> width - 1, $me-> height - 1, 1,
				$me-> dark3DColor, $me-> light3DColor, $me-> backColor);
			my @c = $editor-> cursorLog;
			$canvas-> text_out( sprintf("%s %d:%d", ($self-> {modified} ? '*' : ' '),
				$c[0]+1,$c[1]+1), 4, ( $me-> height - $canvas-> font-> height) / 2);
		}
	);

	for ( qw( syntaxHilite autoIndent persistentBlock)) {
		next unless exists $i-> {$_};
		$self-> menu-> checked( $_, $i-> {$_});
		$editor-> $_( $i-> {$_} ? 1 : 0);
	}
	if ( length $i-> {blockType}) {
		$self-> menu-> check( "bt-$i->{blockType}");
		$editor-> blockType( $i-> {blockType});
	}

	push @editors, $editor;

	$editor-> textRef( \$VB::code );
	$self-> {modified} = 0;

	return %profile;
}

sub flush
{
	$VB::code = '';
	if ( $VB::editor) {
		$VB::editor-> Editor-> text('');
		$VB::editor-> {modified} = 0;
		$VB::editor-> Indicator-> repaint;
	}
}

sub on_close
{
	my $i = $VB::main-> {iniFile}-> section('Editor');
	my $e = $_[0]-> Editor;
	$i-> {$_} = ( $e-> $_() ? 1 : 0) for qw( syntaxHilite autoIndent persistentBlock);
	$i-> {blockType} = $e-> blockType;
	@editors = grep { $_ != $e } @editors;
	sync_code;
}

sub on_destroy
{
	$VB::form-> {modified} = 1 if $VB::form && $_[0]-> {modified};
	$VB::editor = undef;
}

1;
