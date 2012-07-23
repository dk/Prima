#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk> 
#     Anton Berezin  <tobez@plab.ku.dk>
#
#  $Id$
#
# edit block types
package bt;
use constant CUA          =>  0;
use constant Vertical     =>  1;
use constant Horizontal   =>  2;

package Prima::Edit;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);

use strict;
use Prima::Const;
use Prima::Classes;
use Prima::ScrollBar;
use Prima::IntUtils;

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	ParseSyntax   => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	my $font = $_[ 0]-> get_default_font;
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
				return if $_[0]-> {readOnly};
				$_[0]-> has_selection ? $_[0]-> delete_block : $_[0]-> delete_char; 
			}],
			[ Backspace      => 0, 0, kb::Backspace, sub {$_[0]-> back_char unless $_[0]-> {readOnly}}],
			[ DeleteChunk    => 0, 0, '^Y',          sub {$_[0]-> delete_current_chunk unless $_[0]-> {readOnly}}],
			[ DeleteToEnd    => 0, 0, '^E',          sub {$_[0]-> delete_to_end unless $_[0]-> {readOnly}}],
			[ DupLine        => 0, 0, '^K',          sub {$_[0]-> insert_line($_[0]-> cursorY, $_[0]-> get_line($_[0]-> cursorY)) unless $_[0]-> {readOnly}}],
			[ DeleteBlock    => 0, 0, '@D',          sub {$_[0]-> delete_block unless $_[0]-> {readOnly}}],
			[ SplitLine      => 0, 0, kb::Enter,     sub {$_[0]-> split_line if !$_[0]-> {readOnly} && $_[0]-> {wantReturns}}],
			[ SplitLine2     => 0, 0, km::Ctrl|kb::Enter,sub {$_[0]-> split_line if !$_[0]-> {readOnly} && !$_[0]-> {wantReturns}}],
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
# undo         
			[ Undo            => 0, 0, km::Alt|kb::Backspace, q(undo)],
			[ Redo            => 0, 0, '^R', q(redo)],
		],
		autoIndent        => 1,
		autoHScroll       => 1,
		autoVScroll       => 1,
		blockType         => bt::CUA,
		borderWidth       => 1,
		cursorSize        => [ $::application-> get_default_cursor_width, $font-> { height}],
		cursorVisible     => 1,
		cursorX           => 0,
		cursorY           => 0,
		cursorWrap        => 0,
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
		hScroll           => 0,
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
		topLine           => 0,
		vScroll           => 0,
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
	$p-> {autoHScroll} = 0 if exists $p-> {hScroll};
	$p-> {autoVScroll} = 0 if exists $p-> {vScroll};
}

sub init
{
	my $self = shift;
	for ( qw( autoIndent topLine  offset resetDisabled blockType persistentBlock
		tabIndent readOnly wantReturns wantTabs))
		{ $self-> {$_} = 1; }
	for ( qw( wordWrap hScroll vScroll rows maxLineCount maxLineLength maxLineWidth
		scrollTransaction maxLine maxChunk capLen cursorY cursorX cursorWrap
		cursorXl cursorYl syntaxHilite hiliteNumbers hiliteQStrings hiliteQQStrings
		notifyChangeLock modified borderWidth autoHScroll autoVScroll blockShiftMark
	))
		{ $self-> {$_} = 0;}
	$self-> { insertMode}   = $::application-> insertMode;
	for ( qw( markers lines chunkMap hiliteIDs hiliteChars hiliteREs )) { $self-> {$_} = []}
	for ( qw( selStart selEnd selStartl selEndl)) { $self-> {$_} = [0,0]}
	$self-> {defcw} = $::application-> get_default_cursor_width;
	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;
	$self-> {undo} = [];
	$self-> {redo} = [];
	$profile{selection} = [@{$profile{selStart}}, @{$profile{selEnd}}];
	for ( qw( hiliteNumbers hiliteQStrings hiliteQQStrings hiliteIDs hiliteChars hiliteREs
		autoHScroll autoVScroll
		textRef syntaxHilite autoIndent persistentBlock blockType hScroll vScroll borderWidth
		topLine  tabIndent readOnly offset wordDelimiters wantTabs wantReturns
		wordWrap cursorWrap markers undoLimit))
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
		$self-> {maxFixedLength} = int( $size[0] / $mw);
		$self-> {tabs} = ' 'x$ti;
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
			$#chunkMap = scalar @{$self-> {lines}} * 2;
			@chunkMap = ();
			my $j = 0;
			for ( @{$self-> {lines}})
			{
				my $i;
				my $breaks = $self-> text_wrap( $_, $size[0], $twOpts, $ti);
				for ( $i = 0; $i < scalar @{$breaks} / 2; $i++)
				{
				#  push( @chunkMap, $$breaks[$i * 2]);
				#  push( @chunkMap, $$breaks[$i * 2 + 1]);
				#  push( @chunkMap, $j);
					push( @chunkMap, $$breaks[$i * 2], $$breaks[$i * 2 + 1], $j);
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
		$self-> {maxChunk} = $self-> {wordWrap} ? (scalar @{$self-> {chunkMap}}/3-1) : $self-> {maxLine};
		$self-> {yTail} = ( $yTail > 0) ? 1 : 0;
# updating selections
		$self-> selection( @{$self-> {selStart}}, @{$self-> {selEnd}});
# updating cursor
		$self-> cursor( $self-> cursor);
		my $chunk = $self-> get_chunk( $self-> {cursorYl});
		my $x     = $self-> {cursorXl};
		$self-> {cursorAtX}      = $self-> get_chunk_width( $chunk, 0, $x);
		$self-> {cursorInsWidth} = $self-> get_chunk_width( $chunk, $x, 1);
	}
# positioning cursor
	my $cx  = $a[0] + $self-> {cursorAtX} - $self-> {offset};
	my $cy  = $a[1] + $yTail + ($self-> {rows} - $self-> {cursorYl} + $self-> {topLine } - 1) * $fh;
	my $xcw = $self-> {insertMode} ? $cw : $self-> {cursorInsWidth};
	my $ycw = $fh;
	$ycw -= $a[1] - $cy, $cy = $a[1] if $cy < $a[1];
	$xcw = $size[0] + $a[0] - $cx - 1 if $cx + $xcw >= $size[0] + $a[0];
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
		my $lw = $self-> {maxLineWidth};
		if ( $self-> {autoHScroll}) {
			my $hs = ( $lw > $w) ? 1 : 0;
			if ( $hs != $self-> {hScroll}) {
				$self-> hScroll( $hs);
				$w = $self-> width - $self-> {indents}-> [0] - $self-> {indents}-> [2];
			}
		}
		$self-> {hScrollBar}-> set(
			max      => $self-> {wordWrap} ? 0 : $lw - $w,
			whole    => $lw < $w ? $w : $lw,
			value    => $self-> {offset},
			partial  => $w,
			pageStep => $lw / 5,
			step     => $self-> font-> width,
		) if $self-> {hScroll};
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
		my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(ParseSyntax));
		my @syntax;
		$#syntax = $self-> {maxLine};
		@syntax = ();
		my $i = 0;
		$self-> push_event;
		for ( @{$self-> {lines}}) {
			my $sref = undef;
			$notifier-> ( @notifyParms, $_, $sref);
			push( @syntax, $sref);
			last if $i++ > 50; # test speed...
		}
		$self-> pop_event;
		$self-> {syntax} = \@syntax;
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
				push @doers, "/\\G$re/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteREs}-> [$i+1] . "; redo; };\n";
			}
		}
		if ($self-> {hiliteIDs}) {
			my $i;
			for ($i = 0; $i < scalar @{$self-> {hiliteIDs}} - 1; $i+=2) {
				my $re = join '|', @{$self-> {hiliteIDs}-> [$i]};
				push @doers, "/\\G\\b($re)\\b/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteIDs}-> [$i+1] . "; redo; };\n";
			}
		}
		push @doers, '/\\G\\b(\\d+)\\b/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteNumbers} . "; redo; };\n"
			if defined $self-> {hiliteNumbers};
		push @doers, '/\\G("[^\\"\\\\]*(?:\\\\.[^\\"\\\\]*)*")/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteQQStrings} . "; redo; };\n"
			if defined $self-> {hiliteQQStrings};
		push @doers, '/\\G(\'[^\\\'\\\\]*(?:\\\\.[^\\\'\\\\]*)*\')/gc && do { ' .
			$rest . 'push @a, length($1), ' .
			$self-> {hiliteQStrings} . "; redo; };\n"
			if defined $self-> {hiliteQStrings};
		if ($self-> {hiliteChars}) {
			my $i;
			for ($i = 0; $i < scalar @{$self-> {hiliteChars}} - 1; $i+=2) {
				my $re = quotemeta $self-> {hiliteChars}-> [$i];
				push @doers, "/\\G([$re]+)/gc && do { " .
					$rest . 'push @a, length($1), ' .
					$self-> {hiliteChars}-> [$i+1] . "; redo; };\n";
			}
		}
		$self-> {syntaxer} = eval(<<SYNTAXER);
		sub {
			my ( \$self, \$line) = \@_;
			my \@a;
			my \$l = 0;
			\$_ = \$line; study;
			{
				@doers
				/\\G(.)/gc && do { \$l++; redo; };
			}
			$rest
			\$_[2] = \\\@a;
		};
