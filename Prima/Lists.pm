#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk>
#     Anton Berezin  <tobez@tobez.org>
package Prima::Lists;

# contains:
#   AbstractListViewer
#   AbstractListBox
#   ListViewer
#   ListBox
#   ProtectedListBox

use strict;
use warnings;
use Prima qw(ScrollBar StdBitmap Utils);

package
    ci;

BEGIN {
eval 'use constant Grid => 1 + MaxId;' unless exists $ci::{Grid};
}

package Prima::AbstractListViewer;
use base qw(
	Prima::Widget
	Prima::Widget::Fader
	Prima::Widget::GroupScroller
	Prima::Widget::ListBoxUtils
	Prima::Widget::MouseScroller
);
__PACKAGE__->inherit_core_methods('Prima::Widget::GroupScroller');

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	%{Prima::Widget::Fader-> notification_types()},
	SelectItem  => nt::Default,
	DrawItem    => nt::Action,
	Stringify   => nt::Action,
	MeasureItem => nt::Action,
	DragItem    => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		align          => ta::Left,
		autoHeight     => 1,
		extendedSelect => 0,
		drawGrid       => 1,
		dragable       => 0,
		focusedItem    => -1,
		gridColor      => cl::Black,
		integralHeight => 0,
		integralWidth  => 0,
		itemHeight     => $def-> {font}-> {height},
		itemWidth      => $def-> {width} - 2,
		multiSelect    => 0,
		multiColumn    => 0,
		offset         => 0,
		topItem        => 0,
		scaleChildren  => 0,
		selectable     => 1,
		selectedItems  => [],
		vertical       => 1,
		widgetClass    => wc::ListBox,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
	$p-> { multiSelect} = 1 if
		exists $p-> { extendedSelect} &&
		$p-> {extendedSelect} &&
		!exists $p-> {multiSelect};
	$p-> { autoHeight} = 0 if
		exists $p-> { itemHeight} &&
		!exists $p-> {autoHeight};
	my $multi_column = exists($p->{multiColumn}) ?
		$p->{multiColumn} : $default->{multiColumn};
	my $vertical = exists($p->{vertical}) ?
		$p->{vertical} : $default->{vertical};
	$p-> { integralHeight} = 1 if
		! exists $p->{integralHeight} and
		( not($multi_column) or $vertical);
	$p-> { integralWidth} = 1 if
		! exists $p->{integralWidth} and
		$multi_column and not($vertical);
}

sub init
{
	my $self = shift;
	for ( qw( lastItem topItem focusedItem))
		{ $self-> {$_} = -1; }
	for ( qw(
		scrollTransaction gridColor dx dy 
		itemWidth offset multiColumn count autoHeight multiSelect
		extendedSelect dragable ))
		{ $self-> {$_} = 0; }
	for ( qw( drawGrid itemHeight integralWidth integralHeight vertical align))
		{ $self-> {$_} = 1; }
	$self-> {selectedItems} = {};
	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;
	$self-> {selectedItems} = {} unless $profile{multiSelect};
	for ( qw(
		gridColor offset multiColumn
		itemHeight autoHeight itemWidth multiSelect extendedSelect integralHeight
		integralWidth focusedItem topItem selectedItems dragable
		vertical drawGrid align))
		{ $self-> $_( $profile{ $_}); }
	$self-> reset;
	$self-> reset_scrolls;
	return %profile;
}


sub draw_items
{
	my ($self, $canvas) = (shift, shift);
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
	$self-> push_event;
	for ( @_) { $notifier-> ( @notifyParms, $canvas, @$_); }
	$self-> pop_event;
}

sub item2rect
{
	my ( $self, $item, @size) = @_;
	my @a = $self-> get_active_area( 0, @size);

	if ( $self-> {multiColumn}) {
		$item -= $self-> {topItem};
		my $who = $self-> {vertical} ? 'rows' : 'columns';
		my ($j,$i,$ih,$iw,$dg) = (
			$self-> {$who} ? (
				int( $item / $self-> {$who} - (( $item < 0) ? 1 : 0)),
				$item % $self-> {$who}
			) : (-1, 1),
			$self-> {itemHeight},
			$self-> {itemWidth},
			$self-> {drawGrid}
		);
		($i,$j)=($j,$i) unless $self->{vertical};

		return
			$a[0] + $j * ( $iw + $dg),
			$a[3] - $ih * ( $i + 1),
			$a[0] + $j * ( $iw + $dg) + $iw,
			$a[3] - $ih * ( $i + 1) + $ih;
	} else {
		my ($i,$ih) = ( $item - $self-> {topItem}, $self-> {itemHeight});
		return $a[0], $a[3] - $ih * ( $i + 1), $a[2], $a[3] - $ih * $i;
	}
}

sub on_paint
{
	my ($self,$canvas)   = @_;
	my @size   = $canvas-> size;

	unless ( $self-> enabled) {
		$self-> color( $self-> disabledColor);
		$self-> backColor( $self-> disabledBackColor);
	}
	my ( $ih, $iw, $dg, @a) = (
		$self-> { itemHeight},
		$self-> {itemWidth}, $self-> {drawGrid},
		$self-> get_active_area( 1, @size)
	);

	my $i;
	my $j;
	my $locWidth = $a[2] - $a[0] + 1;
	my @invalidRect = $canvas-> clipRect;
	$self-> draw_border( $canvas, undef, @size);

	if ( $self-> {multiColumn}) {
		my $xstart  = $a[0];
		my $yend    = $size[1] - $self-> {active_rows} * $ih - 1;
		my $uncover = $self->{uncover};
		my $ymiddle = $a[1] + $uncover->{y} + $self->{yedge} - 1
			if defined($uncover);

		for ( $i = 0; $i < $self-> {partial_columns}; $i++) {
			my $y = (
				defined($uncover) and
				$i >= $uncover->{x} and
				$i < $self-> {active_columns}
			) ?
			$ymiddle :
			(( $i < $self->{active_columns}) ?
				$yend :
				$a[3]
			);
			$canvas-> clear(
				$xstart, $a[1],
				( $xstart + $iw - 1 > $a[2]) ?
					$a[2] :
					$xstart + $iw - 1,
				$y
			) if $xstart >= $a[0] and $y >= $a[1];
			$xstart += $iw + $dg;
		}

		if ( $self-> {drawGrid}) {
			my $c = $canvas-> color;
			$canvas-> color( $self-> {gridColor});
			for ( $i = 1; $i < 1 + $self-> {whole_columns}; $i++) {
				$canvas-> line(
					$a[0] + $i * ( $iw + $dg) - 1, $a[1],
					$a[0] + $i * ( $iw + $dg) - 1, $a[3]
				);
			}
			$canvas-> color( $c);
		}
	} else {
		$canvas-> clear( @a[0..2], $a[1] + $self-> {uncover})
			if defined $self-> {uncover};
	}

	my $focusedState = $self-> focused ? ( exists $self-> {unfocState} ? 0 : 1) : 0;
	$self-> {unfocVeil} = ( $focusedState && $self-> {focusedItem} < 0 && $locWidth > 0) ? 1 : 0;
	my $foci = $self-> {focusedItem};

	if ( $self-> {count} > 0 && $locWidth > 0) {
		$canvas-> clipRect( @a);
		my @paintArray;
		my $item = $self-> {topItem};
		if ( $self-> {multiColumn})
		{
			my $di = $self-> {vertical} ? 1 : $self-> {active_columns};
			MAIN:for ( $j = 0; $j < $self-> {active_columns}; $j++)
			{
				$item = $self-> {topItem} + $j unless $self-> {vertical};
				for ( $i = 0; $i < $self-> {active_rows}; $i++)
				{
					if ( $self-> {vertical}) {
						last MAIN if $item > $self-> {lastItem};
					} else {
						last if $item > $self-> {lastItem};
					}
					my @itemRect = (
						$a[0] + $j * ( $iw + $dg),
						$a[3] - $ih * ( $i + 1) + 1,
						$a[0] + $j * ( $iw + $dg) + $iw,
						$a[3] - $ih * ( $i + 1) + $ih + 1
					);
					$item += $di, next if
						$itemRect[3] < $invalidRect[1] ||
						$itemRect[1] > $invalidRect[3] ||
						$itemRect[2] < $invalidRect[0] ||
						$itemRect[0] > $invalidRect[2];

					my $sel = $self-> {multiSelect} ?
						exists $self-> {selectedItems}-> {$item} :
						(( $self-> {focusedItem} == $item) ? 1 : 0);
					my $foc = ( $foci == $item) ? $focusedState : 0;
					$foc = 1 if $item == 0 && $self-> {unfocVeil};
					my $prelight = (defined($self->{prelight}) && ($self->{prelight} == $item)) ? 1 : 0;

					push( @paintArray, [
						$item,          # item number
						$itemRect[0], $itemRect[1],
						$itemRect[2]-1, $itemRect[3]-1,
						$sel, $foc, $prelight,  # selected and focused states
						$j,             # column
					]);
					$item += $di;
				}
			}
		} else {
			for ( $i = 0; $i < $self-> {rows}; $i++) {
				last if $item > $self-> {lastItem};
				my @itemRect = (
					$a[0], $a[3] - $ih * ( $i + 1) + 1,
					$a[2], $a[3] - $ih * $i
				);
				$item++, next if
					$itemRect[3] < $invalidRect[1] ||
					$itemRect[1] > $invalidRect[3];

				my $sel = $self-> {multiSelect} ?
					exists $self-> {selectedItems}-> {$item} :
					(( $foci == $item) ? 1 : 0);
				my $foc = ( $foci == $item) ? $focusedState : 0;
				$foc = 1 if $item == 0 && $self-> {unfocVeil};
				my $prelight = (defined($self->{prelight}) && ($self->{prelight} == $item)) ? 1 : 0;

				push( @paintArray, [
					$item,      # item number
					$itemRect[0] - $self-> {offset}, $itemRect[1],  # logic rect
					$itemRect[2], $itemRect[3],                     #
					$sel, $foc, $prelight, # selected and focused state
					0, #column,
				]);
				$item++;
			}
		}
		$self-> draw_items( $canvas, @paintArray);
	}
}

