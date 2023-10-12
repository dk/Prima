=pod

=head1 NAME

examples/editor.pl - basic text editor

=head1 FEATURES

Demonstrates usage of the L<Prima::Edit> class and the standard find/replace
dialogs.

=cut

use strict;
use warnings;

use Prima qw(Edit Application MsgBox Dialog::FileDialog Dialog::FindDialog
Dialog::FontDialog sys::GUIException);

package Indicator;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	my %def = %{$_[ 0]->SUPER::profile_default};
	return {
		%def,
		editor   => undef,
		text     => '',
		growMode => gm::Floor,
		left     => 0,
		bottom   => 0,
		height   => $::application->font->height + 2,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self->SUPER::init(@_);
	$self-> {editor} = $profile{editor};
	$self-> reset;
	return %profile;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	$canvas-> rect3d( 0, 0, $self-> width - 1, $self-> height - 1,
		1, $self-> dark3DColor, $self-> light3DColor, $self-> backColor);
	$canvas-> text_out( $self-> text,
		4, ( $self-> height - $canvas-> font-> height) / 2);
}

sub reset
{
	my $self    = $_[0];
	my $editor  = $self-> {editor};
	my @c = $editor-> cursorLog;
	$self-> text( sprintf("%s %d:%d", ($editor-> modified ? '*' : ' '),
		$c[0]+1,$c[1]+1));
	$self-> repaint;
}


package Editor;
use vars qw(@ISA);
@ISA = qw(Prima::Edit);

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	my @accelItems = @{$def{accelItems}};
	my @acc = (
		[ PushMark  => 0, 0, km::Ctrl|kb::Down, q(push_mark)],
		[ PopMark   => 0, 0, km::Ctrl|kb::Up,   q(pop_mark)],
	);
	splice( @accelItems, -1, 0, @acc);
	return {
		%def,
		accelItems => \@accelItems,
	}
}

sub set_cursor
{
	my $self = shift;
	my @c = $self-> cursor;
	$self-> SUPER::set_cursor(@_);
	return if $c[0] == $_[0] && $c[1] == $_[1];
	$self-> owner-> {status}-> reset
		if $self-> owner-> {status} && !$self-> change_locked;
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

package EditorWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);
use Prima::sys::FS;

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		fileName => undef,
		utf8     => 1,
		menuItems => [
			[ '~File' => [
				[ '~New'        => q(new_window)],
				[ '~Open...'    => 'F3' => kb::F3, q(open_file)],
				[ '~Save'       => 'F2' => kb::F2, q(save_file)],
				[ 'Save ~as...' => q(save_as)],
				[],
				['E~xit'        => 'Alt+X' => '@X' => sub {$::application-> close}]
			]],
			[ '~Edit' => [
				['~Cut'   => 'Ctrl+Del'   => kb::NoKey, sub{$_[0]-> {editor}-> cut}],
				['C~opy'  => 'Ctrl+Ins'   => kb::NoKey, sub{$_[0]-> {editor}-> copy}],
				['~Paste' => 'Shift+Ins'  => kb::NoKey, sub{$_[0]-> {editor}-> paste}],
				['~Delete' => 'Shift+Del' => kb::NoKey, sub{$_[0]-> {editor}-> delete_block}],
				['~Select all' => 'Ctrl+A' => '^A' => sub { $_[0]->{editor}->select_all }],
				[],
				['~Find...' => 'Esc'      => kb::Esc   , q(find)],
				['~Replace...'=> 'Ctrl+S' => '^S'      , q(replace)],
				['Find ~next' => 'Ctrl+L' => '^L'      , q(find_next)],
				[],
				['~Undo' => 'Alt+Backspace' => kb::NoKey   , sub {$_[0]-> {editor}-> undo}],
				['~Redo' => 'Ctrl+R'        => kb::NoKey   , sub {$_[0]-> {editor}-> redo}],
			]],
			['~Options' => [
				[ '@syx' => '~Syntax hilite' => sub{ $_[0]-> {editor}-> syntaxHilite( $_[2] )}],
				[ '@*aid' => '~Auto indent' => sub{ $_[0]-> {editor}-> autoIndent( $_[2] )}],
				[ '@*wwp' => '~Word wrap' => sub{ $_[0]-> {editor}-> wordWrap( $_[2] )}],
				[ '@cwp' => '~Cursor wrap' => sub{ $_[0]-> {editor}-> cursorWrap( $_[2] )}],
				[ '@tab' => 'Enter ~tabs in text' => sub{ $_[0]-> {editor}-> wantTabs( $_[2] )}],
				[ '@psb' => '~Persistent blocks' => sub{ $_[0]-> {editor}-> persistentBlock( $_[2] )}],
				[],
				[ '@*hsc' => '~Horizontal scrollbar' => sub{ $_[0]-> {editor}-> hScroll( $_[2])}],
				[ '@*vsc' => '~Vertical scrollbar'   => sub{ $_[0]-> {editor}-> vScroll( $_[2])}],
				[],
				(
					['utf'  => 'UTF-8 mode' => sub {
						my $utf8_mode = $_[0]-> menu-> utf-> toggle;
						$_[0]-> {utf8} = $utf8_mode;
						$::application-> wantUnicodeInput($utf8_mode);
					}]
				),
				[ '@'.($::application->textDirection ? '*' : '').'rtl' => 
					'~Right-to-left' => sub{ $_[0]-> {editor}-> textDirection( $_[2] )}],
				[ '@*ligation' => '~Ligatures' => sub{ $_[0]-> {editor}-> textLigation( $_[2] )}],
				[ 'Set ~font' => q(setfont)],
			]]
		],
	}
}

