#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  Modifications by Anton Berezin <tobez@tobez.org>
package Prima::Widget::Header;

use strict;
use warnings;
use Prima::Classes;
use base qw(
	Prima::Widget
	Prima::Widget::Fader
);

use constant CaptureBrimWidth => 2;

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	%{Prima::Widget::Fader-> notification_types()},
	DrawItem    => nt::Action,
	MeasureItem => nt::Action,
	MoveItem    => nt::Action,
	SizeItem    => nt::Action,
	SizeItems   => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		offset      => 0,
		items       => [],
		widths      => [],
		pressed     => -1,
		clickable   => 1,
		scalable    => 1,
		draggable   => 1,
		minTabWidth => 2,
		vertical    => 0,
		selectable  => 0,
		syncPaint   => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	if ( exists $p->{dragable} ) {
		warn "Property `dragable` is renamed to `draggable`, please adapt your code";
		$p->{draggable} = delete $p->{dragable};
	}
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self = shift;
	$self-> {$_} = 0 for qw(offset count maxWidth clickable scalable minTabWidth vertical draggable);
	$self-> {$_} = -1 for qw(pressed);
	$self-> {widths} = [];
	$self-> {items} = [];
	my %profile = $self-> SUPER::init(@_);
	$self-> {fontHeight} = $self-> font-> height;
	$self-> {resetDisabled} = 1;
	$self-> $_( $profile{$_})
		for ( qw( vertical minTabWidth items widths offset pressed clickable scalable draggable));
	if ( scalar @{$profile{widths}} == 0) {
		$self-> autowidths;
		$self-> repaint;
	}
	return %profile;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size = $canvas-> size;
	my @c = $self-> enabled ?
	( $self-> color, $self-> backColor) :
	( $self-> disabledColor, $self-> disabledBackColor);
	my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);

	my ($prelightPart, $prelightColor) = (-1);
	if ( defined $self->{prelight} ) {
		$prelightColor = $self-> fader_prelight_color($c[1]);
		$prelightPart = $self->{prelight};
	}
	my $flat = $self->skin eq 'flat';
	my $showlines;

	if ( $flat ) {
		$canvas-> backColor($c[1]);
		$canvas-> clear;
		$showlines = $self->{mouse_in};
	} else {
		$canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, 1, @c3d, $c[1]);
		$showlines = 1;
	}
	my $v = $self-> {vertical};
	my ( $x, $y) = ( - $self-> {offset}, ( $size[1] - $self-> {fontHeight}) / 2);
	my $i;

	my $pressed = $self-> {pressed};
	@c3d = reverse @c3d if $v;
	my ( $wx, $cx) = ( $self-> {widths}, $self-> {count});
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
	$self-> push_event;
	my ( $d, $lim) = $v ? ( $x, $size[1]) : ( $x, $size[0]);
	for ( $i = 0; $i < $cx; $i++) {
		next unless $$wx[$i];
		if ( $d + $$wx[$i] + 2 < 1) {
			$d += $$wx[$i] + 2;
			next;
		}
		my $mx = ( $d + $$wx[$i] + 1 > $lim - 2) ? ($lim - 2) : ($d + $$wx[$i] + 1);
		$v ?
			$canvas-> clipRect( 1, $d < 1 ? 1 : $d, $size[0] - 2, $mx) :
			$canvas-> clipRect( $d < 1 ? 1 : $d, 1, $mx, $size[1] - 2);
		if ( $i == $pressed && $flat ) {
			$canvas-> color($self->hiliteBackColor);
			$canvas-> bar(0,0,@size);
			$canvas-> color($self->hiliteColor);
		} elsif ( $i == $prelightPart) {
			$canvas-> color($prelightColor);
			$canvas-> bar(0,0,@size);
			$canvas-> color( $c[0]);
		} else {
			$canvas-> color( $c[0]);
		}

		$v ?
			$notifier-> ( @notifyParms, $canvas, $i, 1, $d + 1, $size[0] - 2, $mx - 1, $d + 4) :
			$notifier-> ( @notifyParms, $canvas, $i, $d + 1, 1, $mx - 1, $size[1] - 2, $y);
		goto NO_LINES if $flat;
		if ( $i == $pressed) {
			$canvas-> color( $c3d[1]);
			$v ?
				$canvas-> line( $size[0] - 2, $d, $size[0] - 2, $d + $$wx[$i]) :
				$canvas-> line( $d, $size[1] - 2, $d + $$wx[$i], $size[1] - 2);
		} else {
			$canvas-> color( $c3d[0]);
		}
		$v ?
			$canvas-> line( 1, $d, $size[0] - 2, $d) :
			$canvas-> line( $d, 1, $d, $size[1] - 2);
		if ( $i == $pressed) {
			$canvas-> color( $c3d[0]);
			$v ?
				$canvas-> line( 1, $d, 1, $d + $$wx[$i]) :
				$canvas-> line( $d, 1, $d + $$wx[$i], 1);
		} else {
			$canvas-> color( $c3d[1]);
		}
	NO_LINES:
		$d += $$wx[$i] + 1;
		$v ?
			$canvas-> line( 1, $d, $size[0] - 2, $d) :
			$canvas-> line( $d, 1, $d, $size[1] - 2)
			if $showlines;
		last if $d > $lim - 3;
		$d++;
	}
	$self-> pop_event;
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> {fontHeight} = $self-> font-> height;
}

