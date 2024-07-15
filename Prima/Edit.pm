#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk>
#     Anton Berezin  <tobez@plab.ku.dk>

# edit block types
use strict;
use warnings;
package
    bt;
use constant CUA          =>  0;
use constant Vertical     =>  1;
use constant Horizontal   =>  2;

package Prima::Edit;
use Prima qw(ScrollBar Drawable::Glyphs);
use base qw(
	Prima::Widget Prima::Widget::MouseScroller Prima::Widget::GroupScroller
	Prima::Widget::UndoActions Prima::Widget::BidiInput
);
__PACKAGE__->inherit_core_methods('Prima::Widget::GroupScroller');

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	ParseSyntax   => nt::Action,
);

sub notification_types { return \%RNT; }
}

use constant CM_CHAR_OFS    => 0;
use constant CM_CHAR_LEN    => 1;
use constant CM_Y           => 2;
use constant CM_CLUSTER_LEN => 3;
use constant CM_SIZE        => 4;

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	my $font = $_[ 0]-> get_default_font;
	my $rtl  = $::application-> textDirection;
	return {
		%def,
		accelItems => [
# navigation
			[ CursorDown   => 0, 0, kb::Down                , sub{$_[0]-> cursor_down}],
			[ CursorUp     => 0, 0, kb::Up                  , sub{$_[0]-> cursor_up}],
			[ CursorLeft   => 0, 0, kb::Left                , sub{$_[0]-> cursor_left}],
			[ CursorRight  => 0, 0, kb::Right               , sub{$_[0]-> cursor_right}],
			[ PageUp       => 0, 0, kb::PgUp                , sub{$_[0]-> cursor_pgup}],
			[ PageDown     => 0, 0, kb::PgDn                , sub{$_[0]-> cursor_pgdn}],
			[ Home         => 0, 0, kb::Home                , sub{$_[0]-> cursor_home}],
			[ End          => 0, 0, kb::End                 , sub{$_[0]-> cursor_end}],
			[ CtrlPageUp   => 0, 0, kb::PgUp|km::Ctrl       , sub{$_[0]-> cursor_cpgup}],
			[ CtrlPageDown => 0, 0, kb::PgDn|km::Ctrl       , sub{$_[0]-> cursor_cpgdn}],
			[ CtrlHome     => 0, 0, kb::Home|km::Ctrl       , sub{$_[0]-> cursor_chome}],
			[ CtrlEnd      => 0, 0, kb::End |km::Ctrl       , sub{$_[0]-> cursor_cend}],
			[ WordLeft     => 0, 0, kb::Left |km::Ctrl      , sub{$_[0]-> word_left}],
			[ WordRight    => 0, 0, kb::Right|km::Ctrl      , sub{$_[0]-> word_right}],
			[ ShiftCursorDown   => 0, 0, km::Shift|kb::Down                , q(cursor_shift_key)],
			[ ShiftCursorUp     => 0, 0, km::Shift|kb::Up                  , q(cursor_shift_key)],
			[ ShiftCursorLeft   => 0, 0, km::Shift|kb::Left                , q(cursor_shift_key)],
			[ ShiftCursorRight  => 0, 0, km::Shift|kb::Right               , q(cursor_shift_key)],
			[ ShiftPageUp       => 0, 0, km::Shift|kb::PgUp                , q(cursor_shift_key)],
			[ ShiftPageDown     => 0, 0, km::Shift|kb::PgDn                , q(cursor_shift_key)],
			[ ShiftHome         => 0, 0, km::Shift|kb::Home                , q(cursor_shift_key)],
			[ ShiftEnd          => 0, 0, km::Shift|kb::End                 , q(cursor_shift_key)],
			[ ShiftCtrlPageUp   => 0, 0, km::Shift|kb::PgUp|km::Ctrl       , q(cursor_shift_key)],
			[ ShiftCtrlPageDown => 0, 0, km::Shift|kb::PgDn|km::Ctrl       , q(cursor_shift_key)],
			[ ShiftCtrlHome     => 0, 0, km::Shift|kb::Home|km::Ctrl       , q(cursor_shift_key)],
			[ ShiftCtrlEnd      => 0, 0, km::Shift|kb::End |km::Ctrl       , q(cursor_shift_key)],
			[ ShiftWordLeft     => 0, 0, km::Shift|kb::Left |km::Ctrl      , q(cursor_shift_key)],
			[ ShiftWordRight    => 0, 0, km::Shift|kb::Right|km::Ctrl      , q(cursor_shift_key)],
			[ Insert         => 0, 0, kb::Insert , sub {$_[0]-> insertMode(!$_[0]-> insertMode)}],
# edit keys
			[ Delete         => 0, 0, kb::Delete,    sub {
				$_[0]-> has_selection ? $_[0]-> delete_block : $_[0]-> delete_char;
			}],
			[ Backspace      => 0, 0, kb::Backspace, sub {
				$_[0]-> has_selection ? $_[0]-> delete_block : $_[0]-> back_char;
			}],
			[ CtrlBackspace  => 0, 0, kb::Backspace|km::Ctrl, sub {
				$_[0]-> has_selection ? $_[0]-> delete_block : $_[0]-> delete_word(1);
			}],
			[ CtrlDelete     => 0, 0, kb::Delete|km::Ctrl, sub {
				$_[0]-> has_selection ? $_[0]-> delete_block : $_[0]-> delete_word(0);
			}],
			[ DeleteToEnd    => 0, 0, '^E',          q(delete_to_end) ],
			[ DupLine        => 0, 0, '^K',          sub {$_[0]-> insert_line($_[0]-> cursorY, $_[0]-> get_line($_[0]-> cursorY)) }],
			[ DeleteBlock    => 0, 0, '@D',          q(delete_block) ],
			[ SplitLine      => 0, 0, kb::Enter,     sub {$_[0]-> split_line if $_[0]-> {wantReturns}}],
			[ SplitLine2     => 0, 0, km::Ctrl|kb::Enter,sub {$_[0]-> split_line if !$_[0]-> {wantReturns}}],
# block keys
			[ CancelBlock    => 0, 0, '@U',          q(cancel_block)],
			[ MarkVertical   => 0, 0, '@B',          q(mark_vertical)],
			[ MarkHorizontal => 0, 0, '@L',          q(mark_horizontal)],
			[ CopyBlock      => 0, 0, '@C',          q(copy_block)],
			[ OvertypeBlock  => 0, 0, '@O',          q(overtype_block)],
# clipboard keys
			[ Cut            => 0, 0, km::Shift|kb::Delete, q(cut)],
			[ Copy           => 0, 0, km::Ctrl |kb::Insert, q(copy)],
			[ Paste          => 0, 0, km::Shift|kb::Insert, q(paste)],
			[ CutMS          => 0, 0, '^X', q(cut)],
			[ CopyMS         => 0, 0, '^C', q(copy)],
			[ PasteMS        => 0, 0, '^V', q(paste)],
			[ SelectAll      => 0, 0, '^A', q(select_all)],
# undo
			[ Undo           => 0, 0, '^Z', q(undo)],
			[ Redo           => 0, 0, '^Y', q(redo)],
		],
		autoIndent        => 1,
		blockType         => bt::CUA,
		cursorSize        => [ $::application-> get_default_cursor_width, $font-> { height}],
		cursorVisible     => 1,
		cursorX           => 0,
		cursorY           => 0,
		cursorWrap        => 0,
		dndAware          => "Text",
		insertMode        => 0,
		hiliteNumbers     => cl::Green,
		hiliteQStrings    => cl::LightBlue,
		hiliteQQStrings   => cl::LightBlue,
		hiliteIDs         => [[qw(
abs accept alarm atan2 bind binmode bless caller chdir chmod chomp chop chown
chr chroot close closedir connect continue cos crypt defined
delete die do dump each endgrent endhostent endnetent endprotoent endpwent
endservent eof eval exec exists exit exp fcntl fileno flock for fork format
formline getc getgrent getgrgid getgrnam gethostbyaddr gethostbyname gethostent
getlogin getnetbyaddr getnetbyname getnetent getpeername getpgrp getppid
getpriority getprotobyname getprotobynumber getprotoent getpwent getpwnam
getpwuid getservbyname getservbyport getservent getsockname getsockopt glob
gmtime goto grep hex if import index int ioctl join keys kill last lc lcfirst
length link listen local localtime log lstat m map mkdir msgctl msgget msgrcv
msgsnd my next no oct open opendir ord our pack package pipe pop pos print
printf prototype push q qq qr quotemeta qw qx rand read readdir readline
readlink readpipe recv redo ref rename require reset return reverse rewinddir
rindex rmdir s scalar seek seekdir select semctl semget semop send setgrent
sethostent setnetent setpgrp setpriority setprotoent setpwent setservent
setsockopt shift shmctl shmget shmread shmwrite shutdown sin sleep socket
socketpair sort splice split sprintf sqrt srand stat study sub substr symlink
syscall sysopen sysread sysseek system syswrite tell telldir tie tied time
times tr truncate uc ucfirst umask undef unless unlink unpack unshift untie use
utime values vec wait waitpid wantarray warn while write y
		)], cl::Blue],
		hiliteChars       => ['~!@#$%^&*()+-=[]{};:\'"\\|?.,<>/', cl::Blue],
		hiliteREs         => [ '(#.*)$', cl::Gray, '(\/\/.*)$', cl::Gray],
		markers           => [],
		modified          => 0,
		offset            => 0,
		pointerType       => cr::Text,
		persistentBlock   => 0,
		readOnly          => 0,
		selection         => [0, 0, 0, 0],
		selStart          => [0, 0],
		selEnd            => [0, 0],
		selectable        => 1,
		syntaxHilite      => 0,
		tabIndent         => 8,
		textRef           => undef,
		textDirection     => $rtl,
		textLigation      => 1,
		topLine           => 0,
		undoLimit         => 1000,
		wantTabs          => 0,
		wantReturns       => 1,
		widgetClass       => wc::Edit,
		wordDelimiters    => ".()\"',#$@!%^&*{}[]?/|;:<>-= \xff\t",
		wordWrap          => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	if ( exists( $p-> { selection}))
	{
		my $s = $p-> {selection};
		$p-> {selStart} = [$$s[0], $$s[1]];
		$p-> {selEnd  } = [$$s[2], $$s[3]];
	}
	$p-> { text} = '' if exists( $p-> { textRef});
}

sub init
{
	my $self = shift;
	for ( qw( autoIndent topLine  offset resetDisabled blockType persistentBlock
		tabIndent wantReturns wantTabs))
		{ $self-> {$_} = 1; }
	for ( qw( readOnly wordWrap rows maxLineCount maxLineLength maxLineWidth
		scrollTransaction maxLine maxChunk capLen cursorY cursorX cursorWrap
		cursorXl cursorYl syntaxHilite hiliteNumbers hiliteQStrings hiliteQQStrings
		notifyChangeLock modified blockShiftMark
		textDirection textLigation
	))
		{ $self-> {$_} = 0;}
	$self-> { insertMode}   = $::application-> insertMode;
	for ( qw( markers lines chunkMap hiliteIDs hiliteChars hiliteREs )) { $self-> {$_} = []}
	for ( qw( selStart selEnd selStartl selEndl)) { $self-> {$_} = [0,0]}
	$self-> {defcw} = $::application-> get_default_cursor_width;
	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;
	$self-> init_undo(\%profile);
	$profile{selection} = [@{$profile{selStart}}, @{$profile{selEnd}}];
	for ( qw( hiliteNumbers hiliteQStrings hiliteQQStrings hiliteIDs hiliteChars hiliteREs
		text textRef syntaxHilite autoIndent persistentBlock blockType 
		topLine  tabIndent readOnly offset wordDelimiters wantTabs wantReturns
		wordWrap cursorWrap markers textDirection textLigation ))
		{ $self-> $_( $profile{ $_}); }
	delete $self-> {resetDisabled};
	$self-> {uChange} = 0;
	$self-> reset;
	$self-> selection( @{$profile{selection}});
	for ( qw( cursorX cursorY))
	{ $self-> $_( $profile{ $_}); }
	$self-> reset_scrolls;
	$self-> {modified} = 0;
	return %profile;
}

sub reset
{
	my $self = $_[0];
	return if $self-> {resetDisabled};
	my @a    = ( $self-> {indents}-> [0], $self-> {indents}-> [1]);
	my @size = $self-> get_active_area(2);
	my $cw   = $self-> {defcw};
	my $ti   = $self-> {tabIndent};
	my $uC   = $self-> {uChange};
	my $mw;
	$size[0] -= $cw;
	if ( $uC < 2) {
		$self-> {fixed} = $self-> font-> pitch == fp::Fixed;
		$self-> {averageWidth} = $self-> font-> width;
		$mw                    = $self-> {averageWidth};
	}

# changes that apply to string output must issue recalculation here.
#
# Calculating wrap chunks ( if necessary) and build chunkMap.
# chunkMap is actual only in wordWrap = 1 mode; it maps lines to real lines.
# it's structure is: [ subline offset, subline length, line index].
	if ( $self-> {wordWrap}) {
		if ( $uC < 2) {
			my $twOpts  = tw::WordBreak|tw::CalcTabs|tw::NewLineBreak|tw::ReturnChunks;
			my @chunkMap;
			$self-> begin_paint_info;
			my $j = 0;
			for my $line ( @{$self-> {lines}})
			{
				my @glyphs = $self-> text_shape_with_tabs_replaced( $line ) || ();
				my $breaks = $self-> text_wrap( $line, $size[0], $twOpts, 1, 0, -1, @glyphs);
				for (my $i = 0; $i < @$breaks; $i+=2) {
					my @chunk = ( undef ) x CM_SIZE;
					$chunk[CM_CHAR_OFS] = $$breaks[$i];
					$chunk[CM_CHAR_LEN] = 
						(( $i == @$breaks - 2) ? length($line) : $$breaks[$i + 2]) -
						$$breaks[$i];
					$chunk[CM_Y]        = $j;
					push @chunkMap, @chunk;
				}
				$j++;
			}
			$self-> end_paint_info;
			$self-> {chunkMap}      = \@chunkMap;
			$self-> {maxLineWidth} = $size[0];
		}
	} else {
# fast ( but not exact) calculation of maximal line width.
		if ( $uC == 0) {
			my $max = 0;
			my $maxLinesCount = 0;
			for ( @{$self-> {lines}}) {
				my $l = length( $_);
				$max  = $l, $maxLinesCount = 0 if $max < $l;
				$maxLinesCount++ if $l == $max;
			}
			$self-> {maxLineLength} = $max;
			$self-> {maxLineCount}  = $maxLinesCount;
		}
		if ( $uC < 2) {
			$self-> {maxLineWidth}  = $self-> {maxLineLength} * $mw;
		}
	}
	my $fh    = $self-> font-> height;
	$self-> {rows}  = int($size[1] / $fh);
	my $yTail = $size[1] - $self-> {rows} * $fh;

	if ( $uC < 2) {
		$self-> {maxLine}  = scalar @{$self-> {lines}} - 1;
		$self-> {maxChunk} = $self-> {wordWrap} ? 
			(scalar @{$self-> {chunkMap}}/CM_SIZE-1) : 
			$self-> {maxLine};
		$self-> {yTail} = ( $yTail > 0) ? 1 : 0;
		$self-> selection( @{$self-> {selStart}}, @{$self-> {selEnd}});
		$self-> cursor( $self-> cursor);
	}

# positioning cursor
	my $cx  = $a[0] + $self-> {cursorAtX} - $self-> {offset};
	my $cy  = $a[1] + $yTail + ($self-> {rows} - $self-> {cursorYl} + $self-> {topLine } - 1) * $fh;
	my $xcw; 
	if ($self-> {insertMode}) {
		$xcw = $cw;
		$cx += $self->{cursorInsWidth} if $self->{textDirection};
	} else {
		$xcw = $self-> {cursorInsWidth};
	}
	my $ycw = $fh;
	$ycw -= $a[1] - $cy, $cy = $a[1] if $cy < $a[1];
	$xcw = $size[0] + $a[0] - $cx if $cx + $xcw >= $size[0] + $a[0];
	$self-> cursorVisible( $xcw > 0);
	if ( $xcw > 0) {
		$self-> cursorPos( $cx, $cy);
		$self-> cursorSize( $xcw, $ycw);
	}
	$self-> {uChange} = 0;
}