SYNTAXER
	}
}


sub draw_colorchunk
{
	my ( $self, $canvas, $chunk, $i, $x, $y, $clr) = @_;
	my $sd = $self-> {syntax}-> [$i];
	unless ( defined $sd) {
		$self-> notify(q(ParseSyntax), $chunk, $sd);
		$self-> {syntax}-> [$i] = $sd;
	}
	my ( $cc,$j);
	my $ofs = 0;

	for ( $j = 0; $j < scalar @{$sd} - 1; $j += 2) {
		my $xd = $self-> get_chunk_width( $chunk, $ofs, $$sd[$j], \$cc);
		$canvas-> color(( $$sd[$j+1] == cl::Fore) ? $clr : $$sd[$j+1]);
		$cc =~ s/\t/$self->{tabs}/g;
		$canvas-> text_out( $cc, $x, $y);
		$x += $xd;
		$ofs += $$sd[$j];
	}
}

sub on_paint
{
# local variables definition area
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;
	my @clr    = $self-> enabled ?
	( $self-> color, $self-> backColor) :
	( $self-> disabledColor, $self-> disabledBackColor);
	my @sclr   = ( $self-> hiliteColor, $self-> hiliteBackColor);
	my ( $bw, $fh, $tl, $lc, $rc, $ofs, $yt, $tabs, $cw, $bt, $issel,
		$sh, $sx) = ( $self-> {borderWidth}, $self-> font-> height,
		$self-> {topLine }, $self-> {maxChunk}+1, $self-> {rows}, $self-> {offset},
		$self-> {yTail}, $self-> {tabs},  $self-> {defcw}, $self-> {blockType},
		$self-> has_selection, $self-> {syntaxHilite}, $self-> {syntax},
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
		# $canvas-> clipRect( $bw, $bw + $dy, $size[0] - $bw - $dx - 1, $size[1] - $bw - 1);
		$canvas-> clipRect( $a[0], $a[1], $a[2] - 1, $a[3] - 1);
		$canvas-> bar( 0, 0, @size);
	} else {
		$self-> draw_border( $canvas, $clr[1], @size);
		# $canvas-> clipRect( $bw, $bw + $dy, $size[0] - $bw - $dx - 1, $size[1] - $bw - 1);
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
	my $x    = $a[0] - $ofs;
# painting selection
	my @sel;
	my @cuaXs;
	if ( $issel) {
		@sel  = (@{$self-> {selStartl}}, @{$self-> {selEndl}});
		if ( $bt == bt::CUA) {
			@cuaXs   = (
				$a[0] - $ofs + $self-> get_chunk_width( 
					$self-> get_chunk( $sel[1]), 0, $sel[0]
				),
				$a[0] - $ofs + $self-> get_chunk_width( 
					$self-> get_chunk( $sel[3]), 0, $sel[2]
				)
			);
			my $cSet = 0;
			if ( $sel[1] == $sel[3]) {
				if ( $sel[1] >= $tl && $sel[ 1] < $lim) {
					$cSet = 1;
					$canvas-> color( $sclr[ 1]);
					$canvas-> bar( 
						$cuaXs[0], $y - $fh * ( $sel[1] - $tl - 1) - 1, 
						$cuaXs[1]-1, $y - $fh * ( $sel[1] - $tl)
					);
				}
			} else {
				if ( $sel[1] >= $tl && $sel[ 1] < $lim) {
					$cSet = 1;
					$canvas-> color( $sclr[ 1]);
					$canvas-> bar( 	
						$cuaXs[0], $y - $fh * ( $sel[1] - $tl - 1) - 1, 
						$size[0], $y - $fh * ( $sel[1] - $tl)
					);
				}
				if ( $sel[3] >= $tl && $sel[ 3] < $lim) {
					$canvas-> color( $sclr[ 1]) unless $cSet;
					$cSet = 1;
					$canvas-> bar( 
						0, $y - $fh * ( $sel[3] - $tl - 1) - 1, 
						$cuaXs[1]-1, $y - $fh * ( $sel[3] - $tl)
					);
				}
				if ( 
					$sel[3] -1 > $sel[1] && 
					( $sel[1] + 1 < $lim || $sel[ 3] - 1 >= $tl)
				) {
					$canvas-> color( $sclr[ 1]) unless $cSet;
					$cSet = 1;
					$canvas-> bar( 
						0, $y - $fh * ( $sel[1] - $tl) - 1, 
						$size[0], $y - $fh * ( $sel[3] - $tl - 1)
					);
				}
			}
			$canvas-> color( $clr[0]) if $cSet;
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
					my $c = $self-> get_chunk( $i);
					$c =~ s/\t/$tabs/g;
					$canvas-> text_out( $c, $x, $horz_y);
					$horz_y -= $fh;
				}
				$canvas-> color( $clr[0]);
			}
		}
	}
	my $cSet = 0;