sub on_drawitem
{
	my ( $self, $canvas, $index, $left, $bottom, $right, $top, $y) = @_;
	$canvas-> text_shape_out( $self-> {items}-> [$index], $left, $y);
}

sub on_measureitem
{
	my ( $self, $index, $result) = @_;
	$$result = $self-> {vertical} ?
		$self-> {fontHeight} :
		$self-> get_text_width( $self-> {items}-> [$index]);
}

sub point2area
{
	my ( $self, $x, $y, $useBorders) = @_;
	my $i;
	my $pressable = $self-> {clickable} || $self-> {draggable};
	return if !$self-> {scalable} && !$pressable;
	my $lim;
	if ( $self-> {vertical}) {
		return undef if ( $x < 1 || $x > $self-> width - 1) && !$useBorders;
		$lim = $y;
	} else {
		return undef if ( $y < 1 || $y > $self-> height - 1) && !$useBorders;
		$lim = $x;
	}

	my $cbw = $self-> {scalable} ? CaptureBrimWidth : 0;
	my $sx = - $self-> {offset} + 1 + $cbw;
	my $c = $self-> {count};
	my $wx = $self-> {widths};
	for ( $i = 0; $i < $c; $i++) {
		next unless $$wx[$i];
		$sx += $$wx[$i] - $cbw * 2;
		if ( $lim < $sx) {
			return $pressable ? $i : undef;
		}
		$sx += $cbw * 2 + 2;
		if ( $lim < $sx) {
			return $self-> {scalable} ? -($i+1) : $i;
		}
	}
	return undef;
}

sub tab2offset
{
	my ( $self, $item) = @_;
	my $i;
	my $c = $self-> {count};
	my $x = 1;
	for ( $i = 0; $i < $item; $i++) {
		next unless $self-> {widths}-> [$i];
		$x += $self-> {widths}-> [$i] + 2;
	}
	return $x;
}

sub tab2rect
{
	my ( $self, $id) = @_;
	my $offset = $self-> tab2offset( $id) - $self-> {offset} - 1;
	return $self-> {vertical} ?
		( 1, $offset, $self-> width - 1, $offset + $self-> {widths}-> [$id] + 2) :
		( $offset, 1, $offset + $self-> {widths}-> [$id] + 2, $self-> height - 1);
}