sub reset_cursor
{
	my $self = $_[0];
	$self-> {uChange} = 2;
	$self-> reset;
	$self-> {uChange} = 0;
}

sub reset_render
{
	my $self = $_[0];
	$self-> {uChange} = 1;
	$self-> reset;
	$self-> {uChange} = 0;
}

sub reset_scrolls
{
	my $self = $_[0];
	return if $self-> {resetDisabled};
	if ( $self-> {scrollTransaction} != 1) {
		$self-> vScroll( $self-> {maxChunk} >= $self-> {rows}) if $self-> {autoVScroll};
		$self-> {vScrollBar}-> set(
			max      => $self-> {maxChunk} - $self-> {rows} + 1,
			pageStep => $self-> {rows},
			whole    => $self-> {maxChunk} + 1,
			partial  => $self-> {rows},
			value    => $self-> {topLine },
		) if $self-> {vScroll};
	}
	if ( $self-> {scrollTransaction} != 2) {
		my $w = $self-> width - $self-> {indents}-> [0] - $self-> {indents}-> [2];
		my $lw = $self-> {maxLineWidth} + $self->{defcw} + 1;
		if ( $self-> {autoHScroll}) {
			my $hs = ( $lw > $w) ? 1 : 0;
			if ( $hs != $self-> {hScroll}) {
				$self-> hScroll( $hs);
				$w = $self-> width - $self-> {indents}-> [0] - $self-> {indents}-> [2];
			}
		}
		if ($self-> {hScroll}) {
			my $step  = $self-> font-> width;
			my $value = int($self->{offset} / $step) * $step +
				($self->textDirection ? $step : 0);
			$self-> {hScrollBar}-> set(
				max      => $self-> {wordWrap} ? 0 : ($lw - $w),
				whole    => ($lw < $w) ? $w : $lw,
				value    => $value,
				partial  => $w,
				pageStep => $lw / 5,
				step     => $step,
			);
		}
	}
}

sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 1;
	$self-> topLine ( $scr-> value);
	$self-> {scrollTransaction} = 0;
}

sub HScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 2;
	$self-> offset( $scr-> value);
	$self-> {scrollTransaction} = 0;
}

sub reset_syntax
{
	my $self = $_[0];
	if ( $self-> {syntaxHilite}) {
		$self-> {syntax} = [];
	} else {
		$self-> {syntax} = undef;
	}
}

sub reset_syntaxer
{
	my $self = $_[0];
	unless ($self-> {hiliteNumbers} ||
			$self-> {hiliteQStrings} ||
			$self-> {hiliteQQStrings} ||
			$self-> {hiliteIDs} ||
			$self-> {hiliteChars} ||
			$self-> {hiliteREs}) {
		$self-> {syntaxer} = sub {$_[2]=[];};
	} else {
		my @doers;
		my $rest = 'push @a, $l, cl::Fore if $l; $l = 0;';
		if ($self-> {hiliteREs}) {
			my $i;
			for ($i = 0; $i < scalar @{$self-> {hiliteREs}} - 1; $i+=2) {
				my $re = $self-> {hiliteREs}-> [$i];
				push @doers, "\$line =~ /\\G$re/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteREs}-> [$i+1] . "; redo; };\n";
			}
		}
		if ($self-> {hiliteIDs}) {
			my $i;
			for ($i = 0; $i < scalar @{$self-> {hiliteIDs}} - 1; $i+=2) {
				my $re = join '|', @{$self-> {hiliteIDs}-> [$i]};
				push @doers, "\$line =~ /\\G\\b($re)\\b/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteIDs}-> [$i+1] . "; redo; };\n";
			}
		}
		push @doers, '$line =~ /\\G\\b(\\d+)\\b/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteNumbers} . "; redo; };\n"
			if defined $self-> {hiliteNumbers};
		push @doers, '$line =~ /\\G("[^\\"\\\\]*(?:\\\\.[^\\"\\\\]*)*")/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteQQStrings} . "; redo; };\n"
			if defined $self-> {hiliteQQStrings};
		push @doers, '$line =~ /\\G(\'[^\\\'\\\\]*(?:\\\\.[^\\\'\\\\]*)*\')/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteQStrings} . "; redo; };\n"
			if defined $self-> {hiliteQStrings};
		if ($self-> {hiliteChars}) {
			my $i;
			for ($i = 0; $i < scalar @{$self-> {hiliteChars}} - 1; $i+=2) {
				my $re = quotemeta $self-> {hiliteChars}-> [$i];
				push @doers, "\$line =~ /\\G([$re]+)/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteChars}-> [$i+1] . "; redo; };\n";
			}
		}
		$self-> {syntaxer} = <<SYNTAXER;
		sub {
			my ( \$self, \$line) = \@_;
			my \@a;
			my \$l = 0;
			study(\$line);
			{
				@doers
				\$line =~ /\\G(.)/gc && do { \$l++; redo; };
			}
			$rest
			\@{\$_[2]} = \@a;
		};
SYNTAXER
		$self-> {syntaxer} = eval $self->{syntaxer};
		if ( $@) {
			warn "Error compiling highlighting regexes: $@\n";
			$self-> {syntaxer} = sub { @{$_[2]} = ( length($_[1]), cl::Fore ) };
		}
	}
}

sub _syntax_entry
{
	my ( $self, $index, $chunk, $syntax ) = @_;

	return $syntax unless length $chunk;

	my @entry;
	my $map = $self->get_shaped_chunk($index)->log2vis;
	my @colors = (0) x @$map;
	my $at     = 0;

	for ( my $j = 0; $j < $#$syntax; $j += 2 ) {
		my ( $len, $clr ) = @$syntax[$j,$j+1];
		$colors[ $map->[$at++] // next ] = $clr while $len--;
	}
	my $last_color = -1;
	for my $clr ( @colors ) {
		if ( $last_color == $clr ) {
			$entry[-2]++;
		} else {
			push @entry, 1, $last_color = $clr;
		}
	}

	return \@entry;
}

sub draw_colorchunk
{
	my ( $self, $canvas, $i, $x, $y, $clr) = @_;

	my $sd = $self-> {syntax}-> [$i];
	unless ( defined $sd) {
		my $text = $self->get_chunk($i);
		$sd = [];
		$self-> notify(q(ParseSyntax), $text, $sd);
		$sd = $self-> {syntax}-> [$i] = $self->_syntax_entry($i, $text, $sd);
	}

	my $s = $self->get_shaped_chunk($i);
	unless (@$sd) {
		$canvas-> color($clr);
		$canvas-> text_out( $s, $x, $y);
		return;
	}

	for ( my ($j,$last) = (0,0); $j < @$sd; $j += 2) {
		$canvas-> color(( $$sd[$j+1] == cl::Fore) ? $clr : $$sd[$j+1]);
		$canvas-> text_out( $s, $x, $y, $last, $$sd[$j]);
		$x += $canvas->get_text_width($s, 0, $last, $$sd[$j]);
		$last += $$sd[$j];
	}
}

sub paint_selection
{
	my ( $self, $canvas, $index, $x, $y, $width, $height, $sx1, $sx2, $color, $clipRect ) = @_;

	my $restore_clip;
	my $line_end;

	$sx1 = 0 if $sx1 eq 'start';
	$line_end = 1, $sx2 = $self->get_chunk_cluster_length($index) - 1 if $sx2 eq 'end';

	if ( $sx2 < 0 || ($line_end && $sx1 > $sx2) ) {
		if ( $line_end && $sx1 > $sx2 ) {
			my $glyphs = $self-> get_shaped_chunk($index);
			$canvas-> color( $self->hiliteBackColor );
			$canvas-> bar($x + $glyphs->get_width($canvas), $y, $width, $y + $height);
		}
		if ( $self->{syntaxHilite}) {
			$self-> draw_colorchunk( $canvas, $index, $x, $y, $color);
		} else {
			$canvas-> color( $color );
			$canvas-> text_out( $self-> get_shaped_chunk($index), $x, $y);
		}
		return;
	}

	($sx1, $sx2) = ($sx2, $sx1) if $sx2 < $sx1;

	my $glyphs = $self-> get_shaped_chunk($index);
	my $chunks = $glyphs->selection_chunks_clusters($glyphs-> selection2range($sx1, $sx2));
	my @r =$glyphs-> selection2range($sx1, $sx2);

	my @cr = @$clipRect;
	my $rx = $x;
	$glyphs->selection_walk( $chunks, 0, undef, sub {
		my ( $offset, $length, $selected ) = @_;

		$cr[0] = $rx;
		$rx += $glyphs->get_sub_width($canvas, $offset, $length);
		$cr[2] = ($line_end && $selected && $offset + $length >= $sx2) ? $width : ($rx - 1);

		$self->clipRect(@cr);
		$restore_clip = 1;

		if ( $selected ) {
			$canvas-> color( $self->hiliteBackColor );
			$canvas-> bar(0, $y, $width, $y + $height);
			$canvas-> color( $self->hiliteColor );
			$canvas-> text_out( $glyphs, $x, $y);
		} elsif ( $self->{syntaxHilite}) {
			$self-> draw_colorchunk( $canvas, $index, $x, $y, $color);
		} else {
			$canvas-> color( $color );
			$canvas-> text_out( $glyphs, $x, $y);
		}
	});

	$self-> clipRect( @$clipRect) if $restore_clip;
}

sub rtl_offset
{
	my $self = shift;
	my @size = $self->get_active_area(2);
	my $span = $self->{maxLineWidth};
	$span = $size[0] if $size[0] > $span;
	return $span - $self->{defcw} - 1;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;
	my @clr    = $self-> enabled ?
	( $self-> color, $self-> backColor) :
	( $self-> disabledColor, $self-> disabledBackColor);
	my @sclr   = ( $self-> hiliteColor, $self-> hiliteBackColor);
	my ( $bw, $fh, $tl, $lc, $ofs, $bt, $issel, $rtl) = (
		$self-> {borderWidth}, $self-> font-> height, $self-> {topLine},
		$self-> {maxChunk}+1, $self-> {offset}, $self-> {blockType},
		$self-> has_selection, $self-> {textDirection},
	);
	my @a = $self-> get_active_area( 0, @size);

	# drawing sheet
	my @clipRect = $self-> clipRect;
	if (
		$clipRect[0] > $a[0] &&
		$clipRect[1] > $a[1] &&
		$clipRect[2] < $a[2] &&
		$clipRect[3] < $a[3]
	) {
		$canvas-> color( $clr[ 1]);
		$canvas-> clipRect( $a[0], $a[1], $a[2] - 1, $a[3] - 1);
		$canvas-> bar( 0, 0, @size);
	} else {
		$self-> draw_border( $canvas, $clr[1], @size);
		$canvas-> clipRect( $a[0], $a[1], $a[2] - 1, $a[3] - 1);
	}
	$canvas-> color( $clr[0]);
	my $i;
	my $y = $a[3] - $fh;
	my $lim = int(( $a[3] - $clipRect[1]) / $fh) + $tl + 1;
	{
		my $fx = int(( $a[3] - $clipRect[3]) / $fh) + $tl;
		$fx = $tl if $fx < $tl;
		$y -= ( $fx - $tl) * $fh;
		$tl = $fx;
	}
	$lim = $lc if $lim > $lc;
	my $x = $a[0] - $ofs;
	$x += $self->rtl_offset if $rtl;

	# painting selection background as a big block
	my @sel;
	if ( $issel) {
		@sel  = (@{$self-> {selStartl}}, @{$self-> {selEndl}});
		if ( $bt == bt::CUA) {
			# paint simple selection over many lines
			if (
				$sel[1] != $sel[3] &&
				$sel[3] - 1 > $sel[1] &&
				( $sel[1] + 1 < $lim || $sel[ 3] - 1 >= $tl)
			) {
				$canvas-> color( $sclr[ 1]);
				$canvas-> bar(
					0, $y - $fh * ( $sel[1] - $tl) - 1,
					$size[0], $y - $fh * ( $sel[3] - $tl - 1)
				);
			}
		} elsif ( $bt == bt::Horizontal) {
			if ( $sel[1] < $lim || $sel[ 3] >= $tl) {
				$canvas-> color( $sclr[ 1]);
				$canvas-> bar( 0, $y - $fh * ( $sel[1] - $tl - 1) - 1, $size[0], $y - $fh * ( $sel[3] - $tl));

				# painting horizontal block lines, if available
				$canvas-> color( $sclr[0]);
				my ($from, $to) = (
					( $tl > $sel[1]) ?  $tl : $sel[1],
					( $lim < ($sel[3]+1)) ? $lim : ( $sel[3] + 1)
				);
				my $horz_y = $y - ( $from - $tl) * $fh;
				for ( $i = $from; $i < $to; $i++)
				{
					my $s = $self->get_shaped_chunk($i);
					my $X = $x - ($rtl ? $s->get_width($canvas) : 0);
					$canvas-> text_out( $s, $X, $horz_y);
					$horz_y -= $fh;
				}
				$canvas-> color( $clr[0]);
			}
		}
	}

	# painting lines
	for ( $i = $tl; $i < $lim; $i++)
	{
		my $X = $x - ($rtl ? $self->get_shaped_chunk($i)->get_width($canvas) : 0);
		if ( $issel && $i >= $sel[1] && $i <= $sel[3])
		{
			# painting selected lines
			if ( $bt == bt::CUA) {
				if ( $sel[1] == $sel[3]) {
					$self-> paint_selection( $canvas, $i, $X, $y, $size[0], $fh - 1, $sel[0], $sel[2] - 1, $clr[0], \@clipRect);
				} elsif ( $i == $sel[1]) {
					$self-> paint_selection( $canvas, $i, $X, $y, $size[0], $fh - 1, $sel[0], 'end'      , $clr[0], \@clipRect);
				} elsif ( $i == $sel[3]) {
					$self-> paint_selection( $canvas, $i, $X, $y, $size[0], $fh - 1, 'start', $sel[2] - 1, $clr[0], \@clipRect);
				} else {
					$canvas-> color( $sclr[0]);
					$canvas-> text_out( $self-> get_shaped_chunk($i), $X, $y);
				}
			} elsif ( $bt == bt::Vertical) {
				$self-> paint_selection( $canvas, $i, $X, $y, $size[0], $fh - 1, $sel[0], $sel[2] - 1, $clr[0], \@clipRect);
			}
		} else {
			$self-> paint_selection( $canvas, $i, $X, $y, $size[0], $fh - 1, -1, -1, $clr[0], \@clipRect);
		}
		$y -= $fh;
	}

	if ( my $dt = $self->{drop_transaction}) {
		$self->color(cl::Fore);
		$self-> bar(@$dt[0,1], $dt->[2]-1,$dt->[3]-1);
	}
}

sub point2xy
{
	my ( $self, $x, $y) = @_;
	my ( $fh, $ofs, $avg, @a) = (
		$self-> font-> height, $self-> {offset},
		$self-> {averageWidth}, $self-> get_active_area
	);
	my ( $rx, $ry, $inBounds);
	$inBounds = !( $x <= $a[0] || $x > $a[2] || $y < $a[1] || $y > $a[3]);
	$x -= $a[0];
	$y -= $a[1];
	my ( $w, $h) = ( $a[2] - $a[0], $a[3] - $a[1]);

	$y  = $h + $fh if $y > $h;
	$y  = -$fh if $y < 0;
	$x  = $w + $avg * 2 if $x > $w + $avg * 2;
	$x  = - $avg * 2 if $x < - $avg * 2;
	$ry = int(( $h - $y) / $fh) + $self-> {topLine};
	$ry = 0 if $ry < 0;
	$ry = $self-> {maxChunk} if $ry > $self-> {maxChunk};
	$rx = 0;

	my $s = $self-> get_shaped_chunk($ry);
	$x -= $self->rtl_offset - $s-> get_width($self) if $self->{textDirection};
	my $ofsx = $ofs + $x;
	$rx = $s->x2cluster($self, $ofsx);
	return $self-> logical_to_visual( $rx, $ry), $inBounds;
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left && $btn != mb::Middle;
	my @xy = $self-> point2xy( $x, $y);
	return unless $xy[2];

	if ( $btn == mb::Middle) {
		$self-> cursor( @xy);
		my $cp = $::application-> bring('Primary');
		return if !$cp || $self-> {readOnly};
		$self-> insert_text( $cp-> text, 0);
		$self-> clear_event;
		return;
	}

	my @sel = $self->selection;
	if (
		$self->has_selection &&
		!$self->{drop_transaction} && 
		(
			($sel[1] == $sel[3] && $xy[1] == $sel[1] && $xy[0] >= $sel[0] && $xy[0] < $sel[2]) ||
			($xy[1] == $sel[1] && $xy[1] < $sel[3] && $xy[0] >= $sel[0]) ||
			($xy[1] == $sel[3] && $xy[1] > $sel[1] && $xy[0] < $sel[2]) ||
			($xy[1] > $sel[1] && $xy[1] < $sel[3])
		)
	) {
		$self-> {drag_transaction} = 1;
		my $act = $self-> begin_drag(
			text       => $self->get_selected_text,
			actions    => dnd::Copy|( $self->{readOnly} ? 0 : dnd::Move),
			self_aware => 0,
		);
		$self-> {drag_transaction} = 0;
		$self-> delete_block if !$self->{readOnly} && $act == dnd::Move;
		if ($act < 0 ) {
			$self-> cancel_block unless $self->persistentBlock;
			$self-> cursor( @xy);
		}
		return;
	}

	$self-> cursor( @xy);
	$self-> {mouseTransaction} = 1;
	if ( $self-> {persistentBlock} && $self-> has_selection) {
		$self-> {mouseTransaction} = 2;
	} else {
		$self-> start_block unless exists $self-> {anchor};
	}
	$self-> capture(1);
	$self-> clear_event;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	return if $btn != mb::Left;
	$self-> capture(0);
	$self-> end_block unless $self-> {mouseTransaction} == 2;
	$self-> {mouseTransaction} = undef;
	$self-> clear_event;

	return if $self-> {writeOnly} || !$self-> has_selection;
	my $cp = $::application-> bring('Primary');
	$cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransaction};
	my @xy = $self-> point2xy( $x, $y);
	$self-> clear_event;
	if ( $xy[2])
	{
		$self-> scroll_timer_stop;
	} else {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
	}
	$self-> {delayPanning} = 1;
	$self-> blockShiftMark(1);
	$self-> cursor( @xy);
	$self-> blockShiftMark(0);
	$self-> update_block unless $self-> {mouseTransaction} == 2;
	$self-> realize_panning;
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$z *= $self-> {rows} if $mod & km::Ctrl;
	my $newTop = $self-> topLine  - $z;
	my $maxTop = $self-> {maxChunk} - $self-> {rows} + 1;
	$self-> topLine ( $newTop > $maxTop ? $maxTop : $newTop);
	$self-> clear_event;
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $nth) = @_;

	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	my @xy = $self-> point2xy( $x, $y);
	return unless $xy[2];

	my $s = $self-> get_chunk($xy[1]);
	my $sl = $self-> get_chunk_cluster_length( $xy[1]);
	$self-> clear_event;

	return if $nth < 2 || $nth > 3;
	return $self-> selection( 0, $xy[1], $sl, $xy[1]) if $nth == 3;

	$self-> cancel_block;
	$self-> cursor( @xy);

	my $p = $self->visual_to_physical(@xy);
	my ($l,$r);
	$sl = length $s;
	return unless $sl;

	$p = $sl-1 if $p >= $sl;
	my $word = quotemeta($self-> {wordDelimiters});
	my $nonword = "[$word]";
	$word = "[^$word]";
	if ( substr($s,$p,1) =~ /$word/) {
		substr($s,0,$p) =~ /($word*)$/;
		$l = $p - length $1;
		substr($s,$p) =~ /^($word*)/;
		$r = $p + length $1;
	} else {
		substr($s,0,$p) =~ /($nonword*)$/;
		$l = $p - length $1;
		substr($s,$p) =~ /^($nonword*)/;
		$r = $p + length $1;
	}

	$l = $self-> physical_to_visual( $l, $xy[1] );
	$r = $self-> physical_to_visual( $r - 1, $xy[1]);
	($l > $r) ? $l++ : $r++;

	$self-> selection( $l, $xy[1], $r, $xy[1] );
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;
	return if $self-> {readOnly};
	return if $mod & km::DeadKey;
	$mod &= ( km::Shift|km::Ctrl|km::Alt);
	$self-> notify(q(MouseUp),0,0,0) if $self-> {mouseTransaction};
	if ( $key == kb::Tab && !$self-> {wantTabs}) {
		return unless $mod & km::Ctrl;
		$mod &= ~km::Ctrl;
	}
	if  (
		( $code >= ord(' ') || ( $code == ord("\t"))) &&
		(( $mod  & (km::Alt | km::Ctrl)) == 0) &&
		(( $key == kb::NoKey) || ( $key == kb::Space) || ( $key == kb::Tab))
	) {
		$self-> begin_undo_group;

		my $block = $self-> has_selection;
		$self-> delete_block if $block;

		my @cs = $self-> cursor;
		my $chr = chr $code;
		utf8::upgrade($chr) if $mod & km::Unicode;
		my ($c, $s) = $self-> get_line_for_edit(@cs);
		my ($new_text, $new_offset) = $self->handle_bidi_input(
			action     => (($block || $self->insertMode) ? q(insert) : q(overtype)),
			at         => $cs[0],
			input      => $chr x $repeat,
			text       => $c,
			rtl        => $self->textDirection,
			glyphs     => $s,
			n_clusters => $s->n_clusters,
		);
		$self-> set_line( $cs[1], $new_text, (($block || $self->insertMode) ?
			(q(add), $cs[0], $repeat) :
			q(overtype)
		));

		$self-> cursor(
			$self->text_shape_with_tabs_replaced($new_text)->index2cluster($new_offset, 1),
			$cs[1]
		);
		$self-> end_undo_group;
		$self-> clear_event;
	}
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> reset_shaping_caches;
	$self-> reset_render;
	$self-> reset_scrolls;
}

