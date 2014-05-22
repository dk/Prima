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
#  Created by Vadim Belman <voland@plab.ku.dk>
#
#  $Id$

use strict;
use Prima::Const;
use Prima::Classes;
use Prima::ScrollBar;
use Prima::Utils;

# Terminal specific constants
package 
    tm;

use constant WordBegin      => 0x0000;
use constant WordEnd        => 0x0001;

package Prima::AbstractTerminal;
use vars qw( @ISA);
@ISA = qw( Prima::Widget);

{
	my %RNT = (
		%{Prima::Widget-> notification_types()},
		ExecCommand => nt::Action,
	);

	sub notification_types {
		return \%RNT;
	}
}

my $defaultAccelItems = [
	['Line begin' => 'Home' => kb::Home,  'Home'],
	['Line end' => 'End' => kb::End, 'End'],
	['Cursor left' => 'Left' => kb::Left, 'cursorLeft'],
	['Cursor right' => 'Right' => kb::Right, 'cursorRight'],
	['Cursor up' => 'Up' => kb::Up, 'cursorUp'],
	['Cursor down' => 'Down' => kb::Down, 'cursorDown'],
	['Cursor left by word' => 'Ctrl-Left' => ( km::Ctrl | kb::Left), 'wordLeft'],
	['Cursor right by word' => 'Ctrl-Right' => ( km::Ctrl | kb::Right), 'wordRight'],
	['Ins' => 'Ins' => kb::Insert, 'toggleInsMode'],
	['Delete char' => 'Del' => kb::Delete, 'deleteChar'],
	['Delete left char' => 'Backspace' => kb::Backspace, 'deleteLeftChar'],
	['Delete word right' => 'Alt-W' => '@W' => 'deleteWordRight'],
	['Delete word left' => 'Ctrl-Backspace' => ( km::Ctrl | kb::Backspace), 'deleteWordLeft'],
	['Delete word inplace' => 'Ctrl-Del' => ( km::Ctrl | kb::Delete), 'deleteWord'],
	['Delete up to line begin' => 'Ctrl-U' => '^U' => 'deleteToLineBegin'],
	['Delete up to line end' => 'Ctrl-K' => '^K' => 'deleteToLineEnd'],
	['Execute command' => 'Enter' => kb::Enter, 'enterPressed'],
	['Previous prompt' => 'Ctrl-Up' => ( km::Ctrl | kb::Up), 'previousPrompt'],
	['Next prompt' => 'Ctrl-Down' => ( km::Ctrl | kb::Down), 'nextPrompt'],
];