my $windows = 0;

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $fn = $profile{fileName};
	my $cap = '';
	$self-> menu-> utf-> check if $self-> {utf8} = $profile{utf8};
	if ( defined $fn) {
		if ( open FILE, '<'.($profile{utf8} ? ':encoding(utf-8)' : ''), $fn) {
			if ( ! defined read( FILE, $cap, _s $fn)) {
				Prima::MsgBox::message("Cannot read file $fn:$!");
				$fn = undef;
			}
			close FILE;
		} else {
			Prima::MsgBox::message("Cannot open file $fn:$!");
			$fn = undef;
		}
	}
	$fn = '.Untitled' unless defined $fn;
	my $fh = $::application->font->height + 2;
	$self-> {editor} = $self-> insert( Editor =>
		name      => 'Edit',
		textRef   => \$cap,
		origin    => [ 0, $fh],
		size      => [ $self-> width, $self-> height - $fh],
		hScroll   => 1,
		vScroll   => 1,
		growMode  => gm::Client,
		wordWrap => 1,
	);
	undef $cap;
	$self-> text( $fn);
	$self-> {status} = $self-> insert( Indicator =>
		name    => 'StatusBar',
		width   => $self-> width,
		editor  => $self-> {editor},
	);
	$self-> {editor}-> focus;
	$self-> {findData} = undef;
	$windows++;
	return %profile;
}


sub on_close
{
	my $self = $_[0];
	if ( $self-> {editor}-> modified) {
		my $r =  Prima::MsgBox::message_box(
			$self-> text,
			'File '.$self-> text. ' has been modified.  Save?',
			mb::YesNoCancel|mb::Warning);
		return if mb::No == $r;
		$self-> clear_event, return if mb::Cancel == $r;
		$self-> clear_event, return unless $self-> save_file;
	}
}

sub on_destroy
{
	$::application-> close unless --$windows;
}

sub new_window
{
	my $self = $_[0];
	my $ww = EditorWindow-> create(
		size   => [$self-> size],
		left   => $self-> left + 10,
		bottom => $self-> bottom - 10,
		font   => $self-> font,
		utf8   => $self-> {utf8},
	);
	$ww-> {editor}-> focus;
	return $ww;
}

sub open_file
{
	my $self = $_[0];
	my $f = Prima::open_file;
	if ( defined $f) {
		my $ww = EditorWindow-> create(
			size     => [$self-> size],
			left     => $self-> left + 10,
			bottom   => $self-> bottom - 10,
			fileName => $f,
			font     => $self-> font,
			utf8     => $self-> {utf8},
		);
		$ww-> {editor}-> focus;
	}
}