sub is_default_selection
{
	return $_[0]-> {unfocVeil};
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }
sub on_enter   { $_[0]-> redraw_items( $_[0]-> focusedItem); }

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	return if $mod & km::DeadKey;

	$mod &= ( km::Shift|km::Ctrl|km::Alt);
	$self-> notify(q(MouseUp),0,0,0) if defined $self-> {mouseTransaction};

	if ( $mod & km::Ctrl && $self-> {multiSelect}) {
		my $c = chr ( $code & 0xFF);
		if ( $c eq '/' || $c eq chr(ord('\\')-ord('@'))) {
			$self-> selectedItems(( $c eq '/') ? [0..$self-> {count}-1] : []);
			$self-> clear_event;
			return;
		}
	}
	return if ( $code & 0xFF) && ( $key == kb::NoKey);

	if ( scalar grep { $key == $_ } (
		kb::Left,kb::Right,kb::Up,kb::Down,kb::Home,kb::End,kb::PgUp,kb::PgDn
	)) {
		my $newItem = $self-> {focusedItem};
		my $doSelect = 0;
		if (
			$mod == 0 ||
			( $mod & km::Shift && $self-> {multiSelect} && $self-> { extendedSelect})
		) {
			my $pgStep  = $self-> {whole_rows} - 1;
			$pgStep = 1 if $pgStep <= 0;
			my $cols = $self-> {whole_columns};
			my $mc = $self-> {multiColumn};
			my $dx = $self-> {vertical} ? $self-> {rows} : 1;
			my $dy = $self-> {vertical} ? 1 : $self-> {active_columns};
			if ( $key == kb::Up)   {
				$newItem -= $dy;
			} elsif ( $key == kb::Down) {
				$newItem += $dy;
			} elsif ( $key == kb::Left) {
				$newItem -= $dx if $mc
			} elsif ( $key == kb::Right) {
				$newItem += $dx if $mc
			} elsif ( $key == kb::Home) {
				$newItem = $self-> {topItem}
			} elsif ( $key == kb::End)  {
				$newItem = $mc ?
					$self-> {topItem} + $self-> {whole_rows} * $cols - 1 :
					$self-> {topItem} + $pgStep;
			} elsif ( $key == kb::PgDn) {
				$newItem += $mc ?
					$self-> {whole_rows} * $cols :
					$pgStep
			} elsif ( $key == kb::PgUp) {
				$newItem -= $mc ?
					$self-> {whole_rows} * $cols :
					$pgStep
			};
			$doSelect = $mod & km::Shift;
		}

		if (
			( $mod & km::Ctrl) ||
			(
				(( $mod & ( km::Shift|km::Ctrl))==(km::Shift|km::Ctrl)) &&
				$self-> {multiSelect} &&
				$self-> { extendedSelect}
			)
		) {
			if ( $key == kb::PgUp || $key == kb::Home) { $newItem = 0};
			if ( $key == kb::PgDn || $key == kb::End)  { $newItem = $self-> {count} - 1};
			$doSelect = $mod & km::Shift;
		}
		if ( $doSelect ) {
			my ( $a, $b) = (
				defined $self-> {anchor} ?
					$self-> {anchor} :
					$self-> {focusedItem},
				$newItem
			);
			( $a, $b) = ( $b, $a) if $a > $b;
			$self-> selectedItems([$a..$b]);
			$self-> {anchor} = $self-> {focusedItem} unless defined $self-> {anchor};
		} else {
			$self-> selectedItems([$self-> focusedItem]) if exists $self-> {anchor};
			delete $self-> {anchor};
		}
		$self-> offset( $self-> {offset} + 5 * (( $key == kb::Left) ? -1 : 1))
			if !$self-> {multiColumn} && ($key == kb::Left || $key == kb::Right);
		$self-> focusedItem( $newItem >= 0 ? $newItem : 0);
		$self-> clear_event;
		return;
	} else {
		delete $self-> {anchor};
	}

	if ( $mod == 0 && ( $key == kb::Space || $key == kb::Enter)) {
		$self-> toggle_item( $self-> {focusedItem}) if
			$key == kb::Space &&
			$self-> {multiSelect} &&
			!$self-> {extendedSelect};

		$self-> clear_event;
		$self-> notify(q(Click)) if $key == kb::Enter && ($self-> focusedItem >= 0);
		return;
	}
}

sub on_leave
{
	my $self = $_[0];
	if ( $self-> {mouseTransaction}) {
		$self-> capture(0) if $self-> {mouseTransaction};
		$self-> {mouseTransaction} = undef;
	}
	$self-> redraw_items( $self-> focusedItem);
}