#\subsection{profile\_default}
sub profile_default {
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		borderWidth             =>         2,
		hScroll                 =>         0,
		vScroll                 =>         0,
		pointerType             =>  cr::Text,
		lineWrap                =>         0,
		prompt                  =>      '$ ',
		topItem                 =>     undef,
		bottomItem              =>     undef,
		vAlign                  =>     undef,

		items                   =>        [],  # Items array

		syncPaint               =>         0,

		accelItems              => $defaultAccelItems,

		autoAdjustPos           =>         1,
		insertMode              =>         1, # Insert or replace mode for typing.
		overrideMode            =>         1, # Insert a new item if inserting in the middle
															# of items array or override old one.
		reusePrompts            =>         1,
		mousePromptSelection    =>         1,
		wordRightMode           => tm::WordEnd,
		syncPromptChange        =>         0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

#\subsection{profile\_check\_in}
sub profile_check_in {
	my ( $self, $prf, $default) = @_;
	$self-> SUPER::profile_check_in( $prf, $default);
	#my @scrlWidth = Prima::Application->get_default_scrollbar_metrics();
	$prf-> { topItem} = undef if ! exists( $prf-> { topItem}) &&
											exists( $prf-> { bottomItem});
	$prf-> { borderWidth} = 0 unless $prf-> { borderWidth} >= 0;
}

#\subsection{calculate\_top\_bottom}
sub calculate_top_bottom {
	my $self = $_[0];

	if ( ( $self-> { vAlign} == ta::Bottom) ||
			( !defined( $self-> topItem) && defined( $self-> { bottomItem}))) {
		$self-> { bottomItem} = $#{ $self-> { items}} unless defined( $self-> { bottomItem});
		$self-> { yOffset} = $self-> { itemRow}-> [ $self-> { bottomItem}] +
			$self-> { itemHeights}-> [ $self-> { bottomItem}] - 1 - $self-> { textRows};

		$self-> { topItem} = $self-> row2Item( $self-> { yOffset});
		$self-> { vAlign} = ta::Bottom;
	}
	else {
		$self-> { topItem} = 0 unless defined( $self-> { topItem});
		$self-> { yOffset} = $self-> { itemRow}-> [ $self-> { topItem}];
		$self-> { bottomItem} = $self-> row2Item( $self-> { yOffset} + $self-> { textRows} - 1);
	}
}

#\subsection{calculate\_text\_preferences}
sub calculate_text_preferences {
	my $self = $_[0];

	$self-> { charHeight} = $self-> { termView}-> font-> { height};
	$self-> { charWidth} = $self-> { termView}-> font-> { width};

	my $textHeight = $self-> { termView}-> height;
	$self-> { textRows} = Prima::Utils::ceil( $textHeight / $self-> { charHeight});
	$self-> { nonIntRows} = ( $self-> { textRows} * $self-> { charHeight}) != $textHeight;
	my $textWidth = $self-> { termView}-> width;
	$self-> { textCols} = Prima::Utils::ceil( $textWidth / $self-> { charWidth} );
	$self-> { nonIntCols} = ( $self-> { textCols} * $self-> { charWidth}) != $textWidth;
	$self-> { vShift} = ( $self-> { textRows} - 1) * $self-> { charHeight};
}

#\subsection{row2Item}
sub row2Item {
	my ( $self, $row) = @_;
	return 0 if $row < 0;
	my $items = $self-> { items};
	return $#{ $items} if $row >= $self-> { totalRows};
	my $itemRow = $self-> { itemRow};
	my $itemHeights = $self-> { itemHeights};
	my ( $min, $max) = ( 0, $#{ $items});
	my $i;

	for (;;) {
		$i = int ( ( $min + $max) / 2);
		if ( $itemRow-> [ $i] < $row) {
			$min = $i;
		}
		else {
			$max = $i;
		}
		last if ( ( $itemRow-> [ $i] <= $row) && ( ( $itemRow-> [ $i] + $itemHeights-> [ $i]) > $row));
		if ( ( ( $max - $min)) == 1 && ( $i == int( ( $min + $max) / 2))) {
			$i++;
			last if ( $i == $#{ $items}) ||
				( ( $itemRow-> [ $i] <= $row) && ( ( $itemRow-> [ $i] + $itemHeights-> [ $i]) > $row));
		}
	}
	return $i;
}

#\subsection{trect2prect}
sub trect2prect {
	my ( $self, $xLeft, $yTop, $xRight, $yBottom) = @_;
	my ( $cW, $cH, $wH) = ( $self-> { charWidth},
									$self-> { charHeight},
									$self-> { termView}-> height,
								);
	my $textRows = $self-> { textRows};

	#print "Translating $xLeft,$yTop $xRight,$yBottom";
	return [
		$cW * $xLeft,
		( $textRows - $yBottom - 1) * $cH,
		$cW * ( $xRight + 1),
		( $textRows - $yTop) * $cH,
	];
}

#\subsection{prect2trect}
sub prect2trect {
	my ( $self, $xLeft, $yBottom, $xRight, $yTop) = @_;
	my ( $cW, $cH, $tH) =
		( $self-> { charWidth},
			$self-> { charHeight},
			$self-> { textRows} * $self-> { charHeight},
		);

	#print "wH: $wH, cH: $cH, yTop: $yTop";
	return (
		Prima::Utils::floor( $xLeft / $cW),
		Prima::Utils::floor( ( $tH - 1 - $yTop) / $cH),
		Prima::Utils::floor( $xRight / $cW),
		Prima::Utils::floor( ( $tH - 1 - $yBottom) / $cH),
		);
}

#\subsection{point2item}
sub pos2item {
	my ( $self, $x, $y) = @_;

	return -1 if ( $y < 0) || ( $y > ( $self-> height - 1));

	my ( $row) = $self-> { yOffset} + $self-> { textRows} - int( $y / $self-> { charHeight}) - 1;
	my ( $idx) = $self-> row2Item( $row);
	my ( $ilen) = length( $self-> { items}-> [ $idx]);
	my ( $cP) = int( $x / $self-> { charWidth}) + $self-> { xOffset} +
			( $row - $self-> { itemRow}-> [ $idx]) * $self-> { textCols};
	$cP = $ilen if $cP > $ilen;
	return ( $idx, $cP);
}

#\subsection{set\_view\_item\_pos}
sub set_view_item_pos {
	my ( $self, $i) = @_;
	my $item = $self-> { items}-> [ $i];
	my $itemRow = $self-> { itemRow}-> [ $i] - $self-> { yOffset};
	my $itemBottomRow = $itemRow + $self-> { itemHeights}-> [ $i];

	if ( ( $itemRow >= $self-> { textRows}) || ( $itemBottomRow < 0)) {
		#print "Hiding ", $item->name;
		$item-> hide;
		return;
	}

	#print "Setting view pos for $i ($self->{ items}->[ $i])";

	my ( $itemHeight) = $self-> { items}-> [ $i]-> height;
	$itemHeight ||= 1;
	my $itemBottomY = ( $self-> { textRows} - $itemBottomRow) * $self-> { charHeight} +
							int( ( ( $self-> { itemHeights}-> [ $i] * $self-> { charHeight}) %
									$itemHeight) / 2);
	my $itemLeftX = ( $self-> { itemXShifts}-> [ $i] - $self-> { xOffset} * $self-> { charWidth});

	#print "Setting view ", $self->{ items}->[ $i]->name, " to $itemLeftX,$itemBottomY";

	$item-> origin( $itemLeftX, $itemBottomY);
	$item-> show unless $item-> visible;
}

#\subsection{VScroll\_Change}
sub VScroll_Change {
	my ( $self, $scr) = @_;
	return if $self-> { ignoreScrollChanges};
	$self-> set_offsets( undef, $scr-> value);
}

#\subsection{HScroll\_Change}
sub HScroll_Change {
	my ( $self, $scr) = @_;
	return if $self-> { ignoreScrollChanges};
	$self-> set_offsets( $scr-> value, undef);
}

#\subsection{init}
sub init {
	my $self = shift;

	$self-> { IAmInitializing} = 1; # Could be changed to alive method!!!

	for ( qw( charHeight charWidth cursorPos textRows textCols vScroll hScroll
		totalRows rightBorder leftBorder borderWidth autoAdjustPos
		prompt vAlign lineWrap xOffset yOffset xOffsetOld yOffsetOld
		ignoreScrollChanges vShift maxWidth clientResize clientWidth
		IAmResetting topItem insertMode overrideMode wordRightMode
		syncPromptChange reusePrompts mousePromptSelection
	)) {
		$self-> { $_} = 0;
	}
	for ( qw( itemsChanged)) {
		$self-> { $_} = 1;
	}
	for ( qw( viewItems itemHeights itemWidths itemRow prompts)) {
		$self-> { $_} = [];
	}

	my %profile = $self-> SUPER::init( @_);

	$self-> { items} = $profile{ items} if defined( $profile{ items});
	@{ $self-> { prompts}} = map( 0, 0 .. $#{ $self-> { items}});

	for ( qw( borderWidth hScroll vScroll lineWrap bottomItem topItem vAlign
		prompt autoAdjustPos insertMode overrideMode wordRightMode
		syncPromptChange reusePrompts mousePromptSelection)
	) {
		$self-> $_( $profile{ $_});
	}
	$self-> add_empty_command_line;

	my ( $popup, $popupItems) = ( $self-> { popup}, $self-> { popupItems});
	$self-> { popup} = undef;
	$self-> { termView} =
	$self-> insert( 'Widget',
		name => 'TermView',
		rect => [
			$self-> { borderWidth},
			$self-> { borderWidth} + ( $self-> { hScroll} ? $self-> { hScrollBar}-> height : 0),
			$self-> width - $self-> { borderWidth} - ( $self-> { vScroll} ? $self-> { vScrollBar}-> width : 0),
			$self-> height - $self-> { borderWidth},
		],
		syncPaint => 0,
		growMode => gm::Client,
		cursorVisible => 1,
		selectable => 1,
		ownerColor => 1,
		ownerBackColor => 1,
	);

	$self-> { updateTimer} =
	$self-> insert( 'Timer',
		owner => $self,
		name => 'UpdateTimer',
		timeout => 500,
	);
	$self-> { updateTimer}-> stop;

	$self-> { charsPrinted} = $self-> { linesPrinted} = 0; # puts uses this counters.

	$self-> { __DYNAS__}-> { onExecCommand} = $profile{ onExecCommand};

	$self-> { IAmInitializing} = 0; # This must be placed only and only just before reset call;
	$self-> reset_view_items;
	$self-> reset;
	$self-> set_cursor_shape;

	return %profile;
}

#\subsection{set}
sub set {
	my ( $self, %profile) = @_;
	$self-> { __DYNAS__}-> { onExecCommand} = $profile{ onExecCommand},
		delete $profile{ onExecCommand} if defined( $profile{ onExecCommand});
	$self-> SUPER::set( %profile);
}

#sub reset {
#    use Benchmark;
#    my $self = $_[0];
#    my $t = timeit(1, sub {$self->_reset}); print "Reset: ", timestr($t);
#}

#\subsection{reset}
sub reset {
	my ( $self) = @_;
	my $i;
	my ( $topView, $bottomView) = ( $self-> { topItem}, $self-> { bottomItem});

	return if $self-> { IAmInitializing} || $self-> { IAmResetting};
	$self-> { IAmResetting} = 1;

	#print "resetting";

	if ( $self-> { termView}-> font-> pitch != fp::Fixed) {
		$self-> { termView}-> font-> pitch( fp::Fixed);
		return;
	}

	$self-> { vAlign} = ( defined $self-> { topItem} ? ta::Top : ta::Bottom)
		unless defined( $self-> { vAlign});

	$self-> calculate_text_preferences;

	if ( $self-> { clientResize}) {
		my @tmvrect = (
			$self-> { borderWidth},
			$self-> { borderWidth} + ( $self-> { hScroll} ? $self-> { hScrollBar}-> height : 0),
			$self-> width - $self-> { borderWidth} - ( $self-> { vScroll} ? $self-> { vScrollBar}-> width : 0),
			$self-> height - $self-> { borderWidth},
		);
		#print "tmvrect : @tmvrect\n";
		$self-> { termView}-> rect( @tmvrect);
		if ( $self-> { hScroll}) {
				$self-> HScroll-> origin( $self-> { borderWidth}, $self-> { borderWidth});
				$self-> HScroll-> width( $self-> { termView}-> width + 1);
		}
		if ( $self-> { vScroll}) {
				$self-> VScroll-> origin( $tmvrect[2], $tmvrect[1]);
				$self-> VScroll-> height( $self-> { termView}-> height);
		}
		$self-> insert_bone;
		$self-> { clientResize} = 0;
	}

	if ( ( ! defined( $self-> { oldWidth})) || ( $self-> { oldWidth} != $self-> { width})) {
		$#{ $self-> { itemXRight}} = $#{ $self-> { itemXLeft}} = $#{ $self-> { itemXShifts}} =
		$#{ $self-> { itemRow}} = $#{ $self-> { itemWidths}} = $#{ $self-> { itemHeights}} = $#{ $self-> { items}};
		$self-> { totalRows} = 0;
		$self-> { rightBorder} = 0;
		$self-> { leftBorder} = 0;
		$self-> { maxWidth} = 0;
		my ( $width, $item);
		my ( $items, $itemsChanged, $itemWidths, $itemXRight, $itemXLeft, $itemHeights, $itemXShifts, $itemRow) =
			( $self-> { items}, $self-> { itemsChanged}, $self-> { itemWidths}, $self-> { itemXRight}, $self-> { itemXLeft},
				$self-> { itemHeights}, $self-> { itemXShifts}, $self-> { itemRow});
		my ( $cW, $cH, $lineWrap, $textCols, $focused) =
			( $self-> { charWidth}, $self-> { charHeight}, $self-> { lineWrap}, $self-> { textCols}, $self-> { focused});
		my ( $rightBorder, $leftBorder, $maxWidth, $totalRows) =
			( $self-> { rightBorder}, $self-> { leftBorder}, $self-> { maxWidth}, $self-> { totalRows});
		my ( $itemXShift, $iXRight, $iXLeft);
		my $itemLastN = $#{ $self-> { items}};
		if ( $lineWrap) {
			for ( $i = 0; $i <= $itemLastN; $i++) {

				$item = $items-> [ $i];
				$itemXShift = $itemXShifts-> [ $i];
				if ( ref $item) {
					$itemWidths-> [ $i] = Prima::Utils::ceil( ( $item-> width + $itemXShift) / $cW);
					$itemXRight-> [ $i] = Prima::Utils::floor( $item-> right / $cW);
					$itemXLeft-> [ $i] = Prima::Utils::floor( $itemXShift / $cW);
					$itemHeights-> [ $i] = Prima::Utils::ceil( $item-> height / $cH);
					$itemHeights-> [ $i] ||= 1;
				}
				else {
					$width = length( $item) + ( $i == $focused ? 1 : 0);
					$itemWidths-> [ $i] = ( $width > $textCols) ? $textCols : $width;
					$itemXRight-> [ $i] = $itemWidths-> [ $i] - 1;
					$itemXLeft-> [ $i] = 0;
					$itemHeights-> [ $i] = Prima::Utils::ceil( $width / $textCols);
				}

				$itemHeights-> [ $i] ||= 1; # to avoid zero heights

				$iXRight = $itemXRight-> [ $i];
				$iXLeft = $itemXLeft-> [ $i];

				$rightBorder = $iXRight if $iXRight > $rightBorder;
				$leftBorder = $iXLeft if $iXLeft < $leftBorder;
				$width = $rightBorder - $leftBorder + 1;
				$maxWidth = $width if $width > $maxWidth;
				$itemRow-> [ $i] = $totalRows;
				$totalRows += $itemHeights-> [ $i];
			}
		}
		else {
			for ( $i = 0; $i <= $itemLastN; $i++) {

				$item = $items-> [ $i];
				$itemXShift = $itemXShifts-> [ $i];
				if ( ref $item) {
					$itemWidths-> [ $i] = Prima::Utils::ceil( ( $item-> width + $itemXShift) / $cW);
					$itemXRight-> [ $i] = Prima::Utils::floor( $item-> right / $cW);
					$itemXLeft-> [ $i] = Prima::Utils::floor( $itemXShift / $cW);
					$itemHeights-> [ $i] = Prima::Utils::ceil( $item-> height / $cH);
					$itemHeights-> [ $i] ||= 1; # to avoid zero heights
				}
				else {
					$width = length( $item) + ( $i == $focused ? 1 : 0);
					$itemWidths-> [ $i] = $width;
					$itemXRight-> [ $i] = $itemWidths-> [ $i] - 1;
					$itemXLeft-> [ $i] = 0;
					$itemHeights-> [ $i] = 1;
				}

				$iXRight = $itemXRight-> [ $i];
				$iXLeft = $itemXLeft-> [ $i];

				$rightBorder = $iXRight if $iXRight > $rightBorder;
				$leftBorder = $iXLeft if $iXLeft < $leftBorder;
				$width = $rightBorder - $leftBorder + 1;
				$maxWidth = $width if $width > $maxWidth;
				$itemRow-> [ $i] = $totalRows;
				$totalRows += $itemHeights-> [ $i];
			}
		}
		$self-> { rightBorder} = $rightBorder;
		$self-> { leftBorder} = $leftBorder;
		$self-> { maxWidth} = $maxWidth;
		$self-> { totalRows} = $totalRows;
		$self-> calculate_top_bottom;

		$self-> { oldWidth} = $self-> { width};
	}

	$self-> { xOffset} = $self-> { rightBorder} - $self-> { textCols} + 1
		if ( $self-> { xOffset} + $self-> { textCols}) > $self-> { rightBorder};
	$self-> { xOffset} = $self-> { leftBorder} if $self-> { xOffset} < $self-> { leftBorder};
	$self-> { yOffset} = ( $self-> { nonIntRows} ? -1 : 0) if $self-> { yOffset} == 0;

	for ( $i = 0; $i <= $#{ $self-> { viewItems}}; $i++ ) {
		$self-> set_view_item_pos( $self-> { viewItems}-> [ $i]);
	}

	$self-> reset_scrolls;
	$self-> reset_cursor;

	$self-> { termView}-> focus unless $self-> { termView}-> focused;

	$self-> { IAmResetting} = 0;
	#$self->repaint;
}

#\subsection{reset\_scrolls}
sub reset_scrolls {
	my $self = shift;

	return if $self-> { IAmInitializing} || $self-> { ignoreScrollChanges};

	$self-> { ignoreScrollChanges} = 1;

	my $maxWidth = $self-> { maxWidth};

	my $hpartial = $self-> { textCols} < $maxWidth ? $self-> { textCols} : $maxWidth;
	my $hmax = $self-> { rightBorder} - $self-> { textCols} + 1;
	my $hmin = $self-> { leftBorder};
	$hmax = $hmin if $hmax < $hmin;
	my $hval = $self-> { xOffset} > $hmax ? $hmax : ( $self-> { xOffset} < $hmin ? $hmin : $self-> { xOffset}) ;
	my $vval = $self-> { yOffset};
	my $vmax = $self-> { totalRows} - $self-> { textRows};
	$vmax = 0 if $vmax < 0;
	my $vmin = $self-> { nonIntRows} ?
		( $self-> { totalRows} < $self-> { textRows} ?
			( $self-> { vAlign} == ta::Bottom ?
				0 :
				$vval) :
			-1) :
		0;

	#print "vval : $vval";

	$self-> { hScrollBar}-> set(
		min      => $hmin,
		max      => $hmax,
		step     => 1,
		pageStep => Prima::Utils::ceil( $self-> { textCols} / 3),
		whole    => $maxWidth,
		partial  => $hpartial,
		value    => $hval,
	) if $self-> { hScroll};

	#print "vScrollBar: min: $vmin, max: $vmax, whole: ", $self->{ totalRows} - $vmin, ", value: $vval";
	$self-> { vScrollBar}-> set(
		min      => $vmin,
		max      => $vmax,
		step     => 1,
		pageStep => $self-> { textRows},
		whole    => $self-> { totalRows} - $vmin,
		partial  => $self-> { textRows} < $self-> { totalRows} ? 
			$self-> { textRows} : $self-> { totalRows},
		value    => $vval,
	) if $self-> { vScroll};

	$self-> { ignoreScrollChanges} = 0;
}

#\subsection{buildinItem}
sub buildinItem {
	my ( $self, $item) = @_;

	if ( ! defined( $item-> { __DYNAS__}-> { onSize}) || $item-> { __DYNAS__}-> { onSize} != \&itemOnSize) {
		$item-> { termView}-> { onSize} = $item-> { __DYNAS__}-> { onSize};
		$item-> set( onSize => \&itemOnSize);
	}

	if ( ! defined( $item-> { __DYNAS__}-> { onMove}) || $item-> { __DYNAS__}-> { onMove} != \&itemOnMove) {
		$item-> { termView}-> { onMove} = $item-> { __DYNAS__}-> { onMove};
		$item-> set( onMove => \&itemOnMove);
	}

	$item-> notify( 'TerminalInsert', $self);
}

#\subsection{insert\_view}
sub insert_view {
	my ( $self, $item, $i) = @_;
	if ( ref( $item) eq 'ARRAY') {
		my ( $view);
		$view = $self-> { termView}-> insert( @{ $item});
		( print "Item creation failed for $item->[ 0]\n", return) if ! $view;
		$item = $view;
	}
	elsif ( $item-> isa('Prima::Widget')) {
		$item-> owner( $self-> TermView);
	}
	$self-> buildinItem( $item);
	if ( ! $item-> isa( 'Prima::Widget')) {
		croak( "Item #$i not a Widget descendant") unless $item-> isa( 'Prima::Widget');
	}
	if ( defined( $i)) {
		$self-> { items}-> [ $i] = $item;
		$self-> { itemXShifts}-> [ $i] = $item-> left;
	}
	return $item;
}

#\subsection{reset\_view\_items}
sub reset_view_items {
	my ( $self) = @_;
	my $i;

	$#{ $self-> { itemXShifts}} = $#{ $self-> { items}};

	for ( $i=0; $i <= $#{ $self-> { items}}; $i++) {
		my $item = $self-> { items}-> [ $i];
		if ( ref $item) {
			$item = $self-> insert_view( $item, $i);
			push @{ $self-> { viewItems}}, $i;
		}
		else {
			$self-> { itemXShifts}-> [ $i] = 0;
		}
	}
}

#\subsection{set\_cursor\_shape}
sub set_cursor_shape {
	my ( $self) = @_;
	return unless ( ! $self-> { IAmInitializing});
	$self-> { termView}-> cursorSize( $self-> { charWidth}, $self-> { insertMode} ? 2 : $self-> { charHeight});
}

#\subsection{update\_cursor}
sub reset_cursor {
	my ( $self) = @_;
	my ( $cursorCol, $cursorRow) = $self-> get_cursor_coordinates;
	if ( ( $cursorRow >= $self-> { textRows}) ||
			( $cursorCol >= $self-> { textCols}) ||
			( $cursorRow < 0) || ( $cursorCol < 0)) {
		$self-> { termView}-> cursorVisible(0);
	}
	else {
		#print "setting cursor to ",$self->{ charWidth} * $cursorCol, "x", $self->{ charHeight} * ( $self->{ textRows} - $cursorRow - 1);
		$self-> { termView}-> cursorVisible(1);
		$self-> { termView}-> set_cursor_pos( $self-> { charWidth} * $cursorCol, $self->{ charHeight} * ( $self->{textRows} - $cursorRow - 1));
	}
}

#\subsection{adjust\_terminal\_window}
sub adjust_terminal_window {
	my ( $self, $forced) = @_;

	return unless ( ! ( $self-> { inCommandExecution} && ! $forced)) || $self-> { autoAdjustPos};

	my ( $cPos, $focused, $newXOffset, $newYOffset, $textRows, $textCols) =
		( $self-> { cursorPos}, $self-> { focused}, $self-> { xOffset}, $self-> { yOffset},
			$self-> { textRows}, $self-> { textCols});
	my ( $cursorCol, $cursorRow) = $self-> get_cursor_coordinates;

	if ( $cursorCol < 0) {
		$newXOffset += $cursorCol;
	}
	elsif ( $cursorCol >= $textCols) {
		$newXOffset += $cursorCol - $textCols + 1;
	}

	if ( $cursorRow < 0) {
		$newYOffset += $cursorRow;
	}
	elsif ( $cursorRow >= $textRows) {
		$newYOffset += $cursorRow - $textRows + 1;
	}

	#print "Setting x,y to ", $newX || 'undef', ",", $newY || 'undef';
	if ( ( $newXOffset != $self-> { xOffset}) || ( $newYOffset != $self-> { yOffset})) {
		$self-> set_offsets( $newXOffset, $newYOffset);
		$self-> reset_scrolls;
	}
}

#\subsection{update\_cursor}
sub update_cursor {
	my ( $self, $forced) = @_;
	my ( $cursorCol, $cursorRow) = $self-> get_cursor_coordinates;
	if ( ( ( $cursorRow >= $self-> { textRows}) ||
			( $cursorCol >= $self-> { textCols}) ||
			( $cursorRow < 0) ||
			( $cursorCol < 0)
			)
		) {
		$self-> adjust_terminal_window( $forced);
	}
	else {
		$self-> reset_cursor;
	}
}

#\subsection{set\_offsets}
sub set_offsets {
	my ( $self, $newX, $newY) = @_;
	return unless ! $self-> { IAmInitializing};
	my ( $xOffset, $yOffset, $textRows, $textCols) =
		( $self-> { xOffset}, $self-> { yOffset}, $self-> { textRows}, $self-> { textCols});
	my ( $hRectTop, $hRectBottom);  # Top and Bottom of the rectange which should be
												# redrawn for a horizontal shift.
	my ( $hs, $vs) = ( 0, 0);
	my ( @rect2Scroll) = ( 0, 0, $self-> { termView}-> width, $self-> { termView}-> height);
	my ( $topView, $bottomView) = ( $self-> { topItem}, $self-> { bottomItem});
	my $i;
	my $yOffsetLimit = $self-> { totalRows} < $self-> { textRows} ? 0 : ( $self-> { totalRows} - $self-> { textRows});

	#print "Setting offsets to $newX,$newY (1)";
	$newX = defined( $newX) ?
		( $newX < $self-> { leftBorder} ?
			$self-> { leftBorder} :
			( ( $newX + $self-> { textCols} - 1) > $self-> { rightBorder} ?
				$self-> { rightBorder} - $self-> { textCols} :
				$newX)) :
		$self-> { xOffset};
	$newY = defined( $newY) ? ( $newY > $yOffsetLimit ? $yOffsetLimit : $newY) : $self-> { yOffset};
	#print "Setting offsets to $newX,$newY (2)";

	$self-> { yOffsetOld} = $yOffset;
	$self-> { xOffsetOld} = $xOffset;

	if ( defined( $newY) && ( $newY != $yOffset)) {
		$vs = ( $newY - $yOffset) * $self-> { charHeight};
		$self-> { yOffset} = $newY;
		$topView = $self-> { topItem} if ( $self-> { topItem} = $self-> row2Item( $newY)) < $topView;
		$bottomView = $self-> { bottomItem}
			if ( $self-> { bottomItem} = $self-> row2Item( $newY + $self-> { textRows} - 1)) > $bottomView;
	}

	if ( defined( $newX) && ( $newX != $xOffset)) {
		$hs = ( $xOffset - $newX) * $self-> { charWidth};
		$self-> { xOffset} = $newX;
	}

	$self-> { ignoreViewsMove} = 1;
	$self-> { termView}-> scroll( $hs, $vs, withChildren => 1);
	$self-> { termView}-> update_view if ! $self-> syncPaint;
	$self-> { ignoreViewsMove} = 0;

	if ( $self-> { yOffset} != $self-> { yOffsetOld}) {
		my $items = $self-> { items};
		my $visibleTop = $self-> { yOffset};
		my $visibleBottom = $visibleTop + $self-> { textRows} - 1;
		my $hiddenTop = $self-> { yOffsetOld} < $visibleTop ? $self-> { yOffsetOld} : ( $visibleBottom + 1);
		my $hiddenBottom = $hiddenTop + $self-> { textRows} - 1;
		$hiddenBottom = $visibleTop - 1 if $hiddenBottom < $visibleBottom;

		my $visItemTop = $self-> row2Item( $visibleTop);
		my $visItemBottom = $self-> row2Item( $visibleBottom);
		my $hidItemTop = $self-> row2Item( $hiddenTop);
		my $hidItemBottom = $self-> row2Item( $hiddenBottom);
		if ( $hidItemTop == $visItemBottom) {
			$hidItemTop++;
		}
		elsif ( $hidItemBottom == $visItemTop) {
			$hidItemBottom--;
		}

		#print "Hiding from $hidItemTop to $hidItemBottom ($hiddenTop - $hiddenBottom)";
		#print "Show from $visItemTop to $visItemBottom ($visibleTop - $visibleBottom)";

		for ( $i=$hidItemTop; $i<=$hidItemBottom; $i++) {
			$self-> set_view_item_pos( $i) if ref $items-> [ $i];
		}
		for ( $i=$visItemTop; $i<=$visItemBottom; $i++) {
			$self-> set_view_item_pos( $i) if ref $items-> [ $i];
		}
	}

	$self-> reset_cursor;

	if ( $self-> { vScroll}) {
		$self-> { vScrollBar}-> value( $newY);
	}
	if ( $self-> { hScroll}) {
		$self-> { hScrollBar}-> value( $newX);
	}
}

#\subsection{drawItem}
sub drawItem {
	my ( $self, $term, $item) = @_;
	my $itemRow = $self-> { itemRow}-> [ $item] - $self-> { yOffset};
	my $vShift = $self-> { vShift};
	my $itemY = $vShift - $itemRow * $self-> { charHeight};
	my ( $xLeft, $xRight) = @{ $self-> { updateCols}};
	my $xStrOffset = $self-> { xOffset} + $xLeft;
	my $xOutOffset = $xStrOffset < 0 ? ( $xLeft - $self-> { xOffset}) : ( $xLeft);
	$xOutOffset = 0 if $xOutOffset < 0;
	$xStrOffset = 0 if $xStrOffset < 0;

	if ( ! ref $self-> { items}-> [ $item]) {
		if ( $self-> { lineWrap}) {
			my $i;

			for ( $i = 0; $i < $self-> { itemHeights}-> [ $item] && $itemY >= 0; $i++) {
				my $outLine = substr( $self-> { items}-> [ $item], $i*$self-> { textCols}, $self-> { textCols});
				#print "updating cols $xLeft,$xRight for $item, row $itemRow, y:$itemY, xOffset:$self->{ xOffset}";
				$term-> text_out( substr( $outLine, $xStrOffset, $xRight - $xLeft + 1),
							$xOutOffset * $self-> { charWidth}, $itemY)
					if $xStrOffset <= length( $outLine);
				$itemY -= $self-> { charHeight};
				$itemRow++;
			}
		}
		else {
			#print "updating cols $xLeft,$xRight for $item, row $itemRow, y:$itemY, xOffset:$xOffset at ",$xLeft * $self->{ charWidth} if $xOffset <= length( $self->{ items}->[ $item]);
			$term-> text_out( substr( $self-> { items}-> [ $item], $xStrOffset, $xRight - $xLeft + 1),
						$xOutOffset * $self-> { charWidth}, $itemY)
				if $xStrOffset <= length( $self-> { items}-> [ $item]);
		}
	}
}

#\subsection{TermView\_Size}
sub TermView_Size {
	#print "TermView_Size->reset";
	$_[0]-> reset;
}

#\subsection{TermView\_Popup}
sub TermView_Popup {
	my ( $self, $term, $mouseDriven, $x, $y) = @_;
	my $popup = $self-> popup;
	if ( $popup && $popup-> autoPopup) {
		$self-> { mousePos} = $mouseDriven ? [ $x, $y] : undef;
		$popup-> popup( $x, $y);
		$term-> clear_event;
	}
}

#\subsection{TermView\_FontChanged}
sub TermView_FontChanged {
	my ( $self, $term) = @_;
	#$self-> SUPER::on_fontchanged( @_);

	if ( $term-> font-> pitch == fp::Fixed) {
		#print "TermView_FontChanged->reset";
		$self-> reset;
		$self-> set_cursor_shape;
	}
	else {
		$term-> font-> pitch( fp::Fixed);
	}
}

#\subsection{TermView\_Paint}
sub TermView_Paint {
	my ( $self, $term) = @_;

	my $clr = $term-> color;

	my @clipRect = $term-> clipRect;
	my @textClip = $self-> prect2trect( @clipRect);
	#print "Clipping ", ($term->size)[0], "x", ($term->size)[1]," by @clipRect, @textClip, $self->{ textCols} x $self->{ textRows}, cH: ",$self->{ charHeight};

	my $topItem = $self-> row2Item( $textClip[1] + $self-> { yOffset});
	my $bottomItem = $self-> row2Item( $textClip[3] + $self-> { yOffset});

	$term-> color( $term-> backColor);
	$term-> bar( @clipRect);
	$term-> color( $clr);

	my $i;

	#print "from $topItem to $bottomItem (rows: $self->{ textRows})";

	$self-> { updateCols} = [ $textClip[0], $textClip[2]];

	for ( $i=$topItem; $i<=$bottomItem; $i++) {
		$self-> drawItem( $term, $i);
	}

	#$term->color( cl::Green);
	#$term->rectangle( @clipRect);
	#$term->color( $clr);

	$self-> { updateCols} = $self-> { updateLines} = undef;

	@textClip = @clipRect = undef;
}

#\subsection{TermView\_KeyDown}
sub TermView_KeyDown {
	my ( $self, $term, $code, $key, $mod) = @_;
	if  ( (( $code & 0xFF) >= ord(' ')) &&
		( ( $mod  & ( km::Alt | km::Ctrl)) == 0) &&
		( ( $key == kb::NoKey) || ( $key == kb::Space))
		) {
		$self-> put_str( chr( $code & 0xFF));
		$term-> clear_event();
	}
}

#\subsection{TermView\_MouseClick}
sub TermView_MouseClick {
	my ( $self, $term, $btn, $shft, $x, $y) = @_;

	if ( ( $btn == mb::Left) && $self-> { mousePromptSelection}) {
		my ( $i, $cursorPos) = $self-> pos2item( $x, $y);
		if ( $self-> { prompts}-> [ $i] > 0) {
			$self-> { focused} = $i;
			$self-> { cursorPos} = $cursorPos < $self-> { prompts}-> [ $i] ?
				$self-> { prompts}-> [ $i] :
				$cursorPos;
			$self-> update_cursor;
		}
	}
}

#\subsection{UpdateTimer\_Tick}
sub UpdateTimer_Tick {
	my ( $self, $timer) = @_;

	$self-> adjust_terminal_window( 1);
}

#\subsection{on\_paint}
sub on_paint {
	my ( $self, $canvas) = @_;
	$self-> draw_border( $canvas, $self-> backColor, $self-> size );
}

#\subsection{on\_postmessage}
sub on_postmessage {
	my ( $self, $cmd, $data) = @_;

	if ( $cmd eq 'viewChanged') {
		$self-> viewChanged( $data);
	}
}

#\subsection{enterPressed}
sub enterPressed {
	my ( $self) = @_;
	my ( $commandString) = substr( $self-> { items}-> [ $self-> { focused}], $self-> { prompts}-> [ $self-> { focused}]);
	my ( $cursorCol, $cursorRow) = $self-> get_cursor_coordinates;
	if ( $cursorRow == ( $self-> { textRows} - 1)) {
		$self-> { totalRows}++;
		$self-> set_offsets( undef, $self-> { yOffset} + 2);
		$self-> { totalRows}--;
	}
	$self-> { ignoreScrollChanges} = 1;
	$self-> { termView}-> hide_cursor;
	$self-> { updateTimer}-> start;
	$self-> { charsPrinted} = $self-> { linesPrinted} = 0;
	$self-> { focused}++;
	$self-> { cursorPos} = 0;
	$self-> { commandExecutionStarted} = $self-> { inCommandExecution} = 1;
	$self-> notify( 'ExecCommand', $commandString);
	$self-> { commandExecutionStarted} = $self-> { inCommandExecution} = 0;
	$self-> { updateTimer}-> stop;
	$self-> { termView}-> show_cursor;
	$self-> { ignoreScrollChanges} = 0;
	$self-> set_offsets( 0, undef);
	$self-> add_empty_command_line;
}

#\subsection{add\_empty\_command\_line}
sub add_empty_command_line {
	my $self = $_[0];

	my ( $idx);

	if ( $self-> { IAmInitializing}) {
		push @{$self-> { items}}, $self-> { prompt};
		$idx = $#{$self-> { items}};
		$self-> { itemHeights}-> [ $idx] = 1;
		$self-> { cursorPos} = $self-> { itemWidths}-> [ $idx] =
		$self-> { prompts}-> [ $idx] = length( $self-> { prompt});
		$self-> { focused} = $idx;
	}
	else {
		my $focused = $self-> { focused};
		while ( $focused <= $#{ $self-> { items}} && $self-> { prompts}-> [ $focused] == 0) {
				$focused++;
		}
		$self-> { focused} = $focused;
		if ( ( $focused > $#{ $self-> { items}}) || ( ! $self-> { reusePrompts})) {
			$self-> insert_item( '', $self-> { prompt});
		}
		else {
			$self-> prompt( $self-> { prompt}) if $self-> { syncPromptChange};
		}
		$self-> { cursorPos} = $self-> { prompts}-> [ $self-> { focused}];
		$self-> update_cursor;
	}
}

#\subsection{insert\_bone}
sub insert_bone
{
	my $self = $_[0];
	my $bw   = $self-> {borderWidth};
	$self-> {bone}-> destroy if defined $self-> {bone};
	return unless $self-> { hScroll} && $self-> { vScroll};
	$self-> {bone} = $self-> insert( q(Widget),
		name   => q(Bone),
		origin => [ $bw + $self-> { hScrollBar}-> width, $bw],
		size   => [ $self-> {vScrollBar}-> width, $self-> {hScrollBar}-> height],
		ownerBackColor => 1,
		growMode  => gm::GrowLoX,
		pointerType=> cr::Arrow,
		onPaint   => sub {
			my ( $self, $owner, $w, $h) = ($_[0], $_[0]-> owner, $_[0]-> size);
			$self-> color( $self-> backColor);
			$self-> bar( 0, 1, $w - 2, $h - 1);
			$self-> color( $owner-> light3DColor);
			$self-> line( 0, 0, $w - 1, 0);
			$self-> line( $w - 1, 0, $w - 1, $h - 1);
		},
	);
}

#\subsection{set\_h\_scroll}
sub set_h_scroll
{
	my ( $self, $hs) = @_;
	return if $hs == $self-> {hScroll};
	my $bw = $self-> {borderWidth};
	if ( $self-> {hScroll} = $hs) {
		$self-> {hScrollBar} = $self-> insert( q(ScrollBar),
			name     => q(HScroll),
			vertical => 0,
			origin   => [ $bw, $bw],
			growMode => gm::GrowHiX,
			width    => $self-> width - 2 * $bw - ( $self-> {vScroll} ? $self-> {vScrollBar}-> width : 0),
			pointerType => cr::Arrow,
			firstClick => 1,
		);
	} else {
		$self-> {hScrollBar}-> destroy;
	}
	$self-> { clientResize} = 1;
	$self-> reset;
	$self-> repaint;
}

#\subsection{set\_v\_scroll}
sub set_v_scroll
{
	my ( $self, $vs) = @_;
	return if $vs == $self-> {vScroll};
	my $bw = $self-> {borderWidth};
	my @size = $self-> size;
	if ( $self-> {vScroll} = $vs) {
		$self-> {vScrollBar} = $self-> insert( q(ScrollBar),
			name     => q(VScroll),
			vertical => 1,
			left     => $size[0] - $bw - $Prima::ScrollBar::stdMetrics[0],
			top      => $size[1] - $bw,
			bottom   => $bw + ( $self-> {hScroll} ? $self-> {hScrollBar}-> height : 0),
			growMode => gm::GrowLoX | gm::GrowHiY,
			pointerType => cr::Arrow,
			firstClick => 1,
		);
	} else {
		$self-> { vScrollBar}-> destroy;
	}
	$self-> { clientResize} = 1;
	$self-> reset;
	$self-> repaint;
}

#\subsection{set\_border\_width}
sub set_border_width {
	my ( $self, $bw) = @_;

	return unless defined( $bw) && ( $bw != $self-> { borderWidth});

	$self-> { borderWidth} = $bw;
	$self-> { clientResize} = 1 unless $self-> { IAmInitializing};

	#print "set_border_width->reset";
	$self-> reset;
	$self-> repaint;
}

#\subsection{set\_top\_item}
sub set_top_item {
	my ( $self, $ti) = @_;

	if ( defined( $ti)) {
		$self-> { vAlign} = ta::Top;
		if ( $self-> { IAmInitializing}) {
			$self-> { topItem} = $ti;
		}
		else {
			$self-> set_offsets( undef, $self-> { itemRow}-> [ $ti]) unless $ti > $#{ $self-> { itemRow}};
		}
	}
}

#\subsection{set\_bottom\_item}
sub set_bottom_item {
	my ( $self, $bi) = @_;

	if ( defined( $bi)) {
		$self-> { vAlign} = ta::Bottom;
		if ( $self-> { IAmInitializing}) {
			$self-> { bottomItem} = $bi;
		}
		elsif ( $bi <= $#{ $self-> { itemRow}}) {
			my $brow = $self-> { itemRow}-> [ $bi] + $self-> { itemHeights}-> [ $bi] - $self-> { textRows};
			$self-> set_offsets( undef, $brow > 0 ? $brow : 0);
		}
	}
}

#\subsection{set\_alignment}
sub set_alignment {
	my ( $self, $al) = @_;

	return unless defined( $al) && ( $al != $self-> { vAlign});

	$self-> { vAlign} = $al;

	if ( $al == ta::Top) {
		my $newY = $self-> { itemRow}-> [ $self-> { topItem}];
		$newY ||= -1 if $self-> { nonIntRows};
		#print "Setting yOffset into ", $newY;
		$self-> set_offsets( undef, $newY);
	}
	elsif ( $al == ta::Bottom) {
		#print "Setting yOffset into ", $self->{ itemRow}->[ $self->{ bottomItem}] + $self->{ itemHeights}->[ $self->{ bottomItem}] - $self->{ textRows};
		$self-> set_offsets( undef,
			$self-> { itemRow}-> [ $self-> { bottomItem}] +
			$self-> { itemHeights}-> [ $self-> { bottomItem}] -
			$self-> { textRows}
		);
	}
	#$self->reset;
}

#\subsection{set\_line\_wrapping}
sub set_line_wrapping {
	my ( $self, $lw) = @_;

	return unless defined( $lw) && ( $lw != $self-> { lineWrap});

	$self-> { lineWrap} = $lw;
	$self-> { xOffsetOld} = $self-> { xOffset} = 0;
	#print "set_line_wrapping->reset";
	$self-> reset;
	$self-> repaint;
}

#\subsection{set\_prompt}
sub set_prompt {
	my ( $self, $prompt) = @_;

	return unless defined( $prompt);

	my ( $focused) = $self-> { focused};


	if ( defined( $focused)) {
		my ( $item) = $self-> { items}-> [ $focused];
		if ( $self-> { syncPromptChange} && ! ref $item) {
			my ( $ilen, $plen) =
				( length( $item || ''), $self-> { prompts}-> [ $focused] || 0);
			if ( $plen > 0) {
				$self-> { cursorPos} += length( $prompt) - $plen;
				substr( $self-> { items}-> [ $focused], 0, $plen) = $prompt;
				$self-> { prompts}-> [ $focused] = length( $prompt);
				$self-> item_changed( $focused);
				if ( length( $prompt) != $plen) {
					$ilen = length( $self-> { items}-> [ $focused]) if length( $self-> { items}-> [ $focused]) > $ilen;
				}
				else {
					$ilen = $plen;
				}
				$self-> invalidate_line( $focused, 0, $ilen);
			}
		}
	}

	$self-> { prompt} = $prompt;
}

#\subsection{set\_sync\_prompt\_change}
sub set_sync_prompt_change {
	my ( $self, $spc) = @_;
	return unless defined $spc;
	$self-> { syncPromptChange} = $spc != 0;
}

#\subsection{set\_mouse\_prompt\_selection}
sub set_mouse_prompt_selection {
	my ( $self, $mps) = @_;
	return unless defined $mps;
	$self-> { mousePromptSelection} = $mps != 0;
}

#\subsection{set\_auto\_adjust\_pos}
sub set_auto_adjust_pos {
	my ( $self, $aap) = @_;

	return unless defined( $aap);

	$self-> { autoAdjustPos} = $aap;
}

#\subsection{set\_insert\_mode}
sub set_insert_mode {
	my ( $self, $insert_mode) = @_;

	return unless defined( $insert_mode);

	$self-> { insertMode} = ( $insert_mode != 0);
	$self-> set_cursor_shape;
}

#\subsection{set\_override\_mode}
sub set_override_mode {
	my ( $self, $override_mode) = @_;

	return unless defined( $override_mode);

	$self-> { overrideMode} = ( $override_mode != 0);
}

#\subsection{set\_reuse\_prompts}
sub set_reuse_prompts {
	my ( $self, $reuse_prompts) = @_;

	return unless defined( $reuse_prompts);

	$self-> { reusePrompts} = ( $reuse_prompts != 0);
}

#\subsection{set\_word\_right\_mode}
sub set_word_right_mode {
	my ( $self, $wrm) = @_;
	croak("Wrong wordRightMode passed")
		unless defined( $wrm) && ( ( $wrm == tm::WordEnd) || ( $wrm == tm::WordBegin));
	$self-> { wordRightMode} = $wrm;
}

#\subsection{get\_integral\_size}
sub get_integral_size {
	my ( $self) = @_;

	return [
		$self-> { textCols} * $self-> { charWidth} +
		( $self-> vScroll ? $self-> { vScrollBar}-> width : 0) + $self-> { borderWidth} * 2,

		$self-> { textRows} * $self-> { charHeight} +
		( $self-> hScroll ? $self-> { hScrollBar}-> height : 0) + $self-> { borderWidth} * 2
	];
}

#\subsection{get\_cursor\_coordinates}
sub get_cursor_coordinates {
	my ( $self) = @_;

	my ( $cursorPos, $focused) = ( $self-> { cursorPos}, $self-> { focused});
	my ( $itemRow, $cursorX, $cursorY);

	if ( ! defined( $self-> { itemRow}-> [ $focused])) {
		$focused--;
		$itemRow = $self-> { itemRow}-> [ $focused] + $self-> { itemHeights}-> [ $focused];
	}
	else {
		$itemRow = $self-> { itemRow}-> [ $focused];
	}

	if ( $self-> { lineWrap}) {
		$cursorX = $cursorPos % $self-> { textCols};
		$cursorY = $self-> { itemRow}-> [ $focused] + int( $cursorPos / $self-> { textCols});
	}
	else {
		$cursorX = $cursorPos;
		$cursorY = $self-> { itemRow}-> [ $focused];
	}

	return ( $cursorX - $self-> { xOffset}, $cursorY - $self-> { yOffset});
}

#\subsection{item\_changed}
sub item_changed {
	my ( $self, $i) = @_;

	my ( $textCols, $textRows) = ( $self-> { textCols}, $self-> { textRows});
	my ( $items, $itemsChanged, $itemWidths, $itemXRight, $itemXLeft, $itemHeights,
			$itemXShifts, $itemRow, $viewItems, $cursorPos) =
		( $self-> { items}, $self-> { itemsChanged}, $self-> { itemWidths}, $self-> { itemXRight},
			$self-> { itemXLeft}, $self-> { itemHeights}, $self-> { itemXShifts}, $self-> { itemRow},
			$self-> { viewItems}, $self-> { cursorPos});
	my $item = $items-> [ $i];
	my ( $cW, $cH, $lineWrap, $focused, $xOffset, $yOffset) =
		( $self-> { charWidth}, $self-> { charHeight}, $self-> { lineWrap}, $self-> { focused},
			$self-> { xOffset}, $self-> { yOffset});
	my ( $rightBorder, $leftBorder, $maxWidth, $totalRows) =
		( $self-> { rightBorder}, $self-> { leftBorder}, $self-> { maxWidth}, $self-> { totalRows});
	my $itemLastN = $#{ $items};

	my $itemXShift = $itemXShifts-> [ $i]; # Remember! This is one of the changable parameters for a inserted views!

	my ( $newHeight, $newWidth, $newXRight, $newXLeft, $newXOffset, $newYOffset) =
		( $itemHeights-> [ $i], $itemWidths-> [ $i], $itemXRight-> [ $i], $itemXLeft-> [ $i],
			$self-> { xOffset}, $self-> { yOffset});

	if ( ref $item) {
		$newWidth = Prima::Utils::ceil( ( $item-> width + $itemXShift) / $cW);
		$newXRight = Prima::Utils::floor( ( $item-> right + $xOffset * $cW) / $cW);
		$newXLeft = Prima::Utils::floor( $itemXShift / $cW);
		$newHeight = Prima::Utils::ceil( $item-> height / $cH);
		$newHeight ||= 1;
	}
	else {
		my $width = length( $item) + ( $i == $focused ? 1 : 0);

		if ( $lineWrap) {
				$newHeight = Prima::Utils::ceil( $width / $textCols);
				$newWidth = ( $width > $textCols) ? $textCols : $width;
		}
		else {
				$newHeight = 1;
				$newWidth = $width;
		}
		$newHeight ||= 1;
		$newXRight = $newWidth - 1;
		$newXLeft = 0;
	}

	my $vShiftVal = ( $newHeight - $itemHeights-> [ $i]);
	my ( $resetScrolls, $needRectScroll) = ( $vShiftVal != 0, 0);
	my ( $scrollRect, $vS);

	#print "vShiftVal: $vShiftVal; newHeight: $newHeight";

	if ( $newXRight > $rightBorder) {
		$self-> { rightBorder} = $newXRight;
		$resetScrolls = 1;
	}

	if ( $newXLeft < $leftBorder) {
		$self-> { leftBorder} = $newXLeft;
		$resetScrolls = 1;
	}

	if ( ( $self-> { rightBorder} - $self-> { leftBorder} + 1) > $maxWidth) {
		$self-> { maxWidth} = ( $self-> { rightBorder} - $self-> { leftBorder} + 1);
		$resetScrolls = 1;
	}

	my $itemBottom = $itemRow-> [ $i] + $itemHeights-> [ $i] - 1;
	$itemBottom = 0 if $itemBottom < 0; # To fix -1 itemBottom problem for newly inserted
													# items;

	if ( $vShiftVal != 0) {
		my $j;

		#tex Lets change all \textbf{itemRow} values starting from the next item up to
		#tex the end of items list.
		#tex Actually, it's a good idea to move this loop outside of the condition
		#tex and make it calculate \textbf{maxWidth / leftBorder / rightBorder} as well.
		for ( $j = ( $i + 1); $j <= $itemLastN; $j++) {
				$itemRow-> [ $j] += $vShiftVal;
		}
		$self-> { totalRows} += $vShiftVal;

		# Now we must shift the client window region below the changed
		# item if necessary.
		if ( $itemBottom < $yOffset) {
				#print "Changing yOffset";
				$self-> { yOffset} = $newYOffset += $vShiftVal;
		}
		elsif ( $itemRow-> [ $i] < ( $yOffset + $textRows) && $itemBottom >= $yOffset &&
					( $i <= $itemLastN) && $newHeight != $itemHeights-> [ $i]) {
				#print "Calculating scrollRect";
				$scrollRect = $self-> trect2prect( 0,
					( $vShiftVal > 0 ?
						$itemBottom + 1 :
						$itemRow-> [ $i] + $itemHeights-> [ $i]) - $yOffset,
					$textCols - 1, $textRows - 1);
				$needRectScroll = 1;
		}
	}

	my ( $cursorX, $cursorY) = $self-> get_cursor_coordinates;

	#print "Cursor now at $cursorX,$cursorY";

	if ( $self-> { autoAdjustPos} && ! $self-> { inCommandExecution}) {
		if ( $i == $self-> { focused}) {
				#print "Adjusting for changed focused item";
				if ( ( $cursorX >= $textCols) || ( $cursorX < 0)) {
					$newXOffset += $cursorX < 0 ? $cursorX : $cursorX - $textCols + 1;
				}
				if ( ( ( $cursorY >= $textRows) || ( $cursorY < 0))) {
					$newYOffset += $cursorY < 0 ? $cursorY : $cursorY - $textRows + 1;
				}
		}
		elsif ( ( $itemRow-> [ $i] + $newHeight - 1) >= ( $yOffset + $textRows) &&
					$itemRow-> [ $i] < ( $yOffset + $textRows)) {
				#print "Adjusting for visible not focused item";
				$newYOffset = $itemRow-> [ $i] + $newHeight - $textRows;
		}
	}

	$itemHeights-> [ $i] = $newHeight;
	$itemWidths-> [ $i] = $newWidth;
	$itemXRight-> [ $i] = $newXRight;
	$itemXLeft-> [ $i] = $newXLeft;

	if ( $needRectScroll) {
		#$vS = ( $newYOffset - $self->{ yOffset} - $vShiftVal) * $cH;
		$vS = ( - $vShiftVal) * $cH;

		my ( @r) = $self-> { termView}-> rect;
		#print "Scrolling rect by $vShiftVal => $vS (@{ $scrollRect}), (@r)";

		$self-> { termView}-> scroll( 0, $vS, confineRect => $scrollRect);

		my ( $j);
		for ( $j = 0; $j <= $#{ $viewItems}; $j++) {
				my ( $vi) = $viewItems-> [ $j];
				$self-> set_view_item_pos( $vi) if $vi > $i;
		}
	}

	$self-> set_view_item_pos( $i) if ref $item;

	#print $itemRow->[ $i] - $newYOffset, "-", $itemRow->[ $i] - $newYOffset + $itemHeights->[ $i] - 1;

	#print "1.Now xOffset is $self->{ xOffset}; $newXOffset; leftBorder: $self->{ leftBorder}; rightBorder = $self->{ rightBorder}; maxWidth: $self->{ maxWidth}";

	$self-> reset_scrolls if $resetScrolls;
	#print "VScroll after: ", $self->{ vScrollBar}->value;

	if ( ( $newXOffset != $self-> { xOffset}) || ( $newYOffset != $self-> { yOffset})) {
		#print "Offsets: $newXOffset, $newYOffset";
		$self-> set_offsets( $newXOffset, $newYOffset);
	}
	else {
		# $self->{ termView}->update_view;
		$self-> reset_cursor;
	}
	#print "2.Now xOffset is $self->{ xOffset}";
}

#\subsection{invalidate\_line}
sub invalidate_line {
	my ( $self, $i, $sPos, $len) = @_;

	my @clipRect;

	if ( $self-> { lineWrap}) {
		my ( $textCols, $textRows) =
			( $self-> { textCols}, $self-> { textRows});
		my $sRow = int( $sPos / $textCols) + $self-> { itemRow}-> [ $i] - $self-> { yOffset};
		my $sCol = $sPos % $textCols - $self-> { xOffset};
		my $sWidth = ( $sCol + $len) > $textCols ? $textCols - $sCol : $len;
		$len -= $sWidth;
		$self-> { termView}-> invalidate_rect( @{ $self-> trect2prect( $sCol, $sRow, $sCol + $sWidth - 1, $sRow)});
		my $wholeLines = int( $len / $textCols);
		$sRow++;
		if ( $wholeLines > 0) {
			$len -= $textCols * $wholeLines;
			$self-> { termView}-> invalidate_rect( @{ $self-> trect2prect( 0, $sRow, $textCols - 1, $sRow + $wholeLines - 1)});
			$sRow += $wholeLines;
		}
		$self-> { termView}-> invalidate_rect( @{ $self-> trect2prect( 0, $sRow, $len - 1, $sRow)}) unless $len == 0;
	}
	else {
		my $leftX = $sPos - $self-> { xOffset};
		my $rightX = $leftX + $len - 1;
		my $Y = $self-> { itemRow}-> [ $i] - $self-> { yOffset};

		$self-> { termView}-> invalidate_rect( @{ $self-> trect2prect( $leftX, $Y, $rightX, $Y)});
	}
}

#stex
#\subsection{set\_line}
#
#Modes supported by \texttt{\bf set\_line}:
#
#\begin{tabular}{|l|l|}
#\hline\texttt{\it\bf add} & Method adds a string to the end of line. \\
#\hline\texttt{\it\bf insert} & Method inserts a string into the line. \\
#\hline\texttt{\it\bf set} & Method changes entire line. \\
#\hline\texttt{\it\bf change} & Method changes a part of line.\\\hline
#\end{tabular}\\
#etex
sub set_line {
	my ( $self, $i, $str, $p) = @_;

	croak("Wrong parameters passed to the method set_line")
		unless defined( $str) && defined( $i) && ( ( $i >= 0) && ( $i <= ( $#{ $self-> { items}} + 1)));
	if ( defined( $self-> { items}-> [ $i])) {
		return if ref $self-> { items}-> [ $i];
	}
	else {
		$self-> { items}-> [ $i] = '';
	}

	my ( $itemChanged, $cpos, $clen) = ( 1, 0, 0);

	my ( $mode) = defined( $p-> { mode}) ? $p-> { mode} : 'set';

	if ( $mode eq 'set') {
		my ( $oldlen) = length( $self-> { items}-> [ $i]);
		$self-> { items}-> [ $i] = $str;
		$clen = length( $str) > $oldlen ? length( $str) : $oldlen;
	}
	elsif ( $mode eq 'add') {
		$cpos = length( $self-> { items}-> [ $i]);
		$self-> { items}-> [ $i] .= $str;
		$clen = length( $str);
	}
	elsif ( $mode eq 'insert') {
		$cpos = defined( $p-> { spos}) ? $p-> { spos} : 0;
		substr( $self-> { items}-> [ $i], $cpos, 0) = $str;
		$clen = length( $self-> { items}-> [ $i]) - $cpos;
		$itemChanged = length( $str) > 0;
	}
	elsif ( $mode eq 'change') {
		my( $oldlen, $slen) =
			( length( $self-> { items}-> [ $i]),
				defined( $p-> { slen}) ? $p-> { slen} : length( $str));
		$cpos = defined( $p-> { spos}) ? $p-> { spos} : 0;
		substr( $self-> { items}-> [ $i], $cpos, $slen) = $str;
		my ( $newlen) = length( $self-> { items}-> [ $i]);
		$itemChanged = $oldlen != $newlen;
		$clen = $itemChanged ? ( $oldlen > $newlen ? $oldlen : $newlen) - $cpos : $slen;
	}
	else {
		croak("Unknown mode passed for method set_line");
	}

	if ( $itemChanged) {
		#print "Item $i changed";
		$self-> item_changed( $i);
	}
	else {
		$self-> reset_cursor;
	}
	#print "Invalidating from $cpos to $clen";
	$self-> invalidate_line( $i, $cpos, $clen);
}

#\subsection{viewChanged}
sub viewChanged {
	my ( $self, $idx) = @_;

	#print "Item #$idx changed its size";
	if ( $idx != -1) {
		$self-> item_changed( $idx);
	}
}

#\subsection{itemOnSize}
sub itemOnSize {
	my ( $itemSelf) = @_;

	# This counter should prevent from recursive notification processing.
	$itemSelf-> { termView}-> { onSize_processing}++;

	$itemSelf-> { termView}-> { onSize}-> ( @_) if defined( $itemSelf-> { termView}-> { onSize});

	if ( $itemSelf-> { termView}-> { onSize_processing} < 2) {
		my $self = $itemSelf-> owner-> owner;

		my $i;
		my $idx = -1;

		for ( $i = 0; $i <= $#{ $self-> { viewItems}}; $i++) {
			#print $self->{ items}->[ $self->{ viewItems}->[ $i]], " eq ", $itemSelf;
			if ( $self-> { items}-> [ $self-> { viewItems}-> [ $i]] eq $itemSelf) {
				$idx = $self-> { viewItems}-> [ $i];
				last;
			}
		}

		$self-> post_message( 'viewChanged', $idx) unless $idx == -1;
	}

	$itemSelf-> { termView}-> { onSize_processing}--;
}

#\subsection{itemOnSize}
sub itemOnMove {
	my ( $itemSelf) = @_;

	#print "itemOnMove";

	# This counter should prevent from recursive notification processing.
	$itemSelf-> { termView}-> { onMove_processing}++;

	$itemSelf-> { termView}-> { onMove}-> ( @_) if defined( $itemSelf-> { termView}-> { onMove});

	if ( $itemSelf-> { termView}-> { onMove_processing} < 2) {
		my $self = $itemSelf-> owner-> owner;

		my $i;
		my $idx = -1;

		for ( $i = 0; $i <= $#{ $self-> { viewItems}}; $i++) {
			#print $self->{ items}->[ $self->{ viewItems}->[ $i]], " eq ", $itemSelf;
			if ( $self-> { items}-> [ $self-> { viewItems}-> [ $i]] eq $itemSelf) {
				$idx = $self-> { viewItems}-> [ $i];
				last;
			}
		}

		if ( ( $idx != -1) && ( ! $self-> { ignoreViewsMove}) &&
			( $self-> { itemXShifts}-> [ $idx] - $self-> { xOffset} * $self-> { charWidth}) != $self-> { items}-> [ $idx]-> left) {
			#print "Should be:", $self->{ itemXShifts}->[ $idx] - $self->{ xOffset} * $self->{ charWidth}, ", is: ", $self->{ items}->[ $idx]->left;
			#print "Processing position change for $idx";

			$self-> { itemXShifts}-> [ $idx] = $self-> { items}-> [ $idx]-> left + $self-> { xOffset} * $self-> { charWidth};

			$self-> post_message( 'viewChanged', $idx);
		}
	}

	$itemSelf-> { termView}-> { onMove_processing}--;
}

#\subsection{delete\_view\_item}
sub remove_view_item {
	my ( $self, $i) = @_;

	return if ! ref $self-> { items}-> [ $i];

	my ( $item) = $self-> { items}-> [ $i];

	croak( "Not a reference to a Widget descendant") unless $item-> isa( "Prima::Widget");

	$item-> notify( 'TerminalRemove', $self);
	$item-> destroy, return 1
		if $item-> notify( 'Close');
	return 0;
}

#\subsection{insert\_item}
sub insert_item {
	my ( $self, $item, $prompt, $i) = @_;

	my ( $immutableFocused) = 0;
	my ( $items, $itemWidths, $itemXRight, $itemXLeft, $itemHeights,
			$itemXShifts, $itemRow) =
		( $self-> { items}, $self-> { itemWidths}, $self-> { itemXRight}, $self-> { itemXLeft},
			$self-> { itemHeights}, $self-> { itemXShifts}, $self-> { itemRow});

	if ( ! defined( $i)) {
		$immutableFocused = 1;
		$i = $self-> { focused};
	}

	if ( $i < ( - $#{ $items} - 1)) {
		croak("Incorrect index passed to insert_item");
	}

	my ( $idx) = $i < 0 ? scalar( @{ $items}) - $i : $i;

	return if ( $idx <= $#{ $items}) && ( ! defined( $items-> [ $idx]));

	if ( ref $item) {
		$item = $self-> insert_view( $item);
	}

	splice( @{ $items}, $idx, 0, ( ( defined( $item) && ref $item) ?
												$item :
												( defined( $prompt) ?
													( $prompt . $item) :
													undef)));
	$item ||= '';
	$prompt ||= '';
	splice( @{ $itemWidths}, $idx, 0, 0);
	splice( @{ $itemXRight}, $idx, 0, 0);
	splice( @{ $itemXLeft}, $idx, 0, 0);
	splice( @{ $itemHeights}, $idx, 0, 0);
	splice( @{ $itemXShifts}, $idx, 0, ref( $item) ? $item-> left : 0);
	splice( @{ $itemRow}, $idx, 0, ( $idx == 0 ? 0 : $itemRow-> [ $idx - 1] + $itemHeights-> [ $idx - 1]));
	splice( @{ $self-> { prompts}}, $idx, 0, length( $prompt));


	my $j;
	my $viewInsPos = 0;

	for ( $j = 0; $j <= $#{ $self-> { viewItems}}; $j++) {
		if ( $self-> { viewItems}-> [ $j] >= $idx) {
				#print "Shifting down view: ", $self->{ viewItems}->[ $j];
				$self-> { viewItems}-> [ $j]++;
				#print "Shifted down view: ", $self->{ viewItems}->[ $j];
		}
		else {
				$viewInsPos = $j;
		}
	}

	$self-> { focused}++ if ( ! $immutableFocused) && ( $self-> { focused} >= $idx);

	$self-> item_changed( $idx);
	if ( ref $item) {
		splice( @{ $self-> { viewItems}}, $viewInsPos, 0, $idx);
		$self-> set_view_item_pos( $idx);
		$self-> repaint;
	}
	else {
		$self-> invalidate_line( $idx, 0, ( $self-> { textCols} < length( $items-> [ $idx])) ?
			length( $items-> [ $idx]) :
			$self-> { textCols})
	}
}

#\subsection{delete_item}
sub delete_item {
	my ( $self, $i) = @_;

	my ( $immutableFocused) = 0;
	my ( $items, $itemWidths, $itemXRight, $itemXLeft, $itemHeights,
			$itemXShifts, $itemRow) =
		( $self-> { items}, $self-> { itemWidths}, $self-> { itemXRight}, $self-> { itemXLeft},
			$self-> { itemHeights}, $self-> { itemXShifts}, $self-> { itemRow});

	if ( ! defined( $i)) {
		$immutableFocused = 1;
		$i = $self-> { focused}
	}

	if ( $i < ( - $#{ $items} - 1)) {
		croak("Incorrect index passed to insert_item");
	}

	my ( $idx) = $i < 0 ? scalar( @{ $items}) - $i : $i;

	if ( ( $idx == $self-> { focused}) && ( $self-> { prompts}-> [ $idx] > 0)) {
		return 0;
	}

	my ( $item) = $items-> [ $idx];

	if ( defined( $item) && ( ref $item)) {
		return 0 unless $self-> remove_view_item( $idx);
	}

	if ( ! ref $item) {
		$self-> invalidate_line( $idx, 0, length( $item));
	}

	$self-> { totalRows} -= $self-> { itemHeights}-> [ $idx];
	if ( $idx < $#{ $self-> { items}}) {
		# Tricky way to simulate changing of the next item for the item_changed
		$self-> { itemRow}-> [ $idx + 1] = $self-> { itemRow}-> [ $idx];
		$self-> { itemHeights}-> [ $idx + 1] += $self-> { itemHeights}-> [ $idx];
	}

	splice( @{ $items}, $idx, 1);
	splice( @{ $itemWidths}, $idx, 1);
	splice( @{ $itemXRight}, $idx, 1);
	splice( @{ $itemXLeft}, $idx, 1);
	splice( @{ $itemHeights}, $idx, 1);
	splice( @{ $itemXShifts}, $idx, 1);
	splice( @{ $itemRow}, $idx, 1);
	splice( @{ $self-> { prompts}}, $idx, 1);

	my ( $j, $viewDelPos);
	for ( $j = 0; $j <= $#{ $self-> { viewItems}}; $j++) {
		if ( $self-> { viewItems}-> [ $j] > $idx) {
				$self-> { viewItems}-> [ $j]--;
		}
		elsif ( $self-> { viewItems}-> [ $j] == $idx) {
				$viewDelPos = $j;
		}
	}
	if ( defined $viewDelPos) {
		splice( @{ $self-> { viewItems}}, $viewDelPos, 1);
	}

	$self-> { focused}-- unless $immutableFocused || $self-> { focused} < $idx;

	my ( $ilen) = 0;
	if ( $idx <= $#{ $self-> { items}}) {
		$self-> item_changed( $idx);
		if ( ! ref $self-> { items}-> [ $idx]) {
			$self-> invalidate_line( $idx, 0, int( length( $self-> { items}-> [ $idx]) /
				$self-> { textCols} + 1
				) * $self-> { textCols});
		}
	}

	$self-> update_view;

	return 1;
}

#\subsection{put\_prepare\_item}
sub put_prepare_item {
	my ( $self, $newLine, $init_item) = @_;

	$init_item = '' unless defined( $init_item);

	my ( $focused, $items) = ( $self-> { focused}, $self-> { items});

	if ( $focused > $#{ $items}) {
		$focused = $self-> { focused} = $#{ $items} + 1;
		$self-> insert_item( $init_item, '');
		$self-> { cursorPos} = 0;
	}
	else {
		my ( $item) = $items-> [ $focused];

		if ( defined( $item)) {
			if ( ( $self-> { commandExecutionStarted}) && ( ! $self-> { overrideMode})) {
				$self-> { commandExecutionStarted} = 0;
				$self-> insert_item( $init_item, '');
			}
			elsif ( ref $item) {
				if ( ! $self-> remove_view_item( $focused)) {
					$self-> insert_item( $init_item, '');
				}
				else {
					$items-> [ $focused] = ref $init_item ?
						$self-> insert_view( $init_item) :
						$init_item;
				}
			}
			elsif ( ( $newLine && ( ! $self-> { overrideMode})) || ( $self-> { prompts}-> [ $focused] > 0)) {
				$self-> insert_item( $init_item, '');
			}
		}
		else {
			$items-> [ $focused] = ref $init_item ?
				$self-> insert_view( $init_item, $focused) :
				$init_item;
		}
	}
	$self-> { focused} = $focused;
}

#\subsection{putv}
sub putv {
	my ( $self, $view) = @_;

	return unless defined $view;

	my ( $focused) = $self-> { focused};

	if ( ref $view) {
		$self-> put_prepare_item( 0, $view);
	}
	else {
		$self-> put_prepare_item( 0, [ @_[ 1.. $#_]]);
	}
	$self-> item_changed( $focused);
}

#\subsection{print}
sub print {
	my ( $self, @strs) = @_;

	foreach ( @strs) {
		$self-> puts( $_);
	}
}

#\subsection{puts}
sub puts {
	my ( $self, $str) = @_;

	my $focused = $self-> { focused};

	$self-> put_prepare_item;

	my ( @strs) = split /((?:\n)|(?:\r)|(?:\t)|(?:\010)|(?:\007))/, $str;
	foreach ( @strs) {
		my ( $cP) = ( $self-> { cursorPos});
		if ( $_ eq "\n") {
			$self-> { linesPrinted}++;
			$self-> { charsPrinted} = 0;
			$self-> { focused} = ++$focused;
			$self-> { cursorPos} = 0;
			$self-> put_prepare_item( 1);
		}
		elsif ( $_ eq "\r") {
			$self-> { cursorPos} = 0;
			$self-> update_cursor( 1);
		}
		elsif ( $_ eq "\b") {
			$self-> { cursorPos}-- if $self-> { cursorPos} > 0;
			$self-> update_cursor( 1);
		}
		elsif ( $_ eq "\007") {
			$self-> adjust_terminal_window( 1);
			Prima::Utils::sound( 500, 100);
		}
		else {
			if ( $_ eq "\t") {
				$_ = ' ' x ( int( $cP / 8 + 1) * 8 - $cP);
			}
			my $slen = length( $_);
			$self-> { charsPrinted} += $slen;
			$self-> set_line( $focused, $_, { mode => 'change', spos => $cP, slen => $slen});
			$self-> { cursorPos} += $slen;
			if ( $self-> { lineWrap}) {
				$self-> { linesPrinted} += int( $self-> { charsPrinted} / $self-> { textCols});
				$self-> { charsPrinted} %= $self-> { textCols};
			}
			elsif ( $self-> { charsPrinted} > ( $self-> { textCols} * 3 / 4)) {
				$self-> { charsPrinted} = 0;
				$self-> adjust_terminal_window( 1);
			}
			$self-> { termView}-> update_view;
		}
		if ( $self-> { linesPrinted} > ( $self-> { textCols} / 2)) {
			$self-> { linesPrinted} = 0;
			$self-> adjust_terminal_window( 1);
		}
	}
}

#\subsection{put\_str}
sub put_str {
	my ( $self, $str) = @_;

	my $focused = $self-> { focused};
	my ( $cP, $slen) = ( $self-> { cursorPos}, length( $str));

	croak("Can't print into a empty item") unless defined( $self-> { items}-> [ $focused]);
	croak("Can't print into a Widget item") if ref $self-> { items}-> [ $focused];

	$self-> { cursorPos} += $slen;
	$self-> set_line( $focused, $str, { mode => ( $self-> { insertMode} ? 'insert' : 'change'), spos => $cP, slen => $slen});
}

#\subsection{set\_cursor\_pos}
sub set_cursor {
	my ( $self, $cP) = @_;

	my ( $items, $prompts, $focused) =
		( $self-> { items}, $self-> { prompts}, $self-> { focused});

	return if ( ref $items-> [ $focused]) || ( $cP < $prompts-> [ $focused]) || ( $cP > length( $items-> [ $focused]));

	$self-> { cursorPos} = $cP;
	$self-> update_cursor;
}

#\subsection{hScroll}
sub hScroll {
	$#_ ? $_[0]-> set_h_scroll( $_[1]) : return $_[0]-> { hScroll};
}

#\subsection{vScroll}
sub vScroll {
	$#_ ? $_[0]-> set_v_scroll( $_[1]) : return $_[0]-> { vScroll};
}

#\subsection{borderWidth}
sub borderWidth {
	$#_ ? $_[0]-> set_border_width( $_[1]) : return $_[0]-> { borderWidth};
}

#\subsection{topItem}
sub topItem {
	$#_ ? $_[0]-> set_top_item( $_[1]) : return $_[0]-> { topItem};
}

#\subsection{bottomItem}
sub bottomItem {
	$#_ ? $_[0]-> set_bottom_item( $_[1]) : return $_[0]-> { bottomItem};
}

#\subsection{vAlign}
sub vAlign {
	$#_ ? $_[0]-> set_alignment( $_[1]) : return $_[0]-> { vAlign};
}

#\subsection{lineWrap}
sub lineWrap {
	$#_ ? $_[0]-> set_line_wrapping( $_[1]) : return $_[0]-> { lineWrap};
}

#\subsection{prompt}
sub prompt {
	$#_ ? $_[ 0]-> set_prompt( $_[ 1]) : return $_[ 0]-> { prompt};
}

#\subsection{syncPromptChange}
sub syncPromptChange {
	$#_ ? $_[ 0]-> set_sync_prompt_change( $_[ 1]) : return $_[ 0]-> { syncPromptChange};
}

#\subsection{mousePromptSelection}
sub mousePromptSelection {
	$#_ ? $_[ 0]-> set_mouse_prompt_selection( $_[ 1]) : return $_[ 0]-> { mousePromptSelection};
}

#\subsection{autoAdjustPos}
sub autoAdjustPos {
	$#_ ? $_[0]-> set_auto_adjust_pos( $_[1]) : return $_[0]-> { autoAdjustPos};
}

#\subsection{cursor}
sub cursor {
	$#_ ? $_[ 0]-> set_cursor( $_[ 1]) : return $_[ 0]-> { cursorPos};
}

#\subsection{insertMode}
sub insertMode {
	$#_ ? $_[ 0]-> set_insert_mode( $_[ 1]) : return $_[ 0]-> { insertMode};
}

#\subsection{overrideMode}
sub overrideMode {
	$#_ ? $_[ 0]-> set_override_mode( $_[ 1]) : return $_[ 0]-> { overrideMode};
}

#\subsection{reusePrompts}
sub reusePrompts {
	$#_ ? $_[ 0]-> set_reuse_prompts( $_[ 1]) : return $_[ 0]-> { reusePrompts};
}

#\subsection{wordRightMode}
sub wordRightMode {
	$#_ ? $_[ 0]-> set_word_right_mode( $_[ 1]) : return $_[ 0]-> { wordRightMode};
}

# Primitives

#\subsection{leftmostPos}
sub leftmostPos {
	my ( $self) = @_;
	return ( ref $self-> { items}-> [ $self-> { focused}]) || ( $self-> { cursorPos} <= $self-> { prompts}-> [ $self-> { focused}]);
}

#\subsection{rightmostPos}
sub rightmostPos {
	my ( $self) = @_;
	return ( ref $self-> { items}-> [ $self-> { focused}]) || ( $self-> { cursorPos} >= length( $self-> { items}-> [ $self-> { focused}]));
}

#\subsection{Home}
sub Home {
	my ( $self) = @_;
	$self-> { cursorPos} = $self-> { prompts}-> [ $self-> { focused}];
	my ( $cursorCol, $cursorRow) = $self-> get_cursor_coordinates;
	if ( $cursorCol < 0) {
		$self-> set_offsets( 0, undef);
	}
	$self-> update_cursor;
}

#\subsection{End}
sub End {
	my ( $self) = @_;
	$self-> { cursorPos} = length( $self-> { items}-> [ $self-> { focused}]);
	$self-> update_cursor;
}

#\subsection{cursorLeft}
sub cursorLeft {
	my ( $self) = @_;
	return if $self-> leftmostPos;
	$self-> { cursorPos}--;
	$self-> update_cursor;
}

#\subsection{cursorRight}
sub cursorRight {
	my ( $self) = @_;
	return if $self-> rightmostPos;
	$self-> { cursorPos}++;
	$self-> update_cursor;
}

#\subsection{wordLeft}
sub wordLeft {
	my ( $self) = @_;
	return if $self-> leftmostPos;
	my ( $plen, $cP) =
		( $self-> { prompts}-> [ $self-> { focused}],
			$self-> { cursorPos},
		);
	my ( $str) = substr( $self-> { items}-> [ $self-> { focused}], $plen, $cP - $plen);
	$str =~ s/\w+\W*$//;
	$self-> { cursorPos} = $plen + length( $str);
	$self-> update_cursor;
}

#\subsection{wordRight}
sub wordRight {
	my ( $self) = @_;
	return if $self-> rightmostPos;
	my ( $plen, $cP) =
		( $self-> { prompts}-> [ $self-> { focused}],
			$self-> { cursorPos},
		);
	my ( $str) = substr( $self-> { items}-> [ $self-> { focused}], $cP);
	if ( $self-> { 'wordRightMode'} == tm::WordEnd) {
		$str =~ s/^(\W*\w+).*$/$1/;
	}
	else {
		$str =~ s/^(\w*\W+).*$/$1/;
	}
	$self-> { cursorPos} += length( $str);
	$self-> update_cursor;
}

#\subsection{toggleInsMode}
sub toggleInsMode {
	my ( $self) = @_;
	$self-> insertMode( !$self-> insertMode);
}

#\subsection{deleteChar}
sub deleteChar {
	my ( $self) = @_;
	return if $self-> rightmostPos;
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $self-> { cursorPos}, slen => 1});
}

#\subsection{deleteLeftChar}
sub deleteLeftChar {
	my ( $self) = @_;
	return if $self-> leftmostPos;
	$self-> { cursorPos}--;
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $self-> { cursorPos}, slen => 1});
}

#\subsection{deleteWordRight}
sub deleteWordRight {
	my ( $self) = @_;
	return if $self-> rightmostPos;
	my ( $str) = substr( $self-> { items}-> [ $self-> { focused}], $self-> { cursorPos});
	$str =~ s/^((\w+)|(\W+)).*$/$1/;
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $self-> { cursorPos}, slen => length( $str)});
}

#\subsection{deleteWordLeft}
sub deleteWordLeft {
	my ( $self) = @_;
	return if $self-> leftmostPos;
	my ( $plen, $cP) =
		( $self-> { prompts}-> [ $self-> { focused}],$self-> { cursorPos});
	my ( $str) = substr( $self-> { items}-> [ $self-> { focused}], $plen, $cP - $plen);
	$str =~ s/^(?:.*?)((\w+)|(\W+))$/$1/;
	$cP = ( $self-> { cursorPos} -= length( $str));
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $cP, slen => length( $str)});
	$self-> update_cursor;
}

#\subsection{deleteWord}
sub deleteWord {
	my ( $self) = @_;
	my ( $str, $cP) = ( $self-> { items}-> [ $self-> { focused}], $self-> { cursorPos});
	return if ( ref $str) || ( substr( $str, $cP, 1) !~ /\w/);
	substr( $str, $cP) =~ s/^\w+\W*//;
	substr( $str, 0, $cP) =~ s/(\w+)$//;
	$self-> { cursorPos} -= length( $1);
	$self-> set_line( $self-> { focused}, $str);
	$self-> update_cursor;
}

#\subsection{deleteToLineBegin}
sub deleteToLineBegin {
	my ( $self) = @_;
	return if $self-> leftmostPos;
	my ( $plen, $cP) =
		( $self-> { prompts}-> [ $self-> { focused}], $self-> { cursorPos});
	$self-> { cursorPos} = $plen;
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $plen, slen => ( $cP - $plen)});
	$self-> update_cursor;
}

#\subsection{deleteToLineEnd}
sub deleteToLineEnd {
	my ( $self) = @_;
	return if $self-> rightmostPos;
	my ( $cP, $slen) =
		( $self-> { cursorPos}, length( $self-> { items}-> [ $self-> { focused}]));
	$self-> set_line( $self-> { focused}, '', { mode => 'change', spos => $cP, slen => $slen});
}

#\subsection{cursorUp}
sub cursorUp {
	my ( $self) = @_;
	return if ( ! $self-> { 'lineWrap'}) || ( ref $self-> { items}-> [ $self-> { focused}]) ||
				( ( $self-> { cursorPos} - $self-> { prompts}-> [ $self-> { focused}]) < $self-> { textCols});
	$self-> { cursorPos} -= $self-> { textCols};
	$self-> update_cursor;
}

#\subsection{cursorDown}
sub cursorDown {
	my ( $self) = @_;
	return if ( ! $self-> { 'lineWrap'}) || ( ref $self-> { items}-> [ $self-> { focused}]) ||
				( ( $self-> { cursorPos} + $self-> { textCols}) > length( $self-> { items}-> [ $self-> { focused}]));
	$self-> { cursorPos} += $self-> { textCols};
	$self-> update_cursor;
}

#\subsection{previousPrompt}
sub previousPrompt {
	my ( $self) = @_;

	my ( $focused, $items) =
		( $self-> { focused} - 1, $self-> { items});

	while ( ( $focused >= 0) && ( $self-> { prompts}-> [ $focused]) == 0) {
		$focused--;
	}

	if ( $focused >= 0) {
		$self-> { focused} = $focused;
		$self-> { cursorPos} = $self-> { prompts}-> [ $focused];
		$self-> update_cursor;
	}
}

#\subsection{nextPrompt}
sub nextPrompt {
	my ( $self) = @_;

	my ( $focused, $items) =
		( $self-> { focused} + 1, $self-> { items});

	while ( ( $focused <= $#{ $self-> { items}}) && ( $self-> { prompts}-> [ $focused]) == 0) {
		$focused++;
	}

	if ( $focused <= $#{ $self-> { items}}) {
		$self-> { focused} = $focused;
		$self-> { cursorPos} = $self-> { prompts}-> [ $focused];
		$self-> update_cursor;
	}
}
1;