sub save_file
{
	my $self = $_[0];
	return $self-> save_as if $self-> text eq '.Untitled';
	my $fn = $self-> text;
	if ( open FILE, '>'.($self-> {utf8} ? 'utf8' : ''), $fn) {
		my $cap = $self-> {editor}-> text;
		Encode::_utf8_off($cap) if !$self-> {utf8};
		my $swr = syswrite(FILE,$cap,length($cap));
		close FILE;
		unless (defined $swr && $swr==length($cap)) {
			undef $cap;
			unlink $fn;
			Prima::MsgBox::message_box(
				$self-> text,
				"Cannot save to $fn", mb::Error|mb::OK);
			return 0;
		}
		undef $cap;
		$self-> {editor}-> modified(0);
		$self-> {status}-> reset;
		return 1;
	} else {
		Prima::MsgBox::message_box(
			$self-> text, "Cannot save to $fn", mb::Error|mb::OK);
	}
	return 0;
}

sub save_as
{
	my $self = $_[0];
	my $fn = Prima::save_file;
	my $ret = 0;
	if ( defined $fn) {
	SAVE:while(1) {
		next SAVE unless open FILE, '>'.($self-> {utf8} ? 'utf8' : ''), $fn;
		my $cap = $self-> {editor}-> text;
		Encode::_utf8_off($cap) if !$self-> {utf8};
		my $swr = print FILE $cap;
		close FILE;
		unless ($swr) {
			undef $cap;
			unlink $fn;
			next SAVE;
		}
		undef $cap;
		$self-> {editor}-> modified(0);
		$self-> {status}-> reset;
		$self-> text( $fn);
		$ret = 1;
		last;
	} continue {
		last SAVE unless
			mb::Retry == Prima::MsgBox::message_box(
				$self-> text, "Cannot save to $fn",
				mb::Error|mb::Retry|mb::Cancel
			);
	}}
	return $ret;
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
	$findDialog = Prima::Dialog::FindDialog-> create unless $findDialog;
	$findDialog-> set( %prf, findStyle => $findStyle);
	$findDialog-> Find-> items($fd-> {findItems});
	$findDialog-> Replace-> items($fd-> {replaceItems}) unless $findStyle;
	my $ret = 0;
	my $rf  = $findDialog-> execute;
	if ( $rf != mb::Cancel) {
		{ for( @props) { $self-> {findData}-> {$_} = $findDialog-> $_()}}
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
	my $e = $self-> {editor};
	my $p = $self-> {findData};
	my @scope;
	FIND:{
		if ( $$p{scope} != fds::Cursor) {
			if ( $e-> has_selection) {
			my @sel = $e-> selection;
				@scope = ($$p{scope} == fds::Top) ? ($sel[0],$sel[1]) : ($sel[2], $sel[3]);
			} else {
				@scope = ($$p{scope} == fds::Top) ? (0,0) : (-1,-1);
			}
		} else {
			@scope = $e-> cursor;
		}
		my @n = $e-> find( $$p{findText}, @scope, $$p{replaceText}, $$p{options});
		if ( !defined $n[0]) {
			Prima::MsgBox::message("No matches found");
			return;
		}
		$e-> cursor(($$p{options} & fdo::BackwardSearch) ? $n[0] : $n[2], $n[1]);
		$_ = $e-> physical_to_visual($_,$n[1]) for @n[0,2];
		$e-> selection( $n[0], $n[1], $n[2], $n[1]);
		unless ( $$p{asFind}) {
			if ( $$p{options} & fdo::ReplacePrompt) {
				my $r = Prima::MsgBox::message_box( $self-> text,
				"Replace this text?",
				mb::YesNoCancel|mb::Information|mb::NoSound);
				redo FIND if ($r == mb::No) && ($$p{result} == mb::ChangeAll);
				last FIND if $r == mb::Cancel;
			}
			$e-> set_line( $n[1], $n[3]);
			redo FIND if $$p{result} == mb::ChangeAll;
		}
	}
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

my $fontDialog;

sub setfont
{
	my $self = $_[0];
	$fontDialog = Prima::Dialog::FontDialog-> create() unless $fontDialog;
	$fontDialog-> logFont( $self-> font);
	return unless $fontDialog-> execute;
	$self-> font( $fontDialog-> logFont);
}

package Generic;

my @fn = @ARGV;
@fn = (undef) unless scalar @fn;

for ( @fn) {
my $w = EditorWindow-> create(
        origin => [ 10, 100],
        size   => [ $::application-> width - 820, $::application-> height - 150],
	fileName => $_,
	font => {
		size => 16,
		name => 'Courier New',
	},
);
}

run Prima;