sub on_size
{
	my $self = $_[0];
	$self-> reset_render;
	$self-> reset_scrolls;
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }

sub on_enter
{
	my $self = $_[0];
	$self-> insertMode( $::application-> insertMode);
}

sub on_change { $_[0]-> {modified} = 1;}

sub on_parsesyntax { $_[0]-> {syntaxer}-> (@_); }

sub on_dragbegin
{
	my $self = shift;
	$self->{drop_transaction} = [];
}

sub on_dragover
{
	my ($self, $clipboard, $action, $mod, $x, $y, $ref) = @_;
	$ref->{allow} = 1;
	my $dt;
	if ( $dt = $self->{drop_transaction} and @$dt) {
		$self-> invalidate_rect(@$dt);
	}
	$self-> cursor( $self-> point2xy( $x, $y));
	my @cp = $self->cursorPos;
	my @cs = $self->cursorSize;
	$self->{drop_transaction} = [@cp, $cp[0] + $self->{defcw}, $cp[1] + $cs[1]];
	$self-> invalidate_rect(@{ $self->{drop_transaction} });
}

sub on_dragend
{
	my ($self, $clipboard, $action, $mod, $x, $y, $ref) = @_;
	my $dt;
	if ( $dt = $self->{drop_transaction} and @$dt ) {
		$self-> invalidate_rect(@$dt);
	}
	delete $self->{drop_transaction};
	return unless $clipboard;
	my $cap = $clipboard->text;
	$self->insert_text($cap) if defined $cap;
}

sub set_block_type
{
	my ( $self, $bt) = @_;
	return if $bt == $self-> {blockType};
	$self-> push_group_undo_action('blockType', $self-> {blockType});
	$self-> {blockType} = $bt;
	return unless $self-> has_selection;
	$self-> reset_render;
	$self-> repaint;
}

sub reset_shaping_caches
{
	my $self = shift;
	undef $self->{shapedChunk};
	undef $self->{shapedIndex};
	undef $self->{shapedClusters};
}