# painting lines
	for ( $i = $tl; $i < $lim; $i++)
	{
		my $c = $self-> get_chunk( $i);
		if ( $issel && $i >= $sel[1] && $i <= $sel[3])
		{
	# painting selected lines
			if ( $bt == bt::CUA) {
				if ( $sel[1] == $sel[3])
				{
					my $cl = $sel[2] - length( $c);
					$c .= ' 'x$cl if $cl > 0;
					my $lc;
					if ( $sh) { 
						$self-> draw_colorchunk(
							$canvas, $c, $i, 
							$x, $y, $clr[0]
						); 
					} else {
						$lc = substr( $c, 0, $sel[0]); 
						$lc =~ s/\t/$tabs/g;
						$canvas-> text_out( $lc, $x, $y);
						
						$lc = substr( $c, $sel[2], length($c)); 
						$lc =~ s/\t/$tabs/g;
						$canvas-> text_out( $lc, $cuaXs[1], $y);
					}
					$lc = substr( $c, $sel[0], $sel[2] - $sel[0]); 
					$lc =~ s/\t/$tabs/g;
					$canvas-> color( $sclr[0]);
					$canvas-> text_out( $lc, $cuaXs[0], $y);
					$canvas-> color( $clr[0]);
				} elsif ( $i == $sel[1]) {
					my $cl = $sel[0] - length( $c);
					$c .= ' 'x$cl if $cl > 0;
					my $lc;
					if ( $sh) { 
						$self-> draw_colorchunk(
							$canvas, $c, $i, 
							$x, $y, $clr[0]
						);
					} else {
						$lc = substr( $c, 0, $sel[0]); 
						$lc =~ s/\t/$tabs/g;
						$canvas-> text_out( $lc, $x, $y);
					}
					$canvas-> color( $sclr[0]);
					$lc = substr( $c, $sel[0], length( $c)); 
					$lc =~ s/\t/$tabs/g;
					$canvas-> text_out( $lc, $cuaXs[0], $y);
					$cSet = 1;
				} elsif ( $i == $sel[3]) {
					my $cl = $sel[2] - length( $c);
					$c .= ' 'x$cl if $cl > 0;
					if ( $sh) { 
						$self-> draw_colorchunk(
							$canvas, $c, $i, 
							$x, $y, $clr[0]); 
					} else {
						$canvas-> color( $clr[0]);
						$lc = substr( $c, $sel[2], length( $c)); 
						$lc =~ s/\t/$tabs/g;
						$canvas-> text_out( $lc, $cuaXs[1], $y);
					}
					$canvas-> color( $sclr[0]);
					my $lc = substr( $c, 0, $sel[2]); 
					$lc =~ s/\t/$tabs/g;
					$canvas-> text_out( $lc, $x, $y);
					$canvas-> color( $clr[0]);
				} else {
					$c =~ s/\t/$tabs/g;
					$canvas-> color( $sclr[0]) unless $cSet;
					$cSet = 1;
					$canvas-> text_out( $c, $x, $y);
				}
			} elsif ( $bt == bt::Vertical) {
				my @vXs = (
					$self-> get_chunk_width( $c, 0, $sel[0]) - $ofs + $a[0],
					$self-> get_chunk_width( $c, 0, $sel[2]) - $ofs + $a[0]
				);
				$canvas-> color( $sclr[1]);
				$canvas-> bar( $vXs[0], $y, $vXs[1]-1, $y + $fh - 1);
				$canvas-> color( $clr[0]);
				my $cl = $sel[2] - length( $c);
				$c .= ' 'x$cl if $cl > 0;
				my $lc;
				if ( $sh) { 
					$self-> draw_colorchunk(
						$canvas, $c, $i, 
						$x, $y, $clr[0]
					);
				} else {
					$lc = substr( $c, 0, $sel[0]); 
					$lc =~ s/\t/$tabs/g;
					$canvas-> text_out( $lc, $x, $y);
					$lc = substr( $c, $sel[2], length($c)); 
					$lc =~ s/\t/$tabs/g;
					$canvas-> text_out( $lc, $vXs[1], $y);
				}
				$lc = substr( $c, $sel[0], $sel[2] - $sel[0]); 
				$lc =~ s/\t/$tabs/g;
				$canvas-> color( $sclr[0]);
				$canvas-> text_out( $lc, $vXs[0], $y);
				$canvas-> color( $clr[0]);
			}
		} else {
		# painting syntaxed lines
			if ( $sh) {
				$self-> draw_colorchunk( $canvas, $c, $i, $x, $y, $clr[0]);
			} else {
		# painting normal lines
				$c =~ s/\t/$tabs/g;
				$canvas-> color( $clr[0]);
				$canvas-> text_out( $c, $x, $y);
			}
		}
		$y -= $fh;
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
	$ry = int(( $h - $y) / $fh) + $self-> {topLine };
	$ry = 0 if $ry < 0;
	$ry = $self-> {maxChunk} if $ry > $self-> {maxChunk};
	$rx = 0;

	my $chunk  = $self-> get_chunk( $ry);
	my $cl     = ( $w + $ofs) / ($self-> get_text_width(' ')||1);
	$chunk    .= ' 'x$cl;
	if ( $ofs + $x > 0)
	{
		my $ofsx = $ofs + $x;
		$ofsx = $self-> {maxLineWidth} if $ofsx > $self-> {maxLineWidth};
		$rx = $self-> text_wrap( $chunk, $ofsx, 
			tw::CalcTabs|tw::BreakSingle|tw::ReturnFirstLineLength, $self-> {tabIndent});
	}
	return $self-> make_physical( $rx, $ry), $inBounds;
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left && $btn != mb::Middle;
	my @xy = $self-> point2xy( $x, $y);
	return unless $xy[2];
	$self-> cursor( @xy);

	if ( $btn == mb::Middle) {
		my $cp = $::application-> bring('Primary');
		return if !$cp || $self-> {readOnly};
		$self-> insert_text( $cp-> text, 0);
		$self-> clear_event;
		return;
	}

	
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
	$z = int( $z/120);
	$z *= $self-> {rows} if $mod & km::Ctrl;
	my $newTop = $self-> topLine  - $z;
	my $maxTop = $self-> {maxChunk} - $self-> {rows} + 1;
	$self-> topLine ( $newTop > $maxTop ? $maxTop : $newTop);
	$self-> clear_event;
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;

	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;
	my @xy = $self-> point2xy( $x, $y);
	return unless $xy[2];
	my $s = $self-> get_line( $xy[1]);
	$self-> clear_event;

	if ( !$dbl) {
		if ( $self-> {doubleclickTimer}) {
			$self-> {doubleclickTimer}-> destroy;
			delete $self-> {doubleclickTimer};
			$self-> selection( 0, $xy[1], length $s, $xy[1]);
		}
		return;
	}

	$self-> cancel_block;
	$self-> cursor( @xy);

	my $p = $xy[0];
	my $sl = length $s;
	my ($l,$r);

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

	$self-> selection( $l, $xy[1], $r, $xy[1]);
	$self-> {doubleclickTimer} = Prima::Timer-> create( onTick => sub{
		$self-> {doubleclickTimer}-> destroy;
		delete $self-> {doubleclickTimer};
	}) unless $self-> {doubleclickTimer};
	$self-> {doubleclickTimer}-> timeout( 
		Prima::Application-> get_system_value( sv::DblClickDelay)
	);
	$self-> {doubleclickTimer}-> start;
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
		my @cs = $self-> cursor;
		my $c  = $self-> get_line( $cs[1]);
		my $l = 0;
		$self-> begin_undo_group;
		my $chr = chr $code;
		utf8::upgrade($chr) if $mod & km::Unicode;
		if ( $self-> insertMode) {
			$l = $cs[0] - length( $c), $c .= ' ' x $l 
				if length( $c) < $cs[ 0];
			substr( $c, $cs[0], 0) = $chr x $repeat;
			$self-> set_line( $cs[1], $c, q(add), $cs[0], $l + $repeat);
		} else {
			$l = $cs[0] - length( $c) + $repeat, $c .= ' ' x $l 
				if length( $c) < $cs[ 0] + $repeat;
			substr( $c, $cs[0], $repeat) = $code x $repeat;
			$self-> set_line( $cs[1], $c, q(overtype));
		}
		$self-> cursor( $cs[0] + $repeat, $cs[1]);
		$self-> end_undo_group;
		$self-> clear_event;
	}
}

sub on_fontchanged
{
	my $self = $_[0];
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

sub set_text_ref
{
	my ( $self, $ref) = @_;
	return unless defined $ref;
	$self-> {capLen} = length( $$ref);
	$#{$self-> {lines}} = $self-> {capLen} / 40;
	@{$self-> {lines}} = ();
	@{$self-> {lines}} = split( "\n", $$ref);
	$self-> {maxLine} = scalar @{$self-> {lines}} - 1;
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
		return substr( $$ck[ $$cm[ $index * 3 + 2]], $$cm[ $index * 3], $$cm[ $index * 3 + 1]);
	} else {
		return $$ck[ $index];
	}
}

sub get_line
{
	my ( $self, $index) = @_;
	return $self-> {maxLine} >= 0 ? $self-> {lines}-> [$index] : '';
}

sub get_line_ext
{
	my ( $self, $index) = @_;
	return '' if $self-> {maxLine} < 0;
	return $self-> {lines}-> [ $self-> {wordWrap} ? 
		( $self-> {chunkMap}-> [ $index * 3 + 2]) : 
		$index
	];
}

sub get_line_dimension
{
	my ( $self, $y) = @_;
	return $y, 1 unless $self-> {wordWrap};
	( undef, $y) = $self-> make_logical( 0, $y);
	my ($ret, $ix, $cm) = ( 0, $y * 3 + 2, $self-> {chunkMap});
	$ret++, $ix += 3 while $$cm[ $ix] == $y;
	return $y, $ret;
}


sub get_chunk_org
{
	my ( $self, $index) = @_;
	return $index unless $self-> {wordWrap};
	my $cm = $self-> {chunkMap};
	my $y = $$cm[ $index * 3 + 2];
	$index-- while $y == $$cm[ $index * 3 + 2];
	return $index + 1;
}

sub get_chunk_end
{
	my ( $self, $index) = @_;
	return $index unless $self-> {wordWrap};
	my $cm = $self-> {chunkMap};
	my $y = $$cm[ $index * 3 + 2];
	my $maxY = $self-> {maxChunk};
	return -1 if $maxY < 0;
	$index++ while $index <= $maxY && $y == $$cm[ $index * 3 + 2];
	return $index - 1;
}