sub point2item
{
	my ( $self, $x, $y) = @_;
	my ( $ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);

	if ( $self-> {multiColumn}) {
		my ( $r, $t, $l, $c, $ac) = (
			$self-> {active_rows}, $self-> {topItem}, $self-> {lastItem},
			$self-> {whole_columns}, $self-> {active_columns},
		);
		$x -= $a[0];
		$y -= $a[1] + $self-> {yedge} + ( $self-> {rows} - $self->{active_rows} ) * $ih;
		$x /= $self-> {itemWidth} + $self-> {drawGrid};
		$y /= $ih;
		if ( $self->{whole_rows} > 0) {
			$r -= $self->{rows} - $self->{whole_rows};
		} else {
			$y++;
		}
		$y = $r - $y;
		$x = int( $x - (( $x < 0) ? 1 : 0));
		$y = int( $y - (( $y < 0) ? 1 : 0));
		$y = $r if $y > $r;

		if ( $self-> {vertical}) {
			return $t - $r                if $y < 0 && $x < 1;
			return $t + $r * $x,  -1      if $y < 0 && $x >= 0 && $x < $c;
			return $t + $r * $c           if $y < 0 && $x >= $c;
			return
				$l + $y + 1 - (( $c and $l < $self->{count}-1) ? $r : 0),
				$ac <= $c ? 0 : $r
					if $x > $c && $y >= 0 && $y < $r;
			return $t + $y - $r           if $x < 0 && $y >= 0 && $y < $r;
			return $l + $r                if $x >= $c - 1 && $y >= $r;
			return $t + $r * ($x + 1)-1,
				( $l < $self->{count} -1 ) ? 1 : 0
				if $y >= $r && $x >= 0 && $x < $c;
			return $t + $r - 1            if $x < 0 && $y >= $r;
			return $x * $self->{rows} + $y + $t;
		} else {
			if ( $y >= $r) {
				$x = 0 if $x < 0;
				$x = $ac - 1 if $x >= $ac;
				my $i = $t + $y * $ac + $x;
				return $i if $i <= $self->{count};
				return
					$t + ($r - 1) * $ac + $x,
					( $t + $y * $ac <= $self->{count}) ? 1 : 0
			}
			if ( $y < 0) {
				$x = 0 if $x < 0;
				$x = $ac - 1 if $x >= $ac;
				my $i = $t - $ac + $x;
				return ( $i < 0 && $t == 0) ? $x : $i;
			}
			return $t + $y * $ac, -1 if $x < 0;
			return $t + ( $y + 1) * $ac - 1,
				( $l < $self->{count} -1 ) ? 1 : 0
				if $x >= $ac;
			return $t + $y * $ac + $x;
		}
	} else {
		return $self-> {topItem} - 1 if $y >= $a[3];
		return $self-> {topItem} + $self-> {rows} if $y <= $a[1];
		my $h = $a[3];

		my $i = $self-> {topItem};
		while ( $y > 0) {
			return $i if $y <= $h && $y > $h - $ih;
			$h -= $ih;
			$i++;
		}
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	my $bw = $self-> { borderWidth};
	$self-> clear_event;
	return if $btn != mb::Left;

	my @a = $self-> get_active_area;
	return if defined $self-> {mouseTransaction} ||
		$y < $a[1] || $y >= $a[3] ||
		$x < $a[0] || $x >= $a[2];

	my $item = $self-> point2item( $x, $y);
	my $foc = $item >= 0 ? $item : 0;

	if ( $self-> {multiSelect}) {
		if ( $self-> {extendedSelect}) {
			if ($mod & km::Shift) {
				my $foc = $self-> focusedItem;
				return $self-> selectedItems(( $foc < $item) ?
					[$foc..$item] :
					[$item..$foc]
				);
			} elsif ( $mod & km::Ctrl) {
				return $self-> toggle_item( $item);
			} elsif ( !$mod) {
				$self-> {anchor} = $item;
				$self-> selectedItems([$foc]);
			}
		} elsif ( $mod & (km::Ctrl||km::Shift)) {
			return $self-> toggle_item( $item);
		}
	}

	$self-> {mouseTransaction} =
		(( $mod & ( km::Alt | ($self-> {multiSelect} ? 0 : km::Ctrl))) && $self-> {dragable}) ?
			2 : 1;
	if ( $self-> {mouseTransaction} == 2) {
		$self-> {dragItem} = $foc;
		$self-> {mousePtr} = $self-> pointer;
		$self-> pointer( cr::Move);
	}
	$self-> focusedItem( $foc);
	$self-> capture(1);
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	$self-> clear_event;
	return if $btn != mb::Left || !$dbl;

	$self-> notify(q(Click)) if $self-> focusedItem >= 0;
}

sub update_prelight
{
	my ( $self, $x, $y ) = @_;
	return delete $self->{prelight} if $self->{mouseTransaction};
	return unless $self->enabled;

	my @a = $self-> get_active_area;
	my $prelight;
	if ( $y >= $a[1] && $y < $a[3] && $x >= $a[0] && $x < $a[2]) {
		my ($item, $aux) = $self-> point2item( $x, $y);
		$item = -1 if $item >= $self->{count};
		$prelight = ($item >= 0) ? $item : undef unless defined $aux;
	}
	if ( ( $self->{prelight} // -1 ) != ( $prelight // -1 )) {
		my @redraw = (
			$self->{prelight} // (),
			$prelight // ()
		);
		if ( defined $prelight ) {
			$self-> fader_in_mouse_enter unless defined $self->{prelight};
			$self->{prelight} = $prelight;
		} else {
			$self-> fader_out_mouse_leave;
		}
		$self->redraw_items( @redraw );
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self-> update_prelight($x,$y);
	return unless defined $self-> {mouseTransaction};

	my $bw = $self-> { borderWidth};
	my ($item, $aux) = $self-> point2item( $x, $y);
	my @a = $self-> get_active_area;

	if ( $y >= $a[3] || $y < $a[1] || $x >= $a[2] || $x < $a[0]) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
	} else {
		$self-> scroll_timer_stop;
	}

	if ( $aux) {
		my $top = $self-> {topItem};
		$self-> topItem( $self-> {topItem} + $aux);
		$item += (( $top != $self-> {topItem}) ? $aux : 0);
	}

	if (
		$self-> {multiSelect} &&
		$self-> {extendedSelect} &&
		exists $self-> {anchor} &&
		$self-> {mouseTransaction} != 2
	) {
		my ( $a, $b, $c) = ( $self-> {anchor}, $item, $self-> {focusedItem});
		my $globSelect = 0;
		if (( $b <= $a && $c > $a) || ( $b >= $a && $c < $a)) {
			$globSelect = 1
		} elsif ( $b > $a) {
			if ( $c < $b) {
				$self-> add_selection([$c + 1..$b], 1)
			} elsif ( $c > $b) {
				$self-> add_selection([$b + 1..$c], 0)
			} else {
				$globSelect = 1
			}
		} elsif ( $b < $a) {
			if ( $c < $b) {
				$self-> add_selection([$c..$b], 0)
			} elsif ( $c > $b) {
				$self-> add_selection([$b..$c], 1)
			} else {
				$globSelect = 1
			}
		} else {
			$globSelect = 1
		}

		if ( $globSelect ) {
			( $a, $b) = ( $b, $a) if $a > $b;
			$self-> selectedItems([$a..$b]);
		}
	}

	$self-> focusedItem( $item >= 0 ? $item : 0);
	$self-> offset( $self-> {offset} + 5 * (( $x < $a[0]) ? -1 : 1))
		if $x >= $a[2] || $x < $a[0];
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless defined $self-> {mouseTransaction};

	my @dragnotify;
	if ( $self-> {mouseTransaction} == 2) {
		$self-> pointer( $self-> {mousePtr});
		my $fci = $self-> focusedItem;
		@dragnotify = ($self-> {dragItem}, $fci)
			if $fci != $self-> {dragItem} and $self-> {dragItem} >= 0;
	}

	delete $self-> {mouseTransaction};
	delete $self-> {mouseHorizontal};
	delete $self-> {anchor};

	$self-> capture(0);
	$self-> clear_event;
	$self-> notify(q(DragItem), @dragnotify) if @dragnotify;
}

sub on_mouseenter
{
	my $self = shift;
	$self-> fader_in_mouse_enter;
}

sub on_mouseleave
{
	my $self = shift;
	$self-> fader_out_mouse_leave;
}

sub on_fadeout
{
	my $self = shift;
	my $p = delete $self->{prelight};
	$self->redraw_items($p) if defined $p; # eventual double prelight
}

sub on_faderepaint
{
	my $self = shift;
	return unless defined $self->{prelight};
	$self-> fader_cancel_if_unbuffered;
	$self-> redraw_items( $self->{prelight} );
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;

	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$z *= $self-> {whole_columns}
		if $self-> {multiColumn} and not $self->{vertical};
	$z *= $self-> {whole_rows} if $mod & km::Ctrl;
	my $newTop = $self-> topItem - $z;
	my $cols = $self-> {whole_columns};
	my $maxTop = $self-> {count} - $self-> {whole_rows} * $cols;

	$self-> topItem( $newTop > $maxTop ? $maxTop : $newTop);
	$self-> update_prelight($x,$y);
}

sub on_size
{
	my $self = $_[0];
	$self-> reset;
	$self-> reset_scrolls;
}

sub reset
{
	my $self = $_[0];

	my @size = $self-> get_active_area( 2);
	my $ih   = $self-> {itemHeight};
	my $iw   = $self-> {itemWidth};

	$self-> {whole_rows}   = int( $size[1] / $ih);
	$self-> {partial_rows} = ( $size[1] > $self-> {whole_rows} * $ih ) ? 1 : 0;
	$self-> {whole_rows}   = 0 if $self-> {whole_rows} < 0;
	$self-> {partial_rows} += $self-> {whole_rows};
	$self-> {yedge}        = $size[1] - $self-> {whole_rows} * $ih;
	$self-> {yedge}        = 0 if $self-> {yedge} < 0;

	if ( $self-> {multiColumn}) {
		my $top = $self-> {topItem};
		my $max = $self-> {count} - 1;
		my $dg  = $self-> {drawGrid};

		$self-> {whole_columns}   = int( $size[0] / ( $dg + $iw));
		$self-> {partial_columns} = ( $size[0] > $self-> {whole_columns} * ( $dg + $iw))
						? 1 : 0;
		$self-> {whole_columns}   = 0 if $self-> {whole_columns} < 0;
		$self-> {partial_columns} += $self-> {whole_columns};
		$self-> {uncover} = undef;

		$self-> {rows} = $self-> {integralHeight} ?
				( $self-> {whole_rows} || $self-> {partial_rows} ) :
				$self-> {partial_rows};
		$self-> {columns} = $self-> {integralWidth} ?
				( $self-> {whole_columns} || $self-> {partial_columns} ) :
				$self-> {partial_columns};

		my $seen_items = $self->{rows} * $self-> {columns};
		$self-> {lastItem} = ( $top + $seen_items - 1 > $max) ?
			$max : $top + $seen_items - 1;
		$seen_items = $self-> {lastItem} - $top + 1;

		if ( $self-> {vertical} ) {
			if ( $self-> {rows} > 0) {
				$self-> {active_rows} = ( $seen_items > $self-> {rows} ) ?
					$self->{rows} : $seen_items;
				$self-> {active_columns} =
					int( $seen_items / $self-> {rows}) +
					(( $seen_items % $self-> {rows}) ? 1 : 0);
				$seen_items %= $self->{rows};
				$self-> {uncover} = {
				  	x => $self-> {active_columns} - 1,
					y => $ih * ($self-> {whole_rows} - $seen_items)
				} if $seen_items
			} else {
				$self-> {active_columns} = $self-> {active_rows} = 0;
			}
		} else {
			if ( $self-> {columns} > 0) {
				$self-> {active_columns} = ( $seen_items > $self-> {columns} ) ?
					$self-> {columns} : $seen_items;
				$self-> {active_rows} =
					int( $seen_items / $self-> {columns}) +
					(int( $seen_items % $self-> {columns}) > 0);
				$seen_items %= $self->{columns};
				$self-> {uncover} = {
				  	x => $seen_items,
					y => $ih * ($self-> {whole_rows} - $self-> {active_rows} + 1),
				} if $seen_items
			} else {
				$self-> {active_columns} = $self-> {active_rows} = 0;
			}
		}
		$self-> {xedge} = $size[0] - $self-> {whole_columns} * ($iw + $dg);
		$self-> {xedge} = 0 if $self-> {xedge} < 0;
	} else {
		$self-> {$_} = 1 for qw(partial_columns whole_columns active_columns columns);
		$self-> {xedge} = 0;
		$self-> {rows} = (
				$self-> {integralHeight} and
				$self-> {whole_rows} > 0
			) ?
				$self-> {whole_rows} :
				$self-> {partial_rows};
		my ($max, $last) = (
			$self-> {count} - 1,
			$self-> {topItem} + $self-> {rows} - 1
		);
		$self-> {lastItem} = $max > $last ? $last : $max;
		$self-> {active_rows} = $self->{lastItem} - $self-> {topItem} + 1;
		$self-> {uncover} = $size[1] - $self-> {active_rows} * $ih - 1
			if $self->{active_rows} < $self-> {partial_rows};
	}
	$self-> {uncover} = undef if $size[0] <= 0 or $size[1] <= 0;
}

sub reset_scrolls
{
	my $self = $_[0];

	my $count = $self-> {count};
	my $cols  = $self-> {whole_columns};
	my $rows  = $self-> {whole_rows};
	$cols++ if (
			$self->{whole_columns} == 0 and
			$self->{active_columns} > 0
		) or (
			$self->{partial_columns} > $self->{whole_columns} and
			$self->{yedge} > $self-> {itemHeight} * 0.66
		);
	$rows++ if (
			$self->{whole_rows} == 0 and
			$self->{active_rows} > 0
		) or (
			$self->{partial_rows} > $self->{whole_rows} and
			$self->{xedge} > $self-> {itemWidth} * 0.66
		);

	if ( !($self-> {scrollTransaction} & 1)) {
		$self-> vScroll( $self->{whole_rows} * $self->{whole_columns} < $count)
			if $self-> {autoVScroll};

		$self-> {vScrollBar}-> set(
			step     => ( $self-> {multiColumn} and not $self->{vertical}) ?
					$self-> {active_columns} : 1,
			max      => $count - $self->{whole_rows} * $self->{whole_columns},
			whole    => $count,
			partial  => $rows * $cols,
			value    => $self-> {topItem},
			pageStep => $rows,
		) if $self-> {vScroll};
	}
	if ( !($self-> {scrollTransaction} & 2)) {
		if ( $self-> {multiColumn}) {
			$self-> hScroll( $self->{whole_rows} * $self->{whole_columns} < $count)
				if $self-> {autoHScroll};
			$self-> {hScrollBar}-> set(
				max      => $count - $self->{whole_rows} * $self->{whole_columns},
				step     => $rows,
				pageStep => $rows * $cols,
				whole    => $count,
				partial  => $rows * $cols,
				value    => $self-> {topItem},
			) if $self-> {hScroll};
		} else {
			my @sz = $self-> get_active_area( 2);
			my $iw = $self-> {itemWidth};

			if ( $self-> {autoHScroll}) {
				my $hs = ( $sz[0] < $iw) ? 1 : 0;
				if ( $hs != $self-> {hScroll}) {
					$self-> hScroll( $hs);
					@sz = $self-> get_active_area( 2);
				}
			}

			$self-> {hScrollBar}-> set(
				max      => $iw - $sz[0],
				whole    => $iw,
				value    => $self-> {offset},
				partial  => $sz[0],
				pageStep => $iw / 5,
			) if $self-> {hScroll};
		}
	}
}

sub select_all {
	my $self = $_[0];
	$self-> selectedItems([0..$self-> {count}-1]);
}

sub deselect_all {
	my $self = $_[0];
	$self-> selectedItems([]);
}

sub set_auto_height
{
	my ( $self, $auto) = @_;

	$self-> itemHeight( $self-> font-> height) if $auto;
	$self-> {autoHeight} = $auto;
}

sub set_align
{
	my ( $self, $align) = @_;

	$self-> {align} = $align;
	$self-> repaint;
}

sub reset_indents
{
	my ( $self) = @_;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}


sub set_count
{
	my ( $self, $count) = @_;
	$count = 0 if $count < 0;
	my $oldCount = $self-> {count};
	$self-> { count} = $count;
	my $doFoc = undef;
	if ( $oldCount > $count) {
		for ( keys %{$self-> {selectedItems}}) {
			delete $self-> {selectedItems}-> {$_} if $_ >= $count;
		}
	}
	$self-> reset;
	$self-> reset_scrolls;
	$self-> focusedItem( -1) if $self-> {focusedItem} >= $count;
	$self-> repaint;
}

sub set_extended_select
{
	my ( $self, $esel) = @_;
	$self-> {extendedSelect} = $esel;
}

sub set_focused_item
{
	my ( $self, $foc) = @_;
	my $oldFoc = $self-> {focusedItem};
	$foc = $self-> {count} - 1 if $foc >= $self-> {count};
	$foc = -1 if $foc < -1;
	return if $self-> {focusedItem} == $foc;
	return if $foc < -1;

	$self-> {focusedItem} = $foc;
	$self-> selectedItems([$foc])
		if $self-> {multiSelect} && $self-> {extendedSelect}
			&& ! exists $self-> {anchor} &&
				( !defined($self-> {mouseTransaction}) || $self-> {mouseTransaction} != 2);
	$self-> notify(q(SelectItem), [ $foc], 1)
		if $foc >= 0 && !exists $self-> {selectedItems}-> {$foc};

	my $topSet = undef;
	if ( $foc >= 0) {
		my $mc   = $self-> {multiColumn};
		if ( $mc ) {
			my ( $rows, $cols) = ($mc and not $self->{vertical}) ?
				($self-> {columns} || 1, $self-> {whole_rows} || 1) :
				($self-> {rows}    || 1, $self-> {whole_columns} || 1);
			if ( $foc < $self-> {topItem}) {
				$topSet = $foc - $foc % $rows;
			} elsif ( $foc >= $self-> {topItem} + $rows * $cols - 1) {
				$topSet = $foc - $foc % $rows - $rows * ( $cols - 1);
			}
		} else {
			if ( $foc < $self-> {topItem}) {
				$topSet = $foc;
			} elsif ( $foc >= $self-> {topItem} + $self->{whole_rows}) {
				$topSet = $foc - $self->{whole_rows} + 1;
			}
		}
	}
	$oldFoc = 0 if $oldFoc < 0;
	$self-> redraw_items( $foc, $oldFoc);
	if (
		!$self-> {multiSelect} && !$self-> {extendedSelect} &&
		defined($topSet) &&
		($self->{topItem} - $topSet) == ($oldFoc - $foc)
	) {
		$self-> set_top_item($topSet, $oldFoc - $foc);
	} else {
		$self-> topItem( $topSet) if defined $topSet;
	}
}

sub colorIndex
{
	my ( $self, $index, $color) = @_;
	return ( $index == ci::Grid) ?
		$self-> {gridColor} : $self-> SUPER::colorIndex( $index)
		if $#_ < 2;
	( $index == ci::Grid) ?
		( $self-> gridColor( $color), $self-> notify(q(ColorChanged), ci::Grid)) :
		( $self-> SUPER::colorIndex( $index, $color));
}

sub dragable
{
	return $_[0]-> {dragable} unless $#_;
	$_[0]-> {dragable} = $_[1];
}

sub set_draw_grid
{
	my ( $self, $dg) = @_;
	$dg = ( $dg ? 1 : 0);
	return if $dg == $self-> {drawGrid};

	$self-> {drawGrid} = $dg;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_grid_color
{
	my ( $self, $gc) = @_;
	return if $gc == $self-> {gridColor};
	$self-> {gridColor} = $gc;
	$self-> repaint if $self-> {drawGrid};
}

sub set_integral_height
{
	my ( $self, $ih) = @_;
	return if $self-> {integralHeight} == $ih;
	$self-> {integralHeight} = $ih;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_integral_width
{
	my ( $self, $iw) = @_;
	return if $self-> {integralWidth} == $iw;
	$self-> {integralWidth} = $iw;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_item_height
{
	my ( $self, $ih) = @_;
	$ih = 1 if $ih < 1;
	$self-> autoHeight(0);
	return if $ih == $self-> {itemHeight};
	$self-> {itemHeight} = $ih;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_item_width
{
	my ( $self, $iw) = @_;
	$iw = 1 if $iw < 1;
	return if $iw == $self-> {itemWidth};
	$self-> {itemWidth} = $iw;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_multi_column
{
	my ( $self, $mc) = @_;
	return if $mc == $self-> {multiColumn};
	$self-> offset(0) if $self-> {multiColumn} = $mc;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub set_multi_select
{
	my ( $self, $ms) = @_;
	return if $ms == $self-> {multiSelect};

	unless ( $self-> {multiSelect} = $ms) {
		$self-> selectedItems([]);
		$self-> repaint;
	} else {
		$self-> selectedItems([$self-> focusedItem]);
	}
}

sub set_offset
{
	my ( $self, $offset) = @_;
	$self-> {offset} = 0, return if $self-> {multiColumn};
	my @sz = $self-> size;
	my ( $iw, @a) = ( $self-> {itemWidth}, $self-> get_active_area( 0, @sz));
	my $lc = $a[2] - $a[0];
	if ( $iw > $lc) {
		$offset = $iw - $lc if $offset > $iw - $lc;
		$offset = 0 if $offset < 0;
	} else {
		$offset = 0;
	}
	return if $self-> {offset} == $offset;

	my $oldOfs = $self-> {offset};
	$self-> {offset} = $offset;
	my $dt = $offset - $oldOfs;
	$self-> reset;

	if ( $self-> {hScroll} && !$self-> {multiColumn} && !($self-> {scrollTransaction} & 2)) {
		$self-> {scrollTransaction} |= 2;
		$self-> {hScrollBar}-> value( $offset);
		$self-> {scrollTransaction} &= ~2;
	}

	$self-> scroll( -$dt, 0, clipRect => \@a);
	if ( $self-> focused) {
		my $focId = ( $self-> {focusedItem} >= 0) ? $self-> {focusedItem} : 0;
		$self-> invalidate_rect( $self-> item2rect( $focId, @sz));
	}
}

sub redraw_items
{
	my $self = shift;
	my @sz = $self-> size;
	$self-> invalidate_rect( $self-> item2rect( $_, @sz)) for @_;
}

sub set_selected_items
{
	my ( $self, $items) = @_;
	return if !$self-> { multiSelect} && ( scalar @{$items} > 0);

	my $ptr = $::application-> pointer;
	$::application-> pointer( cr::Wait)
		if scalar @{$items} > 500;

	my $sc = $self-> {count};
	my %newItems;
	for (@{$items}) {
		$newItems{$_}=1 if $_>=0 && $_<$sc;
	}

	my @stateChangers; # $#stateChangers = scalar @{$items};
	my $k;
	while (defined($k = each %{$self-> {selectedItems}})) {
		next if exists $newItems{$k};
		push( @stateChangers, $k);
	};

	my @indices;
	my $sel = $self-> {selectedItems};
	$self-> {selectedItems} = \%newItems;
	$self-> notify(q(SelectItem), [@stateChangers], 0) if scalar @stateChangers;

	while (defined($k = each %newItems)) {
		next if exists $sel-> {$k};
		push( @stateChangers, $k);
		push( @indices, $k);
	};
	$self-> notify(q(SelectItem), [@indices], 1) if scalar @indices;

	$::application-> pointer( $ptr);

	return unless scalar @stateChangers;
	$self-> redraw_items( @stateChangers);
}

sub get_selected_items
{
	return $_[0]-> {multiSelect} ?
		[ sort { $a<=>$b } keys %{$_[0]-> {selectedItems}}] :
		(
			( $_[0]-> {focusedItem} < 0) ? [] : [$_[0]-> {focusedItem}]
		);
}

sub get_selected_count
{
	return scalar keys %{$_[0]-> {selectedItems}};
}

sub is_selected
{
	return exists($_[0]-> {selectedItems}-> {$_[1]}) ? 1 : 0;
}

sub set_item_selected
{
	my ( $self, $index, $sel) = @_;
	return unless $self-> {multiSelect};
	return if $index < 0 || $index >= $self-> {count};
	return if $sel == exists $self-> {selectedItems}-> {$index};

	$sel ?
		$self-> {selectedItems}-> {$index} = 1 :
		delete $self-> {selectedItems}-> {$index};
	$self-> notify(q(SelectItem), [ $index], $sel);
	$self-> invalidate_rect( $self-> item2rect( $index));
}

sub select_item   {  $_[0]-> set_item_selected( $_[1], 1); }
sub unselect_item {  $_[0]-> set_item_selected( $_[1], 0); }
sub toggle_item   {  $_[0]-> set_item_selected( $_[1], $_[0]-> is_selected( $_[1]) ? 0 : 1)}

sub add_selection
{
	my ( $self, $items, $sel) = @_;
	return unless $self-> {multiSelect};
	my @notifiers;
	my $count = $self-> {count};
	my @sz = $self-> size;
	for ( @{$items})
	{
		next if $_ < 0 || $_ >= $count;
		next if exists $self-> {selectedItems}-> {$_} == $sel;

		$sel ?
			$self-> {selectedItems}-> {$_} = 1 :
			delete $self-> {selectedItems}-> {$_};
		push ( @notifiers, $_);
		$self-> invalidate_rect( $self-> item2rect( $_, @sz));
	}
	return unless scalar @notifiers;
	$self-> notify(q(SelectItem), [ @notifiers], $sel) if scalar @notifiers;
}

sub set_top_item
{
	my ( $self, $topItem, $with_focus_shift) = @_;
	$topItem = 0 if $topItem < 0;   # first validation
	$topItem = $self-> {count} - 1 if $topItem >= $self-> {count};
	$topItem = 0 if $topItem < 0;   # count = 0 case
	return if $topItem == $self-> {topItem};

	my $oldTop = $self-> {topItem};
	$self-> {topItem} = $topItem;
	my ( $ih, $iw, @a) = ( $self-> {itemHeight}, $self-> {itemWidth}, $self-> get_active_area);
	my $dt = $topItem - $oldTop;
	$self-> reset;

	if ( !($self-> {scrollTransaction} & 1) && $self-> {vScroll}) {
		$self-> {scrollTransaction} |= 1;
		$self-> {vScrollBar}-> value( $topItem);
		$self-> {scrollTransaction} &= ~1;
	}

	if ( !($self-> {scrollTransaction} & 2) && $self-> {hScroll} && $self-> {multiColumn}) {
		$self-> {scrollTransaction} |= 2;
		$self-> {hScrollBar}-> value( $topItem);
		$self-> {scrollTransaction} &= ~2;
	}

	if ( $self-> { multiColumn}) {
		$iw += $self-> {drawGrid};
		if ( $self-> {vertical}) {
			if ($self->{rows} != 0 && abs($dt) % $self->{rows}) {
				$a[1] += $self->{yedge} if $self->{integralHeight};
				$self-> scroll( 0, $ih * $dt, clipRect => \@a);
				return;
			}

			if ($self->{integralWidth}) {
				$a[2] -= $self->{xedge};
			} elsif ( !defined $with_focus_shift || $with_focus_shift < 0 ) {
				# invalid xedge on the right and exposed stripe on the left make clipRect too large
				$self-> invalidate_rect($a[2] - $self->{xedge}, $a[1], $a[2], $a[3]);
			}
			if ( defined $with_focus_shift ) {
				if ( $with_focus_shift < 0 ) {
					my $dx = $iw + ($self->{integralWidth} ? 0 : $self->{xedge});
					$self-> invalidate_rect($a[2] - $dx, $a[1], $a[2], $a[3]);
					$a[2] -= $dx;
				} else {
					$self-> invalidate_rect($a[0], $a[1], $a[0] + $iw, $a[3]);
					$a[0] += $iw;
				}
			}
			$self-> scroll(
				-( $dt / $self-> {rows}) * $iw, 0,
				clipRect => \@a
			);
		} else {
			if ($self->{columns} > 0 && abs($dt) % $self->{columns}) {
				$a[2] -= $self->{xedge} if $self->{integralWidth};
				$self-> scroll(- $iw * $dt, 0, clipRect => \@a);
				return;
			}

			if ($self->{integralHeight}) {
				$a[1] += $self->{yedge};
			} elsif ( !defined $with_focus_shift || $with_focus_shift < 0 ) {
				$self-> invalidate_rect($a[0], $a[1], $a[2], $a[1] + $self->{yedge})
			}
			if ( defined $with_focus_shift ) {
				if ( $with_focus_shift < 0 ) {
					my $dy = $ih + ($self->{integralHeight} ? 0 : $self->{yedge});
					$self-> invalidate_rect($a[0], $a[1], $a[2], $a[1] + $dy);
					$a[1] += $dy;
				} else {
					$a[3] -= $ih;
					$self-> invalidate_rect($a[0], $a[3], $a[2], $a[3] + $ih);
				}
			}
			$self-> scroll(
				0, ( $dt / $self-> {columns}) * $ih,
				clipRect => \@a
			);
		}
	} else {
		$a[1] += $self-> {yedge}
			if $self-> {integralHeight} and $self-> {whole_rows} > 0;
		if ( defined $with_focus_shift ) {
			if ( $with_focus_shift < 0 ) {
				$a[1] += $ih;
				$a[1] += $self->{yedge} unless $self->{integralHeight};
			} else {
				$a[3] -= $ih;
			}
		}
		$self-> scroll( 0, $dt * $ih, clipRect => \@a);
	}
	$self-> update_view;
}

sub set_vertical
{
	my ( $self, $vertical) = @_;
	return if $self-> {vertical} == $vertical;
	$self-> {vertical} = $vertical;
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}


sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction} & 1;
	$self-> {scrollTransaction} |= 1;
	$self-> topItem( $scr-> value);
	$self-> {scrollTransaction} &= ~1;
}

sub HScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction} & 2;
	$self-> {scrollTransaction} |= 2;
	$self-> {multiColumn} ?
		$self-> topItem( $scr-> value) :
		$self-> offset( $scr-> value);
	$self-> {scrollTransaction} &= ~2;
}

#sub on_drawitem
#{
#	my ($self, $canvas, $itemIndex, $x, $y, $x2, $y2, $selected, $focused, $prelight, $column) = @_;
#}

#sub on_selectitem
#{
#	my ($self, $itemIndex, $selectState) = @_;
#}

#sub on_dragitem
#{
#	my ( $self, $from, $to) = @_;
#}

sub autoHeight    {($#_)?$_[0]-> set_auto_height    ($_[1]):return $_[0]-> {autoHeight}     }
sub align         {($#_)?$_[0]-> set_align          ($_[1]):return $_[0]-> {align}          }
sub count         {($#_)?$_[0]-> set_count          ($_[1]):return $_[0]-> {count}          }
sub extendedSelect{($#_)?$_[0]-> set_extended_select($_[1]):return $_[0]-> {extendedSelect} }
sub drawGrid      {($#_)?$_[0]-> set_draw_grid      ($_[1]):return $_[0]-> {drawGrid}       }
sub gridColor     {($#_)?$_[0]-> set_grid_color     ($_[1]):return $_[0]-> {gridColor}      }
sub focusedItem   {($#_)?$_[0]-> set_focused_item   ($_[1]):return $_[0]-> {focusedItem}    }
sub integralHeight{($#_)?$_[0]-> set_integral_height($_[1]):return $_[0]-> {integralHeight} }
sub integralWidth {($#_)?$_[0]-> set_integral_width ($_[1]):return $_[0]-> {integralWidth } }
sub itemHeight    {($#_)?$_[0]-> set_item_height    ($_[1]):return $_[0]-> {itemHeight}     }
sub itemWidth     {($#_)?$_[0]-> set_item_width     ($_[1]):return $_[0]-> {itemWidth}      }
sub multiSelect   {($#_)?$_[0]-> set_multi_select   ($_[1]):return $_[0]-> {multiSelect}    }
sub multiColumn   {($#_)?$_[0]-> set_multi_column   ($_[1]):return $_[0]-> {multiColumn}    }
sub offset        {($#_)?$_[0]-> set_offset         ($_[1]):return $_[0]-> {offset}         }
sub selectedCount {($#_)?$_[0]-> raise_ro("selectedCount") :return $_[0]-> get_selected_count;}
sub selectedItems {($#_)?shift-> set_selected_items    (@_):return $_[0]-> get_selected_items;}
sub topItem       {($#_)?$_[0]-> set_top_item       ($_[1]):return $_[0]-> {topItem}        }
sub vertical      {($#_)?$_[0]-> set_vertical       ($_[1]):return $_[0]-> {vertical}        }

# section for item text representation

sub get_item_text
{
	my ( $self, $index) = @_;
	my $txt = '';
	$self-> notify(q(Stringify), $index, \$txt);
	return $txt;
}

sub get_item_width
{
	my ( $self, $index) = @_;
	my $w = 0;
	$self-> notify(q(MeasureItem), $index, \$w);
	return $w;
}

sub on_stringify
{
	my ( $self, $index, $sref) = @_;
	$$sref = '';
}


sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	$$sref = 0;
}

sub draw_text_items
{
	my ( $self, $canvas, $first, $last, $step, $x, $y, $textShift, $clipRect) = @_;
	my ($i,$j);
	my ($dx,$iw) = (0);
	if ( $self->{align} != ta::Left ) {
		my @a = $self->item2rect( $first );
		$iw = $a[2] - $a[0];
		$iw = $self->{itemWidth} if $iw < $self->{itemWidth};
	}
	for ( $i = $first, $j = 1; $i <= $last; $i += $step, $j++) {
		my $width = $self-> get_item_width( $i);
		next if $width + $self-> {offset} + $x + 1 < $clipRect-> [0];
		if ( $self->{align} == ta::Center) {
			$dx = ($iw > $width) ? ($iw - $width) / 2 : 0;
		} elsif ( $self->{align} == ta::Right) {
			$dx = ($iw > $width) ? $iw - $width : 0;
		}
		$canvas-> text_shape_out( $self-> get_item_text( $i),
			$x + $dx, $y + $textShift - $j * $self-> {itemHeight} + 1
		);
	}
}

sub std_draw_text_items
{
	my ($self,$canvas) = (shift,shift);
	my @clrs = (
		$self-> color,
		$self-> backColor,
		$self-> colorIndex( ci::HiliteText),
		$self-> colorIndex( ci::Hilite)
	);

	my @clipRect = $canvas-> clipRect;
	my $i;
	my $drawVeilFoc = -1;
	my $atY    = int(( $self-> {itemHeight} - $canvas-> font-> height) / 2 + .5);
	my $ih     = $self-> {itemHeight};
	my $offset = $self-> {offset};
	my $step   = ( $self-> {multiColumn} and !$self-> {vertical}) ?
		$self-> {active_columns} : 1;

	my @colContainer;
	for ( $i = 0; $i < $self-> {columns}; $i++){
		push ( @colContainer, [])
	};
	for ( $i = 0; $i < scalar @_; $i++) {
		push ( @{$colContainer[ $_[$i]-> [8]]}, $_[$i]);
		$drawVeilFoc = $i if $_[$i]-> [6];
	}
	my ( $lc, $lbc) = @clrs[0,1];
	for ( @colContainer) {
		my @normals;
		my @selected;
		my @prelight;
		my ( $lastNormal, $lastSelected) = (undef, undef);
		# sorting items in single column
		{ $_ = [ sort { $$a[0]<=>$$b[0] } @$_]; }
		# calculating conjoint bars
		for ( $i = 0; $i < scalar @$_; $i++) {
			my ( $itemIndex, $x, $y, $x2, $y2, $selected, $focusedItem, $prelighted) = @{$$_[$i]};
			if ( $prelighted ) {
				push ( @prelight, [
					$x, $y, $x2, $y2,
					$$_[$i]-> [0], $$_[$i]-> [0], $selected ? 3 : 2,
				]);
			} elsif ( $selected) {
				if (
					defined $lastSelected &&
					( $y2 + 1 == $lastSelected)
				) {
					${$selected[-1]}[1] = $y;
					${$selected[-1]}[5] = $$_[$i]-> [0];
				} else {
					push ( @selected, [
						$x, $y, $x2, $y2,
						$$_[$i]-> [0], $$_[$i]-> [0], 1
					]);
				}
				$lastSelected = $y;
			} else {
				if (
					defined $lastNormal &&
					( $y2 + 1 == $lastNormal) &&
					( ${$normals[-1]}[3] - $lastNormal < 100))
				{
					${$normals[-1]}[1] = $y;
					${$normals[-1]}[5] = $$_[$i]-> [0];
				} else {
					push ( @normals, [
						$x, $y, $x2, $y2,
						$$_[$i]-> [0], $$_[$i]-> [0], 0
					]);
				}
				$lastNormal = $y;
			}
		}
		# draw items

		for ( @normals, @selected, @prelight) {
			my ( $x, $y, $x2, $y2, $first, $last, $selected) = @$_;
			my $c;
			my $prelight;
			if ($selected & 2) {
				$selected -= 2;
				$prelight = 1;
			}

			$c = $clrs[ $selected ? 3 : 1];
			if ( $c != $lbc) {
				$canvas-> backColor( $c);
				$lbc = $c;
			}

			$self-> draw_item_background( $canvas, $x, $y, $x2, $y2, $prelight);

			$c = $clrs[ $selected ? 2 : 0];
			if ( $c != $lc) {
				$canvas-> color( $c);
				$lc = $c;
			}

			$self-> draw_text_items( $canvas, $first, $last, $step,
				$x, $y2, $atY, \@clipRect);
		}
	}

	# draw veil
	if ( $drawVeilFoc >= 0) {
		my ( $itemIndex, $x, $y, $x2, $y2) = @{$_[$drawVeilFoc]};
		$canvas-> rect_focus( $x + $self-> {offset}, $y, $x2, $y2);
	}
}

package Prima::AbstractListBox;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractListViewer);

sub draw_items
{
	shift-> std_draw_text_items(@_);
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> get_text_width( $self-> get_item_text( $index));
}

package Prima::ListViewer;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractListViewer);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		items         => [],
		autoWidth     => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {items}      = [];
	$self-> {widths}     = [];
	$self-> {maxWidth}   = 0;
	$self-> {autoWidth}  = 0;

	my %profile = $self-> SUPER::init(@_);

	$self-> autoWidth( $profile{autoWidth});
	$self-> items    ( $profile{items});
	$self-> focusedItem  ( $profile{focusedItem});
	return %profile;
}


sub calibrate
{
	my $self = $_[0];
	$self-> recalc_widths;
	$self-> itemWidth( $self-> {maxWidth}) if $self-> {autoWidth};
	$self-> offset( $self-> offset);
}

sub get_item_width
{
	return $_[0]-> {widths}-> [$_[1]];
}

sub on_fontchanged
{
	my $self = $_[0];

	$self-> itemHeight( $self-> font-> height), $self-> {autoHeight} = 1 if $self-> { autoHeight};
	$self-> calibrate;
}

sub recalc_widths
{
	my $self = $_[0];

	my @w;
	my $maxWidth = 0;
	my $i;

	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
	$self-> push_event;
	$self-> begin_paint_info;

	for ( $i = 0; $i < scalar @{$self-> {items}}; $i++) {
		my $iw = 0;
		$notifier-> ( @notifyParms, $i, \$iw);
		$maxWidth = $iw if $maxWidth < $iw;
		push ( @w, $iw);
	}

	$self-> end_paint_info;
	$self-> pop_event;
	$self-> {widths}    = [@w];
	$self-> {maxWidth} = $maxWidth;
}

sub set_items
{
	my ( $self, $items) = @_;
	return unless ref $items eq q(ARRAY);

	my $oldCount = $self-> {count};
	$self-> {items} = [@{$items}];
	$self-> recalc_widths;
	$self-> reset;
	scalar @$items == $oldCount ? $self-> repaint : $self-> SUPER::count( scalar @$items);

	$self-> itemWidth( $self-> {maxWidth}) if $self-> {autoWidth};
	$self-> offset( $self-> offset);
	$self-> selectedItems([]);
}

sub get_items
{
	my $self = shift;
	my @inds = (@_ == 1 and ref($_[0]) eq q(ARRAY)) ? @{$_[0]} : @_;

	my ($c,$i) = ($self-> {count}, $self-> {items});
	for ( @inds) { $_ = ( $_ >= 0 && $_ < $c) ? $i-> [$_] : undef; }
	return wantarray ? @inds : $inds[0];
}

sub insert_items
{
	my ( $self, $where) = ( shift, shift);
	$where = $self-> {count} if $where < 0;
	my ( $is, $iw, $mw) = ( $self-> {items}, $self-> {widths}, $self-> {maxWidth});
	if (@_ == 1 and ref($_[0]) eq q(ARRAY)) {
		return unless scalar @{$_[0]};
		$self-> {items} = [@{$_[0]}];
	} else {
		return unless scalar @_;
		$self-> {items} = [@_];
	}

	$self-> {widths} = [];
	my $num = scalar @{$self-> {items}};
	$self-> recalc_widths;
	splice( @{$is}, $where, 0, @{$self-> {items}});
	splice( @{$iw}, $where, 0, @{$self-> {widths}});
	( $self-> {items}, $self-> {widths}) = ( $is, $iw);
	$self-> itemWidth( $self-> {maxWidth} = $mw)
		if $self-> {autoWidth} && $self-> {maxWidth} < $mw;

	$self-> SUPER::count( scalar @{$self-> {items}});

	$self-> itemWidth( $self-> {maxWidth}) if $self-> {autoWidth};
	$self-> focusedItem( $self-> {focusedItem} + $num)
		if $self-> {focusedItem} >= 0 && $self-> {focusedItem} >= $where;
	$self-> offset( $self-> offset);

	my @shifters;
	for ( keys %{$self-> {selectedItems}}) {
		next if $_ < $where;
		push ( @shifters, $_);
	}
	for ( @shifters) { delete $self-> {selectedItems}-> {$_};     }
	for ( @shifters) { $self-> {selectedItems}-> {$_ + $num} = 1; }
	$self-> repaint if scalar @shifters;
}

sub replace_items
{
	my ( $self, $where) = ( shift, shift);
	return if $where < 0;

	my ( $is, $iw) = ( $self-> {items}, $self-> {widths});
	my $new;
	if (@_ == 1 and ref($_[0]) eq q(ARRAY)) {
		return unless scalar @{$_[0]};
		$new = [@{$_[0]}];
	} else {
		return unless scalar @_;
		$new = [@_];
	}

	my $num = scalar @$new;
	if ( $num + $where >= $self-> {count}) {
		$num = $self-> {count} - $where;
		return if $num <= 0;
		splice @$new, $num;
	}

	$self-> {items} = $new;
	$self-> {widths} = [];
	$self-> recalc_widths;
	splice( @{$is}, $where, $num, @{$self-> {items}});
	splice( @{$iw}, $where, $num, @{$self-> {widths}});
	( $self-> {items}, $self-> {widths}) = ( $is, $iw);

	if ( $self-> {autoWidth}) {
		my $mw = 0;
		for (@{$iw}) {
			$mw = $_ if $mw < $_;
		}
		$self-> itemWidth( $self-> {maxWidth} = $mw);
		$self-> offset( $self-> offset);
	}

	if ( $where <= $self-> {lastItem} && $where + $num >= $self-> {topItem}) {
		$self-> redraw_items( $where .. $where + $num);
	}
}

sub add_items { shift-> insert_items( -1, @_); }

sub delete_items
{
	my $self = shift;
	my ( $is, $iw, $mw) = ( $self-> {items}, $self-> {widths}, $self-> {maxWidth});

	my %indices;
	if (@_ == 1 and ref($_[0]) eq q(ARRAY)) {
		return unless scalar @{$_[0]};
		%indices = map{$_=>1}@{$_[0]};
	} else {
		return unless scalar @_;
		%indices = map{$_=>1}@_;
	}

	my @removed;
	my $wantarray = wantarray;
	my @newItems;
	my @newWidths;
	my $i;
	my $num = scalar keys %indices;
	my ( $items, $widths) = ( $self-> {items}, $self-> {widths});

	$self-> focusedItem( -1) if exists $indices{$self-> {focusedItem}};

	for ( $i = 0; $i < scalar @{$self-> {items}}; $i++) {
		unless ( exists $indices{$i}) {
			push ( @newItems,  $$items[$i]);
			push ( @newWidths, $$widths[$i]);
		} else {
			push ( @removed, $$items[$i]) if $wantarray;
		}
	}

	my $newFoc = $self-> {focusedItem};
	for ( keys %indices) { $newFoc-- if $newFoc >= 0 && $_ < $newFoc; }

	my @selected = sort {$a<=>$b} keys %{$self-> {selectedItems}};
	$i = 0;
	my $dec = 0;
	my $d;
	for $d ( sort {$a<=>$b} keys %indices) {
		while ($i < scalar(@selected) and $d > $selected[$i]) { $selected[$i] -= $dec; $i++; }
		last if $i >= scalar @selected;
		$selected[$i++] = -1 if $d == $selected[$i];
		$dec++;
	}
	while ($i < scalar(@selected)) { $selected[$i] -= $dec; $i++; }
	$self-> {selectedItems} = {};
	for ( @selected) {$self-> {selectedItems}-> {$_} = 1;}
	delete $self-> {selectedItems}-> {-1};

	( $self-> {items}, $self-> {widths}) = ([@newItems], [@newWidths]);
	my $maxWidth = 0;
	for ( @newWidths) { $maxWidth = $_ if $maxWidth < $_; }

	$self-> lock;
	$self-> itemWidth( $self-> {maxWidth} = $maxWidth)
	if $self-> {autoWidth} && $self-> {maxWidth} > $maxWidth;
	$self-> SUPER::count( scalar @{$self-> {items}});
	$self-> focusedItem( $newFoc);
	$self-> unlock;

	return @removed if $wantarray;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	$self-> notify(q(MouseUp),0,0,0) if defined $self-> {mouseTransaction};
	return if $mod & km::DeadKey;

	if (
		(( $code & 0xFF) >= ord(' ')) &&
		( $key == kb::NoKey) &&
		!($mod & (km::Ctrl|km::Alt)) &&
		$self-> {count}
	) {
		my $i;
		my ( $c, $hit, $items) = ( lc chr ( $code & 0xFF), undef, $self-> {items});
		for ( $i = $self-> {focusedItem} + 1; $i < $self-> {count}; $i++) {
			my $fc = substr( $self-> get_item_text($i), 0, 1);
			next unless defined $fc;
			$hit = $i, last if lc $fc eq $c;
		}
		unless ( defined $hit) {
			for ( $i = 0; $i < $self-> {focusedItem}; $i++) {
				my $fc = substr( $self-> get_item_text($i), 0, 1);
				next unless defined $fc;
				$hit = $i, last if lc $fc eq $c;
			}
		}
		if ( defined $hit) {
			$self-> focusedItem( $hit);
			$self-> clear_event;
			return;
		}
	}
	$self-> SUPER::on_keydown( $code, $key, $mod);
}

sub on_dragitem
{
	my ( $self, $from, $to) = @_;
	my ( $is, $iw) = ( $self-> {items}, $self-> {widths});
	if ( $self-> {multiSelect}) {
		my @k = sort { $b <=> $a } keys %{$self-> {selectedItems}};
		my @is = @$is[@k];
		my @iw = @$iw[@k];
		my $nto = $to;
		for my $k ( @k) {
			$nto-- if $k <= $to;
			splice( @$is, $k, 1);
			splice( @$iw, $k, 1);
		}
		$nto++ if $nto != $to;
		splice( @$is, $nto, 0, reverse @is);
		splice( @$iw, $nto, 0, reverse @iw);
		@{$self-> {selectedItems}}{$nto .. $nto + @k - 1} =
			delete @{$self-> {selectedItems}}{@k};
	} else {
		splice( @$is, $to, 0, splice( @$is, $from, 1));
		splice( @$iw, $to, 0, splice( @$iw, $from, 1));
	}
	$self-> repaint;
	$self-> clear_event;
}

sub autoWidth     {($#_)?$_[0]-> {autoWidth} = $_[1]       :return $_[0]-> {autoWidth}      }
sub count         {($#_)?$_[0]-> raise_ro('count')         :return $_[0]-> {count}          }
sub items         {($#_)?$_[0]-> set_items( $_[1])         :return $_[0]-> {items}          }

package Prima::ProtectedListBox;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);

BEGIN {
	for ( qw(
		font color backColor rop rop2
		linePattern lineWidth lineEnd textOutBaseline
		fillPattern clipRect)
	) {
		my $sb = $_;
		$sb =~ s/([A-Z]+)/"_\L$1"/eg;
		$sb = "set_$sb";
		eval <<PROC;
	sub $sb
	{
		my \$self = shift;
		\$self->SUPER::$sb(\@_);
		\$self->{protect}->{$_} = 1 if exists \$self->{protect};
	}
PROC
	}
}

sub draw_items
{
	my ( $self, $canvas, @items) = @_;
	return if $canvas != $self;   # this does not support 'uncertain' drawings due that
	my %protect;                  # it's impossible to override $canvas's methods dynamically
	for ( qw(
		font color backColor rop rop2 linePattern lineWidth
		lineEnd textOutBaseline fillPattern)
	) { $protect{$_} = $canvas-> $_(); }

	my @clipRect = $canvas-> clipRect;
	$self-> {protect} = {};

	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
	$self-> push_event;

	for ( @items) {
		$notifier-> ( @notifyParms, $canvas, @$_);

		$canvas-> clipRect( @clipRect), delete $self-> {protect}-> {clipRect}
			if exists $self-> {protect}-> {clipRect};
		for ( keys %{$self-> {protect}}) { $self-> $_($protect{$_}); }
		$self-> {protect} = {};
	}

	$self-> pop_event;
	delete $self-> {protect};
}

package Prima::ListBox;
use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);

sub get_item_text  { return $_[0]-> {items}-> [$_[1]]; }

sub on_stringify
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> {items}-> [$index];
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	$$sref = $self-> get_text_width( $self-> {items}-> [$index]);
}

sub draw_items
{
	shift-> std_draw_text_items(@_)
}

1;

=pod

=head1 NAME

Prima::Lists - list widgets

=head1 DESCRIPTION

The module provides several listbox classes that differ in the way items in the
list widget are associated with data. The hierarchy of classes is as follows:

	AbstractListViewer
		AbstractListBox
		ListViewer
			ProtectedListBox
			ListBox

The root class C<Prima::AbstractListViewer> provides a common interface that is
though not usable directly.  The main differences between classes are centered
around the way the items are stored. The simplest organization of a text-only
item list, provided by C<Prima::ListBox>, stores an array of text scalars in a
widget. More elaborated storage and representation types are not realized, and
the programmer is urged to use the more abstract classes to derive their own
mechanisms.  For example, for a list of items that contain text strings and
icons see L<Prima::Dialog::FileDialog/"Prima::DirectoryListBox">.  To organize
an item storage different from C<Prima::ListBox> it is usually enough to
overload either the C<Stringify>, C<MeasureItem>, and C<DrawItem> events, or
their method counterparts: C<get_item_text>, C<get_item_width>, and
C<draw_items>.

=head1 Prima::AbstractListViewer

C<Prima::AbstractListViewer> is a descendant of
C<Prima::Widget::GroupScroller>, and some of its properties are not described
here.

The class provides an interface to generic list browsing functionality, plus
functionality for text-oriented lists. The class is not usable directly.

=head2 Properties

=over

=item autoHeight BOOLEAN

If 1, the item height is changed automatically when the widget font is changed;
this is useful for text items.  If 0, the item height is not changed; this is
useful for non-text items.

Default value: 1

=item count INTEGER

An integer property, used to access the number of items in the list.  Since it
is tied to the item storage organization, and hence the possibility of changing
the number of items, this property is often declared as read-only in
descendants of C<Prima::AbstractListViewer>.

=item dragable BOOLEAN

If 1, allows the items to be dragged interactively by pressing the Control key
together with the left mouse button. If 0, item dragging is disabled.

Default value: 1

=item drawGrid BOOLEAN

If 1, vertical grid lines between columns are drawn with C<gridColor>.
Actual only in multi-column mode.

Default value: 1

=item extendedSelect BOOLEAN

Manages the way the user selects multiple items that is only actual when
C<multiSelect> is 1. If 0, the user must click each item to mark it as
selected. If 1, the user can drag the mouse or use the Shift key plus arrow
keys to perform range selection; the Control key can be used to select
individual items.

Default value: 0

=item focusedItem INDEX

Selects the focused item index. If -1, no item is focused.  It is mostly a
run-time property, however, it can be set during the widget creation stage
given that the item list is accessible at this stage as well.

Default value: -1

=item gridColor COLOR

Color used for drawing vertical divider lines for multi-column list widgets.
The list classes support also the indirect way of setting the grid color, as
well as the widget does, via the C<colorIndex> property. To achieve this,
the C<ci::Grid> constant is declared ( for more detail see
L<Prima::Widget/colorIndex> ).

Default value: C<cl::Black>.

=item integralHeight BOOLEAN

If 1, only the items that fit vertically in the widget interiors are drawn. If
0, the partially visible items are drawn also.

Default value: 0

=item integralWidth BOOLEAN

If 1, only the items that fit horizontally in the widget interiors are drawn.
If 0, the partially visible items are drawn also.  Actual only in
the multi-column mode.

Default value: 0

=item itemHeight INTEGER

Selects the height of the items in pixels. Since the list classes do
not support items with variable heights, changes to this property
affect all items.

Default value: default font height

=item itemWidth INTEGER

Selects the width of the items in pixels. Since the list classes do
not support items with variable widths, changes to this property
affect all items.

Default value: default widget width

=item multiSelect BOOLEAN

If 0, the user can only select one item, and it is reported by
the C<focusedItem> property. If 1, the user can select more than one item.
In this case, the C<focusedItem>'th item is not necessarily selected.
To access the selected item list use the C<selectedItems> property.

Default value: 0

=item multiColumn BOOLEAN

If 0, the items are arranged vertically in a single column and the main scroll
bar is vertical. If 1, the items are arranged in several columns, each
C<itemWidth> pixels wide. In this case, the main scroll bar is horizontal.

=item offset INTEGER

Horizontal offset of an item list in pixels.

=item topItem INTEGER

Selects the first item drawn.

=item selectedCount INTEGER

A read-only property. Returns the number of selected items.

=item selectedItems ARRAY

ARRAY is an array of integer indices of selected items.

=item vertical BOOLEAN

Sets the general direction of items in multi-column mode. If 1, items increase
down-to-right. Otherwise, right-to-down.

Doesn't have any effect in single-column mode.
Default value: 1.

=back

=head2 Methods

=over

=item add_selection ARRAY, FLAG

Sets item indices from ARRAY in selected
or deselected state, depending on the FLAG value, correspondingly 1 or 0.

Only for the multi-select mode.

=item deselect_all

Clears the selection

Only for the multi-select mode.

=item draw_items CANVAS, ITEM_DRAW_DATA

Called from within the C<Paint> notification to draw items. The default
behavior is to call the C<DrawItem> notification for every item in the
ITEM_DRAW_DATA array.  ITEM_DRAW_DATA is an array or arrays, where each array
consists of parameters passed to the C<DrawItem> notification.

This method is overridden in some descendant classes to increase the speed of
drawing. For example, C<std_draw_text_items> is the optimized routine for
drawing text-based items. It is used in the C<Prima::ListBox> class.

See L<DrawItem> for the description of the parameters.

=item draw_text_items CANVAS, FIRST, LAST, STEP, X, Y, OFFSET, CLIP_RECT

Called by C<std_draw_text_items> to draw a sequence of text items with indices
from FIRST to LAST, by STEP, on CANVAS, starting at point X, Y, and
incrementing the vertical position with OFFSET. CLIP_RECT is a reference to an
array of four integers given in the inclusive-inclusive coordinates that
represent the active clipping rectangle.

Note that OFFSET must be an integer, otherwise bad effects will be observed
when text is drawn below Y=0

=item get_item_text INDEX

Returns the text string assigned to the INDEXth item.
Since the class does not assume the item storage organization,
the text is queried via the C<Stringify> notification.

=item get_item_width INDEX

Returns width in pixels of the INDEXth item.
Since the class does not assume the item storage organization,
the value is queried via the C<MeasureItem> notification.

=item is_selected INDEX

Returns 1 if the INDEXth item is selected, 0 otherwise.

=item item2rect INDEX, [ WIDTH, HEIGHT ]

Calculates and returns four integers with rectangle coordinates of the INDEXth
item. WIDTH and HEIGHT are optional parameters with pre-fetched dimensions of
the widget; if not set, the dimensions are queried by calling the C<size>
property. If set, however, the C<size> property is not called, thus some
speed-up can be achieved.

=item point2item X, Y

Returns the index of an item that contains the point at (X,Y). If the point
belongs to the item outside the widget's interior, returns the index
of the first item outside the widget's interior in the direction of the point.

=item redraw_items INDICES

Redraws all items in the INDICES array.

=item select_all

Selects all items.

Only for the multi-select mode.

=item set_item_selected INDEX, FLAG

Sets the selection flag on the INDEXth item.
If FLAG is 1, the item is selected. If 0, it is deselected.

Only for the multi-select mode.

=item select_item INDEX

Selects the INDEXth item.

Only for the multi-select mode.

=item std_draw_text_items CANVAS, ITEM_DRAW_DATA

An optimized method, draws text-based items. It is fully compatible with the
C<draw_items> interface and is used in the C<Prima::ListBox> class.

The optimization is derived from the assumption that items maintain common
background and foreground colors, that only differ in the selected and
non-selected states. The routine groups drawing requests for selected and
non-selected items, and then draws items with a reduced number of calls to the
C<color> property.  While the background is drawn by the routine itself, the
foreground ( usually text ) is delegated to the C<draw_text_items> method, so
that the text positioning and eventual decorations would be easier to implement.

ITEM_DRAW_DATA is an array of arrays of scalars, where each array contains
parameters of the C<DrawItem> notification.  See L<DrawItem> for the 
description of the parameters.

=item toggle_item INDEX

Toggles selection of the INDEXth item.

Only for the multi-select mode.

=item unselect_item INDEX

Deselects the INDEXth item.

Only for the multi-select mode.

=back

=head2 Events

=over

=item Click

Called when the user presses the return key or double-clicks on
an item. The index of the item is stored in C<focusedItem>.

=item DragItem OLD_INDEX, NEW_INDEX

Called when the user finishes the drag of an item
from OLD_INDEX to NEW_INDEX position. The default action
rearranges the item list according to the dragging action.

=item DrawItem CANVAS, INDEX, X1, Y1, X2, Y2, SELECTED, FOCUSED, PRELIGHT, COLUMN

Called when the INDEXth item is to be drawn on CANVAS.  X1, Y1, X2, Y2 define
the item rectangle in widget coordinates where the item is to be drawn.
SELECTED, FOCUSED, and PRELIGHT are boolean flags, if the item must be drawn
correspondingly in selected and focused states, with or without the prelight
effect.

=item MeasureItem INDEX, REF

Stores width in pixels of the INDEXth item into the REF
scalar reference. This notification must be called
from within the C<begin_paint_info/end_paint_info> block.

=item SelectItem INDEX, FLAG

Called when an item changes its selection state.
INDEX is the index of the item, FLAG is its new selection
state: 1 if it is selected, 0 if it is not.

=item Stringify INDEX, TEXT_REF

Stores the text string associated with the INDEXth item into TEXT_REF
scalar reference.

=back

=head1 Prima::AbstractListBox

The same as its ascendant C<Prima::AbstractListViewer> except that it does not
the propagate C<DrawItem> message, assuming that all items must be drawn as
text strings.

=head1 Prima::ListViewer

The class implements an item storage mechanism but leaves the definition of
the format of the item to the programmer. The items are accessible via the
C<items> property and several other helper routines.

The class also defines user navigation by accepting character
keyboard input and jumping to the items that have text assigned
with the first letter that matches the input.

C<Prima::ListViewer> is derived from C<Prima::AbstractListViewer>.

=head2 Properties

=over

=item autoWidth BOOLEAN

Selects if the item width must be recalculated automatically
when either the font or item list is changed.

Default value: 1

=item count INTEGER

A read-only property; returns the number of items.

=item items ARRAY

Accesses the storage array of the items. The format of items is not
defined, it is merely treated as one scalar per index.

=back

=head2 Methods

=over

=item add_items ITEMS

Appends an array of ITEMS to the end of the item list.

=item calibrate

Recalculates all item widths. Adjusts C<itemWidth> if
C<autoWidth> is set.

=item delete_items INDICES

Deletes items from the list. INDICES can be either an array
or a reference to an array of item indices.

=item get_item_width INDEX

Returns the width in pixels of the INDEXth item from the internal cache.

=item get_items INDICES

Returns an array of items. INDICES can be either an array or a reference to an
array of item indices.  Depending on the caller context, the results are
different: in the array context the item list is returned; in scalar - only the
first item from the list.

=item insert_items OFFSET, ITEMS

Inserts an array of items at the OFFSET index in the list.  Offset must be a valid
index; to insert items at the end of the list use the C<add_items> method.

ITEMS can be either an array or a reference to an array of items.

=item replace_items OFFSET, ITEMS

Replaces existing items at the OFFSET index in the list. The offset must be a
valid index.

ITEMS can be either an array or a reference to an array of items.

=back

=head1 Prima::ProtectedListBox

A semi-demonstrational class derived from C<Prima::ListViewer>, implements
certain protections for every item during drawing.  Assuming that several item
drawing algorithms can be used in the same widget, C<Prima::ProtectedListBox>
provides a safety layer between these. If an algorithm selects a font or a
color and does not restore the old value, this does not affect the outlook of
other items.

This functionality is implemented by overloading the C<draw_items>
method and also all graphic properties.

=head1 Prima::ListBox

Descendant of C<Prima::ListViewer>, declares that an item must be a single
text string. Incorporating all the functionality of its predecessors, provides
the standard workhorse listbox widget.

=head2 Synopsis

	my $lb = Prima::ListBox-> create(
		items       => [qw(First Second Third)],
		focusedItem => 2,
		onClick     => sub {
			print $_[0]-> get_items( $_[0]-> focusedItem), " is selected\n";
		}
	);

=head2 Methods

=over

=item get_item_text INDEX

Returns the text string associated with the INDEXth item.
Since the item storage organization is implemented, does
so without calling the C<Stringify> notification.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::ComboBox>, L<Prima::Dialog::FileDialog>, F<examples/editor.pl>

=cut