sub set_text_ref
{
	my ( $self, $ref) = @_;
	return unless defined $ref;
	$self-> {capLen} = length( $$ref) || 0;
	$#{$self-> {lines}} = $self-> {capLen} / 40;
	@{$self-> {lines}} = ();
	@{$self-> {lines}} = split( "\n", $$ref // '');
	$self-> {maxLine} = scalar @{$self-> {lines}} - 1;
	$self-> reset_shaping_caches;
	$self-> reset_syntax;
	$self-> reset_scrolls;
	if ( !$self-> {resetDisabled}) {
		$self-> lock;
		$self-> selection(0,0,0,0);
		$self-> reset;
		$self-> cursor($self-> {cursorX}, $self-> {cursorY});
		$self-> unlock;
		$self-> notify(q(Change));
		$self-> reset_scrolls;
	}
}

sub text
{
	unless ($#_) {
		my $hugeScalarRef = $_[0]-> textRef;
		return $$hugeScalarRef;
	} else {
		$_[0]-> textRef( \$_[1]);
	}
}

sub textDirection
{
	return $_[0]-> {textDirection} unless $#_;
	my ( $self, $td ) = @_;
	return if $self->{textDirection} == $td;
	$self-> {textDirection} = $td;
	$self-> reset_shaping_caches;
	$self-> reset_syntax;
	$self-> reset_scrolls;
	$self-> reset;
	$self-> repaint;
}

sub textLigation
{
	return $_[0]-> {textLigation} unless $#_;
	my ( $self, $td ) = @_;
	return if $self->{textLigation} == $td;
	$self-> {textLigation} = $td;
	$self-> reset_shaping_caches;
	$self-> reset_syntax;
	$self-> reset_scrolls;
	$self-> reset;
	$self-> repaint;
}

sub get_text_ref
{
	my $self = $_[0];
	my $hugeScalar = join( "\n", @{$self-> {lines}});
	return \$hugeScalar;
}

sub get_chunk
{
	my ( $self, $index) = @_;
	my $ck = $self-> {lines};
	return '' if $self-> {maxLine} < 0;
	Carp::confess($index) if $index > $self-> {maxChunk};
	if ( $self-> {wordWrap}) {
		my $cm = $self-> {chunkMap};
		$index *= CM_SIZE;
		return substr(
			$$ck[ $$cm[ $index + CM_Y ]],
			$$cm[ $index + CM_CHAR_OFS ],
			$$cm[ $index + CM_CHAR_LEN ]
		);
	} else {
		return $$ck[ $index];
	}
}

sub get_line
{
	my ( $self, $index) = @_;
	return $self-> {maxLine} >= 0 ? $self-> {lines}-> [$index] : '';
}

sub get_line_for_edit
{
	my ($self, $x, $y) = @_;
	my $c  = $self-> get_line( $y);
	if ( defined $x ) {
		my $ll = length($c);
		$c .= ' ' x ($x - $ll) if $x > $ll;
	}
	my $s = $self-> text_shape_with_tabs_replaced($c);
	return ($c, $s);
}

sub text_shape_with_tabs_replaced
{
	my ( $self, $line) = @_;
	return Prima::Drawable::Glyphs->new_empty unless length $line;
	my $s = $self-> text_shape($line,
		rtl          => $self->textDirection,
		level        => ( $self-> textLigation ? ts::Full : ts::Glyphs ),
		replace_tabs => $self->{tabIndent},
		advances     => 1,
	);
	unless ($s) {
		warn "Prima::Edit: shaping error!\n";
		eval { require Data::Dumper; };
		unless ($@) {
			print STDERR Data::Dumper::Dumper($line, $s);
		}
		$s = Prima::Drawable::Glyphs->new_empty;
	}
	return $s;
}

sub get_shaped_chunk
{
	my ( $self, $index) = @_;
	return Prima::Drawable::Glyphs-> new_empty if $self-> {maxLine} < 0;
	return $self-> {shapedChunk} if ($self->{shapedIndex} // -1) == $index;

	$self->reset_shaping_caches;

	$self->{shapedIndex} = $index;
	my $chunk = $self-> get_chunk($index);
	return $self->{shapedChunk} = Prima::Drawable::Glyphs->new_empty
		unless length $chunk;
	return $self->{shapedChunk} = $self-> text_shape_with_tabs_replaced($chunk);
}

sub get_line_dimension
{
	my ( $self, $y) = @_;
	return $y, 1 unless $self-> {wordWrap};
	my (undef, $ly) = $self-> physical_to_logical( undef, $y);
	my ($ret, $ix, $cm) = ( 0, $ly * CM_SIZE + CM_Y, $self-> {chunkMap});
	$ret++, $ix += CM_SIZE while $ix < @$cm && $$cm[$ix] == $y;
	return $ly, $ret;
}

sub get_chunk_cluster_length
{
	my ($self, $index) = @_;
	if ( $self->{wordWrap}) {
		my $cm = $self->{chunkMap};
		my $ix = $index * CM_SIZE + CM_CLUSTER_LEN;
		return 0 if $ix >= @$cm;
		return $$cm[$ix] //= $self->get_shaped_chunk($index)-> n_clusters;
	} else {
		$self->reset_shaping_caches if ($self->{shapedIndex} // -1) != $index;
		return $self->{shapedClusters} if defined $self->{shapedClusters};
		return $self->{shapedClusters} = $self->get_shaped_chunk($index)-> n_clusters;
	}
}

sub get_line_cluster_length
{
	my ($self, $index) = @_;
	my ($y, $n_chunks) = $self-> get_line_dimension($index);
	my $x = 0;
	$x += $self->get_chunk_cluster_length($y + $_) for 0 .. $n_chunks - 1;
	return $x;
}

sub get_chunk_width
{
	my ( $self, $index, $from, $len) = @_;
	return $len * $self->{averageWidth} + 
		(($len + $from <= 0) ? 0 : $self->get_chunk_width($index, 0, $len + $from)) 
			if $from < 0;

	my $n = $self->get_chunk_cluster_length($index);
	return $len * $self->{averageWidth} if $from >= $n;

	my $s = $self->get_shaped_chunk($index);
	return $s->get_sub_width($self, $from, $len) if $from + $len <= $n;

	my $sublen = $n - $from;
	my $tw = $s->get_sub_width($self, $from, $sublen);
	$tw += ($len - $sublen) * $self->{averageWidth};
	return $tw;
}

sub has_selection
{
	my @s  = $_[0]-> selection;
	return !(($s[0] == $s[2]) && ( $s[1] == $s[3]));
}

sub realize_panning
{
	delete $_[0]-> {delayPanning};
	for ( qw( topLine  offset)) {
		my $c = 'delay_' . $_;
		next unless defined $_[0]-> {$c};
		$_[0]-> $_( $_[0]-> {$c});
		delete $_[0]-> {$c};
	}
}

sub set_cursor
{
	my ( $self, $x, $y) = @_;
	my ( $ox, $oy) = ($self-> {cursorX}, $self-> {cursorY});
	my $maxY = $self-> {maxLine};
	$y = $maxY if $y < 0 || $y > $maxY;
	$y = 0 if $y < 0; # ??
	my $max_x = $self-> get_line_cluster_length($y);
	$x = $max_x if $x < 0 || $x > $max_x;

	my ( $lx, $ly) = $self-> visual_to_logical( $x, $y);
	my $cm = $self->{chunkMap};
	my ( $olx, $oly) = ( $self-> {cursorXl}, $self-> {cursorYl});
	$self-> {cursorXl} = $lx;
	$self-> {cursorYl} = $ly;
	my ($atX, $deltaX);
	if ( $self-> textDirection) {
		$deltaX = $self-> get_chunk_width( $ly, $lx - 1, 1);
		$atX    = ($lx > 0) ? $self-> get_chunk_width( $ly, 0, $lx - 1) : -$deltaX;
		$atX += $self->rtl_offset - $self->get_shaped_chunk($ly)->get_width($self);
	} else {
		$atX = $self-> get_chunk_width( $ly, 0, $lx);
		$deltaX = $self-> get_chunk_width( $ly, $lx, 1);
	}

	return if 
		$y == $oy and $x == $ox and $lx == $olx and $ly == $oly and
		# these can change with ligatures
		$atX    == ($self->{cursorAtX} // -1_000_000) and
		$deltaX == ($self->{cursorInsWidth} // -1_000_000);

	my ( $tl, $r, $yt) = ( $self-> {topLine}, $self-> {rows}, $self-> {yTail});
	if ( $ly < $tl) {
		$self-> topLine ( $ly);
	} elsif ( $ly >= $tl + $r) {
		my $nfc = $ly - $r + 1;
		$self-> topLine ( $nfc);
	}
	my $actualWidth = $self-> width -
		$self-> {indents}-> [0] -
		$self-> {indents}-> [2] -
		$self-> {defcw} * 2 - 1;
	my $ofs = $self-> {offset};
	my $avg = $self-> {averageWidth};
	$self-> {cursorX}        = $x;
	$self-> {cursorY}        = $y;
	$self-> {cursorAtX}      = $atX;
	$self-> {cursorInsWidth} = $deltaX;

	if ( $atX < $ofs) {
		my $nofs = $atX;
		$self-> offset( $nofs - $avg);
	} elsif ( $atX >= $ofs + $actualWidth - $deltaX) {
		my $nofs = $atX - $actualWidth * ($self-> textDirection ? 0 : 1) + $deltaX;
		$nofs = $ofs + $avg if $nofs - $ofs < $avg;
		$self-> offset( $nofs);
	}
	# check if last undo record contains cursor movements only, so these movements
	# can be grouped
	$self-> push_undo_action( 'cursor', $self-> {cursorX}, $self-> {cursorY})
		unless $self->has_undo_action('cursor');

	$self-> reset_cursor;
	$self-> cancel_block
		if !$self-> {blockShiftMark} && !$self-> {persistentBlock};
}

sub set_top_line
{
	my ( $self, $tl) = @_;
	$tl = $self-> {maxChunk} if $tl >= $self-> {maxChunk};
	$tl = 0 if $tl < 0;
	return if $self-> {topLine } == $tl;
	if ( $self-> {delayPanning}) {
		$self-> {delay_topLine } = $tl;
		return;
	}
	my $dt = $tl - $self-> {topLine };
	$self-> push_group_undo_action( 'topLine', $self-> {topLine});
	$self-> {topLine } = $tl;
	if ( $self-> {vScroll} && $self-> {scrollTransaction} != 1) {
		$self-> {scrollTransaction} = 1;
		$self-> {vScrollBar}-> value( $tl);
		$self-> {scrollTransaction} = 0;
	}
	$self-> reset_cursor;
	$self-> scroll( 0, $dt * $self-> font-> height,
		clipRect => [ $self-> get_active_area]);
}

sub reset_indents
{
	my ( $self) = @_;
	$self-> reset_render;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_hilite_numbers
{
	my $self = $_[0];
	$self-> {hiliteNumbers} = $_[1];
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_hilite_q_strings
{
	my $self = $_[0];
	$self-> {hiliteQStrings} = $_[1];
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_hilite_qq_strings
{
	my $self = $_[0];
	$self-> {hiliteQQStrings} = $_[1];
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_hilite_ids
{
	my ($self, $hi) = @_;
	if ( $hi) {
		push @{$hi}, cl::Fore if scalar @{$hi} / 2 != 0;
		$hi = [@{$hi}];
	}
	$self-> {hiliteIDs} = $hi;
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_hilite_chars
{
	my ($self, $hi) = @_;
	if ( $hi) {
		push @{$hi}, cl::Fore if scalar @{$hi} / 2 != 0;
		$hi = [@{$hi}];
	}
	$self-> {hiliteChars} = $hi;
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_hilite_res
{
	my ($self, $hi) = @_;
	if ( $hi) {
		push @{$hi}, cl::Fore if scalar @{$hi} / 2 != 0;
		$hi = [@{$hi}];
	}
	$self-> {hiliteREs} = $hi;
	if ( $self-> {syntaxHilite}) {
		$self-> reset_syntaxer;
		$self-> repaint;
	}
}

sub set_insert_mode
{
	my ( $self, $insert) = @_;
	my $oi = $self-> {insertMode};
	$self-> {insertMode} = $insert;
	$self-> reset_cursor if $oi != $insert;
	$::application-> insertMode( $insert);
	$self-> push_group_undo_action( 'insertMode', $oi) if $oi != $insert;
}


sub set_offset
{
	my ( $self, $offset) = @_;
	if ( $self->{wordWrap}) {
		$offset = 0;
	} else {
		my @size = $self->get_active_area(2);
		$offset = $self->{maxLineWidth} - $size[0]
			if $offset > $self->{maxLineWidth} - $size[0];
		$offset = 0 if $offset < 0;
	}
	return if $self-> {offset} == $offset;
	if ( $self-> {delayPanning}) {
		$self-> {delay_offset} = $offset;
		return;
	}
	my $dt = $offset - $self-> {offset};
	$self-> push_group_undo_action( 'offset', $self-> {offset});
	$self-> {offset} = $offset;
	if ( $self-> {hScroll} && $self-> {scrollTransaction} != 2) {
		$self-> {scrollTransaction} = 2;
		$self-> {hScrollBar}-> value( $offset);
		$self-> {scrollTransaction} = 0;
	}
	$self-> reset_cursor;
	$self-> scroll( -$dt, 0,
		clipRect => [ $self-> get_active_area]);
}

sub set_readonly
{
	my ( $self, $ro ) = @_;
	return if ($self->{readOnly} ? 1 : 0) == ($ro ? 1 : 0);
	$self->{readOnly} = $ro;
	$self-> accelTable-> enabled($_, !$ro) for qw(
		Delete Backspace CtrlBackspace CtrlDelete
		DeleteChunk DeleteToEnd DupLine DeleteBlock
		SplitLine SplitLine2
		CopyBlock OvertypeBlock
		Cut Paste CutMS PasteMS
		Undo Redo
	);
}

sub set_selection
{
	my ( $self, $sx, $sy, $ex, $ey) = @_;
	my $maxY = $self-> {maxLine};
	my ( $osx, $osy, $oex, $oey) = $self-> selection;
	my $onsel = ( $osx == $oex && $osy == $oey);
	if ( $maxY < 0) {
		$self-> {selStart}  = [0,0];
		$self-> {selEnd}    = [0,0];
		$self-> {selStartl} = [0,0];
		$self-> {selEndl  } = [0,0];
		$self-> repaint unless $onsel;
		return;
	}
	$sy  = $maxY if $sy < 0 || $sy > $maxY;
	$ey  = $maxY if $ey < 0 || $ey > $maxY;
	( $sy, $ey, $sx, $ex) = ( $ey, $sy, $ex, $sx) if $sy > $ey;
	$osx = $oex = $sx,  $osy = $oey = $ey  if $onsel;
	if ( $sx == $ex && $sy == $ey) {
		$osy  = $maxY if $osy < 0 || $osy > $maxY;
		$oey  = $maxY if $oey < 0 || $oey > $maxY;
		$sx  = $ex  = $osx;
		$sy  = $ey  = $osy;
	}
	my ($fcl, $lcl) = map { length $self-> get_line($_) } ($sy, $ey);
	my $bt = $self-> {blockType};
	$sx = $fcl if ( $bt != bt::Vertical && $sx > $fcl) || ( $sx < 0);
	$ex = $lcl if ( $bt != bt::Vertical && $ex > $lcl) || ( $ex < 0);
	( $sx, $ex) = ( $ex, $sx) if $sx > $ex && (( $sy == $ey && $bt == bt::CUA) || ( $bt == bt::Vertical));
	my ( $lsx, $lsy) = $self-> visual_to_logical( $sx, $sy);
	my ( $lex, $ley) = $self-> visual_to_logical( $ex, $ey);
	( $lsx, $lex) = ( $lex, $lsx) if $lsx > $lex && (( $lsy == $ley && $bt == bt::CUA) || ( $bt == bt::Vertical));
	$sy = $ey if $sx == $ex and $bt == bt::Vertical;
	my ( $_osx, $_osy) = @{$self-> {selStartl}};
	my ( $_oex, $_oey) = @{$self-> {selEndl}};
	$self-> {selStart}  = [ $sx, $sy];
	$self-> {selStartl} = [ $lsx, $lsy];
	$self-> {selEnd}    = [ $ex, $ey];
	$self-> {selEndl}   = [ $lex, $ley];
	return if $sx == $osx && $ex == $oex && $sy == $osy && $ey == $oey;
	return if $sx == $ex && $sy == $ey && $onsel;
	$self-> push_group_undo_action('selection', $osx, $osy, $oex, $oey);
	( $osx, $osy, $oex, $oey) = ( $_osx, $_osy, $_oex, $_oey);
	( $sx, $sy)   = @{$self-> {selStartl}};
	( $ex, $ey)   = @{$self-> {selEndl}};
	$osx = $oex = $sx,  $osy = $oey = $ey  if $onsel;
	if (( $osy > $ey && $oey > $ey) || ( $oey < $sy && $oey < $sy))
	{
		$self-> repaint;
		return;
	}

	# connective selection
	my ( $start, $end);
	if ( $bt == bt::CUA || ( $sx == $osx && $ex == $oex)) {
		if ( $sy == $osy) {
			if ( $ey == $oey) {
				if ( $sx == $osx) {
					$start = $end = $ey;
					$sx = $osx = length $self->get_chunk($start) if $ey != $sy;
				} elsif ( $ex == $oex) {
					$start = $end = $sy;
					$ex = $oex = length $self->get_chunk($start) if $ey != $sy;
				} else {
					($start, $end) = ( $sy, $ey);
				}
			} else {
				( $start, $end) = ( $ey < $oey) ? ( $ey, $oey) : ( $oey, $ey);
			}
		} elsif ( $ey == $oey) {
			( $start, $end) = ( $sy < $osy) ? ( $sy, $osy) : ( $osy, $sy);
		} else {
			$start = ( $sy < $osy) ? $sy : $osy;
			$end   = ( $ey > $oey) ? $ey : $oey;
		}
	} else {
		$start = ( $sy < $osy) ? $sy : $osy;
		$end   = ( $ey > $oey) ? $ey : $oey;
	}
	my ( $ofs, $tl, $fh, $r, $yT) = (
		$self-> {offset}, $self-> {topLine },
		$self-> font-> height, $self-> {rows},
		$self-> {yTail}
	);
	return if $end < $tl || $start >= $tl + $r + $yT;

	my @a = $self-> get_active_area( 0);
	if ( $start == $end && $bt != bt::Horizontal) {
		$self-> begin_paint_info;
		my $glyphs = $self-> get_shaped_chunk($start);
		my $old  = ($osx == $oex) ? [] : $glyphs->selection_chunks_clusters($glyphs-> selection2range($osx, $oex - 1));
		my $new  = ($sx  == $ex)  ? [] : $glyphs->selection_chunks_clusters($glyphs-> selection2range($sx,  $ex - 1));
		my $diff = $glyphs->selection_diff($old, $new);

		my @cr;
		my $x = $a[0] - $self->offset;
		$x += $self->rtl_offset - $glyphs->get_width($self) if $self-> {textDirection};
		$glyphs->selection_walk( $diff, 0, undef, sub {
			my ( $offset, $length, $selected ) = @_;
			my $dx = $glyphs->get_sub_width($self, $offset, $length);
			if ( $selected ) {
				$cr[0] //= $x;
				$cr[1] = $x + $dx;
			}
			$x += $dx;
		});
		$self-> end_paint_info;
		if ( @cr ) {
			$cr[1]++;
			$a[0] = $cr[0] if $a[0] < $cr[0];
			$a[2] = $cr[1] if $a[2] > $cr[1];
		}
	}

	$self-> invalidate_rect(
		$a[0], $a[3] - $fh * ( $end - $tl + 1),
		$a[2], $a[3] - $fh * ( $start - $tl),
	);
}

sub set_tab_indent
{
	my ( $self, $ti) = @_;
	$ti = 0 if $ti < 0;
	$ti = 256 if $ti > 256;
	return if $ti == $self-> {tabIndent};
	$self-> {tabIndent} = $ti;
	$self-> reset;
	$self-> repaint;
}

sub set_syntax_hilite
{
	my ( $self, $sh) = @_;
	return if $sh == $self-> {syntaxHilite};
	$self-> {syntaxHilite} = $sh;
	$self-> reset_syntaxer if $sh;
	$self-> reset_syntax;
	$self-> repaint;
}

sub set_word_wrap
{
	my ( $self, $ww) = @_;
	return if $ww == $self-> {wordWrap};
	$self-> {wordWrap} = $ww;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub cut
{
	my $self = $_[0];
	return if $self-> {readOnly};
	$self-> begin_undo_group;
	$self-> copy;
	$self-> delete_block;
	$self-> end_undo_group;
}

sub copy
{
	my $self = $_[0];
	my $text = $self-> get_selected_text;
	$::application-> Clipboard-> text($text) if defined $text;
}

sub get_selected_text
{
	my $self = $_[0];
	return undef unless $self-> has_selection;
	my $text = '';
	my $bt = $self-> blockType;

	if ( $bt == bt::CUA) {
		my @sel = $self->selection_to_physical;
		if ( $sel[1] == $sel[3]) {
			$text = substr( $self-> get_line( $sel[1]), $sel[0], $sel[2] - $sel[0]);
		} else {
			my $c = $self-> get_line( $sel[1]);
			$text = substr( $c, $sel[0], length( $c) - $sel[0])."\n";
			my $i;
			for ( $i = $sel[1] + 1; $i < $sel[3]; $i++) {
				$text .= $self-> get_line( $i)."\n";
			}
			$c = $self-> get_line( $sel[3]);
			$text .= substr( $c, 0, $sel[2]);
		}
	} elsif ( $bt == bt::Horizontal) {
		my $i;
		my @sel = $self->selection_to_physical;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			$text .= $self-> get_line( $i)."\n";
		}
	} else {
		my $i;
		my @sel = $self->selection;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			my @s2 = $self->selection_to_physical( $sel[0], $i, $sel[2], $i);
			my $c  = $self-> get_line( $i);
			my $cl = $sel[2] - length( $c);
			$c .= ' 'x$cl if $cl > 0;
			$text .= substr($c, $s2[0], $s2[2] - $s2[0])."\n";
		}
		chomp( $text);
	}
	return $text;
}

sub lock_change
{
	my ( $self, $lock) = @_;
	$lock = $lock ? 1 : -1;
	$self-> {notifyChangeLock} += $lock;
	$self-> {notifyChangeLock} = 0 if $lock > 0 && $self-> {notifyChangeLock} < 0;
	$self-> notify(q(Change)) if $self-> {notifyChangeLock} == 0 && $lock < 0;
}

sub change_locked
{
	my $self = $_[0];
	return $self-> {notifyChangeLock} != 0;
}

sub insert_text
{
	my ( $self, $s, $hilite) = @_;
	return if !defined($s) or length( $s) == 0;
	$self-> begin_undo_group;
	$self-> cancel_block unless $self-> {blockType} == bt::CUA;
	my @cs = $self-> cursor;
	my $px = $self-> visual_to_physical(@cs);
	my @ln = split( "\n", $s, -1);
	pop @ln unless length $ln[-1];
	$s = $self-> get_line( $cs[1]);
	my $cl = $cs[0] - length( $s);
	$s .= ' 'x$cl if $cl > 0;
	$cl = 0 if $cl < 0;
	$self-> lock_change(1);
	if ( scalar @ln == 1) {
		substr( $s, $px, 0) = $ln[0];
		$self-> set_line( $cs[1], $s, q(add), $cs[0], $cl + length( $ln[0]));
		my $sx1 = $self-> physical_to_visual($px, $cs[1]);
		my $sx2 = $self-> physical_to_visual($px + length($ln[0]) - 1, $cs[1]);
		($sx1 > $sx2) ? $sx1++ : $sx2++;
		$self-> selection( $sx1, $cs[1], $sx2, $cs[1])
			if $hilite && $self-> {blockType} == bt::CUA;
	} else {
		my $spl = substr( $s, $px, length( $s) - $px);
		substr( $s, $px, length( $s) - $px) = shift @ln;
		$self-> lock;
		$self-> set_line( $cs[1], $s);
		$self-> insert_line( $cs[1] + 1, (@ln, $spl));
		$self-> selection(
			$self-> physical_to_visual(@cs), $cs[1],
			0, $cs[1] + @ln + 1
		) if $hilite && $self-> {blockType} == bt::CUA;
		$self-> unlock;
	}
	$self-> lock_change(0);
	$self-> end_undo_group;
}

sub paste
{
	my $self = $_[0];
	return if $self-> {readOnly};
	$self-> insert_text( $::application-> Clipboard-> text, 1);
}

sub get_chunk_dimension
{
	my ( $self, $ly ) = @_;
	my ($n, $ix, $cm) = ( 0, $ly * CM_SIZE, $self-> {chunkMap});
	$ly--, $ix -= CM_SIZE while $$cm[$ix + CM_CHAR_OFS] != 0;
	$ix += CM_Y;
	my $y = $$cm[$ix];
	$n++ , $ix += CM_SIZE while $ix < @$cm && $$cm[$ix] == $y;
	return $ly, $n;
}

sub visual_to_physical
{
	my ( $self, $x, $y) = @_;
	$x = 0 if $x < 0;

	return $self-> get_shaped_chunk($y)->cluster2index($x) & ~to::RTL
		unless $self-> {wordWrap};

	my $cm = $self->{chunkMap};
	my ($ly, $chunks) = $self-> get_line_dimension($y);
	my ($px, $i) = (0);
	for ( $i = 0; $i < $chunks; $i++) {
		$px = $$cm[($ly + $i) * CM_SIZE + CM_CHAR_OFS];
		my $n_clusters = $self-> get_chunk_cluster_length( $ly + $i );
		last if $x < $n_clusters;
		if ( $i == $chunks - 1 ) {
			$x = $n_clusters if $x > $n_clusters;
			last;
		}
		$x -= $n_clusters;
	}

	return $px + $self-> get_shaped_chunk($ly + $i)->cluster2index($x) & ~to::RTL;
}

sub logical_to_physical
{
	my ( $self, $x, $y) = @_;
	return $self-> visual_to_physical($x,$y) unless $self->{wordWrap};

	$y = $self-> {maxChunk} if $y > $self-> {maxChunk};
	$y = 0 if $y < 0;
	my $cm = $self-> {chunkMap};
	return 0 unless @$cm;

	my ($ly, $chunks) = $self-> get_chunk_dimension($y);
	$x = 0 if $x < 0;
	my $px = 0;
	for ( my $i = 0; $i < $chunks; $i++) {
		$px = $$cm[($ly + $i) * CM_SIZE + CM_CHAR_OFS];
		my $n_clusters = $self-> get_chunk_cluster_length( $ly + $i );
		last if $x < $n_clusters;
		if ( $i == $chunks - 1 ) {
			$x = $n_clusters if $x > $n_clusters;
			last;
		}
		$x -= $n_clusters;
	}
	return $px + $x;
}

sub logical_to_visual
{
	my ( $self, $x, $y) = @_;
	$y = 0 if $y < 0;
	unless ($self->{wordWrap}) {
		$y = $self-> {maxLine} if $y > $self-> {maxLine};
		$x = $self->get_chunk_cluster_length($y) if defined($x) && $x < 0;
		return $x, $y;
	}

	my $cm = $self-> {chunkMap};
	return (0,0) unless @$cm;

	$y = $self-> {maxChunk} if $y > $self-> {maxChunk};
	$x = 0 if defined($x) && $x < 0;
	my $nY = $$cm[ $y * CM_SIZE + CM_Y];

	if ( defined $x) {
		my ($ly) = $self-> get_chunk_dimension($y);
		for (; $ly < $y; $ly++) {
			$x += $self-> get_chunk_cluster_length($ly);
		}
	}
	return $x, $nY;
}

sub physical_to_visual
{
	my ( $self, $x, $y) = @_;
	my ( $lx, $ly ) = $self->{wordWrap} ?
		$self-> physical_to_logical($x,$y) :
		($x, $y);
	return $x - $lx + $self-> get_shaped_chunk($ly)-> index2cluster($lx);
}

sub visual_to_logical
{
	my ( $self, $x, $y) = @_;
	return $x,$y unless $self->{wordWrap};

	my $cm = $self-> {chunkMap};
	return 0,0 unless @$cm;
	my $maxY = $self-> {maxLine};
	$y = $maxY if $y > $maxY || $y < 0;
	$y = 0 if $y < 0;

	my ($l, $r) = ( 0, $self-> {maxChunk} + 1);
	my $i = int($r / 2);
	while (1) {
		my $acd = $$cm[$i * CM_SIZE + CM_Y];
		last if $acd == $y;
		$acd > $y ? $r : $l = $i;
		$i = int(( $l + $r) / 2);
	}

	my ($ly, $chunks) = $self-> get_chunk_dimension($i);
	return undef, $ly unless defined $x;

	$x = 0 if $x < 0;
	for ( $i = 0; $i < $chunks; $i++) {
		my $n_clusters = $self-> get_chunk_cluster_length( $ly + $i );
		last if $x < $n_clusters;
		if ( $i == $chunks - 1 ) {
			$x = $n_clusters if $x > $n_clusters;
			last;
		}
		$x -= $n_clusters;
	}

	return $x, $ly + $i;
}

sub physical_to_logical
{
	my ( $self, $x, $y) = @_;
	return (0,0) if $self-> {maxChunk} < 0;
	unless ($self-> {wordWrap}) {
		$x = $self->physical_to_visual($x, $y) if defined $x;
		return $x, $y;
	}

	($x, $y) = $self->visual_to_logical($x, $y);
	return undef, $y unless defined $x;

	$x = $self-> get_shaped_chunk($y)-> index2cluster($x);
	return $x, $y;
}

sub selection_to_physical
{
	my ($self, @ranges) = @_;
	my @sel = @ranges ? @ranges : $self->selection;
	($sel[0], $sel[2]) = (
		$self->visual_to_physical($sel[0],     $sel[1]),
		$self->visual_to_physical($sel[2] - 1, $sel[3]),
	);
	@sel[0,2] = @sel[2,0] if $sel[1] == $sel[3] && $sel[0] > $sel[2];
	$sel[2]++;
	return @sel;
}

sub start_block
{
	my $self = $_[0];
	return if exists $self-> {anchor};
	my $blockType = $_[1] || $self-> {blockType};
	$self-> selection(0,0,0,0);
	$self-> blockType( $blockType);
	$self-> {anchor} = [ $self-> {cursorX}, $self-> {cursorY}];
}

sub update_block
{
	my $self = $_[0];
	return unless exists $self-> {anchor};
	$self-> selection( @{$self-> {anchor}}, $self-> {cursorX}, $self-> {cursorY});
}

sub end_block
{
	my $self = $_[0];
	return unless exists $self-> {anchor};
	my @anchor = @{$self-> {anchor}};
	delete $self-> {anchor};
	$self-> selection( @anchor, $self-> {cursorX}, $self-> {cursorY});
}

sub cancel_block
{
	delete $_[0]-> {anchor};
	$_[0]-> selection(0,0,0,0);
}

sub set_marking
{
	my ( $self, $mark, $blockType) = @_;
	return if $mark == exists $self-> {anchor};
	$mark ? $self-> start_block( $blockType || $self-> {blockType}) : $self-> end_block;
}

sub cursor_up_down
{
	my ( $self, $d ) = @_;
	$d ||= 1;
	my (undef, $y1) = $self-> logical_to_visual( undef, $self-> {cursorYl});
	my ($x, $y2)    = $self-> logical_to_visual( $self-> {cursorXl}, $self-> {cursorYl} + $d);
	if ( $self-> textDirection ) {
		my $n1 = $self-> get_chunk_cluster_length($y1);
		my $n2 = $self-> get_chunk_cluster_length($y2);
		my $x1 = $n1 - $x;
		$x = $n2 - $x1;
	}
	$self->cursor($x, $y2);
}

sub cursor_down { shift-> cursor_up_down( shift //  1 ) }
sub cursor_up   { shift-> cursor_up_down( shift // -1 ) }

sub cursor_left
{
	my $d = $_[1] || 1;
	my $x = $_[0]-> cursorX;
	if ( $x - $d >= 0) {
		$_[0]-> cursorX( $x - $d)
	} elsif ( $_[0]-> {cursorWrap}) {
		if ( $d == 1) {
			my $y = $_[0]-> cursorY - 1;
			$_[0]-> cursor(( $y < 0) ? (0,0) : (-1, $y));
		} else {
			$_[0]-> cursor_left( $d - 1);
		}
	} else {
		$_[0]-> cursorX( 0);
	}
}

sub cursor_right
{
	my $d = $_[1] || 1;
	my $x = $_[0]-> cursorX;
	if ( $_[0]-> {cursorWrap} ) {
		my $y = $_[0]-> cursorY;
		if ( $x + $d > $_[0]-> get_line_cluster_length($y)) {
			if ( $d == 1) {
				$_[0]-> cursor( 0, $y + 1) if $y < $_[0]-> {maxLine};
			} else {
				$_[0]-> cursor_right( $d - 1);
			}
		} else {
			$_[0]-> cursorX( $x + $d);
		}
	} else {
		$_[0]-> cursorX( $x + $d);
	}
}

sub cursor_home
{
	$_[0]-> begin_undo_group;
	$_[0]-> offset(0);
	$_[0]-> cursorX(0);
	$_[0]-> end_undo_group;
}

sub cursor_end
{
	$_[0]-> cursorX( $_[0]-> get_line_cluster_length( $_[0]->cursorY ) );
}
sub cursor_cend  { $_[0]-> cursorY(-1); $_[0]->cursor_end; }
sub cursor_chome { $_[0]-> cursorY( 0); }
sub cursor_cpgdn { $_[0]-> cursor(-1,-1); }
sub cursor_cpgup { $_[0]-> cursor( 0, 0); }

sub cursor_pgup
{
	my $d = $_[1] || 1;
	my $i;
	for ( $i = 0; $i < $d; $i++) {
		my $cy = $_[0]-> topLine - (
			$_[0]-> {cursorYl} > $_[0]-> topLine  ?
				0 :
				$_[0]-> {rows}
		);
		$_[0]-> cursorLog( $_[0]-> {cursorXl}, $cy < 0 ? 0 : $cy);
	}
}
sub cursor_pgdn  {
	my $d = $_[1] || 1;
	my $i;
	for ( $i = 0; $i < $d; $i++) {
		my ( $tl, $r) = ($_[0]-> topLine , $_[0]-> {rows});
		my $cy = $tl + $r - 1 + (( $_[0]-> {cursorYl} < $tl+$r-1) ? 0 : $r);
		$_[0]-> cursorLog( $_[0]-> {cursorXl}, $cy);
	}
}

sub char_at
{
	my ( $self, $x, $y ) = @_;
	return substr($self->get_line($y), $self-> visual_to_physical($x,$y), 1);
}

sub word_right
{
	my $self = $_[0];
	my $d = $_[1] || 1;
	my $i;
	for ( $i = 0; $i < $d; $i++) {
		my ( $x, $y, $w, $delta, $maxY) = (
			$self-> cursorX, $self-> cursorY,
			$self-> wordDelimiters, 0, $self-> {maxLine}
		);
		my $clen  = $self-> get_chunk_cluster_length($y);
		if ($self-> {cursorWrap}) {
			while ( $x >= $clen) {
				$y++;
				return if $y > $maxY;
				$x = 0;
				$clen  = $self-> get_chunk_cluster_length($y);
			}
		}
		unless ($w =~ quotemeta $self->char_at($x,$y)) {
			$delta++ while ( $w !~ quotemeta $self->char_at($x + $delta, $y)) &&
				$x + $delta < $clen;
		}
		if ( $x + $delta < $clen) {
			$delta++ while ( $w =~ quotemeta $self->char_at($x + $delta, $y)) &&
				$x + $delta < $clen;
		}
		$self-> cursor( $x + $delta, $y);
	}
}

sub word_left
{
	my $self = $_[0];
	my $d = $_[1] || 1;
	my $i;
	for ( $i = 0; $i < $d; $i++) {
		my ( $x, $y, $w, $delta) =
			( $self-> cursorX, $self-> cursorY, $self-> wordDelimiters, 0);
		my $clen  = $self-> get_chunk_cluster_length($y);
		if ($self-> {cursorWrap}) {
			while ( $x == 0) {
				$y--;
				$y = 0, last if $y < 0;
				$x = $clen = $self-> get_chunk_cluster_length($y);
			}
		}
		if ( $w =~ quotemeta( $self->char_at( $x - 1, $y)))
		{
			$delta-- while (( $w =~ quotemeta( $self->char_at( $x + $delta - 1, $y))) &&
				( $x + $delta > 0))
		}
		if ( $x + $delta > 0)
		{
			$delta-- while (!( $w =~ quotemeta( $self->char_at( $x + $delta - 1, $y))) &&
				( $x + $delta > 0))
		}
		$self-> cursor( $x + $delta, $y);
	}
}

sub cursor_shift_key
{
	my ( $self, $menuItem) = @_;
	$self-> begin_undo_group;
	$self-> start_block unless exists $self-> {anchor};
	$menuItem =~ s/Shift//;
	my $action = $self-> accelTable-> action( $menuItem);
	$action = $self-> can( $action) unless ref $action;
	$self-> {delayPanning} = 1;
	$self-> blockShiftMark(1);
	$action-> ( @_);
	$self-> blockShiftMark(0);
	$self-> selection( @{$self-> {anchor}}, $self-> {cursorX}, $self-> {cursorY});
	$self-> realize_panning;
	$self-> end_undo_group;
}

sub blockShiftMark
{
	return $_[0]-> {blockShiftMark} unless $#_;
	my ( $self, $mark) = @_;
	return if $self-> {blockShiftMark} == $mark;
	$self-> push_group_undo_action( 'blockShiftMark', $self-> {blockShiftMark});
	$self-> {blockShiftMark} = $mark;
}

sub mark_vertical
{
	my $self = $_[0];
	if ( exists $self-> {anchor})
	{
		$self-> update_block;
		delete $self-> {restorePersistentBlock}, $self-> persistentBlock(0)
			if $self-> {restorePersistentBlock};
	} else {
		$self-> blockType( bt::Vertical);
		$self-> {restorePersistentBlock} = 1
			unless $self-> persistentBlock;
		$self-> persistentBlock( 1);
		$self-> cursor_shift_key(q(ShiftCursorRight));
	}
}

sub mark_horizontal
{
	my $self = $_[0];
	if ( exists $self-> {anchor})
	{
		$self-> update_block;
		delete $self-> {restorePersistentBlock}, $self-> persistentBlock(0)
			if $self-> {restorePersistentBlock};
	} else {
		$self-> blockType( bt::Horizontal);
		$self-> {restorePersistentBlock} = 1 unless $self-> persistentBlock;
		$self-> persistentBlock( 1);
		$self-> start_block;
		$self-> selection(
			$self-> logical_to_visual( 0, $self-> {cursorYl}),
			$self-> logical_to_visual( -1, $self-> {cursorYl})
		);
	}
}

sub set_line
{
	my ( $self, $y, $line, $operation, $from, $len) = @_;
	my $maxY = $self-> {maxLine};
	$self-> insert_empty_line(0), $y = $maxY = $self-> {maxLine} if $maxY < 0;
	return if $y > $maxY || $y < 0;
	my ( $newDim, $oldDim, $ry) = (0,0,0);
	my ( $fh, $tl, $ofs, @a) = (
		$self-> font-> height, $self-> {topLine }, $self-> {offset}, $self-> get_active_area,
	);
	my @sz = ( $a[2] - $a[0], $a[3] - $a[1]);
	my ( $_from, $_to);
	$self-> begin_undo_group;
	$self-> push_undo_action( 'set_line', $y, $self-> {lines}-> [$y]);
	if ( $self-> {wordWrap}) {
		my @glyphs = $self-> text_shape_with_tabs_replaced( $line ) || ();
		my $breaks = $self-> text_wrap(
			$line,
			$sz[0] - $self-> {defcw},
			tw::WordBreak|tw::CalcTabs|tw::NewLineBreak|tw::ReturnChunks,
			1, 0, -1, @glyphs
		);
		my @chunkMap;
		( undef, $ry) = $self-> physical_to_logical( 0, $y);
		my ($ix, $cm) = ( $ry * CM_SIZE + CM_Y, $self-> {chunkMap});
		my $max_ix = $self-> {maxChunk} * CM_SIZE + CM_Y;
		$oldDim++, $ix += CM_SIZE while $ix <= $max_ix && $$cm[$ix] == $y;
		$newDim = scalar @{$breaks} / 2;
		my $i;
		for (my $i = 0; $i < @$breaks; $i+=2) {
			my @chunk = ( undef ) x CM_SIZE;
			$chunk[CM_CHAR_OFS] = $$breaks[$i];
			$chunk[CM_CHAR_LEN] = 
				(( $i == @$breaks - 2) ? length($line) : $$breaks[$i + 2]) -
				$$breaks[$i];
			$chunk[CM_Y]        = $y;
			push @chunkMap, @chunk;
		}
		splice( @{$cm}, $ry * CM_SIZE, $oldDim * CM_SIZE, @chunkMap);

		$self-> {lines}-> [$y] = $line;
		if ($ry == ($self->{shapedIndex} // -1)) {
			undef $self->{shapedChunk};
			undef $self->{shapedIndex};
		}
		$self-> {maxChunk} -= $oldDim - $newDim;
		if ( $oldDim == $newDim) {
			( $_from, $_to) = ( $ry, $ry + $oldDim);
		} else {
			$self-> topLine(0) if $self-> {maxChunk} < $self-> {rows};
			$self-> reset_scrolls;
		}
	} else {
		my ( $oldL, $newL) = ( length( $self-> {lines}-> [$y]), length( $line));
		$self-> {lines}-> [$y] = $line;
		if ($y == ($self->{shapedIndex} // -1)) {
			undef $self->{shapedChunk};
			undef $self->{shapedIndex};
		}
		if ( $oldL == $self-> {maxLineLength} || $newL > $self-> {maxLineLength})
		{
			my $needReset = 0;
			if ( $newL != $oldL) {
				if ( $oldL == $self-> {maxLineLength}) {
					$self-> {maxLineCount}--;
					$needReset = $self-> {maxLineCount} <= 0;
				}
				if ( $newL > $self-> {maxLineLength}) {
					$self-> {maxLineLength} = $newL;
					$self-> {maxLineCount}  = 1;
					$self-> {maxLineWidth}  = $newL * $self-> {averageWidth};
					$needReset = 0;
				}
				$self-> reset if $needReset;
			}
			$self-> reset_scrolls;
			$self-> repaint if $needReset;
		}
		$_from = $_to = $y;
	}

	$self-> {syntax}-> [$y] = undef;

	if ( defined $operation && $self-> has_selection) {
		if ( $operation ne q(overtype)) {
			$len *= -1 if $operation eq q(delete);
			my @sel = $self-> selection;
			if ( $self-> {blockType} == bt::CUA) {
				if ( $sel[3] == $sel[1] && $sel[1] == $y) {
					$sel[0] += $len if $from < $sel[0];
					if ( $from < $sel[2]) {
						$sel[2] += $len;
						@sel=(0,0,0,0) if $sel[2] <= $from;
					}
				} elsif ( $sel[1] == $y && $from < $sel[0]) {
					$sel[0] += $len;
				} elsif ( $sel[3] == $y && $from < $sel[2]) {
					$sel[2] += $len;
				}
				$sel[0] = 0 if $sel[0] < 0;
				$sel[2] = 0 if $sel[2] < 0;
			} elsif ( $newDim != $oldDim) {
				my @selE = @{$self-> {selEndl}};
				$selE[1] -= $oldDim - $newDim;
				($sel[2], $sel[3]) = $self-> logical_to_visual( @selE);
				@sel = (0,0,0,0) if $sel[3] < $sel[1];
			}
			$self-> selection( @sel);
			delete $self-> {anchor};
			$self-> cancel_block unless $self-> has_selection;
		}
	} else {
		$self-> cancel_block;
	}

	if ( defined $_to)
	{
		$self-> invalidate_rect(
			$a[0], $a[3] - $fh * ( $_to - $tl + 1),
			$a[2], $a[3] - $fh * ( $_from - $tl)
		);
	} else {
		$self-> repaint;
	}
	$self-> cursor( $self-> cursor);
	$self-> end_undo_group;
	$self-> notify(q(Change)) unless $self-> {notifyChangeLock};
}

sub insert_empty_line
{
	my ( $self, $y, $len) = @_;
	my $maxY = $self-> {maxLine};
	$len ||= 1;
	return if $y > $maxY + 1 || $y < 0 || $len == 0;
	my $ly;
	$self-> push_undo_action('delete_line', $y, $len);
	if ( $self-> {wordWrap}) {
		if ( $y > $maxY) {
			$ly = $self-> {maxChunk} + 1;
		} else {
			( undef, $ly) = $self-> physical_to_logical( 0, $y);
		}
		my ($i, $maxC, $cm) = ( 0, $self-> {maxChunk}, $self-> {chunkMap});
		my @empty_chunk = ((undef) x CM_SIZE);
		$empty_chunk[CM_CHAR_OFS] = 0;
		$empty_chunk[CM_CHAR_LEN] = 0;
		$empty_chunk[CM_Y] = $y;
		if ( $y <= $maxY) {
			splice( @{$cm}, $ly * CM_SIZE, 0, (@empty_chunk) x $len);
			for ( $i = $ly + 1; $i < $ly + $len; $i++) {
				$$cm[ $i * CM_SIZE + CM_Y] += $i - $ly;
			}
			for ( $i = $ly + $len; $i <= $maxC + $len; $i++) {
				$$cm[ $i * CM_SIZE + CM_Y] += $len;
			}
		} else {
			push( @{$cm}, (@empty_chunk) x $len);
			for ( $i = $ly; $i < $ly + $len; $i++) {
				$$cm[ $i * CM_SIZE + CM_Y] += $i - $ly;
			}
		}
		$self-> {maxChunk} += $len;
	} else {
		$self-> {maxChunk} += $len;
		$ly = $y;
		$self-> {maxLineCount} += $len if $self-> {maxLineLength} == 0;
	}
	for (@{$self-> {markers}}) {
		$$_[1] += $len if $$_[1] >= $y;
	}
	$self->{shapedIndex} += $len if $y <= ($self->{shapedIndex} // -1);
	splice( @{$self-> {lines}}, $y, 0, ('') x $len);
	$self-> {maxLine} += $len;
	splice( @{$self-> {syntax}}, $y, 0, (undef) x $len)
		if $self-> {syntaxHilite};

	if ( $self-> has_selection) {
		my @sel = $self-> selection;
		unless ( $sel[3] < $y) {
			$sel[1] += $len if $sel[1] >= $y;
			$sel[3] += $len;
		}
		$self-> selection( @sel);
		delete $self-> {anchor};
	}
	my ( $tl, $rc, $yt, $fh, @a) = (
		$self-> {topLine }, $self-> {rows}, $self-> {yTail},
		$self-> font-> height, $self-> get_active_area,
	);
	if (
		$y < $tl + $rc + $yt - 1 &&
		$y + $len > $tl &&
		$y <= $maxY &&
		!$self-> has_selection
	) {
		$self-> scroll( 0, -$fh * $len,
			confineRect => [ @a[0..2], $a[3] - $fh * ( $y - $tl)]);
	}
	$self->reset_scrolls;
	return $ly;
}

sub insert_line
{
	my ( $self, $y, @lines) = @_;
	my $len = scalar @lines;
	my $maxY = $self-> {maxLine};
	return if $y > $maxY + 1 || $y < 0 || $len == 0;
	my $i;
	$self-> begin_undo_group;
	$self-> insert_empty_line( $y, $len);
	$self-> lock_change(1);
	for ( $i = 0; $i < $len; $i++) {
		$self-> set_line( $y + $i, $lines[ $i], q(add), 0, length( $lines[ $i]));
	}
	$self-> lock_change(0);
	$self-> end_undo_group;
}

sub delete_line
{
	my ( $self,$y,$len) = @_;
	my $maxY = $self-> {maxLine};
	$len ||= 1;
	return if $y > $maxY || $y < 0 || $len == 0;

	$self-> begin_undo_group;
	for ( my $i=0; $i < $len; $i++) {
		$self-> push_undo_action( 'set_line', $y+$i, $self-> {lines}-> [$y+$i]);
	}
	$self-> push_undo_action( 'insert_empty_line', $y, $len);

	$len = $maxY - $y + 1 if $y + $len > $maxY + 1;
	my ($lx, $ly);
	if ( $self-> {wordWrap}) {
		( $lx, $ly) = $self-> physical_to_logical( 0, $y);
		$lx = 0;
		my ($i, $maxC, $cm) = ($ly, $self-> {maxChunk}, $self-> {chunkMap});
		$lx++, $i++ while ( $i <= $maxC) and ( $$cm[ $i * CM_SIZE + CM_Y] <= ( $y + $len - 1));
		splice( @{$cm}, $ly * CM_SIZE, $lx * CM_SIZE);
		$self-> {maxChunk} -= $lx;
		for ( $i = $ly; $i <= $maxC - $lx; $i++) { $$cm[ $i * CM_SIZE + CM_Y] -= $len; }
	} else {
		$self-> {maxChunk} -= $len;
		($lx, $ly) = (1, $y);
	}

	my @removed = splice( @{$self-> {lines}}, $y, $len);
	if ( defined ( my $ix = $self->{shapedIndex})) {
		if ( $ix >= $ly && $ix < $ly + $lx) {
			undef $self->{shapedChunk};
			undef $self->{shapedIndex};
		} elsif ( $ix >= $ly + $lx ) {
			$self->{shapedIndex} -= $lx;
		}
	}
	splice( @{$self-> {syntax}}, $y, $len) if $self-> {syntaxHilite};
	for (@{$self-> {markers}}) {
		$$_[1] -= $len if $$_[1] >= $y;
		$$_[1] = 0     if $$_[1] < 0;
	}

	$self-> {maxLine} -= $len;
	if ( $self-> has_selection) {
		my @sel = (@{$self-> {selStartl}}, @{$self-> {selEndl}});
		if ( $sel[3] >= $ly) {
			if ( $sel[1] >= $ly) {
				$sel[1] -= $lx;
				$sel[0] = 0, $sel[1] = $ly if $sel[1] < $ly;
			}
			$sel[3] -= $lx;
			$sel[2] = 0, $sel[3] = $ly if $sel[3] < $ly;
		}
		$self-> selection(
			$self-> logical_to_visual($sel[0], $sel[1]),
			$self-> logical_to_visual($sel[2], $sel[3])
		);
		delete $self-> {anchor};
		$self-> cancel_block unless $self-> has_selection;
	}

	$self-> topLine(0) if $self-> {maxChunk} < $self-> {rows};
	unless ( $self-> {wordWrap}) {
		my $mlv       = $self-> {maxLineLength};
		for ( @removed) {
			$self-> {maxLineCount}-- if length($_) == $mlv;
			if ( $self-> {maxLineCount} <= 0) {
				$self-> reset;
				last;
			}
		}
	}
	$self-> reset_scrolls;
	$self-> cursor( $self-> cursor);
	$self-> end_undo_group;
	$self-> repaint;
	$self-> notify(q(Change)) unless $self-> {notifyChangeLock};
}

sub delete_chunk
{
	my ( $self, $y, $len) = @_;
	my $maxY = $self-> {maxChunk};
	$len ||= 1;
	return if $y > $maxY || $y < 0 || $len == 0;

	$self-> delete_line( $y, $len), return unless $self-> {wordWrap};
	$len = $maxY - $y + 1 if $y + $len > $maxY + 1;
	return if $len == 0;

	my $cm = $self-> {chunkMap};
	my $psy   = $$cm[ $y * CM_SIZE + CM_Y];
	my $pey   = $$cm[($y + $len - 1) * CM_SIZE + CM_Y];
	my $start = $$cm[ $y * CM_SIZE + CM_CHAR_OFS];
	my $end   = $$cm[($y + $len - 1) * CM_SIZE + CM_CHAR_OFS] + $$cm[($y + $len - 1) * CM_SIZE + CM_CHAR_LEN];
	if ( $psy == $pey) {
		my $c  = $self-> {lines}-> [$psy];
		$self-> delete_line( $psy), return
			if $start == 0 && $end == length( $c);
		substr( $c, $start, $end - $start) = '';
		$self-> set_line( $psy, $c, q(delete), $start, $end - $start);
		return;
	}
	$self-> lock;
	my ( $sy, $ey) = ( $psy, $pey);
	my $c;
	$self-> begin_undo_group;
	$self-> lock_change(1);
	if ( $start > 0) {
		$c  = $self-> {lines}-> [$psy];
		my $cs = length( $c) - $start + 1;
		substr( $c, $start, $cs) = '';
		$self-> set_line( $psy, $c, q(delete), $start, $cs);
		$sy++;
	}
	$c  = $self-> {lines}-> [$pey];
	if ( $end < length( $c)) {
		substr( $c, 0, $end) = '';
		$self-> set_line( $pey, $c, q(delete), 0,  $end);
		$ey--;
	}
	$self-> delete_line( $sy, $ey - $sy + 1) if $ey >= $sy;
	$self-> cursor( $self-> {cursorX}, $psy);
	$self-> unlock;
	$self-> lock_change(0);
	$self-> end_undo_group;
}

sub delete_text
{
	my ( $self, $x, $y, $len) = @_;
	my $maxY = $self-> {maxLine};
	$y = $maxY if $y < 0;
	return if $y > $maxY || $y < 0;

	my $c = $self-> {lines}-> [ $y];
	my $l = length( $c);
	$x = $l if $x < 0;

	if ( $x == $l) {
		return if $y == $maxY;
		$self-> lock_change(1);
		$self-> begin_undo_group;
		$self-> set_line( $y, $self-> get_line( $y) . $self-> get_line( $y + 1));
		$self-> delete_line( $y + 1);
		$self-> end_undo_group;
		$self-> lock_change(0);
		return;
	}
	$len = $l - $x if $len + $x >= $l;
	return if $len <= 0;
	substr( $c, $x, $len) = '';
	$self-> set_line( $y, $c, q(delete), $x, $len);
}

sub delete_char
{
	my ($self, $repeat) = @_;
	$repeat ||= 1;
	$self-> begin_undo_group;
	while ( $repeat-- ) {
		my @cs = $self->cursor;
		my ($c,$s) = $self->get_line_for_edit(@cs);
		my ($new_text, $new_offset) = $self->handle_bidi_input(
			action     => ($self->insertMode ? q(delete) : q(cut)),
			at         => $cs[0],
			text       => $c,
			rtl        => $self->textDirection,
			glyphs     => $s,
			n_clusters => $s->n_clusters,
		);
		if ( defined $new_text ) {
			$self-> set_line( $cs[1], $new_text, q(delete), $cs[0], 1);
			$self-> cursor(
				$self->text_shape_with_tabs_replaced($new_text)->index2cluster($new_offset),
				$cs[1]
			);
		} elsif ( $cs[1] < $self->{maxLine} ) {
			$self-> delete_text( length($self->get_line($cs[1])), $cs[1], 1 );
		}
	}
	$self-> end_undo_group;
}

sub back_char
{
	my ($self, $repeat) = @_;
	my @c = $self-> cursor;
	$repeat ||= 1;

	$self-> begin_undo_group;
	while ( $repeat-- ) {
		my @cs = $self->cursor;
		my ($c,$s) = $self->get_line_for_edit(@cs);
		my ($new_text, $new_offset) = $self->handle_bidi_input(
			action     => q(backspace),
			at         => $cs[0],
			text       => $self->get_line($cs[1]),
			rtl        => $self->textDirection,
			glyphs     => $s,
			n_clusters => $s->n_clusters,
		);
		if ( defined $new_text ) {
			$self-> set_line( $cs[1], $new_text, q(delete), $cs[0], 1);
			$self-> cursor(
				$self->text_shape_with_tabs_replaced($new_text)->index2cluster($new_offset),
				$cs[1]
			);
		} elsif ( $cs[1] > 0 ) {
			my $c = $self->get_line($cs[1] - 1);
			my $l = $self->text_shape_with_tabs_replaced($c)->n_clusters;
			$self-> delete_text( length($c), $cs[1] - 1, 1 );
			$self-> cursor($l, $cs[1] - 1);
		}
	}
	$self-> end_undo_group;
}

sub delete_current_chunk
{
	my $self = $_[0];
	$self-> delete_chunk( $self-> {cursorYl});
}

sub delete_to_end
{
	my $self = $_[0];
	my @cs = $self-> cursor;
	my ( $c, $s ) = $self->get_line_for_edit(@cs);
	my $px = $s->cursor2offset($cs[0], $self->textDirection);
	return if $px > length( $c);

	$self-> set_line(
		$cs[1], substr($c, 0, $px), 
		q(delete), $cs[0], $s->n_clusters - $cs[0]
	);
	$s = $self->text_shape_with_tabs_replaced($self->get_line($cs[1]));
	my $last   = $s->index2cluster($s->text_length);
	my $is_rtl = $s->cluster2index($last) & to::RTL;
	$self-> cursor( $last - ($is_rtl ? 1 : 0), $cs[1] );
}

sub delete_word
{
	my ($self, $as_backspace) = @_;
	my $direction = ($self-> textDirection xor $as_backspace) ? 'Left' : 'Right';
	$self-> cursor_shift_key('ShiftWord' . $direction );
	$self-> delete_block;
}

sub delete_block
{
	my $self = $_[0];
	return unless $self-> has_selection;

	$self-> begin_undo_group;
	$self-> push_undo_action('selection', $self-> selection);

	my @sel = ( @{$self-> {selStartl}}, @{$self-> {selEndl}});
	my $bt = $self-> {blockType};
	if ( $bt == bt::Horizontal) {
		$self-> delete_chunk( $sel[1], $sel[3] - $sel[1] + 1);
	} elsif ( $bt == bt::CUA) {
		my $c;
		my @sel = ( @{$self-> {selStart}}, @{$self-> {selEnd}});
		if ( $sel[1] == $sel[3]) {
			$c = $self-> get_line( $sel[1]);
			my @sel2 = $self-> selection_to_physical;
			substr( $c, $sel2[0], $sel2[2] - $sel2[0]) = '';
			$self-> set_line( $sel[1], $c);
		} else {
			my ( $from, $len) = ( $sel[1], $sel[3] - $sel[1]);

			my $res = substr( $self-> get_line( $from), 0, $self-> visual_to_physical(@sel[0,1]));
			$c = $self-> get_line( $sel[3]);
			my $px2 = $self->visual_to_physical( $sel[2] - 1, $sel[3] ) + 1;
			if ( $px2 < length( $c)) {
				$res .= substr( $c, $px2, length( $c) - $px2);
			} elsif ( $sel[3] < $self-> {maxLine}) {
				$res .= $self-> get_line( $sel[3] + 1);
				$len++;
			}
			$self-> lock_change(1);
			$self-> delete_line( $from + 1, $len) if $len > 0;
			$self-> set_line( $from, $res);
			$self-> lock_change(0);
		}
	} else {
		my @sel = ( @{$self-> {selStart}}, @{$self-> {selEnd}});
		my $len = $sel[2] - $sel[0];
		my $i;
		$self-> lock_change(1);
		$self-> lock;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++)
		{
			my $c = $self-> get_line( $i);
			if ( $c ne '') {
				substr( $c, $self-> visual_to_physical($sel[0], $i), $len) = '';
				$self-> set_line( $i, $c);
			}
		}
		$self-> unlock;
		$self-> lock_change(0);
	}

	$self-> cursorLog( $sel[0], $sel[1]);
	$self-> cancel_block;
	$self-> end_undo_group;
}

sub copy_block
{
	my $self = $_[0];
	return if
		$self-> {readOnly} ||
		$self-> {blockType} == bt::CUA ||
		$self-> {wordWrap} ||
		!$self-> has_selection;

	$self-> lock_change(0);
	$self-> lock;
	$self-> begin_undo_group;
	my @lines = split "\n", $self->get_selected_text;
	if ( $self-> {blockType} == bt::Horizontal) {
		$self-> insert_line( $self-> cursorY, @lines);
	} else {
		my @cs = $self-> cursor;
		for ( my $i = $cs[1]; $i < $cs[1] + scalar @lines; $i++) {
			my $line = $lines[ $i - $cs[1]];
			if ( $i <= $self-> {maxLine} ) {
				my $c = $self-> get_line( $i);
				$c .= ' 'x($cs[0]-length($c)) if length($c) < $cs[0];
				my $px = $self-> logical_to_physical($cs[0], $i);
				substr( $c, $px, 0) = $line;
				$self-> set_line( $i, $c);
			} else {
				$self-> insert_line( $i, (' ' x $cs[0] ) . $line);
			}
		}
	}
	$self-> end_undo_group;
	$self-> unlock;
	$self-> lock_change(1);
}

sub overtype_block
{
	my $self = $_[0];
	return if
		$self-> {readOnly} ||
		$self-> {blockType} == bt::CUA ||
		$self-> {wordWrap} ||
		!$self-> has_selection;

	my @sel = $self-> selection;
	$self-> lock_change(0);
	$self-> lock;
	$self-> begin_undo_group;
	my @lines = split "\n", $self->get_selected_text;
	my @cs    = $self-> cursor;
	if ( $self-> {blockType} == bt::Horizontal) {
		for ( my $i = $cs[1]; $i < $cs[1] + scalar @lines; $i++) {
			my $line = $lines[ $i - $cs[1]];
			if ( $i <= $self-> {maxLine} ) {
				$self-> set_line( $i, $line);
			} else {
				$self-> insert_line( $i, $line);
			}
		}
	} else {
		for ( my $i = $cs[1]; $i < $cs[1] + scalar @lines; $i++) {
			my $line = $lines[ $i - $cs[1]];
			if ( $i <= $self-> {maxLine} ) {
				my $c = $self-> get_line( $i);
				$c .= ' 'x($cs[0]-length($c)) if length($c) < $cs[0];
				my $px = $self-> logical_to_physical($cs[0], $i);
				substr( $c, $px, length($line)) = $line;
				$self-> set_line( $i, $c);
			} else {
				$self-> insert_line( $i, (' ' x $cs[0] ) . $line);
			}
		}
	}
	$self-> end_undo_group;
	$self-> unlock;
	$self-> lock_change(1);
}

sub split_line
{
	my $self = $_[0];
	my @cs = $self-> cursor;
	my $to = $self-> visual_to_physical(@cs);
	my $c  = $self-> get_line($cs[1]);
	$c .= ' ' x ($to - length($c)) if length($c) < $to;
	my ( $old, $new) = ( substr( $c, 0, $to), substr( $c, $to, length($c) - $to));
	my $cm = $self->{chunkMap};
	$self-> lock_change(1);
	$self-> begin_undo_group;
	$self-> set_line( $cs[1], $old, q(delete), $to, length($c) - $to);
	my $cshift = 0;
	if ( $self-> {autoIndent}) {
		my $i = 0;
		my $add = '';
		for ( $i = 0; $i < length($old); $i++) {
			my $c = substr( $old, $i, 1);
			last if $c ne ' ' and $c ne '\t';
			$add .= $c;
		}
		$new = $add.$new, $cshift = length($add)
			if length($add) < length($old);
	}
	$self-> insert_line( $cs[1]+1, $new);
	$self-> cursor( $self-> physical_to_visual( $cshift, $cs[1] + 1), $cs[1] + 1);
	$self-> end_undo_group;
	$self-> lock_change(0);
}

sub _search_forward
{
	my ($self, $x, $y, $first_line, $substr, $match, $replace) = @_;
	local $_ = $first_line;
	my ($l, $len) = (0);
	while ( 1) {
		if ( /$match/ ) {
			$l   = $-[0];
			$len = $+[0] - $-[0];
			s/$match/$replace/ if $replace;
			return $l + $x, $y, $len, $substr . $_;
		}
		$y++;
		return if $y > $self->{maxLine};
		$_ = $self->get_line($y);
		$substr = '';
		$l = $x = 0;
	}
}

sub _search_backward
{
	my ($self, $x, $y, $first_line, $substr, $match, $replace) = @_;
	local $_ = $first_line;
	my ($l,$b,$len);
	while ( 1) {
		while (/$match/g) {
			$l = $b;
			$b = pos;
		}
		if ( defined $b ) {
			$l ||= 0;
			pos = $l;
			$l = $_ =~ /$match/;
			$len = $+[0] - $-[0];
			$l   = pos($_) - $len;
			substr($_, $l, $len) = $replace if defined $replace;
			pos = undef;
			return $l, $y, $len, $_ . $substr;
		}
		$y--;
		return if $y < 0;
		$_ = $self-> get_line($y);
		$substr = '';
	}
}

sub find
{
	my ( $self, $line, $x, $y, $replaceLine, $options) = @_;
	$x ||= 0;
	$y ||= 0;
	$options //= 0;
	my $maxY = $self-> {maxLine};
	return if $y > $maxY || $maxY < 0;

	$line = '('.quotemeta( $line).')' unless $options & fdo::RegularExpression;
	$y = $maxY if $y < 0;
	my $c = $self-> get_line( $y);
	my ( $subLine, $re, $re2, $matcher );
	$x = length( $c) if $x < 0;

	$re  .= '\\b' if $options & fdo::WordsOnly;
	$re  .= $line;
	$re  .= '\\b' if $options & fdo::WordsOnly;
	$re2  = '';
	$re2 .= 'i' unless $options & fdo::MatchCase;

	my $match;
	eval {
		$match = eval "qr/$re/$re2";
		return if $@;
	};
	return unless $match;

	if ( $options & fdo::BackwardSearch) {
		$subLine = substr( $c, $x, length( $c) - $x);
		substr( $c, $x, length( $c) - $x) = '';
		$matcher = \&_search_backward;
	} else {
		$subLine = substr( $c, 0, $x);
		substr( $c, 0, $x) = '';
		$matcher = \&_search_forward;
	}
	my ($nX, $nY, $len, $newStr) = $matcher->($self, $x, $y, $c, $subLine, $match, $replaceLine);
	return unless defined $nX;
	my $nX1 = $nX;
	my $nX2 = $nX + $len;
	return ($nX1, $nY, $nX2, $newStr);
}

sub add_marker
{
	my ( $self, $x, $y) = @_;
	push( @{$self-> {markers}}, [$x, $y]);
}

sub delete_marker
{
	my ( $self, $index) = @_;
	return if $index > scalar @{$self-> {markers}};
	splice( @{$self-> {markers}}, $index, 1);
}

sub select_all { $_[0]-> selection(0,0,-1,-1); }

sub autoIndent      {($#_)?($_[0]-> {autoIndent}    = $_[1])                :return $_[0]-> {autoIndent }  }
sub blockType       {($#_)?($_[0]-> set_block_type  ( $_[1]))               :return $_[0]-> {blockType}    }
sub cursor          {($#_)?($_[0]-> set_cursor    ($_[1],$_[2]))            :return $_[0]-> {cursorX},$_[0]-> {cursorY}}
sub cursorLog       {($#_)?($_[0]-> set_cursor    ($_[0]-> logical_to_visual($_[1],$_[2])))            :return $_[0]-> {cursorXl},$_[0]-> {cursorYl}}
sub cursorX         {($#_)?($_[0]-> set_cursor    ($_[1],$_[0]-> {cursorY})):return $_[0]-> {cursorX}    }
sub cursorY         {($#_)?($_[0]-> set_cursor    ($_[0]-> {q(cursorX)},$_[1])):return $_[0]-> {cursorY}    }
sub cursorWrap      {($#_)?($_[0]-> {cursorWrap     }=$_[1])                :return $_[0]-> {cursorWrap     }}
sub topLine         {($#_)?($_[0]-> set_top_line (   $_[1]))               :return $_[0]-> {topLine }    }
sub numLines        { 1 + $_[0]->{maxLine} }
sub hiliteNumbers   {($#_)?$_[0]-> set_hilite_numbers ($_[1])               :return $_[0]-> {hiliteNumbers} }
sub hiliteQStrings  {($#_)?$_[0]-> set_hilite_q_strings($_[1])              :return $_[0]-> {hiliteQStrings} }
sub hiliteQQStrings {($#_)?$_[0]-> set_hilite_qq_strings($_[1])             :return $_[0]-> {hiliteQQStrings} }
sub hiliteChars     {($#_)?$_[0]-> set_hilite_chars     ($_[1])             :return $_[0]-> {hiliteChars    } }
sub hiliteIDs       {($#_)?$_[0]-> set_hilite_ids       ($_[1])             :return $_[0]-> {hiliteIDs      } }
sub hiliteREs       {($#_)?$_[0]-> set_hilite_res       ($_[1])             :return $_[0]-> {hiliteREs      } }
sub insertMode      {($#_)?($_[0]-> set_insert_mode  (    $_[1]))           :return $_[0]-> {insertMode}   }
sub mark            {($#_)?(shift-> set_marking  (    @_   ))               :return exists $_[0]-> {anchor}  }
sub markers         {($#_)?($_[0]-> {markers}    = [@{$_[1]}])              :return $_[0]-> {markers  }    }
sub modified        {($#_)?($_[0]-> {modified }     = $_[1])                :return $_[0]-> {modified }    }
sub offset          {($#_)?($_[0]-> set_offset   (    $_[1]))               :return $_[0]-> {offset   }    }
sub persistentBlock {($#_)?($_[0]-> {persistentBlock}=$_[1])               :return $_[0]-> {persistentBlock}}
sub readOnly        {($#_)? $_[0]-> set_readonly($_[1])                     :return $_[0]-> {readOnly }    }
sub selection       {($#_)? shift-> set_selection   (@_)           : return (@{$_[0]-> {selStart}},@{$_[0]-> {selEnd}})}
sub selStart        {($#_)? $_[0]-> set_selection   ($_[1],$_[2],@{$_[0]-> {selEnd}}): return @{$_[0]-> {'selStart'}}}
sub selEnd          {($#_)? $_[0]-> set_selection   (@{$_[0]-> {'selStart'}},$_[1],$_[2]):return @{$_[0]-> {'selEnd'}}}
sub syntaxHilite    {($#_)? $_[0]-> set_syntax_hilite($_[1])                :return $_[0]-> {syntaxHilite}}
sub tabIndent       {($#_)?$_[0]-> set_tab_indent(    $_[1])                :return $_[0]-> {tabIndent}     }
sub textRef         {($#_)?$_[0]-> set_text_ref  (    $_[1])                :return $_[0]-> get_text_ref;   }
sub wantTabs        {($#_)?($_[0]-> {wantTabs}      = $_[1])                :return $_[0]-> {wantTabs}      }
sub wantReturns     {($#_)?($_[0]-> {wantReturns}   = $_[1])                :return $_[0]-> {wantReturns}   }
sub wordDelimiters  {($#_)?($_[0]-> {wordDelimiters}= $_[1])                :return $_[0]-> {wordDelimiters}}
sub wordWrap        {($#_)?($_[0]-> set_word_wrap(    $_[1]))               :return $_[0]-> {wordWrap }    }

1;

=pod

=head1 NAME

Prima::Edit - standard text editor

=head1 SYNOPSIS

	use Prima qw(Edit Application);
	my $e = Prima::Edit->new(
		text         => 'Hello $world',
		syntaxHilite => 1,
	);
	run Prima;

=for podview <img src="edit.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/edit.gif">

=head1 DESCRIPTION

The class provides text editing capabilities, three types of selection, text wrapping,
syntax highlighting, auto indenting, undo and redo function, and search and replace methods.

The module declares the C<bt::> package that contains integer constants for the selection of block type,
used by the L<blockType> property.

=head1 USAGE

The text is stored line-wise in the C<{lines}> array; to access it use the
L<get_line> method.

All keyboard events except the character input and tab keys are processed by
the accelerator table ( see L<Prima::Menu> ). The default C<accelItems> table
defines names, keyboard combinations, and the corresponding actions to the
class functions. The class does not provide functionality to change these
mappings. To do so, consult L<Prima::Menu/Prima::AccelTable>.

=head2 Coordinates

The class addresses the text space by (X,Y)-coordinates, where X is the visual
cluster offset and Y is the line number. The addressing coordinate system can be
I<visual>, I<physical>, or I<logical>. See below.

Cluster shaping and word wrapping can play a role here. Consider f.ex. a text
string "offset is zero", that for the sake of the example can wrapped by width
and displayed as two lines, "offset" and "is zero". Here, the font substitutes
"ff" text with a single ligature glyph. Here, for example, coord("f") will be
(0,1) in all coordinates, but coord("z") is not:

=over

=item Physical

The X coordinate is a character offset from character line number Y.  These
coordinates are identical with and without the C<wordWrap> flag.  This
coordinate is used for direct text manipulation.

Example: coord("z") is (0,10);

=item Visual

The X coordinate is a glyph cluster offset from the character line number Y. These
coordinates are identical with and without the C<wordWrap> flag. This
coordinate is used for cursor and selection API.

Example: coord("z") is (0,9);

=item Logical

The Y coordinate is the wrapped line index. The C<chunkMap> internal array
contains addresses in the logical coordinates. The X coordinate is a glyph
cluster offset from the line start.  This coordinate is used mostly internally.

To access the text chunk-wise, use the L<get_chunk> method. 

Example: coord("z") is (1,3);

=back

=head1 API

=head2 Events

=over

=item ParseSyntax TEXT, RESULT_ARRAY_REF

Called when syntax highlighting is required - TEXT is a string to be parsed,
and the parsing results to be stored in RESULT_ARRAY_REF, which is a reference
to an array of integer pairs, each representing a single-colored text chunk.
The first integer in the pairs is the length of a chunk, the second - color
value ( C<cl::XXX> constants ).

=back

=head2 Properties

=over

=item autoIndent BOOLEAN

Turns the auto indenting on or off

Default value: 1

=item blockType INTEGER

Defines the type of selection block. Can be one of the following constants:

=over

=item bt::CUA

Normal block, where the first and the last line of the selection can
be partial, and the lines between occupy the whole line. CUA stands for
'common user access'.

Default keys: Shift + arrow keys

See also: L<cursor_shift_key>

=item bt::Vertical

Rectangular block, where all selected lines are of the same offsets and lengths.

Default key: Alt+B

See also: L<mark_vertical>

=item bt::Horizontal

Rectangular block, where the selection occupies the whole line.

Default key: Alt+L

See also: L<mark_horizontal>

=back

=item cursor X, Y

Selects the visual position of the cursor

=item cursorX X

Selects the visual horizontal position of the cursor

=item cursorY Y

Selects the visual vertical position of the cursor

=item cursorWrap BOOLEAN

Selects cursor behavior when moved horizontally outside the line. If 0, the cursor is
not moved. If 1, the cursor moved to the adjacent line.

See also: L<cursor_left>, L<cursor_right>, L<word_left>, L<word_right>.

=item insertMode BOOLEAN

Set the typing mode - if 1, the typed text is inserted, if 0, the new text overwrites
the old text. When C<insertMode> is 0, the cursor shape is thick and covers the whole
character; when 1, it is of the default cursor width.

Default toggle key: Insert

=item hiliteNumbers COLOR

Selects the color for number highlighting

=item hiliteQStrings COLOR

Selects the color for highlighting the single-quoted strings

=item hiliteQQStrings COLOR

Selects the color for highlighting the double-quoted strings

=item hiliteIDs ARRAY

An array of scalar pairs that define words to be highlighted.
The first item in the pair is an array of words, and the second item is
a color value.

=item hiliteChars ARRAY

An array of scalar pairs that define characters to be highlighted.
The first item in the pair is a string of characters, and the second item is
a color value.

=item hiliteREs ARRAY

An array of scalar pairs that define character patterns to be highlighted.
The first item in the pair is a perl regular expression, and the second item is
a color value.

Note: these are tricky. Generally, these assume that whatever is captured in
C<()>, is highlighted, and that capturing parentheses match from the first
character onwards.  So for simple matches like C< (\d+) > (digits) or C< (#.*)
> this works fine. Things become more interesting if you need to check text
after, or especially before the capture.  For this, you need to make sure that
whatever text is matched by a regexp, it must not move the C<pos> pointer as
the regexes are internally concatenated with the C<\G> anchor before the actual
matching takes place  (i.e. starting each time from the position the previous
regex left off), and use C</gc> flags (advancing C<pos> to the match length).
Advancing the C<pos> will nullify color highlighting on the text after the
capture but before the end of the match - so you'll need look-ahead assertions
for this type of match, C<(?=pattern) > and C<(?!pattern)> (see
L<perlre/"Lookaround Assertions"> ).

For example, we have a string C<ABC123abc>, and we want to match I<123> followed by I<abc>.
This won't work:

	hiliteREs => [
		'(123)abc',cl::LightRed,
		'(abc)', cl::LightGreen
	]

while this will:

	hiliteREs => [
		'(123)(?=abc)',cl::LightRed,
		'(abc)', cl::LightGreen 
	]

If you need to look behind, the corresponding assertions C< (?<=pattern) > and
C< (?<!pattern) > could be used, but these are even more restrictive in that
they only support fixed-width looks-behinds (NB: C< \K > won't work because of
C< \G > either). That way, if we want to match 123 that follows ABC, this won't
work:

	hiliteREs =>  [
		'(ABC)',cl::LightBlue,
		'(?<=[ABC]+)(123)',cl::LightRed,
	]

while this will:

	hiliteREs =>  [
		'(ABC)',cl::LightBlue,
		'(?<=[ABC]{3})(123)',cl::LightRed,
	]

=item mark MARK [ BLOCK_TYPE ]

Selects block marking state. If MARK is 1, starts the block marking,
if 0 - stops the block marking. When MARK is 1, BLOCK_TYPE can be used
to set the selection type ( C<bt::XXX> constants ). If BLOCK_TYPE is
unset the value of L<blockType> is used.

=item markers ARRAY

An array of arrays with integer pairs, X and Y, where each represents
visual coordinates in text. Used as anchor storage for fast navigation.

See also: L<add_marker>, L<delete_marker>

=item modified BOOLEAN

A boolean flag that shows if the text was modified. Can be used externally,
to check if the text is to be saved to a file, for example.

=item numLines INTEGER

Returns the number of lines

=item offset INTEGER

Horizontal offset of text lines in pixels.

=item persistentBlock BOOLEAN

Selects whether the selection is canceled as soon as the cursor is moved ( 0 )
or it persists until the selection is explicitly changed ( 1 ).

Default value: 0

=item readOnly BOOLEAN

If 1, no user input is accepted. Manipulations with text are allowed though.

=item selection X1, Y1, X2, Y2

Accepts two pairs of visual coordinates, (X1,Y1) the beginning and (X2,Y2) the end
of the new selection, and sets the block according to the L<blockType> property.

The selection is null if X1 equals to X2 and Y1 equals to Y2.
L<has_selection> method returns 1 if the selection is non-null.

=item selStart X, Y

Manages the selection start. See L<selection>, X1 and Y1.

=item selEnd X, Y

Manages the selection end. See L<selection>, X2 and Y2.

=item syntaxHilite BOOLEAN

Manages the syntax highlighting.

=item tabIndent INTEGER

Maps the tab ( \t ) key to a C<tabIndent> number of space characters.

=item text TEXT

Provides access to all the text data. The lines are separated by
the new line ( \n ) character.

See also: L<textRef>.

=item textDirection BOOLEAN

If set, indicates RTL text input.

=item textLigation BOOLEAN

If set, text may be rendered at better quality with ligation and kerning,
however, that comes with a price that some ligatures may be indivisible and form
clusters (f.ex. I<ff> or I<ffi> ligatures). The cursor cannot be positioned inside of 
a cluster, and thus one can only select them, delete them as a whole, or press
Del/Backspace on the cluster's edge.

=item textRef TEXT_PTR

Provides access to all the text data. The lines are separated by
the new line ( \n ) character. TEXT_PTR is a pointer to a text string.

The property is more efficient than L<text> with large text
because the copying of the text scalar to the stack is eliminated.

See also: L<text>.

=item topLine INTEGER

Selects the first line of the text drawn.

=item undoLimit INTEGER

Sets limit on the number of stored atomic undo operations. If 0,
undo is disabled.

Default value: 1000

=item wantTabs BOOLEAN

Selects the way the tab ( \t ) character is recognized in the user input.  If
1, it is recognized as the verbatim Tab key with an ascii value of 0x09; however,
this disallows the toolkit widget tab-driven navigation. If 0, the tab
character can be entered by pressing the Ctrl+Tab key combination.

Default value: 0

=item wantReturns BOOLEAN

Selects the way the new line ( \n ) character is recognized in the user input.
If 1, it is recognized as the verbatim CR key producing newline character(s);
however, this disallows the default button activation in the toolkit. If 0, the
new line character can be entered by pressing the Ctrl+Enter key combination.

Default value: 1

=item wordDelimiters STRING

Contains a string of characters that are used for locating a word break.
Default STRING value consists of punctuation marks, space and tab characters,
and the C<\xff> character.

See also: L<word_left>, L<word_right>

=item wordWrap BOOLEAN

Selects whether the long lines are wrapped, or can be positioned outside the
horizontal widget borders. A line of text can be represented by more than one
line of screen text ( chunk ) . To access the text chunk-wise, use the L<get_chunk>
method.

=back

=head2 Methods

=over

=item add_marker X, Y

Adds visual coordinates X,Y to the L<markers> property.

=item back_char [ REPEAT = 1 ]

Removes REPEAT times a character left to the cursor. If the cursor is on 0 X-position,
removes the new-line character and concatenates the two lines.

Default key: Backspace

=item cancel_block

Removes the selection block

Default key: Alt+U

=item change_locked

Returns 1 if the logical locking is on, and 0 if it is off.

See also L<lock_change>.

=item copy

Copies the selected text, if any, to the clipboard.

Default key: Ctrl+Insert

=item copy_block

Copies the selected text and inserts it into the cursor position, according to
the L<blockType> value.

Default key: Alt+C

=item cursor_cend

Moves cursor to the last line

Default key: Ctrl+End

=item cursor_chome

Moves cursor to the first line

Default key: Ctrl+Home

=item cursor_cpgdn

Default key: Ctrl+PageDown

Moves cursor to the end of text.

=item cursor_cpgup

Moves cursor to the beginning of text.

Default key: Ctrl+PageUp

=item cursor_down [ REPEAT = 1 ]

Moves cursor REPEAT times down

Default key: Down

=item cursor_end

Moves cursor to the end of the line

Default key: End

=item cursor_home

Moves cursor to the beginning of the line

Default key: Home

=item cursor_left [ REPEAT = 1 ]

Moves cursor REPEAT times left

Default key: Left

=item cursor_right [ REPEAT = 1 ]

Moves cursor REPEAT times right

Default key: Right

=item cursor_up [ REPEAT = 1 ]

Moves cursor REPEAT times up

Default key: Up

=item cursor_pgdn [ REPEAT = 1 ]

Moves cursor REPEAT pages down

Default key: PageDown

=item cursor_pgup [ REPEAT = 1 ]

Moves cursor REPEAT pages up

Default key: PageUp

=item cursor_shift_key [ ACCEL_TABLE_ITEM ]

Performs action of the cursor movement, bound to ACCEL_TABLE_ITEM action
( defined in C<accelTable> or C<accelItems> property ), and extends the
selection block along the cursor movement. Not called directly.

=item cut

Cuts the selected text into the clipboard.

Default key: Shift+Delete

=item delete_block

Removes the selected text.

Default key: Alt+D

=item delete_char [ REPEAT = 1 ]

Deletes REPEAT characters from the cursor position

Default key: Delete

=item delete_line LINE_ID, [ LINES = 1 ]

Removes LINES of text at LINE_ID.

=item delete_current_chunk

Removes the chunk ( or line, if L<wordWrap> is 0 ) at the cursor.

Default key: Ctrl+Y

=item delete_chunk CHUNK_ID, [ CHUNKS = 1 ]

Removes CHUNKS ( or lines, if L<wordWrap> is 0 ) of text at CHUNK_ID

=item delete_marker INDEX

Removes marker INDEX in the L<markers> list.

=item delete_to_end

Removes the text to the end of the chunk.

Default key: Ctrl+E

=item delete_text X, Y, TEXT_LENGTH

Removes TEXT_LENGTH characters at X,Y physical coordinates

=item draw_colorchunk CANVAS, LINE_ID, X, Y, COLOR

Paints the syntax-highlighted chunk taken from LINE_ID line index, at
X, Y. COLOR is used if the syntax highlighting information contains C<cl::Fore>
as a color reference.

=item end_block

Stops the block selection session.

=item find SEARCH_STRING, [ X = 0, Y = 0, REPLACE_LINE = '', OPTIONS ]

Tries to find ( and, if REPLACE_LINE is defined, to replace with )
SEARCH_STRING starting from (X,Y) physical coordinates. OPTIONS is an integer
that is a combination of the C<fdo::> constants; the same constants are used in
L<Prima::Dialog::FindDialog>, which provides a graphic interface to the find and
replace facilities of this class.

Returns X1, Y, X2, NEW_STRING where X1.Y-X2.Y are physical coordinates of
the string found, and NEW_STRING is the replaced version (if any)

=over

=item fdo::MatchCase

If set, the search is case-sensitive.

=item fdo::WordsOnly

If set, SEARCH_STRING must constitute the whole word.

=item fdo::RegularExpression

If set, SEARCH_STRING is a regular expression.

=item fdo::BackwardSearch

If set, the search direction is backward.

=item fdo::ReplacePrompt

Not used in the class, however, used in L<Prima::Dialog::FindDialog>.

=back

See also: F<examples/editor.pl>

=item get_chunk CHUNK_ID

Returns the chunk of text, located at CHUNK_ID.
Returns an empty string if the chunk is nonexistent.

=item get_chunk_cluster_length CHUNK_ID

Return the length of a chunk in clusters

=item get_chunk_dimension CHUNK_ID

Finds the line number the CHUNK_ID belongs to, and returns the first chunk of that line and
how many chunks the line consists of.

=item get_chunk_width TEXT, FROM, LENGTH, [ RETURN_TEXT_PTR ]

Returns the width in pixels of C<substr( TEXT, FROM, LENGTH)>.  If FROM is
larger than the length of TEXT, TEXT is padded with the space characters. Tab character
in TEXT replaced to L<tabIndent> times space character. If the RETURN_TEXT_PTR
pointer is specified, the converted TEXT is stored there.

=item get_line INDEX

Returns the line of text located at INDEX.
Returns an empty string if the line is nonexistent.

=item get_line_cluster_length LINE_ID

Return the length of the line in clusters

=item get_line_dimension INDEX

Returns two integers representing the line at INDEX in the L<wordWrap> mode:
the first value is the corresponding chunk index, and the second is how many
chunks are contained in the line.

See also: L<physical_to_logical>.

=item get_selected_text

Return the currently selected text.

=item has_selection

Returns a boolean value, indicating if the selection block is active.

=item insert_empty_line LINE_ID, [ REPEAT = 1 ]

Inserts REPEAT empty lines at LINE_ID.

=item insert_line LINE_ID, @TEXT

Inserts @TEXT strings at LINE_ID

=item insert_text TEXT, [ HIGHLIGHT = 0 ]

Inserts TEXT at the cursor position. If HIGHLIGHT is set to 1,
the selection block is canceled and the newly inserted text is selected.

=item lock_change BOOLEAN

Increments ( 1 ) or decrements ( 0 ) lock count. Used to defer change notification
in multi-change calls. When the internal lock count hits zero, the C<Change> notification is called.

=item physical_to_logical X, Y

Maps visual X,Y coordinates to the logical coordinate system.
Returns the same values when L<wordWrap> is 0.

=item logical_to_visual X, Y

Maps logical X,Y coordinates to the visual coordinate system.

Returns the same values when L<wordWrap> is 0.

=item visual_to_physical X, Y

Maps visual X,Y coordinates to the physical text offset relative to the Y line

Returns the same X when the line does not contain any right-to-left (RTL) characters or
complex glyphs.

=item physical_to_visual X, Y

Maps test offset X from line Y to the visual X coordinate.

Returns the same X when the line does not contain any right-to-left (RTL) characters or
complex glyphs.

=item mark_horizontal

Starts block marking session with the C<bt::Horizontal> block type.

Default key: Alt+L

=item mark_vertical

Starts block marking session with the C<bt::Vertical> block type.

Default key: Alt+B

=item overtype_block

Copies the selected text and overwrites the text next to the cursor position, according to
the L<blockType> value.

Default key: Alt+O

=item paste

Copies text from the clipboard and inserts it at the cursor position.

Default key: Shift+Insert

=item realize_panning

Performs deferred widget panning, activated by setting C<{delayPanning}> to 1.
The deferred operations are those performed by L<offset> and L<topLine>.

=item set_line LINE_ID, TEXT, [ OPERATION, FROM, LENGTH ]

Changes line at LINE_ID to new TEXT. Hint scalars OPERATION, FROM, and LENGTH
are used to maintain selection and marking data. OPERATION is an arbitrary string,
the ones that are recognized are C<'overtype'>, C<'add'>, and C<'delete'>.
FROM and LENGTH define the range of the change; FROM is the cluster offset and
LENGTH is the length of the changed text.

=item split_line

Splits a line in two at the cursor position.

Default key: Enter ( or Ctrl+Enter if L<wantReturns> is 0 )

=item select_all

Selects all text

=item start_block [ BLOCK_TYPE ]

Begins the block selection session. The block type is BLOCK_TYPE, if it is
specified, or the value of the L<blockType> property is otherwise.

=item update_block

Adjusts the selection inside the block session, extending or shrinking it to
the current cursor position.

=item word_left [ REPEAT = 1 ]

Moves the cursor REPEAT words to the left.

=item word_right [ REPEAT = 1 ]

Moves the cursor REPEAT words to the right.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Dialog::FindDialog>, F<examples/editor.pl>

=cut