sub reset_transaction
{
	my $self = $_[0];
	my $lim = $self-> {vertical} ? $self-> height : $self-> width;
	$self-> {swidth} = $self-> tab2offset( $self-> {tabId}) - $self-> {offset};
	$self-> {maxwidth} = $lim - $self-> {swidth} - 2;
	$self-> {maxwidth} -= $self-> {minTabWidth} if $self-> {tabId} < $self-> {count} - 1;
	if ( $self-> {swidth} < 0) {
		$self-> {minwidth} = -$self-> {swidth} - 1;
		$self-> {minwidth} = $self-> {minTabWidth}
			if $self-> {minwidth} > $self-> {minTabWidth};
	} else {
		$self-> {minwidth} = $self-> {minTabWidth};
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $btn == mb::Left;
	return if $self-> {transaction};
	my $id = $self-> point2area( $x, $y);
	return unless defined $id;
	$self-> capture(1);
	if ( $id < 0) {
		$self-> {transaction} = 2;
		$self-> {anchor} = $self-> {vertical} ? $y : $x;
		$self-> {tabId}  = - $id - 1;
		$self-> {owidth} = $self-> {widths}-> [$self-> {tabId}];
		$self-> reset_transaction;
	} else {
		$self-> {transaction} = 1;
		$self-> {tabId} = $id;
		$self-> pressed( $id);
		$self-> {clickAllowed} = $self-> {clickable};
		$self-> {anchor} = $self-> {vertical} ? $y : $x;
		$self-> {anchor} -= $self-> tab2offset( $id) - $self-> {offset};
	}
	$self-> {pointerPos} = [$self-> pointerPos];
	delete $self-> {pointerSet};
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {transaction};
	return unless $btn == mb::Left;
	my $id = $self-> point2area( $x, $y);

	$self-> capture(0);
	if ( $self-> {transaction} == 1) {
		my @a = $self-> tab2rect( $self-> {tabId});
		if ( $x >= $a[0] && $x < $a[2] && $y >= $a[1] && $y < $a[3]) {
			$self-> notify(q(Click), $self-> {tabId}) if $self-> {clickAllowed};
		}
		$self-> pressed(-1);
	} else {
		$self-> recalc_maxwidth;
	}
	$self-> {transaction} = undef;
}


sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return if $self->{no_mouse_move};
	unless ( $self-> {transaction}) {
		if ( $self-> enabled ) {
			my $p = $self-> point2area( $x, $y);
			my $ptr;
			if ( defined $p && $p < 0) {
				$ptr = $self-> {vertical} ? cr::SizeNS : cr::SizeWE;
			} elsif ( $self-> {draggable} && !$self-> {clickable} && defined $p) {
				$ptr = cr::Move;
			} else {
				$ptr = cr::Default;
			}
			$self-> pointer( $ptr);

			my $prelight = (defined($p) && $p >= 0) ? $p : undef;
			if (( $prelight // -1 ) != ( $self->{prelight} // -1)) {
				if ( defined($self->{prelight} = $prelight)) {
					$self->fader_in_mouse_enter;
				} else {
					$self->fader_out_mouse_leave;
				}
			}

		}
		return;
	}

	if ( $self-> {transaction} == 1) {
		my @a = $self-> tab2rect( $self-> {tabId});
		$self-> pressed(
			( $x >= $a[0] && $x < $a[2] && $y >= $a[1] && $y < $a[3]) ?
			$self-> {tabId} : -1
		);
		return unless $self-> {draggable};
		my @ppos = $self-> pointerPos;
		if ( $self-> {clickable} && !$self-> {pointerSet}) {
			my @p = @{$self-> {pointerPos}};
			if ( abs( $p[0] - $ppos[0]) > 2 || abs( $p[1] - $ppos[1]) > 2) {
				$self-> pointer( cr::Move);
				delete $self-> {pointerPos};
				$self-> {pointerSet} = 1;
			}
		}
		my @lx = $self-> {vertical} ? @a[1,3] : @a[0,2];
		my $d  = $self-> {vertical} ? $y : $x;
		return if $d >= $lx[0] && $d < $lx[1];
		my $osc = $self-> {scalable}; $self-> {scalable} = 0;
		my $p = $self-> point2area( $x, $y, 1); # exclude borders
		$self-> {scalable} = $osc;
		my $o = $self-> {tabId};
		return unless defined $p;
		return if $p == $o;
		$self-> {clickAllowed} = 0;
		my $newpos;
		if ( $self-> {widths}-> [$p] > $self-> {widths}-> [$o]) {
			$ppos[$self-> {vertical} ? 1 : 0] +=
				( $self-> {widths}-> [$p] - $self-> {widths}-> [$o]) * (( $p > $o) ? 1 : -1);
			$newpos = 1;
		}

		splice( @{$self-> {items}}, $p, 0, splice( @{$self-> {items}}, $o, 1));
		splice( @{$self-> {widths}}, $p, 0, splice( @{$self-> {widths}}, $o, 1));
		$self-> {tabId} = $p;
		$self-> repaint;
		$self-> notify(q(MoveItem), $o, $p);
		local $self->{no_mouse_move} = 1;
		$self-> pointerPos( @ppos) if $newpos;
	} else {
		my @sz = $self-> size;
		my $d = $self-> {vertical} ? $y : $x;
		my $nw = $self-> {owidth} + $d - $self-> {anchor};
		$nw = $self-> {maxwidth} if $nw > $self-> {maxwidth};
		$nw = $self-> {minwidth} if $nw < $self-> {minwidth};
		$nw = $self-> {minTabWidth} if $nw < $self-> {minTabWidth};
		my $ow = $self-> {widths}-> [$self-> {tabId}];
		return if $nw == $ow;
		$self-> {widths}-> [$self-> {tabId}] = $nw;
		my $o  = $self-> {swidth} + $ow;
		$self-> {maxWidth} += $nw - $ow;
		$self-> {vertical} ?
			$self-> scroll(
				0, $nw - $ow,
				confineRect => [ 1, $o, $sz[0] - 1, $sz[1] - 1 + abs($nw - $ow)],
				clipRect    => [ 1, 1, $sz[0]-1, $sz[1]-1],
			) : $self-> scroll(
				$nw - $ow, 0,
				confineRect => [ $o, 1, $sz[0] - 1 + abs($nw - $ow), $sz[1] - 1],
				clipRect    => [ 1, 1, $sz[0]-1, $sz[1]-1],
			);
		$self-> notify(q(SizeItem), $self-> {tabId}, $ow, $nw);
	}
}

sub on_mouseenter
{
	my $self = shift;
	return unless $self->enabled;
	$self-> fader_in_mouse_enter;
	$self-> {mouse_in} = 1;
}

sub on_mouseleave
{
	my $self = shift;
	return unless $self->enabled;
	$self-> fader_out_mouse_leave;
	delete $self-> {mouse_in};
}

sub on_fadeout
{
	delete shift->{prelight}
}

sub on_mouseclick
{
	$_[0]-> clear_event;
	return unless $_[5];
	shift-> notify(q(MouseDown), @_);
}


sub on_click
{
#	my ( $self, $index) = @_;
}

sub protect
{
	die "Prima::Widget::Header: Cannot change parameters during transaction\n" if $_[0]-> {transaction};
}

sub autowidths
{
	my ($self) = @_;
	my @r = $self-> calc_autowidths;
	$self-> {widths} = \@r;
	$self-> recalc_maxwidth;
	$self-> notify(q(SizeItems));
}

sub calc_autowidths
{
	my $self = $_[0];
	$self-> protect;
	my $cx = $self-> {count};
	my $i;
	$self-> begin_paint_info;
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
	my @r;
	for ( $i = 0; $i < $cx; $i++) {
		my $result = 0;
		$notifier-> ( @notifyParms, $i, \$result);
		$result = $self-> {minTabWidth} if $result < $self-> {minTabWidth};
		push @r, $result;
		next unless $result;
	}
	$self-> end_paint_info;
	return @r;
}

sub recalc_maxwidth
{
	my $self = $_[0];
	my $mxw = 2;
	for ( @{$self-> {widths}}) {
		$mxw += $_ + 2 if $_;
	}
	$self-> {maxWidth} = $mxw;
}

sub offset
{
	return $_[0]-> {offset} unless $#_;
	my ( $self, $offset) = @_;
	$offset = 0 if $offset < 0;
	$offset = $self-> {maxWidth} - 5 if $offset >= $self-> {maxWidth} - 4;
	return if $offset == $self-> {offset};
	$self-> {offset} = $offset;
	$self-> reset_transaction if $self-> {transaction};
	$self-> repaint;
}

sub clickable
{
	return $_[0]-> {clickable} unless $#_;
	$_[0]-> {clickable} = $_[1];
}

sub vertical
{
	return $_[0]-> {vertical} unless $#_;
	return if $_[0]-> {vertical} == $_[1];
	$_[0]-> protect;
	$_[0]-> {vertical} = $_[1];
	$_[0]-> repaint;
}

sub scalable
{
	return $_[0]-> {scalable} unless $#_;
	$_[0]-> {scalable} = $_[1];
}

sub draggable
{
	return $_[0]-> {draggable} unless $#_;
	$_[0]-> {draggable} = $_[1];
}

sub minTabWidth
{
	return $_[0]-> {minTabWidth} unless $#_;
	my $changed = 0;
	my $m = $_[1];
	$m = 0 if $m < 0;
	$_[0]-> {minTabWidth} = $m;
	for (@{$_[0]-> {widths}}) {
		$_ = $m, $changed = 1 if $_ < $m;
	}
	$_[0]-> recalc_maxwidth;
	$_[0]-> notify(q(SizeItems)) if $changed;
}

sub items
{
	unless ( $#_) {
		return wantarray ? @{$_[0]-> {items}} : [@{$_[0]-> {items}}];
	}
	my ( $self, @items) = @_;
	$self-> protect;
	@items = @{$items[0]} if scalar(@items) == 1 && ref($items[0]) eq 'ARRAY';
	$self-> {items} = [@items];
	my $oc = $self-> {count};
	$self-> {count} = scalar @items;
	if ( $oc > $self-> {count}) {
		splice( @{$self-> {widths}}, $self-> {count});
		$self-> notify(q(SizeItems));
	} elsif ( $oc < $self-> {count}) {
		push( @{$self-> {widths}}, (( $self-> {minTabWidth}) x ( $self-> {count} - $oc)));
		$self-> notify(q(SizeItems));
	}
	$self-> recalc_maxwidth;
	$self-> offset( $self-> offset);
	$self-> repaint;
}

sub widths
{
	unless ( $#_) {
		return wantarray ? @{$_[0]-> {widths}} : [@{$_[0]-> {widths}}];
	}
	my ( $self, @widths) = @_;
	$self-> protect;
	@widths = @{$widths[0]} if scalar(@widths) == 1 && ref($widths[0]) eq 'ARRAY';
	$self-> {widths} = [@widths];
	if ( scalar @widths > $self-> {count}) {
		splice( @{$self-> {widths}}, $self-> {count});
	} elsif ( scalar @widths < $self-> {count}) {
		push( @{$self-> {widths}},
			(( $self-> {minTabWidth}) x ( $self-> {count} - scalar @widths)));
	}
	for ( @{$self-> {widths}}) {
		$_ = $self-> {minTabWidth} if $_ < $self-> {minTabWidth};
	}
	$self-> recalc_maxwidth;
	$self-> offset( $self-> offset);
	$self-> repaint;
	$self-> notify(q(SizeItems));
}

sub pressed
{
	return $_[0]-> {pressed} unless $#_;
	my ( $self, $pid) = @_;
	$pid = -1 if $pid < 0 || $pid >= $self-> {count};
	return if $pid == $self-> {pressed};
	my $opid = $self-> {pressed};
	$self-> {pressed} = $pid;
	if (( $opid < 0) || ( $pid < 0)) {
		$self-> invalidate_rect( $self-> tab2rect( ( $pid < 0) ? $opid : $pid));
	} else {
		$self-> repaint;
	}
}

1;

=pod

=head1 NAME

Prima::Widget::Header - multi-column header widget

=head1 DESCRIPTION

The widget class provides functionality of several button-like
caption tabs, that can be moved and resized by the user.
The class was implemented to serve as a table header
for list and grid widgets.

=head1 API

=head2 Events

=over

=item Click INDEX

Called when the user clicks on the tab positioned at INDEX.

=item DrawItem CANVAS, INDEX, X1, Y1, X2, Y2, TEXT_BASELINE

A callback to draw the tabs. CANVAS is the output object; INDEX is the index of
a tab.  X1,Y2,X2,Y2 are the coordinates of the boundaries of the tab rectangle;
TEXT_BASELINE is a pre-calculated vertical position for eventual centered text
output.

=item MeasureItem INDEX, RESULT

Stores in scalar referenced by RESULT the width or height ( depending
on the L<vertical> property value ) of the INDEXth tab, in pixels.

=item MoveItem OLD_INDEX, NEW_INDEX

Called when the user moves a tab from its old location, specified by OLD_INDEX,
to the NEW_INDEX position. By the time of the call, all internal structures are
updated.

=item SizeItem INDEX, OLD_EXTENT, NEW_EXTENT

Called when the user resizes a tab in the INDEXth position. OLD_EXTENT and NEW_EXTENT
are either the width or height of the tab, depending on the L<vertical> property value.

=item SizeItems

Called when more than one tab changes its extent. This might happen as a result
of both user and programmatic actions.

=back

=head2 Properties

=over

=item clickable BOOLEAN

Selects if the user is allowed to click the tabs.

Default value: 1

=item draggable BOOLEAN

Selects if the user is allowed to move the tabs.

Default value: 1

=item items ARRAY

An array of scalars representing the internal data of the tabs.
By default, the scalars are treated as text strings.

=item minTabWidth INTEGER

A minimal extent in pixels a tab must occupy.

Default value: 2

=item offset INTEGER

An offset on the major axis ( depends on the L<vertical> property value )
that the widget is drawn with. Used for the conjunction with list widgets
( see L<Prima::DetailedList> ), when the list is horizontally or
vertically scrolled.

Default value: 0

=item pressed INTEGER

Contains the index of the currently pressed tab. A -1 value is selected
when no tabs are pressed.

Default value: -1

=item scalable BOOLEAN

Selects if the user is allowed to resize the tabs.

Default value: 1

=item vertical BOOLEAN

If 1, the tabs are aligned vertically;
the L<offset>, L<widths> property, and extent parameters of the callback
notification assume the heights of the tabs.

If 0, the tabs are aligned horizontally, and the extent properties
and parameters assume tab widths.

=item widths ARRAY

Array of integer values, corresponding to the extents of the tabs.
The extents are widths ( C<vertical> is 0 ) or heights ( C<vertical> is 1 ).

=back

=head2 Methods

=over

=item tab2offset INDEX

Returns the offset of the INDEXth tab ( without regard to the L<offset> property value ).

=item tab2rect INDEX

Returns four integers representing the rectangle area occupied by
the INDEXth tab ( without regard to the L<offset> property value ).

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::DetailedList>, F<examples/sheet.pl>.

=cut