sub get_chunk_width
{
	my ( $self, $chunk, $from, $len, $retC) = @_;
	my $cl;
	$cl = $from + $len - length( $chunk) + 1;
	$chunk .= ' 'x$cl if $cl >= 0;
	$chunk  = substr( $chunk, $from, $len);
	$chunk  =~ s/\t/$self->{tabs}/g;
	$$retC = $chunk if $retC;
	return $self-> {fixed} ? 
		( length( $chunk) * $self-> {averageWidth}) : 
		$self-> get_text_width( $chunk);
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
	my $line = $self-> get_line( $y);
	$x = length( $line) if $x < 0;
	my ( $lx, $ly) = $self-> make_logical( $x, $y);
	my ( $olx, $oly) = ( $self-> {cursorXl}, $self-> {cursorYl});
	$self-> {cursorXl} = $lx;
	$self-> {cursorYl} = $ly;
	return if $y == $oy and $x == $ox and $lx == $olx and $ly == $oly;
	my ( $tl, $r, $yt) = ( $self-> {topLine }, $self-> {rows}, $self-> {yTail});
	if ( $ly < $tl) {
		$self-> topLine ( $ly);
	} elsif ( $ly >= $tl + $r) {
		my $nfc = $ly - $r + 1;
		$self-> topLine ( $nfc);
	}
	my $chunk  = $self-> get_chunk( $ly);
	my $atX    = $self-> get_chunk_width( $chunk, 0, $lx);
	my $deltaX = $self-> get_chunk_width( $chunk, $lx, 1);
	my $actualWidth = $self-> width - 
		$self-> {indents}-> [0] - 
		$self-> {indents}-> [2] - 
		$self-> {defcw};
	my $ofs = $self-> {offset};
	my $avg = $self-> {averageWidth};
	if ( $atX < $ofs) {
		my $nofs = $atX;
		$self-> offset( $nofs - $avg);
	} elsif ( $atX >= $ofs + $actualWidth - $deltaX) {
		my $nofs = $atX - $actualWidth + $deltaX;
		$nofs = $ofs + $avg if $nofs - $ofs < $avg;
		$self-> offset( $nofs);
	}          
	# check if last undo record contains cursor movements only, so these movements
	# can be grouped                                
	my $undo = 1;
	if ( !$self-> {undo_in_action} && @{$self-> {undo}} && @{$self-> {undo}-> [-1]}) {
		my $ok = 1;   
		for ( @{$self-> {undo}-> [-1]}) {
			$ok = 0, last if $$_[0] ne 'cursor';
		}
		$undo = 0 if $ok;
	} 
	$self-> push_undo_action( 'cursor', $self-> {cursorX}, $self-> {cursorY}) if $undo;
	$self-> {cursorX}        = $x;
	$self-> {cursorY}        = $y;
	$self-> {cursorAtX}      = $atX;
	$self-> {cursorInsWidth} = $deltaX;

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
	$offset = 0 if $offset < 0;
	$offset = 0 if $self-> {wordWrap};
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
	my ($firstChunk, $lastChunk) = ( $self-> get_line( $sy), $self-> get_line( $ey));
	my ($fcl, $lcl) = ( length( $firstChunk), length( $lastChunk));
	my $bt = $self-> {blockType};
	$sx = $fcl if ( $bt != bt::Vertical && $sx > $fcl) || ( $sx < 0);
	$ex = $lcl if ( $bt != bt::Vertical && $ex > $lcl) || ( $ex < 0);
	( $sx, $ex) = ( $ex, $sx) if $sx > $ex && (( $sy == $ey && $bt == bt::CUA) || ( $bt == bt::Vertical));
	my ( $lsx, $lsy) = $self-> make_logical( $sx, $sy);
	my ( $lex, $ley) = $self-> make_logical( $ex, $ey);
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
				} elsif ( $ex == $oex) {
					$start = $end = $sy;
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
	my @a = $self-> get_active_area( 0);
	return if $end < $tl || $start >= $tl + $r + $yT;
	if ( $start == $end && $bt == bt::CUA) {
		# single connective line paint
		my $chunk;
		my ( $xstart, $xend);
		if ( $sx == $osx) {
			( $xstart, $xend) = ( $ex < $oex) ? ( $ex, $oex) : ( $oex, $ex);
		} elsif ( $ex == $oex) {
			( $xstart, $xend) = ( $sx < $osx) ? ( $sx, $osx) : ( $osx, $sx);
		} else {
			$xstart = ( $sx < $osx) ? $sx : $osx;
			$xend   = ( $ex > $oex) ? $ex : $oex;
		}
		unless ( $self-> {wordWrap}) {
			if ( $start == $sy) {
				$chunk = $firstChunk;
			} elsif ( $start == $ey) {
				$chunk = $lastChunk;
			} else {
				$chunk = $self-> get_chunk( $start);
			}
		} else {
			$chunk = $self-> get_chunk( $start);
		}
		$self-> invalidate_rect(
			$a[0] - $ofs + $self-> get_chunk_width( $chunk, 0, $xstart) - 1,
			$a[3] - $fh * ( $start - $tl + 1),
			$a[0] - $ofs + $self-> get_chunk_width( $chunk, 0, $xend),
			$a[3] - $fh * ( $start - $tl)
		);
	} else {
		# general connected lines paint
		$self-> invalidate_rect(
			$a[0], $a[3] - $fh * ( $end - $tl + 1),
			$a[2], $a[3] - $fh * ( $start - $tl),
		);
	}
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
	$sh = 0 if $self-> {wordWrap};
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
	$self-> syntaxHilite(0) if $ww;
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
	my @sel = $self-> selection;
	my $text = '';
	my $bt = $self-> blockType;
	if ( $bt == bt::CUA) {
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
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
		$text .= $self-> get_line( $i)."\n";
		}
	} else {
		my $i;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			my $c = $self-> get_line( $i);
			my $cl = $sel[2] - length( $c);
			$c .= ' 'x$cl if $cl > 0;
			$text .= substr($c, $sel[0], $sel[2] - $sel[0])."\n";
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
	my @ln = split( "\n", $s, -1);
	pop @ln unless length $ln[-1];
	$s = $self-> get_line( $cs[1]);
	my $cl = $cs[0] - length( $s);
	$s .= ' 'x$cl if $cl > 0;
	$cl = 0 if $cl < 0;
	$self-> lock_change(1);
	if ( scalar @ln == 1) {
		substr( $s, $cs[0], 0) = $ln[0];
		$self-> set_line( $cs[1], $s, q(add), $cs[0], $cl + length( $ln[0]));
		$self-> selection( $cs[0], $cs[1], $cs[0] + length( $ln[0]), $cs[1])
			if $hilite && $self-> {blockType} == bt::CUA;
	} else {
		my $spl = substr( $s, $cs[0], length( $s) - $cs[0]);
		substr( $s, $cs[0], length( $s) - $cs[0]) = $ln[0];
		$self-> lock;
		$self-> set_line( $cs[1], $s);
		shift @ln;
		$self-> insert_line( $cs[1] + 1, (@ln, $spl));
		$self-> selection( $cs[0], $cs[1], length( $ln[-1]), $cs[1]+scalar(@ln))
			if $hilite && $self-> {blockType} == bt::CUA;
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

sub make_logical
{
	my ( $self, $x, $y) = @_;
	return (0,0) if $self-> {maxChunk} < 0;
	return $x, $y unless $self-> {wordWrap};
	my $maxY = $self-> {maxLine};
	$y = $maxY if $y > $maxY || $y < 0;
	$y = 0 if $y < 0;
	my $l = length( $self-> {lines}-> [$y]);
	$x = $l if $x < 0 || $x > $l;
	$x = 0 if $x < 0;
	my $cm = $self-> {chunkMap};
	my $r;
	( $l, $r) = ( 0, $self-> {maxChunk} + 1);
	my $i = int($r / 2);
	my $kk = 0;
	while (1) {
		my $acd = $$cm[$i * 3 + 2];
		last if $acd == $y;
		$acd > $y ? $r : $l   = $i;
		$i = int(( $l + $r) / 2);
		if ( $kk++ > 200) {
			print "bcs dump to $y\n";
			( $l, $r) = ( 0, $self-> {maxChunk} + 1);
			$i = int($r / 2);
			for ( $kk = 0; $kk < 7; $kk++) {
				my $acd = $$cm[$i * 3 + 2];
				print "i:$i [$l $r] f() = $acd\n";
				$acd > $y ? $r : $l   = $i;
				$i = int(( $l + $r) / 2);
			}
			die;
			last;
		}
	}
	$y = $i;
	$i *= 3;
	$i-= 3, $y-- while $$cm[ $i] != 0;
	$i+= 3, $y++ while $x > $$cm[ $i] + $$cm[ $i + 1];
	$x -= $$cm[ $i];
	return $x, $y;
}

sub make_physical
{
	my ( $self, $x, $y) = @_;
	return (0,0) if $self-> {maxLine} < 0;
	return $x, $y unless $self-> {wordWrap};
	my $maxY = $self-> {maxChunk};
	$y = $maxY if $y > $maxY || $y < 0;
	$y = 0 if $y < 0;
	my $cm = $self-> {chunkMap};
	my ( $ofs, $l, $nY) = ( $$cm[ $y * 3], $$cm[ $y * 3 + 1], $$cm[ $y * 3 + 2]);
	$x = $l if $x < 0 || $x > $l;
	$x = 0 if $x < 0;
	return $x + $ofs, $nY;
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

sub cursor_down
{
	my $d = $_[1] || 1;
	$_[0]-> cursorLog( $_[0]-> {cursorXl}, $_[0]-> {cursorYl} + $d);
}

sub cursor_up
{
	return if $_[0]-> {cursorYl} == 0;
	my $d = $_[1] || 1;
	my ( $x, $y) = $_[0]-> make_physical( $_[0]-> {cursorXl}, $_[0]-> {cursorYl} - $d);
	$y = 0 if $y < 0;
	$_[0]-> cursor( $x, $y);
}

sub cursor_left
{
	my $d = $_[1] || 1;
	my $x = $_[0]-> cursorX;
	if ( $x - $d >= 0) {
		$_[0]-> cursorX( $x - $d)
	} elsif ( $_[0]-> {cursorWrap}) {
		if ( $d == 1) {
			my $y = $_[0]-> cursorY - 1;
			$_[0]-> cursor( -1, $y < 0 ? 0 : $y);
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
	if ( $_[0]-> {cursorWrap} || $_[0]-> {wordWrap}) {
		my $y = $_[0]-> cursorY;
		if ( $x + $d > length( $_[0]-> get_line( $y))) {
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
	my ($spaces) = ($_[0]-> get_line( $_[0]-> cursorY) =~ /^([s\t]*)/);
	$_[0]-> begin_undo_group;
	$_[0]-> offset(0);
	$_[0]-> cursorX(0);
	$_[0]-> end_undo_group;
}

sub cursor_end
{
	my ($nonspaces) = ($_[0]-> get_line( $_[0]-> cursorY) =~ /^(.*?)[\s\t]*$/);
	$_[0]-> cursorX( length $nonspaces);

}
sub cursor_cend  { $_[0]-> cursorY(-1); }
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
		my $line  = $self-> get_line( $y);
		my $clen  = length( $line);
		if ($self-> {cursorWrap}) {
			while ( $x >= $clen) {
				$y++;
				return if $y > $maxY;
				$x = 0;
				$line = $self-> get_line( $y);
				$clen = length( $line);
			}
		}
		my $cl = $x - $clen + 1;
		$line .= ' 'x$cl if $cl >= 0;
		unless ($w =~ quotemeta substr $line, $x, 1) {
			$delta++ while ( $w !~ quotemeta substr $line, $x + $delta, 1) &&
				$x + $delta < $clen;
		}
		if ( $x + $delta < $clen) {
			$delta++ while ( $w =~ quotemeta substr $line, $x + $delta, 1) &&
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
	for ( $i= 0;$i<$d; $i++) {
		my ( $x, $y, $w, $delta) =
			( $self-> cursorX, $self-> cursorY, $self-> wordDelimiters, 0);
		my $line = $self-> get_line( $y);
		my $clen = length( $line);
		if ($self-> {cursorWrap}) {
			while ( $x == 0) {
				$y--;
				$y = 0, last if $y < 0;
				$line = $self-> get_line( $y);
				$x = $clen = length( $line);
			}
		}
		my $cl = $x - $clen + 1;
		$line .= ' 'x$cl if $cl >= 0;
		if ( $w =~ quotemeta( substr( $line, $x - 1, 1)))
		{
			$delta-- while (( $w =~ quotemeta( substr( $line, $x + $delta - 1, 1))) &&
				( $x + $delta > 0))
		}
		if ( $x + $delta > 0)
		{
			$delta-- while (!( $w =~ quotemeta( substr( $line, $x + $delta - 1, 1))) &&
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
			$self-> make_physical( 0, $self-> {cursorYl}),
			$self-> make_physical( -1, $self-> {cursorYl})
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
		my $breaks = $self-> text_wrap( 
			$line, 
			$sz[0] - $self-> {defcw}, 
			tw::WordBreak|tw::CalcTabs|tw::NewLineBreak|tw::ReturnChunks, 
			$self-> {tabIndent}
		);
		my @chunkMap;
		( undef, $ry) = $self-> make_logical( 0, $y);
		my ($ix, $cm) = ( $ry * 3 + 2, $self-> {chunkMap});
		my $max_ix = $self-> {maxChunk} * 3 + 2;
		$oldDim++, $ix += 3 while $ix <= $max_ix && $$cm[ $ix] == $y;
		$newDim = scalar @{$breaks} / 2;
		my $i;
		for ( $i = 0; $i < $newDim; $i++) {
			push( @chunkMap, $$breaks[$i * 2], $$breaks[$i * 2 + 1], $y);
		}
		splice( @{$cm}, $ry * 3, $oldDim * 3, @chunkMap);

		$self-> {lines}-> [$y] = $line;
		$self-> {maxChunk} -= $oldDim - $newDim;
		if ( $oldDim == $newDim) {
			( $_from, $_to) = ( $ry, $ry + $oldDim);
		} else {
			$self-> vScroll( $self-> {maxChunk} >= $self-> {rows}) 
				if $self-> {autoVScroll};
			$self-> topLine(0) if $self-> {maxChunk} < $self-> {rows};
			$self-> {vScrollBar}-> set(
				max   => $self-> {maxChunk} - $self-> {rows} + 1,
				whole => $self-> {maxChunk} + 1,
			) if $self-> {vScroll};
		}
	} else {
		my ( $oldL, $newL) = ( length( $self-> {lines}-> [$y]), length( $line));
		$self-> {lines}-> [$y] = $line;
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
			my $lw = $self-> {maxLineWidth};
			if ( $self-> {autoHScroll}) {
				my $hs = ( $lw > $sz[0] ) ? 1 : 0;
				if ( $hs != $self-> {hScroll}) {
					$self-> hScroll( $hs);
					@a = $self-> get_active_area;
					@sz = ( $a[2] - $a[0], $a[3] - $a[1]);
				}
			}
			$self-> {hScrollBar}-> set(
				max      => $lw - $sz[0],
				whole    => $lw,
				partial  => $sz[0],
			) if $self-> {hScroll};
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
				($sel[2], $sel[3]) = $self-> make_physical( @selE);
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
			( undef, $ly) = $self-> make_logical( 0, $y);
		}
		my ($i, $maxC, $cm) = ( 0, $self-> {maxChunk}, $self-> {chunkMap});
		if ( $y <= $maxY) {
			splice( @{$cm}, $ly * 3, 0, ( 0, 0, $y) x $len);
			for ( $i = $ly + 1; $i < $ly + $len; $i++) { 
				$$cm[ $i * 3 + 2] += $i - $ly; 
			}
			for ( $i = $ly + $len; $i <= $maxC + $len; $i++) { 
				$$cm[ $i * 3 + 2] += $len; 
			}
		} else {
			push( @{$cm}, ( 0, 0, $y)x$len);
			for ( $i = $ly; $i < $ly + $len; $i++) { 
				$$cm[ $i * 3 + 2] += $i - $ly; 
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
	splice( @{$self-> {lines}}, $y, 0, ('') x $len);
	$self-> {maxLine} += $len;
	splice( @{$self-> {syntax}}, $y, 0, ([0,cl::Black]) x $len) 
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
	$self-> vScroll( $self-> {maxChunk} >= $self-> {rows}) if $self-> {autoVScroll};
	$self-> {vScrollBar}-> set(
		max      => $self-> {maxChunk} - $self-> {rows} + 1,
		whole    => $self-> {maxChunk} + 1,
		partial  => $self-> {rows},
	) if $self-> {vScroll};
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
	my ( $lx, $ly) = (0,0);
	if ( $self-> {wordWrap}) {
		( $lx, $ly) = $self-> make_logical( 0, $y);
		$lx = 0;
		my ($i, $maxC, $cm) = ($ly, $self-> {maxChunk}, $self-> {chunkMap});
		$lx++, $i++ while ( $i <= $maxC) and ( $$cm[ $i * 3 + 2] <= ( $y + $len - 1));
		splice( @{$cm}, $ly * 3, $lx * 3);
		$self-> {maxChunk} -= $lx;
		for ( $i = $ly; $i <= $maxC - $lx; $i++) { $$cm[ $i * 3 + 2] -= $len; }
	} else {
		$self-> {maxChunk} -= $len;
	}

	my @removed = splice( @{$self-> {lines}}, $y, $len);
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
			$self-> make_physical($sel[0], $sel[1]), 
			$self-> make_physical($sel[2], $sel[3])
		);
		delete $self-> {anchor};
		$self-> cancel_block unless $self-> has_selection;
	}

	$self-> vScroll( $self-> {maxChunk} >= $self-> {rows}) if $self-> {autoVScroll};
	$self-> topLine(0) if $self-> {maxChunk} < $self-> {rows};
	$self-> {vScrollBar}-> set(
		max   => $self-> {maxChunk} - $self-> {rows} + 1,
		whole => $self-> {maxChunk} + 1,
	) if $self-> {vScroll};

	unless ( $self-> {wordWrap}) {
		my $mlv       = $self-> {maxLineLength};
		for ( @removed) {
			$self-> {maxLineCount}-- if length($_) == $mlv;
			if ( $self-> {maxLineCount} <= 0) {
				$self-> reset;
				my $lw = $self-> {maxLineWidth};
				my $w  = $self-> width - 
					$self-> {indents}-> [0] - 
					$self-> {indents}-> [2];
				if ( $self-> {autoHScroll}) {
					my $hs = ( $lw > $w) ? 1 : 0;
					if ( $hs != $self-> {hScroll}) {
						$self-> hScroll( $hs);
						$w = $self-> width - 
							$self-> {indents}-> [0] - 
							$self-> {indents}-> [2];
					}
				}
				$self-> {hScrollBar}-> set(
					max      => $lw - $w,
					whole    => $lw,
					partial  => $w,
				) if $self-> {hScroll};
				last;
			}
		}
	}
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
	my $psy   = $$cm[ $y * 3 + 2];
	my $pey   = $$cm[($y + $len - 1) * 3 + 2];
	my $start = $$cm[ $y * 3];
	my $end   = $$cm[($y + $len - 1) * 3] + $$cm[($y + $len - 1) * 3 + 1];
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
	return if $x < 0;

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
	my $self = $_[0];
	$self-> delete_text( $self-> cursor, $_[1] || 1);
}

sub back_char
{
	my $self = $_[0];
	my @c = $self-> cursor;
	my $d = $_[1] || 1;

	$self-> begin_undo_group;
	if ( $c[0] >= $d) {
		$self-> delete_text( $c[0] - $d, $c[1], $d);
		$self-> cursorX( $c[0] - $d);
	} elsif ( $c[1] > 0) {
		$self-> cursor( -1, $c[1] - 1);
		$self-> delete_text( -1, $c[1] - 1);
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
	my $c = $self-> get_line( $cs[1]);
	return if $cs[ 0] > length( $c);

	$self-> set_line( $cs[1], substr( $c, 0, $cs[0]), q(delete), $cs[0], length( $c) - $cs[0]);
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
			substr( $c, $sel[0], $sel[2] - $sel[0]) = '';
			$self-> set_line( $sel[1], $c);
		} else {
			my ( $from, $len) = ( $sel[1], $sel[3] - $sel[1]);
			my $res = substr( $self-> get_line( $from), 0, $sel[0]);
			$c = $self-> get_line( $sel[3]);
			if ( $sel[2] < length( $c)) {
				$res .= substr( $c, $sel[2], length( $c) - $sel[2]);
			} elsif ( $sel[3] < $self-> {maxLine}) {
				$res .= $self-> get_line( $sel[3] + 1);
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
				substr( $c, $sel[0], $len) = '';
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

	my @sel = $self-> selection;
	$self-> lock_change(0);
	$self-> lock;
	$self-> begin_undo_group;
	if ( $self-> {blockType} == bt::Horizontal) {
		my @lines;
		my $i;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) { 
			push @lines, $self-> get_line( $i);
		}
		$self-> insert_line( $self-> cursorY, @lines);
	} else {
		my @lines;
		my $i;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			my $c = $self-> get_line( $i);
			$c .= ' ' x ($sel[2]-length($c)) 
				if length($c) < $sel[2];
			push( @lines, substr( $c, $sel[0], $sel[2]-$sel[0]));
		}
		my @cs = $self-> cursor;
		for ( $i = $cs[1]; $i < $cs[1] + scalar @lines; $i++) {
			my $c = $self-> get_line( $i);
			$c .= ' 'x($cs[0]-length($c)) if length($c) < $cs[0];
			substr( $c, $cs[0], 0) = $lines[ $i - $cs[1]];
			$self-> set_line( $i, $c);
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
	if ( $self-> {blockType} == bt::Horizontal) {
		my $i;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			$self-> set_line( $i, $self-> get_line( $i));
		}
	} else {
		my @lines;
		my $i;
		for ( $i = $sel[1]; $i <= $sel[3]; $i++) {
			my $c = $self-> get_line( $i);
			$c .= ' ' x ($sel[2]-length($c)) if length($c) < $sel[2];
			push( @lines, substr( $c, $sel[0], $sel[2]-$sel[0]));
		}
		my @cs = $self-> cursor;
		my $bx = $sel[3] - $sel[1] + 1;
		for ( $i = $cs[1]; $i < $cs[1] + scalar @lines; $i++) {
			my $c = $self-> get_line( $i);
			$c .= ' ' x ($cs[0]-length($c)) 
				if length($c) < $cs[0];
			substr( $c, $cs[0], $bx) = $lines[ $i - $cs[1]];
			$self-> set_line( $i, $c);
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
	my $c = $self-> get_line( $cs[1]);
	$c .= ' 'x($cs[0]-length($c)) if length($c) < $cs[0];
	my ( $old, $new) = ( substr( $c, 0, $cs[0]), substr( $c, $cs[0], length( $c) - $cs[0]));
	$self-> lock_change(1);
	$self-> begin_undo_group;
	$self-> set_line( $cs[1], $old, q(delete), $cs[0], length( $c) - $cs[0]);
	my $cshift = 0;
	if ( $self-> {autoIndent}) {
		my $i = 0;
		my $add = '';
		for ( $i = 0; $i < length( $old); $i++) {
			my $c = substr( $old, $i, 1);
			last if $c ne ' ' and $c ne '\t';
			$add .= $c;
		}
		$new = $add.$new, $cshift = length( $add) 
			if length( $add) < length( $old);
	}
	$self-> insert_line( $cs[1]+1, $new);
	$self-> cursor( $cshift, $cs[1] + 1);
	$self-> end_undo_group;
	$self-> lock_change(0);
}

sub begin_undo_group 
{ 
	my $self = $_[0];
	return if !$self-> {undoLimit};
	if ( $self-> {undo_in_action}) {
		push @{$self-> {redo}}, [] unless $self-> {grouped_undo}++;
	} else {
		push @{$self-> {undo}}, [] unless $self-> {grouped_undo}++;
		$self-> {redo} = [] if !$self-> {redo_in_action};
	}
}

sub end_undo_group   
{ 
	my $self = $_[0];
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	$self-> {grouped_undo}-- if $self-> {grouped_undo} > 0;
	# skip last record if empty
	pop @{$self-> {$ref}} 
		if !$self-> {grouped_undo} && 
			@{$self-> {$ref}} && 
			0 == @{$self-> {$ref}-> [-1]};
	shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
}

sub push_undo_action   
{ 
	my $self = shift;
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	my $action = [ @_ ];
	if ( $self-> {grouped_undo}) {
		push @{$self-> {$ref}-> [-1]}, $action;
	} else {
		push @{$self-> {$ref}}, [ $action ];
		shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
		$self-> {redo} = [] 
			if !$self-> {redo_in_action} && !$self-> {undo_in_action};
	}
}              

sub push_group_undo_action   
{ 
	my $self = shift;
	return if !$self-> {undoLimit};
	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	return $self-> push_undo_action(@_) if $self-> {grouped_undo};

	push @{$self-> {$ref}}, [] unless @{$self-> {$ref}};
	$self-> {grouped_undo} = 1;
	$self-> push_undo_action(@_);
	$self-> {grouped_undo} = 0;
}

sub undo
{
	my $self = $_[0];
	return if $self-> {undo_in_action} || !$self-> {undoLimit};
	return unless @{$self-> {undo}};
	my $group = pop @{$self-> {undo}};
	return unless $group && @$group;

	$self-> {undo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {undo_in_action} = 0;
}

sub redo
{
	my $self = $_[0];
	return if !$self-> {undoLimit};
	return unless @{$self-> {redo}};
	my $group = pop @{$self-> {redo}};
	return unless $group && @$group;

	$self-> {redo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {redo_in_action} = 0;
}

sub undoLimit
{
	return $_[0]-> {undoLimit} unless $#_;

	my ( $self, $ul) = @_;
	$self-> {undoLimit} = $ul if $ul >= 0;
	splice @{$self-> {undo}}, 0, $ul - @{$self-> {undo}} if @{$self-> {undo}} > $ul;
}

sub find
{
	my ( $self, $line, $x, $y, $replaceLine, $options) = @_;
	$x ||= 0;
	$y ||= 0;
	my $maxY = $self-> {maxLine};
	return if $y > $maxY || $maxY < 0;

	$line = '('.quotemeta( $line).')' unless $options & fdo::RegularExpression;
	$replaceLine = quotemeta( $replaceLine), $replaceLine =~ s[\\(\$\d+)][$1]g
		if !($options & fdo::RegularExpression) && defined $replaceLine;
	$y = $maxY if $y < 0;
	my $c = $self-> get_line( $y);
	my ( $l, $b, $len, $subLine, $re, $re2, $esub);
	$x = length( $c) if $x < 0;
	$re  = '/';
	$re .= '\\b' if $options & fdo::WordsOnly;
	$re .= "$line";
	$re .= '\\b' if $options & fdo::WordsOnly;
	$re .= '/';
	$re2 = '';
	$re2.= 'i' unless $options & fdo::MatchCase;

	unless ( $options & fdo::BackwardSearch) {
		$l = 0;
		$subLine = substr( $c, 0, $x);
		substr( $c, 0, $x) = '';
		$esub = eval(<<FINDER);
sub {
	\$_ = \$c;
	while ( 1) {
	if ( $re$re2) {
		\$l   = length(\$`);
		\$len = length(\$&);
		s$re$replaceLine/$re2 if defined \$replaceLine;
		return \$l+\$x, \$y, \$len, \$subLine.\$_;
	}
	\$y++;
	return if \$y > \$maxY;
	\$_ = \$self-> get_line( \$y);
	\$subLine = '';
	\$l = \$x = 0;
	}
}
FINDER
	} else {
		$re2 .= 'g';
		$l    = $b = undef;
		$subLine = substr( $c, $x, length( $c) - $x);
		substr( $c, $x, length( $c) - $x) = '';
		$esub = eval(<<FINDER);
sub {
	\$_ = \$c;
	while ( 1) {
		\$l = \$b, \$b = pos(\$_) while $re$re2;
		if ( defined \$b) {
			\$l ||= 0;
			pos = \$l;
			\$l = $re$re2;
			\$len = length(\$&);
			\$l   = pos(\$_) - \$len;
			substr(\$_, \$l, \$len) = \"$replaceLine\" if defined \$replaceLine;
			pos = undef;
			return \$l, \$y, \$len, \$_.\$subLine;
		}
		\$y--;
		return if \$y < 0;
		\$_ = \$self-> get_line( \$y);
		\$subLine = '';
	
}
FINDER
	}
	return $esub-> ();
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
sub cursorLog       {($#_)?($_[0]-> set_cursor    ($_[0]-> make_physical($_[1],$_[2])))            :return $_[0]-> {cursorXl},$_[0]-> {cursorYl}}
sub cursorX         {($#_)?($_[0]-> set_cursor    ($_[1],$_[0]-> {cursorY})):return $_[0]-> {cursorX}    }
sub cursorY         {($#_)?($_[0]-> set_cursor    ($_[0]-> {q(cursorX)},$_[1])):return $_[0]-> {cursorY}    }
sub cursorWrap      {($#_)?($_[0]-> {cursorWrap     }=$_[1])                :return $_[0]-> {cursorWrap     }}
sub topLine         {($#_)?($_[0]-> set_top_line (   $_[1]))               :return $_[0]-> {topLine }    }
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
sub readOnly        {($#_)?($_[0]-> {readOnly }     = $_[1])                :return $_[0]-> {readOnly }    }
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

__DATA__

=pod

=head1 NAME

Prima::Edit - standard text editing widget

=head1 SYNOPSIS

	use Prima::Edit;
	my $e = Prima::Edit-> create(
		text         => 'Hello $world',
		syntaxHilite => 1,
	);
	$e-> selection( 1, 1, 1, 2);

=head1 DESCRIPTION

The class provides text editing capabilities, three types of selection, text wrapping,
syntax highlighting, auto indenting, undo and redo function, search and replace methods.

The module declares C<bt::> package, that contains integer constants for selection block type,
used by L<blockType> property.

=head1 USAGE

The class addresses the text space by (X,Y)-coordinates,
where X is character offset and Y is line number. The addressing can be
'physical' and 'logical', - in logical case Y is number of line of text.
The difference can be observed if L<wordWrap> property is set to 1, when a single 
text string can be shown as several sub-strings, called I<chunks>.

The text is stored line-wise in C<{lines}> array; to access it use L<get_line> method.
To access the text chunk-wise, use L<get_chunk> method.

All keyboard events, except the character input and tab key handling, are 
processed by the accelerator table ( see L<Prima::Menu> ). The default
C<accelItems> table defines names, keyboard combinations, and the corresponding
actions to the class functions. The class does not provide functionality to change
these mappings. To do so, consult L<Prima::Menu/Prima::AccelTable>.

=head1 API

=head2 Events

=over

=item ParseSyntax TEXT, RESULT_ARRAY_REF

Called when syntax highlighting is requires - TEXT is a string to be parsed,
and the parsing results to be stored in RESULT_ARRAY_REF, which is a reference
to an array of integer pairs, each representing a single-colored text chunk.
The first integer in the pairs is the length of a chunk, the second - color
value ( C<cl::XXX> constants ).

=back

=head2 Properties

=over

=item autoIndent BOOLEAN

Selects if the auto indenting feature is on.

Default value: 1

=item blockType INTEGER

Defines type of selection block. Can be one of the following constants:

=over

=item bt::CUA

Normal block, where the first and the last line of the selection can
be partial, and the lines between occupy the whole line. CUA stands for
'common user access'.

Default keys: Shift + arrow keys

See also: L<cursor_shift_key>

=item bt::Vertical

Rectangular block, where all selected lines are of same offsets and lengths.

Default key: Alt+B 

See also: L<mark_vertical>

=item bt::Horizontal

Rectangular block, where the selection occupies the whole line.

Default key: Alt+L

See also: L<mark_horizontal>

=back

=item cursor X, Y

Selects physical position of the cursor

=item cursorX X

Selects physical horizontal position of the cursor

=item cursorY Y

Selects physical vertical position of the cursor

=item cursorWrap BOOLEAN

Selects cursor behavior when moved horizontally outside the line. If 0, the cursor is
not moved. If 1, the cursor moved to the adjacent line. 

See also: L<cursor_left>, L<cursor_right>, L<word_left>, L<word_right>.

=item insertMode BOOLEAN

Governs the typing mode - if 1, the typed text is inserted, if 0, the text overwrites
the old text. When C<insertMode> is 0, the cursor shape is thick and covers the whole
character; when 1, it is of default width.

Default toggle key: Insert

=item hiliteNumbers COLOR

Selects the color for number highlighting

=item hiliteQStrings COLOR

Selects the color for highlighting the single-quoted strings

=item hiliteQQStrings COLOR

Selects the color for highlighting the double-quoted strings

=item hiliteIDs ARRAY

Array of scalar pairs, that define words to be highlighted.
The first item in the pair is an array of words; the second item is
a color value.

=item hiliteChars ARRAY

Array of scalar pairs, that define characters to be highlighted.
The first item in the pair is a string of characters; the second item is
a color value.

=item hiliteREs ARRAY

Array of scalar pairs, that define character patterns to be highlighted.
The first item in the pair is a perl regular expression; the second item is
a color value.

=item mark MARK [ BLOCK_TYPE ]

Selects block marking state. If MARK is 1, starts the block marking,
if 0 - stops the block marking. When MARK is 1, BLOCK_TYPE can be used
to set the selection type ( C<bt::XXX> constants ). If BLOCK_TYPE is 
unset the value of L<blockType> is used.

=item markers ARRAY

Array of arrays with integer pairs, X and Y, where each represents
a physical coordinates in text. Used as anchor storage for fast navigation.

See also: L<add_marker>, L<delete_marker>

=item modified BOOLEAN

A boolean flag that shows if the text was modified. Can be used externally,
to check if the text is to be saved to a file, for example.

=item offset INTEGER

Horizontal offset of text lines in pixels. 

=item persistentBlock BOOLEAN

Selects whether the selection is cancelled as soon as the cursor is moved ( 0 )
or it persists until the selection is explicitly changed ( 1 ). 

Default value: 0

=item readOnly BOOLEAN

If 1, no user input is accepted.

=item selection X1, Y1, X2, Y2

Accepts two pair of coordinates, ( X1,Y1) the beginning and ( X2,Y2) the end
of new selection, and sets the block according to L<blockType> property.

The selection is null if X1 equals to X2 and Y1 equals to Y2. 
L<has_selection> method returns 1 if the selection is non-null.

=item selStart X, Y

Manages the selection start. See L<selection>, X1 and Y1.

=item selEnd X, Y

Manages the selection end. See L<selection>, X2 and Y2.

=item syntaxHilite BOOLEAN

Governs the syntax highlighting. Is not implemented for word wrapping mode.

=item tabIndent INTEGER

Maps tab ( \t ) key to C<tabIndent> amount of space characters.

=item text TEXT

Provides access to all the text data. The lines are separated by 
the new line ( \n ) character. 

See also: L<textRef>.

=item textRef TEXT_PTR

Provides access to all the text data. The lines are separated by 
the new line ( \n ) character. TEXT_PTR is a pointer to text string. 

The property is more efficient than L<text> with the large text,
because the copying of the text scalar to the stack stage is eliminated.

See also: L<text>.

=item topLine INTEGER

Selects the first line of the text drawn.

=item undoLimit INTEGER

Sets limit on number of stored atomic undo operations. If 0,
undo is disabled.

Default value: 1000 

=item wantTabs BOOLEAN

Selects the way the tab ( \t ) character is recognized in the user input.
If 1, it is recognized by the Tab key; however, this disallows the toolkit 
widget tab-driven navigation. If 0, the tab character can be entered by
pressing Ctrl+Tab key combination.

Default value: 0

=item wantReturns BOOLEAN

Selects the way the new line ( \n ) character is recognized in the user input.
If 1, it is recognized by the Enter key; however, this disallows the toolkit 
default button activation. If 0, the new line character can be entered by
pressing Ctrl+Enter key combination.

Default value: 1


=item wordDelimiters STRING

Contains string of character that are used for locating a word break. 
Default STRING value consists of punctuation marks, space and tab characters,
and C<\xff> character.

See also: L<word_left>, L<word_right>


=item wordWrap BOOLEAN

Selects whether the long lines are wrapped, or can be positioned outside the horizontal
widget inferior borders. If 1, L<syntaxHilite> is not used. A line of text can be represented
by more than one line of screen text ( chunk ) . To access the text chunk-wise, use L<get_chunk>
method.

=back

=head2 Methods

=over

=item add_marker X, Y

Adds physical coordinated X,Y to L<markers> property.

=item back_char [ REPEAT = 1 ] 

Removes REPEAT times a character left to the cursor. If the cursor is on 0 x-position,
removes the new-line character and concatenates the lines.

Default key: Backspace

=item begin_undo_group

Opens bracket for group of actions, undone as single operation. 
The bracket is closed by calling C<end_undo_group>.  

=item cancel_block

Removes the selection block

Default key: Alt+U

=item change_locked

Returns 1 if the logical locking is on, 0 if it is off. 

See also L<lock_change>.

=item copy

Copies the selected text, if any, to the clipboard.

Default key: Ctrl+Insert

=item copy_block

Copies the selected text and inserts it into the cursor position, according to
the L<blockType> value.

Default key: Alt+C

=item cursor_cend

Moves cursor to the bottom line

Default key: Ctrl+End

=item cursor_chome

Moves cursor to the top line

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

Delete REPEAT characters from the cursor position

Default key: Delete

=item delete_line LINE_ID, [ LINES = 1 ]

Removes LINES of text at LINE_ID.

=item delete_current_chunk 

Removes the chunk ( or line, if L<wordWrap> is 0 ) at the cursor.

Default key: Ctrl+Y

=item delete_chunk CHUNK_ID, [ CHUNKS = 1 ] 

Removes CHUNKS ( or lines, if L<wordWrap> is 0 ) of text at CHUNK_ID

=item delete_marker INDEX

Removes marker INDEX in L<markers> list.

=item delete_to_end

Removes text to the end of the chunk.

Default key: Ctrl+E

=item delete_text X, Y, TEXT_LENGTH

Removes TEXT_LENGTH characters at X,Y physical coordinates

=item draw_colorchunk CANVAS, TEXT, LINE_ID, X, Y, COLOR

Paints the syntax-highlighted chunk of TEXT, taken from LINE_ID line index, at
X, Y. COLOR is used if the syntax highlighting information contains C<cl::Fore>
as color reference.

=item end_block

Stops the block selection session.

=item end_undo_group

Closes bracket for group of actions, opened by C<begin_undo_group>.  

=item find SEARCH_STRING, [ X = 0, Y = 0, REPLACE_LINE = '', OPTIONS ]

Tries to find ( and, if REPLACE_LINE is defined, to replace with it ) 
SEARCH_STRING from (X,Y) physical coordinates. OPTIONS is an integer
that consists of the C<fdo::> constants; the same constants are used
in L<Prima::EditDialog>, which provides graphic interface to the find and replace
facilities of L<Prima::Edit>.

=over

=item fdo::MatchCase

If set, the search is case-sensitive.

=item fdo::WordsOnly

If set, SEARCH_STRING must constitute the whole word.

=item fdo::RegularExpression

If set, SEARCH_STRING is a regular expression.

=item fdo::BackwardSearch

If set, the search direction is backwards.

=item fdo::ReplacePrompt

Not used in the class, however, used in L<Prima::EditDialog>.

=back

=item get_chunk CHUNK_ID

Returns chunk of text, located at CHUNK_ID. 
Returns empty string if chunk is nonexistent.

=item get_chunk_end CHUNK_ID

Returns the index of chunk at CHUNK_ID, corresponding to the last chunk 
of same line.

=item get_chunk_org CHUNK_ID

Returns the index of chunk at CHUNK_ID, corresponding to the first chunk 
of same line.

=item get_chunk_width TEXT, FROM, LENGTH, [ RETURN_TEXT_PTR ]

Returns the width in pixels of C<substr( TEXT, FROM, LENGTH)>.
If FROM is larger than length of TEXT, TEXT is
padded with space characters. Tab character in TEXT replaced to L<tabIndent> times
space character. If RETURN_TEXT_PTR pointer is specified, the
converted TEXT is stored there.

=item get_line INDEX

Returns line of text, located at INDEX. 
Returns empty string if line is nonexistent.

=item get_line_dimension INDEX

Returns two integers, representing the line at INDEX in L<wordWrap> mode:
the first value is the corresponding chunk index, the second is how many
chunks represent the line.

See also: L<make_logical>.

=item get_line_ext CHUNK_ID

Returns the line, corresponding to the chunk index.

=item has_selection

Returns boolean value, indicating if the selection block is active.

=item insert_empty_line LINE_ID, [ REPEAT = 1 ]

Inserts REPEAT empty lines at LINE_ID.

=item insert_line LINE_ID, @TEXT

Inserts @TEXT strings at LINE_ID

=item insert_text TEXT, [ HIGHLIGHT = 0 ] 

Inserts TEXT at the cursor position. If HIGHLIGHT is set to 1,
the selection block is cancelled and the newly inserted text is selected.

=item lock_change BOOLEAN

Increments ( 1 ) or decrements ( 0 ) lock count. Used to defer change notification
in multi-change calls. When internal lock count hits zero, C<Change> notification is called. 

=item make_logical X, Y

Maps logical X,Y coordinates to the physical and returns the integer pair.
Returns same values when L<wordWrap> is 0.

=item make_physical X, Y

Maps physical X,Y coordinates to the logical and returns the integer pair.

Returns same values when L<wordWrap> is 0.

=item mark_horizontal

Starts block marking session with C<bt::Horizontal> block type.

Default key: Alt+L

=item mark_vertical

Starts block marking session with C<bt::Vertical> block type.

Default key: Alt+B

=item overtype_block

Copies the selected text and overwrites the text next to the cursor position, according to
the L<blockType> value.

Default key: Alt+O

=item paste

Copies text from the clipboard and inserts it in the cursor position.

Default key: Shift+Insert

=item realize_panning

Performs deferred widget panning, activated by setting C<{delayPanning}> to 1.
The deferred operations are those performed by L<offset> and L<topLine>.

=item redo

Re-applies changes, formerly rolled back by C<undo>.

=item set_line LINE_ID, TEXT, [ OPERATION, FROM, LENGTH ]

Changes line at LINE_ID to new TEXT. Hint scalars OPERATION, FROM and LENGTH
used to maintain selection and marking data. OPERATION is an arbitrary string,
the ones that are recognized are C<'overtype'>, C<'add'>, and C<'delete'>.
FROM and LENGTH define the range of the change; FROM is a character offset and
LENGTH is a length of changed text.

=item split_line

Splits a line in two at the cursor position.

Default key: Enter ( or Ctrl+Enter if L<wantReturns> is 0 )

=item select_all

Selects all text

=item start_block [ BLOCK_TYPE ]

Begins the block selection session. The block type if BLOCK_TYPE, if it is
specified, or L<blockType> property value otherwise.

=item undo

Rolls back changes into internal array, which size cannot extend C<undoLimit>
value. In case C<undoLimit> is 0, no undo actions can be made.

=item update_block

Adjusts the selection inside the block session, extending of shrinking it to 
the current cursor position.

=item word_left [ REPEAT = 1 ]

Moves cursor REPEAT words to the left.

=item word_right [ REPEAT = 1 ]

Moves cursor REPEAT words to the right.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::EditDialog>, L<Prima::IntUtils>, F<examples/editor.pl>

=cut 
