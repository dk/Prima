package Prima::Grids;

use strict;
use warnings;
use Prima;

package
    ci;

BEGIN {
	eval 'use constant Grid           => 1 + MaxId;' unless exists $ci::{Grid};
	eval 'use constant IndentCellFore => 2 + MaxId;' unless exists $ci::{IndentCellFore};
	eval 'use constant IndentCellBack => 3 + MaxId;' unless exists $ci::{IndentCellBack};
}

package
    gsci;

use constant COL_INDEX => 0;
use constant ROW_INDEX => 1;
use constant V_FULL    => 2;
use constant V_LEFT    => 3;
use constant V_BOTTOM  => 4;
use constant V_RIGHT   => 5;
use constant V_TOP     => 6;
use constant V_RECT    => 3,4,5,6;
use constant LEFT      => 7;
use constant BOTTOM    => 8;
use constant RIGHT     => 9;
use constant TOP       => 10;
use constant RECT      => 7,8,9,10;

package Prima::AbstractGridViewer;
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
	DrawCell      => nt::Action,
	GetRange      => nt::Action,
	GetAlignment  => nt::Action,
	Measure       => nt::Action,
	SelectCell    => nt::Default,
	SetExtent     => nt::Action,
	Stringify     => nt::Action,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		allowChangeCellHeight   => 0,
		allowChangeCellWidth    => 0,
		cellIndents             => [ 0, 0, 0, 0],
		clipCells               => 1,
		columns                 => 1,
		constantCellWidth       => undef,
		constantCellHeight      => undef,
		drawHGrid               => 1,
		drawVGrid               => 1,
		focusedCell             => [0, 0],
		gridColor               => cl::Black,
		gridGravity             => 3,
		indentCellBackColor     => cl::Gray,
		indentCellColor         => cl::Black,
		leftCell                => 0,
		multiSelect             => 0,
		rows                    => 1,
		topCell                 => 0,
		scaleChildren           => 0,
		selectable              => 1,
		widgetClass             => wc::ListBox,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;

	$self-> {$_} = -1 for qw( leftCell topCell);
	$self-> {$_} = 0 for qw(
		scrollTransaction gridColor dx dy
		leftCell topCell multiSelect visibleCols visibleRows
		indentCellColor indentCellBackColor clipCells cache_geometry_requests
		allowChangeCellWidth allowChangeCellHeight gridGravity
	);
	$self-> {$_} = 1 for qw( drawHGrid drawVGrid columns rows);
	$self-> {focusedCell} = [0,0];
	$self-> {cellIndents} = [0,0,0,0];
	$self-> {selection} = [-1,-1,-1,-1];

	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;

	$self-> $_( $profile{ $_}) for qw(
		allowChangeCellHeight allowChangeCellWidth
		constantCellWidth constantCellHeight
		drawHGrid drawVGrid gridColor 
		columns rows cellIndents
		leftCell topCell multiSelect focusedCell indentCellColor
		indentCellBackColor clipCells gridGravity
	);
	$self-> reset;
	return %profile;
}

sub cache_geometry_requests
{
	my ( $self, $do_cache) = @_;
	return if $self-> {cache_geometry_requests} == $do_cache;
	if (( $self-> {cache_geometry_requests} = $do_cache)) {
		$self-> {geometry_cache_row} = {};
		$self-> {geometry_cache_column} = {};
	} else {
		delete $self-> {geometry_cache_row};
		delete $self-> {geometry_cache_column};
	}
}

sub deselect_all {
	my $self = $_[0];
	$self-> selection(-1,-1,-1,-1);
}

sub draw_cells
{
	my ($self, $canvas, $cols, $rows, $active_area) = @_;
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawCell));
	my @selection = $self-> selection;
	my @f = $self-> focused ? $self-> focusedCell : ( -1, -1);
	$self-> push_event;
	my @prelight    = @{$self->{prelight} // [-1,-1]};
	my ( $xsel, $ysel, $xprelight, $yprelight);
	my ( $clipV, $clipH) = ( $self-> {clipCells} == 1, $self-> {clipCells} == 2);
	for ( @$cols) {
		my ( $col, $xtype, $br, $x1, $x2, $X1, $X2) = @$_;
		$canvas-> clipRect( $x1, $$active_area[1], $x2, $$active_area[3]) if $clipV;
		$xsel = $col >= $selection[0] && $col <= $selection[2];
		$xprelight = $col == $prelight[0];
		for ( @$rows) {
			my ( $row, $ytype, $br, $y1, $y2, $Y1, $Y2) = @$_;
			$ysel = $row >= $selection[1] && $row <= $selection[3] if $xsel;
			$yprelight = $row == $prelight[1];
			$canvas-> clipRect( $x1, $y1, $x2, $y2) if $clipH;
			$notifier-> ( @notifyParms,
				$canvas,
				$col, $row, $xtype || $ytype,
				$x1, $y1, $x2, $y2,
				$X1, $Y1, $X2, $Y2,
				$xsel && $ysel,
				( $col == $f[0] && $row == $f[1]) ? 1 : 0,
				$xprelight && $yprelight
			);
		}
	}
	$self-> pop_event;
}

sub draw_text_cells
{
	my ( $self, $canvas, $screen_rects, $cell_rects, $cell_indices, $font_height) = @_;
	my $i;
	my @clip = $canvas-> clipRect if $self-> {clipCells} == 2;
	for ( $i = 0; $i < @$screen_rects; $i++) {
		my @r = @{$$cell_rects[$i]};
		$canvas-> clipRect( @{$$screen_rects[$i]}) if $self-> {clipCells} == 2;
		my ($ha, $va) = $self-> get_cell_alignment(@{$$cell_indices[$i]});
		my ( $x, $y ) = ($r[0], $r[1]);
		my $text = $self-> get_cell_text( @{$$cell_indices[$i]});
		if ( $ha == ta::Right ) {
			my $dx = $r[2] - $r[0] - $self-> get_text_width($text);
			$x += $dx if $dx > 0;
		} elsif ( $ha == ta::Center ) {
			my $dx = ($r[2] - $r[0] - $self-> get_text_width($text)) / 2;
			$x += $dx if $dx > 0;
		}
		if ( $va == ta::Top ) {
			my $dy = $r[3] - $r[1] - $font_height;
			$y += $dy if $dy > 0;
		} elsif ( $va == ta::Middle ) {
			my $dy = ($r[3] - $r[1] - $font_height) / 2;
			$y += $dy if $dy > 0;
		}
		$canvas-> text_shape_out( $text, $x, $y);
	}
	$canvas-> clipRect( @clip) if $self-> {clipCells} == 2;
}

sub get_cell_area
{
	my ( $self, @size) = @_;
	my @a = $self-> get_active_area( 1, @size);
	my @r;
	my @px = @{$self-> {pixelCellIndents}};
	$r[0] = $a[0] + $px[0];
	$r[1] = $a[1] + $px[3];
	$r[2] = $a[2] - $px[2];
	$r[3] = $a[3] - $px[1];
	if ( $self-> {lastColEmpty}) {
		$r[2]-- if $self-> {drawVGrid};
	}
	if ( $self-> {lastRowEmpty}) {
		$r[3]-- if $self-> {drawHGrid};
	}
	return @r;
}

sub get_cell_text
{
	my ( $self, $col, $row) = @_;
	my $txt = '';
	$self-> notify(q(Stringify), $col, $row, \$txt);
	return $txt;
}

sub get_cell_alignment
{
	my ( $self, $col, $row) = @_;
	my ($ha, $va) = ( ta::Left, ta::Middle );
	$self-> notify(q(GetAlignment), $col, $row, \$ha, \$va);
	return $ha, $va;
}

sub get_range
{
	my ( $self, $vertical, $index) = @_;
	my ( $min, $max) = ( 1, 16384 ); # actually, no real restriction on $max -
					# just a reasonable non-undef value
	$self-> notify(q(GetRange), $vertical, $index, \$min, \$max);
	$min = 1 if $min < 1;
	$max = $min if $max < $min;
	return $min, $max;
}

sub get_screen_cell_info
{
	my ( $self, $x, $y) = @_;
	my ( $colsDraw, $rowsDraw) = ( $self-> {colsDraw}, $self-> {rowsDraw});
	my ( $col, $row, $c, $r, $i);
	$i = 0;
	for ( @$colsDraw) {
		$i++, next unless $x == $$_[0];
		$col = $i;
		$c = $_;
	}
	return unless defined $col;
	$i = 0;
	for ( @$rowsDraw) {
		$i++, next unless $y == $$_[0];
		$row = $i;
		$r = $_;
	}
	return unless defined $row;
	my ( $dx, $dy) = ( $self-> {dx}, $self-> {dy});
	return
		$col, $row,
		(
		( $$c[3] == $$c[5]) && ( $$c[4] == $$c[6]) &&
		( $$r[3] == $$r[5]) && ( $$r[4] == $$r[6])
		),
		$$c[3]-$dx, $$r[3]+$dy, $$c[4]-$dx+1, $$r[4]+$dy+1,
		$$c[5]-$dx, $$r[5]+$dy, $$c[6]-$dx+1, $$r[6]+$dy+1,
	;
}

sub has_selection
{
	return $_[0]-> {selection}-> [0] >= 0;
}

sub point2cell
{
	my ( $self, $x, $y, $NoGrid) = @_;
	my @a = $self-> get_active_area( 0);

	$x += $self-> {dx};
	$y -= $self-> {dy};

	my ($cx, $cy, %hints) = (-2, -2);
	my ( $colsDraw, $rowsDraw) = ( $self-> {colsDraw}, $self-> {rowsDraw});

	# check widget borders first
	if ( $x < $a[0]) {
		# left border
		$cx = -1;
		$hints{x} = -1;
	} elsif ( $x >= $a[2] -
			(($self-> {drawVGrid} && $self-> {cellIndents}-> [2] > 0) ?
				$self-> {drawVGrid} : 0)) {
		# right border
		$cx = -1;
		$hints{x} = +2;
	}
	if ( $y < $a[1]) {
		# bottom border
		$cy = -1;
		$hints{y} = +2;
	} elsif ( $y >= $a[3] -
		(($self-> {drawHGrid} && $self-> {cellIndents}-> [1] > 0) ?
			$self-> {drawVGrid} : 0)) {
		# top border
		$cy = -1;
		$hints{y} = -1;
	}
	return $cx, $cy, %hints, 'exterior', 1
		if defined $hints{x} && defined $hints{y};

	# check if it is the grid
	if ( !$NoGrid && $self-> {drawVGrid}) {
		my $i = -1;
		my $lax = $self-> allowChangeCellWidth ? $self-> {gridGravity} : 0;
		my $skipLast = ( $self-> {cellIndents}-> [2] > 0) ? scalar(@{$self-> {vGrid}}) - 1 : -1;
		for ( @{$self-> {vGrid}}) {
			$i++;
			next if $x < $$_[0] - $lax || $x > $$_[0] + $lax || $i == $skipLast;
			$hints{x_grid}    = 1;
			$hints{grid}      = 1;
			if ( $self-> {cellIndents}-> [2] > 0 &&
				$i >= scalar(@{$self-> {vGrid}}) - $self-> {cellIndents}-> [2] - 1) {
				$hints{x_right} = 1;
				$i++ unless $self-> {lastColEmpty};
			} else {
				$hints{x_left} = 1;
			}
			$hints{x} = 0;
			$hints{y} = +1 unless defined $hints{y};
			return $$colsDraw[$i][0], $cy, %hints;
		}
	}
	if ( !$NoGrid && $self-> {drawHGrid}) {
		my $i = -1;
		my $lax = $self-> allowChangeCellHeight ? $self-> {gridGravity} : 0;
		my $skipLast = ( $self-> {cellIndents}-> [3] > 0) ?
			scalar(@{$self-> {hGrid}}) - 1 : -1;

		for ( @{$self-> {hGrid}}) {
			$i++;
			next if $y < $$_[0] - $lax || $y > $$_[0] + $lax || $i == $skipLast;

			$hints{y_grid}    = 1;
			$hints{grid}      = 1;
			if ( $self-> {cellIndents}-> [3] > 0 &&
				$i >= scalar(@{$self-> {hGrid}}) - $self-> {cellIndents}-> [3] - 1) {
				$hints{y_bottom} = 1;
				$i++ unless $self-> {lastRowEmpty};
			} else {
				$hints{y_top} = 1;
			}

			$hints{x} = +1 unless defined $hints{x};
			$hints{y} = 0;

			return $cx, $$rowsDraw[$i][0], %hints;
		}
	}

	# check other areas
	if ( defined $hints{x}) {
		# nop
	} elsif ( $x > $$colsDraw[-1][4] + $self-> {drawVGrid}) {
		# right whitespace
		$cx = -1;
		$hints{x} = +1;
	} elsif ( $self-> {lastColEmpty} &&
		$x < $a[2] - $self-> {pixelCellIndents}-> [2] &&
		$x > $a[2] - $self-> {pixelCellIndents}-> [2] - $self-> {lastColTail} -
			(($self-> {cellIndents}-> [2] > 0) ? $self-> {drawVGrid} : 0)
	) {
		# gap
		$cx = -1;
		$hints{x} = +1;
		$hints{x_gap} = 1;
		$hints{x_type} = 1;
	} else {
	# cycle cells to find who is it
		my $i = 0;
		my $dv = $self-> {drawVGrid};
		for ( @$colsDraw) {
			if ( $x <= $$_[4]) {
				$cx = $$_[0];
				$hints{x} = 0;
				if (( $hints{x_type} = $$_[1]) != 0) {
					$hints{x_type} =
						( $x > $a[0] + $self-> {pixelCellIndents}-> [0]) ?
							(( $x < $a[2] - $self-> {pixelCellIndents}-> [2] - 1) ?
							0 : +1)
						: -1;
				}
				last;
			}
			$i++;
		}
		unless ( defined $hints{x}) { # last column grid not catched when $NoGrid is set
			$hints{x} = 0;
			$hints{x_type} = 0; # XXX unsure
			$cx = $$colsDraw[-1][0];
		}
	}

	if ( defined $hints{y}) {
		# nop
	} elsif ( $y < $$rowsDraw[-1][3] + $self-> {drawHGrid}) {
		# bottom whitespace
		$cy = -1;
		$hints{y} = +1;
	} elsif (
		$self-> {lastRowEmpty} &&
		$y > $a[1] + $self-> {pixelCellIndents}-> [3] &&
		$y < $a[1] + $self-> {pixelCellIndents}-> [3] + $self-> {lastRowTail} -
			(($self-> {cellIndents}-> [3] > 0) ? $self-> {drawHGrid} : 0)
	) {
		# gap
		$cy = -1;
		$hints{y} = +1;
		$hints{y_gap} = 1;
		$hints{y_type} = 1;
	} else {
	# cycle cells to find who is it
		my $i = 0;
		my $dh = $self-> {drawHGrid};
		for ( @$rowsDraw) {
			if ( $y >= $$_[3]) {
				$cy = $$_[0];
				$hints{y} = 0;
				if (( $hints{y_type} = $$_[1]) != 0) {
					$hints{y_type} = ( $y < $a[3] - $self-> {pixelCellIndents}-> [1] - 1) ?
					(( $y > $a[1] + $self-> {pixelCellIndents}-> [3]) ? 0 : +1) : -1;
				}
				last;
			}
			$i++;
		}
		unless ( defined $hints{y}) { # last row grid not catched when $NoGrid is set
			$hints{y} = 0;
			$hints{y_type} = 0; # XXX unsure
			$cy = $$colsDraw[-1][0];
		}
	}

	# area type
	if ( $hints{x} == 0 && $hints{y} == 0) {
		if ( $hints{x_type} == 0 && $hints{y_type} == 0) {
			$hints{normal} = 1;
		} else {
			$hints{indent} = 1;
		}
	} else {
		$hints{exterior} = 1;
	}

	return $cx, $cy, %hints;
}


sub redraw_cell
{
	my ( $self, $x, $y) = @_;
	my @info = $self-> get_screen_cell_info( $x, $y);
	return unless scalar @info;
	$self-> invalidate_rect( @info[gsci::V_RECT]);
}

# Because grid is non-linear, x or y position shift results in that
# number visible cells and rows is different . Therefore grid operates
# two scroll modes - pixel and cell. The cell mode is the default, where
# the scroll step is a cell. If, however, a cell is single and cannot be
# fit, scrolling is switched to pixel-wise. This behavior is reflected via
# {colUnits} and {rowUnits} boolean fields. The limits are {columns} and
# {colSpan}, and {rows} and {rowSpan} respectively for either mode.
# {colSpan} and {rowSpan} are used internally for cell unit mode also.

sub reset
{
	my ( $self, @par_sz) = @_;
	my ( $O, $T, $r, $c, $dh, $dv) = (
		$self-> {leftCell}, $self-> {topCell}, $self-> {rows}, $self-> {columns},
		$self-> {drawHGrid}, $self-> {drawVGrid});
	my ( $i, $W, $H, $lastw, $lasth) = ( 0, 0, 0, 0, 0);
	my @scroll_steps = ( 0, 0);
	@par_sz = $self-> size unless @par_sz;

	$self-> cache_geometry_requests(1);
	$self-> begin_paint_info unless $self-> {NoBulkPaintInfo};

	my @in = @{$self-> {cellIndents}};
	my @px = ( 0,0,0,0);
	for ( $i = 0; $i < $in[0]; $i++) {
		$px[0] += $self-> columnWidth($i) + $dv;
	}
	for ( $i = 0; $i < $in[1]; $i++) {
		$px[1] += $self-> rowHeight($i) + $dh;
	}
	for ( $i = 0; $i < $in[2]; $i++) {
		$px[2] += $self-> columnWidth($c - $i - 1) + $dv;
	}
	for ( $i = 0; $i < $in[3]; $i++) {
		$px[3] += $self-> rowHeight($r - $i - 1) + $dh;
	}
	$self-> {pixelCellIndents} = \@px;

	# calculate dimension of a minimal operational field
	$W += $self-> columnWidth( $O++) + $dv
		if $c > $in[0] + $in[2];
	$H += $self-> rowHeight( $T++) + $dh
		if $r > $in[1] + $in[3];

	# select unit mode
REPEAT_LAYOUT:
	my ( $w, $h, $o, $t) = ( $W, $H, $O, $T);
	my @sz = $self-> get_active_area( 2, @par_sz);
	$self-> {colUnits} = ( $w + $px[0] + $px[2] <= $sz[0] ) ? 1 : 0;
	$self-> {rowUnits} = ( $h + $px[1] + $px[3] <= $sz[1] ) ? 1 : 0;

	# calculate the last possible visible row
	$i = $r - $in[3] - 1;
	my $maxh = $sz[1] - $px[1] - $px[3];
	my $yh = $self-> rowHeight( $i) + $dh;
	while ( $i > $in[1] ) {
		my $dh = $self-> rowHeight( $i - 1) + $dh;
		last if $yh + $dh > $maxh;
		$yh += $dh;
		$i--;
	}
	$self-> {rowMax} = $i;

	# calculate the last possible visible column
	my $maxw = $sz[0] - $px[0] - $px[2];
	$i = $c - $in[2] - 1;
	my $xw = $self-> columnWidth( $i) + $dv;
	while ( $i > $in[0] ) {
		my $dw = $self-> columnWidth( $i - 1) + $dv;
		last if $xw + $dw > $maxw;
		$xw += $dw;
		$i--;
	}
	$self-> {colMax} = $i;

	# if span is more than minimal, calculate how many cells can be fit in screen
	if ( $self-> {colUnits}) {
		$sz[0] -= $px[0] + $px[2];
		while ( $w < $sz[0] && $o < $c - $in[2]) {
			$lastw = $w;
			$w += $self-> columnWidth( $o++) + $dv;
		}
		$self-> {dx} = 0;
		$self-> {lastColEmpty} = ($in[2] > 0) ? $w < $sz[0] : 0;
		$self-> {lastColTail} = ( $w > $sz[0] ) ?
			$sz[0] - $lastw :
			(( $in[2] > 0) ? $sz[0] - $w : 0);
		$self-> {colSpan} = $lastw +
			( $self-> {lastColEmpty} ? $self-> {lastColTail} : 0);
	} else {
		$self-> {lastColEmpty} = 0;
		$self-> {lastColTail}  = 0;
		$self-> {colSpan} = $w + $px[0] + $px[2];
		$self-> {dx} = $self-> {colSpan} - $sz[0]
			if $self-> {dx} > $self-> {colSpan} - $sz[0];
	}
	if ( $self-> {rowUnits}) {
		$sz[1] -= $px[1] + $px[3];
		while ( $h < $sz[1] && $t < $r - $in[3]) {
			$lasth = $h;
			$h += $self-> rowHeight( $t++) + $dh;
		}
		$self-> {dy} = 0;
		$self-> {lastRowEmpty} = ( $in[3] > 0) ? $h < $sz[1] : 0;
		$self-> {lastRowTail} = ( $h > $sz[1] ) ?
			$sz[1] - $lasth :
			(( $in[3] > 0) ? $sz[1] - $h : 0);
		$self-> {rowSpan} = $lasth + ($self-> {lastRowEmpty} ?
			$self-> {lastRowTail} : 0);
	} else {
		$self-> {lastRowEmpty} = 0;
		$self-> {lastRowTail} = 0;
		$self-> {rowSpan} = $h + $px[1] + $px[3];
		$self-> {dy} = $self-> {rowSpan} - $sz[1]
			if $self-> {dy} > $self-> {rowSpan} - $sz[1];
	}
	$self-> {visibleCols} = $o - $self-> {leftCell};
	$self-> {visibleRows} = $t - $self-> {topCell};
	$self-> {fullCols} =  $self-> {visibleCols} -
		(( !$self-> {lastColEmpty} && $self-> {lastColTail} > 0) ? 1 : 0);
	$self-> {fullRows} =  $self-> {visibleRows} -
		(( !$self-> {lastRowEmpty} && $self-> {lastRowTail} > 0) ? 1 : 0);

	my $vr = $self-> {visibleRows} + $in[1] + $in[3];
	my $vc = $self-> {visibleCols} + $in[0] + $in[2];

	# calculate breadth vectors
	my ( @colsDraw, @rowsDraw) = ();

	# Determine cells to be drawn
	#
	# colsDraw and rowsDraw contain arrays of cell and row geometry, with each
	# item laid out as follows:
	# 0: cell #
	# 1: type; 0 - normal cell, 1 - indent cell
	# 2: visible cell breadth
	# 3: visible cell start
	# 4: visible cell end
	# 5: real cell start
	# 6: real cell end
	# The coordinates are in inclusive-inclusive coordinate system, and
	# do not include eventual grid space, and gaps between indent and
	# normal cells.
	$o = $self-> {leftCell};
	$t = $self-> {topCell};
	# horizontal
	push @colsDraw, map {[$_, 1, $self-> columnWidth($_) + $dv]} 0 .. $in[0] - 1
		if $in[0] > 0;
	if ( $self-> {colUnits}) {
		push @colsDraw, map {[$_, 0, $self-> columnWidth($_) + $dv]}
			$o .. $o + $self-> {visibleCols} - 1;
		if ( !$self-> {lastColEmpty} && $self-> {lastColTail} > 0) {
			$colsDraw[-1][6] = $colsDraw[-1][2] - $self-> {lastColTail};
			$colsDraw[-1][2] = $self-> {lastColTail};
		}
	} else {
		push @colsDraw, [ $o, 0, $self-> columnWidth($o) + $dv];
	}
	push @colsDraw, map {[$_, 1, $self-> columnWidth($_) + $dv]} $c - $in[2] .. $c - 1
	if $in[2] > 0;
	# and vertical
	push @rowsDraw, map {[$_, 1, $self-> rowHeight($_) + $dh]} 0 .. $in[1] - 1
	if $in[1] > 0;
	if ( $self-> {rowUnits}) {
		push @rowsDraw, map {[$_, 0, $self-> rowHeight($_) + $dh]}
			$t .. $t + $self-> {visibleRows} - 1;
		if ( !$self-> {lastRowEmpty} && $self-> {lastRowTail} > 0) {
			$rowsDraw[-1][5] = $self-> {lastRowTail} + $dh - $rowsDraw[-1][2];
			$rowsDraw[-1][2] = $self-> {lastRowTail};
		}
	} else {
		push @rowsDraw, [ $t, 0, $self-> rowHeight($t) + $dh];
	}
	push @rowsDraw, map {[$_, 1, $self-> rowHeight($_) + $dh]} $r - $in[3] .. $r - 1
		if $in[3] > 0;

	$i = $self-> {indents}-> [0];
	my $j = 0;
	for ( @colsDraw) {
		$$_[3] = $i;
		$$_[4] = $i + $$_[2] - 1 - $dv;
		$$_[5] += $$_[3];
		$$_[6] += $$_[4];
		$i += $$_[2];
		$i += $self-> {lastColTail}
			if $self-> {lastColEmpty} && $in[2] > 0 && $$_[0] == $c - $in[2] - 1;
		$j++;
	}

	$i = $par_sz[1] - $self-> {indents}-> [3];
	$j = 0;
	for ( @rowsDraw) {
		$$_[3] = $i - $$_[2] + $dh;
		$$_[4] = $i - 1;
		$$_[5] += $$_[3];
		$$_[6] += $$_[4];
		$i -= $$_[2];
		$i -= $self-> {lastRowTail}
			if $self-> {lastRowEmpty} && $in[3] > 0 && $$_[0] == $r - $in[3] - 1;
	}

	$self-> {colsDraw} = \@colsDraw;
	$self-> {rowsDraw} = \@rowsDraw;

	# assign grid anchor points
	my ( @vgrid, @hgrid);
	if ( $dh) {
		@hgrid = map {[ $$_[3] - 1, $colsDraw[-1][4], $colsDraw[0][3]]} @rowsDraw;
		splice @hgrid, -$in[3], 0,
			[$rowsDraw[-$in[3]][4] + $dh, $colsDraw[-1][4], $colsDraw[0][3]]
				if $self-> {rowUnits} && $self-> {lastRowEmpty} && $in[3] > 0;
		# split lines over the gap
		if ( $self-> {lastColEmpty}) {
			my %excludes = ( $#hgrid => 1, $#hgrid - $in[3] => 1);
			$excludes{$in[1]-1} = 1 if $in[1] > 0;
			$i = 0;
			for ( @hgrid) {
				next if $excludes{$i++};
				splice @$_, 2, 0, $colsDraw[-$in[2]][3], $colsDraw[-$in[2]-1][4];
			}
		}
	}
	$self-> {hGrid} = \@hgrid;
	if ( $dv) {
		@vgrid = map {[ $$_[4] + 1, $rowsDraw[-1][3], $rowsDraw[0][4]]} @colsDraw;
		splice @vgrid, -$in[2], 0,
			[$colsDraw[-$in[2]][3] - $dv, $rowsDraw[-1][3], $rowsDraw[0][4]]
				if $self-> {colUnits} && $self-> {lastColEmpty} && $in[2] > 0;
		# split lines over the gap
		if ( $self-> {lastRowEmpty}) {
			my %excludes = ( $#vgrid => 1, $#vgrid - $in[2] => 1);
			$excludes{$in[0]-1} = 1 if $in[0] > 0;
			$i = 0;
			for ( @vgrid) {
				next if $excludes{$i++};
				splice @$_, 2, 0, $rowsDraw[-$in[3]][4], $rowsDraw[-$in[3]-1][3];
			}
		}
	}
	$self-> {vGrid} = \@vgrid;

	# scroll bars may change geometry and cause repaints
	$self-> end_paint_info unless $self-> {NoBulkPaintInfo};

	# adjust scrollbars
	my @scrolls = ( $self-> {hScroll}, $self-> {vScroll});
	if ( !($self-> {scrollTransaction} & 1)) {
		if ( $self-> {rowUnits}) {
			$self-> vScroll( $vr < $r)
				if $self-> {autoVScroll};
			$self-> {vScrollBar}-> set(
				max      => $self-> {rowMax} - $in[1],
				pageStep => $vr,
				whole    => $r,
				partial  => $vr,
				value    => $self-> {topCell} - $in[1],
			) if $self-> {vScroll};
		} else {
			$self-> vScroll( $self-> {dy} < $self-> {rowSpan})
				if $self-> {autoVScroll};
			my @sz = $self-> get_active_area(2);
			$self-> {vScrollBar}-> set(
				max      => $self-> {rowSpan} - $sz[1],
				pageStep => $sz[1],
				whole    => $self-> {rowSpan},
				partial  => $sz[1],
				value    => $self-> {dy},
			) if $self-> {vScroll};
		}
	}
	if ( !($self-> {scrollTransaction} & 2)) {
		if ( $self-> {colUnits}) {
			$self-> hScroll( $vc < $c)
				if $self-> {autoHScroll};
			$self-> {hScrollBar}-> set(
				max      => $self-> {colMax} - $in[0],
				pageStep => $vc,
				whole    => $c,
				partial  => $vc,
				value    => $self-> {leftCell} - $in[0],
			) if $self-> {hScroll};
		} else {
			$self-> hScroll( $self-> {dx} < $self-> {colSpan})
				if $self-> {autoHScroll};
			my @sz = $self-> get_active_area(2);
			$self-> {hScrollBar}-> set(
				max      => $self-> {colSpan} - $sz[0],
				pageStep => $sz[0],
				whole    => $self-> {colSpan},
				partial  => $sz[0],
				value    => $self-> {dx},
			) if $self-> {hScroll};
		}
	}

	# check if auto-scrolling changed the layout, and reset it again,
	# but no more than once for each dimension
	if ( $self-> {hScroll} != $scrolls[0] || $self-> {vScroll} != $scrolls[1] ) {
		$scroll_steps[0]++ if $self-> {hScroll} != $scrolls[0];
		$scroll_steps[1]++ if $self-> {vScroll} != $scrolls[1];
		if ( $scroll_steps[0] < 2 && $scroll_steps[1] < 2) {
			$lastw = $lasth = 0;
			$self-> begin_paint_info unless $self-> {NoBulkPaintInfo};
			goto REPEAT_LAYOUT
		}
	}

	$self-> cache_geometry_requests(0);
}

sub select_all {
	my $self = $_[0];
	$self-> selection(0,0,$self-> {columns},$self-> {rows});
}

sub std_draw_text_cells
{
	my ($self, $canvas, $cols, $rows, $active_area) = @_;
	my @colors = (
		$self-> color,
		$self-> backColor,
		$self-> hiliteColor,
		$self-> hiliteBackColor,
		$self-> indentCellColor,
		$self-> indentCellBackColor,
		$self-> gridColor,
	);
	my @selection = $self-> selection;
	my @f = $self-> focused ? $self-> focusedCell : ( -1, -1);
	my @focRect;
	my $font_height = $self-> font-> height;
	my @prelight    = @{$self->{prelight} // [-1,-1]};
	my ( $xsel, $ysel, $xprelight, $yprelight);
	my ( $clipV, $clipH) = ( $self-> {clipCells} == 1, $self-> {clipCells} == 2);
	my @clipRect = $canvas-> clipRect;
	for ( @$cols) {
		my ( $col, $xtype, $br, $x1, $x2, $X1, $X2) = @$_;
		$canvas-> clipRect( $x1, $$active_area[1], $x2, $$active_area[3])
			if $clipV;
		$xsel      = $col >= $selection[0] && $col <= $selection[2];
		$xprelight = $col == $prelight[0];
		my $last_type;
		my @bars;
		my @rects;
		my @cellids;
		for ( @$rows) {
			my ( $row, $ytype, $br, $y1, $y2, $Y1, $Y2) = @$_;
			$ysel      = $row >= $selection[1] && $row <= $selection[3] if $xsel;
			$yprelight = $row == $prelight[1] if $xprelight;
			my $type = ($xtype || $ytype) ? 2 : (($xsel && $ysel) ? 1 : 0);
			$type |= 4 if $xprelight && $yprelight;
			if ( defined($last_type) && $type != $last_type) {
				$canvas-> set(
					color     => $colors[($last_type & 3) * 2],
					backColor => $colors[($last_type & 3) * 2 + 1],
				);
				$self-> draw_item_background($canvas, @$_, $last_type & 4) for @bars;
				$self-> draw_text_cells( $canvas, \@bars, \@rects, \@cellids, $font_height);
				@bars = @rects = @cellids = ();
			}
			$last_type = $type;
			push @bars,  [$x1, $y1, $x2, $y2];
			push @rects, [$X1, $Y1, $X2, $Y2];
			push @cellids, [ $col, $row ];
			@focRect = ($x1, $y1, $x2, $y2) if $col == $f[0] && $row == $f[1];
		}

		if ( defined $last_type) {
			$canvas-> set(
				color     => $colors[($last_type & 3) * 2],
				backColor => $colors[($last_type & 3) * 2 + 1],
			);
			$self-> draw_item_background($canvas, @$_, $last_type & 4) for @bars;
			$self-> draw_text_cells( $canvas, \@bars, \@rects, \@cellids, $font_height);
		}
	}
	$canvas-> clipRect( @clipRect);
	$canvas-> rect_focus( @focRect) if @focRect;
}

sub on_size
{
	my ( $self, $ox, $oy, $x, $y) = @_;
	$self-> reset( $x, $y);
}

sub on_disable { $_[0]-> repaint; }
sub on_enable  { $_[0]-> repaint; }
sub on_enter   { $_[0]-> redraw_cell( $_[0]-> focusedCell); }

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	$self-> notify(q(MouseUp),0,0,0) if defined $self-> {mouseTransaction};
	return if $mod & km::DeadKey;

	$mod &= ( km::Shift|km::Ctrl|km::Alt);

	if ( scalar grep { $key == $_ }
		(kb::Left,kb::Right,kb::Up,kb::Down,kb::Home,kb::End,kb::PgUp,kb::PgDn))
	{
		my @f = @{$self-> {focusedCell}};
		my $doSelect;
		if ( $mod == 0 || ( $mod & (km::Shift|km::Ctrl))) {
				if ( $key == kb::Up)   { $f[1]-- }
			elsif ( $key == kb::Down) { $f[1]++ }
			elsif ( $key == kb::Left) { $f[0]-- }
			elsif ( $key == kb::Right){ $f[0]++ }
			elsif ( $key == kb::Home) {
				$f[0] = (($mod & km::Ctrl) ? 0 :
					($self-> {leftCell} - (( $f[0] == $self-> {leftCell}) ?
						$self-> {fullCols} : 0)));
			}
			elsif ( $key == kb::End)  {
				my $e = $self-> {leftCell} + $self-> {fullCols} - 1;
				$f[0] = (($mod & km::Ctrl) ?
					$self-> {columns} :
					$e + (
						($f[0] == $e) ?
						$self-> {fullCols} : 0
					));
			}
			elsif ( $key == kb::PgUp) {
				$f[1] = (($mod & km::Ctrl) ? 0 :
					($self-> {topCell} - (( $f[1] == $self-> {topCell}) ?
						$self-> {fullRows} :
						0)
					));
			}
			elsif ( $key == kb::PgDn) {
				my $e = $self-> {topCell} + $self-> {fullRows} - 1;
				$f[1] = (($mod & km::Ctrl) ? $self-> {rows} :
						($e + (($f[1] == $e ) ?
							$self-> {fullRows} :
							0)
						));
			}
			$doSelect = $mod & km::Shift;
		}
		if ( $doSelect ) {
			my @sel = exists($self-> {anchor}) ?
				@{$self-> {anchor}} :
				@{$self-> {focusedCell}};

			$self-> selection( @sel, @f);
			$self-> {anchor} = [ $self-> focusedCell ] unless exists $self-> {anchor};
		} else {
			$self-> selection( @f, @f ) if exists $self-> {anchor};
			delete $self-> {anchor};
		}
		$self-> focusedCell( @f);
		$self-> clear_event;
		return;
	} else {
		delete $self-> {anchor};
	}

	if ( $mod == 0 && ( $key == kb::Space || $key == kb::Enter)) {
		$self-> clear_event;
		$self-> notify(q(Click)) if $key == kb::Enter;
		return;
	}
}

sub on_leave
{
	my $self = $_[0];
	if ( $self-> {mouseTransaction})
	{
		$self-> capture(0) if $self-> {mouseTransaction};
		$self-> {mouseTransaction} = undef;
	}
	$self-> redraw_cell( $self-> focusedCell);
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	$self-> clear_event;
	return if $btn != mb::Left || !$dbl;

	my ( $cx, $cy, %hints) = $self-> point2cell( $x, $y);
	$self-> notify(q(Click)) if $hints{normal} || $hints{indent};
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;

	my ( $cx, $cy, %hints) = $self-> point2cell( $x, $y);
	# print "$_($hints{$_})," for keys %hints; print "X($cx),Y($cy)\n";return;
	if ( $hints{normal}) {
		if ( $self-> {multiSelect}) {
			if ( $mod & km::Shift) {
				$self-> selection( $cx, $cy, @{$self-> {focusedCell}});
			} else {
				$self-> selection( $cx, $cy, $cx, $cy);
			}
			$self-> {anchor} = [ $cx, $cy ];
		}
		$self-> focusedCell( $cx, $cy);
		$self-> {mouseTransaction} = 1;
		$self-> capture(1);
		$self-> clear_event;
		return;
	}

	if ( defined($hints{x_grid}) && $self-> allowChangeCellWidth) {
		$self-> pointerType( cr::SizeWE);
		my %d;
		if ( $hints{x_right}) {
			my @info = $self-> get_screen_cell_info( $cx, $self-> {topCell});
			$d{range}    = [ $self-> get_range( 0, $cx) ];
			$d{v_begins} = $info[gsci::V_LEFT] - $self-> {lastColTail};
			$d{v_ends}   = $self-> right - $self-> {indents}-> [2] - 1;
			$d{index}    = $cx;
			$d{mode}     = 0;
			$d{offset}   = $info[gsci::RIGHT];
		} else {
			my @info = $self-> get_screen_cell_info( $cx, $self-> {topCell});
			$d{range}    = [ $self-> get_range( 0, $cx) ];
			$d{offset}   = $info[gsci::LEFT];
			$d{v_begins} = $d{offset} + $d{range}-> [0];
			$d{v_begins} = $info[gsci::V_LEFT] if $d{v_begins} < $info[gsci::V_LEFT];
			$d{v_ends}   = $self-> right - $self-> {indents}-> [2] - 1;
			$d{index}    = $cx;
			$d{mode}     = 1;
		}
		$d{breadth} = $self-> columnWidth($d{index});
		$self-> {dragSizeInfo} = \%d;
		$self-> {mouseTransaction} = 2;
		$self-> capture(1);
		$self-> clear_event;
		return;
	} elsif ( defined($hints{y_grid}) && $self-> allowChangeCellHeight) {
		$self-> pointerType( cr::SizeNS);
		my %d;
		if ( $hints{y_bottom}) {
			my @info = $self-> get_screen_cell_info( $self-> {leftCell}, $cy);
			$d{range}    = [ $self-> get_range( 1, $cy) ];
			$d{v_begins} = $info[gsci::V_TOP]  + $self-> {lastRowTail};
			$d{v_ends}   = $self-> bottom - $self-> {indents}-> [3] - 1;
			$d{index}    = $cy;
			$d{mode}     = 0;
			$d{offset}   = $info[gsci::V_BOTTOM];
		} else {
			my @info = $self-> get_screen_cell_info( $self-> {leftCell}, $cy);
			$d{range}    = [ $self-> get_range( 1, $cy) ];
			$d{offset}   = $info[gsci::TOP];
			$d{v_begins} = $d{offset} + $d{range}-> [0];
			$d{v_begins} = $info[gsci::V_TOP] if $d{v_begins} < $info[gsci::V_TOP];
			$d{v_ends}   = $self-> bottom - $self-> {indents}-> [3] - 1;
			$d{index}    = $cy;
			$d{mode}     = 1;
		}
		$d{breadth} = $self-> rowHeight($d{index});
		$self-> {dragSizeInfo} = \%d;
		$self-> {mouseTransaction} = 3;
		$self-> capture(1);
		$self-> clear_event;
		return;
	}
}

sub update_prelight_and_pointer
{
	my ( $self, $x, $y) = @_;
	return delete $self->{prelight} if $self-> {mouseTransaction};
	return unless $self-> enabled;

	my ( $cx, $cy, %hints) = $self-> point2cell( $x, $y );
	my @prelight = (-1,-1);
	my @old      = @{$self->{prelight} // [-1,-1]};
	my $fade     = 1;
	if ( defined($hints{x_grid}) && $self-> allowChangeCellWidth) {
		$self-> pointerType( cr::SizeWE);
		$fade = 0;
	} elsif ( defined($hints{y_grid}) && $self-> allowChangeCellHeight) {
		$self-> pointerType( cr::SizeNS);
		$fade = 0;
	} else {
		@prelight = ($cx, $cy) if $hints{normal};
		$self-> pointerType( cr::Default);
	}

	if ( join('.', @prelight ) ne join('.', @old)) {
		$self->redraw_cell( @old ) if $old[0]      >= 0;
		if ( $prelight[0] >= 0 ) {
			$self->{prelight} = \@prelight;
			$self-> redraw_cell( @prelight );
			$self-> fader_in_mouse_enter unless defined $self->{prelight};
		} elsif ( $fade ) {
			$self-> fader_out_mouse_leave;
		} else {
			$self-> {prelight} = undef;
		}
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self-> clear_event;
	$self-> update_prelight_and_pointer($x,$y);
	return unless $self-> {mouseTransaction};

	my ( $cx, $cy, %hints) = $self-> point2cell( $x, $y, defined($self-> {mouseTransaction}));

	if ( $self-> {mouseTransaction} == 1) {
		unless ( $hints{normal}) {
			$self-> scroll_timer_start unless $self-> scroll_timer_active;
			return unless $self-> scroll_timer_semaphore;
			$self-> scroll_timer_semaphore(0);
		} else {
			$self-> scroll_timer_stop;
		}

		my ( $t, $o);
		if ( $hints{x} != 0 || (defined( $hints{x_type}) && $hints{x_type} != 0)) {
			my ( $x1, $x2) = (
				$self-> {leftCell},
				$self-> {leftCell} + $self-> {fullCols} - 1
			);
			my $xd = ( $hints{x} == 0) ? $hints{x_type} : $hints{x};
			if ( $xd < 0) {
				if ( $self-> {focusedCell}-> [0] > $x1) {
					$cx = $x1;
				} else {
					$o = $self-> {leftCell} - 1;
					$cx = $x1 - 1;
				}
			} else {
				$cx = $self-> {focusedCell}-> [0] + 1;
				$cx = $x2 + 1 if $cx < $x1 || $cx > $x2 + 1;
			}
		}
		if ( $hints{y} != 0 || (defined( $hints{y_type}) && $hints{y_type} != 0)) {
			my ( $y1, $y2) = (
				$self-> {topCell},
				$self-> {topCell} + $self-> {fullRows} - 1
			);
			my $yd = ( $hints{y} == 0) ? $hints{y_type} : $hints{y};
			if ( $yd < 0) {
				if ( $cy > $y1) {
					$cy = $y1;
				} else {
					$t = $self-> {topCell} - 1;
					$cy = $y1 - 1;
				}
			} else {
				$cy = $self-> {focusedCell}-> [1] + 1;
				$cy = $y2 + 1 if $cy < $y1 || $cy > $y2 + 1;
			}
		}
		$self-> selection( $cx, $cy, @{$self-> {anchor}}) if $self-> {anchor};
		$self-> leftCell( $o) if defined $o;
		$self-> topCell( $t) if defined $t;
		$self-> focusedCell( $cx, $cy);
	} elsif ( $self-> {mouseTransaction} == 2) {
		my @a = $self-> get_active_area( 1);
		$x = $a[0] if $x < $a[0];
		$x = $a[2] if $x > $a[2];
		my $d = $self-> {dragSizeInfo};
		$x = $d-> {v_begins} if $x < $d-> {v_begins};
		$x = $d-> {v_ends} if $x > $d-> {v_ends};
		$x = $d-> {mode} ? $x - $d-> {offset} : $d-> {offset} - $x;
		$x = $d-> {range}-> [0] if $x < $d-> {range}-> [0];
		$x = $d-> {range}-> [1] if $x > $d-> {range}-> [1];
		if ( $x != $d-> {breadth}) {
			$self-> columnWidth( $d-> {index}, $x);
			$d-> {breadth} = $self-> columnWidth( $d-> {index});
		}
	} elsif ( $self-> {mouseTransaction} == 3) {
		my @a = $self-> get_active_area( 1);
		$y = $a[1] if $y < $a[1];
		$y = $a[3] if $y > $a[3];
		my $d = $self-> {dragSizeInfo};
		$y = $d-> {v_begins} if $y > $d-> {v_begins};
		$y = $d-> {v_ends} if $y < $d-> {v_ends};
		$y = $d-> {mode} ? $d-> {offset} - $y :$y - $d-> {offset};
		$y = $d-> {range}-> [0] if $y < $d-> {range}-> [0];
		$y = $d-> {range}-> [1] if $y > $d-> {range}-> [1];
		if ( $y != $d-> {breadth}) {
			$self-> rowHeight( $d-> {index}, $y);
			$d-> {breadth} = $self-> rowHeight( $d-> {index});
		}
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $btn != mb::Left;
	return unless defined $self-> {mouseTransaction};

	delete $self-> {mouseTransaction};
	delete $self-> {anchor};
	delete $self-> {dragSizeInfo};
	$self-> capture(0);
	$self-> clear_event;

	my ( $cx, $cy, %hints) = $self-> point2cell( $x, $y);
	if ( defined($hints{x_grid}) && $self-> allowChangeCellWidth) {
		$self-> pointerType( cr::SizeWE);
	} elsif ( defined($hints{y_grid}) && $self-> allowChangeCellHeight) {
		$self-> pointerType( cr::SizeNS);
	} else {
		$self-> pointerType( cr::Default);
	}
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$z *= ( $self-> {visibleRows} || 1) if $mod & km::Ctrl;
	my $newTop = $self-> {topCell} - $z;
	$self-> topCell( $newTop);
	$self-> update_prelight_and_pointer($x, $y);
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
	my $prelight = delete $self->{prelight};
	$self-> redraw_cell( @$prelight ) if defined $prelight;
}

sub on_faderepaint
{
	my $self = shift;
	return unless defined $self->{prelight};
	$self-> fader_cancel_if_unbuffered;
	$self-> redraw_cell( @{$self->{prelight}} );
}

sub on_paint
{
	my ($self,$canvas)   = @_;
	my @size   = $canvas-> size;
	unless ( $self-> enabled) {
		$self-> color( $self-> disabledColor);
		$self-> backColor( $self-> disabledBackColor);
	}
	my ( $r, $c, $o, $t, $dv, $dh, $dx, $dy) = (
		$self-> {rows}, $self-> {columns},
		$self-> {leftCell}, $self-> {topCell}, $self-> {drawVGrid}, $self-> {drawHGrid},
		$self-> {dx}, $self-> {dy},
	);
	my @a = $self-> get_active_area( 1, @size);
	my ($i,$j);
	my @px = @{$self-> {pixelCellIndents}};
	my @clipRect = $canvas-> clipRect;
	$self-> draw_border( $canvas, undef, @size);
	$canvas-> clipRect( @a);
	if ( $self-> {visibleCols} <= 0 || $self-> {visibleRows} <= 0) {
		$canvas-> clear( @a);
		return;
	}
	# intersect @clipRect with @a to avoid drawing cells behind scrollbars
	for ( 0, 1) {
		$clipRect[$_] = $a[$_] if $clipRect[$_] < $a[$_];
		$clipRect[$_+2] = $a[$_+2] if $clipRect[$_+2] > $a[$_+2];
	}

	my @clipCells;
	my @colsDraw = map { [ @$_ ] } @{$self-> {colsDraw}};
	my @rowsDraw = map { [ @$_ ] } @{$self-> {rowsDraw}};
	my @in = @{$self-> {cellIndents}};

	# find columns to draw, by assigning @clipCells, a clipRect in cell units and
	# calculating the final geometry of cells
	$j = 0;
	for ( @colsDraw) {
		my $c = $_;
		$$c[$_] -= $dx for 3..6;
		$clipCells[0] = $j
			if !defined($clipCells[0]) && $$c[4] + $dv >= $clipRect[0];
		$clipCells[2] = $j
			if !defined($clipCells[2]) && $$c[4] + $dv >= $clipRect[2];
		$j++;
	}
	$clipCells[0] = 0 unless defined $clipCells[0];
	$clipCells[2] = $#colsDraw unless defined $clipCells[2];

	$j = 0;
	for ( @rowsDraw) {
		my $c = $_;
		$$c[$_] += $dy for 3..6;
		$clipCells[3] = $j
			if !defined($clipCells[3]) && $$c[3] - $dv <= $clipRect[3];
		$clipCells[1] = $j
			if !defined($clipCells[1]) && $$c[3] - $dv <= $clipRect[1];
		$j++;
	}
	$clipCells[3] = 0 unless defined $clipCells[3];
	$clipCells[1] = $#rowsDraw unless defined $clipCells[1];

	# if right and top indent cells present, the space for them must
	# be allocated +1 pixel for extra line between indent and empty space
	my @extras = (
		($px[0] > 0) ? $dv : 0, ($px[1] > 0) ? $dh : 0,
		($px[2] > 0) ? $dv : 0, ($px[3] > 0) ? $dh : 0
	);

	# clear undrawable area
	if ( !$self-> {colUnits} || $px[2] == 0) {
		$canvas-> clear( $colsDraw[-1][4] + $dv + 1, @a[1..3])
			if $colsDraw[-1][4] < $a[2];
	} elsif ( $self-> {lastColEmpty}) {
		my $right  = $a[2] - $px[2] - $extras[2];
		my $left   = $a[2] - $px[2] - $self-> {lastColTail} + 1;
		my $bk = $canvas-> backColor;
		if ( $self-> {lastColTail} > $dv) {
			if ( $self-> {rowUnits}) {
				$canvas-> clear( $left, $a[1] + $px[3] + $extras[3],
									$right, $a[3] - $px[1]);
			} else {
				$canvas-> clear( $left,
					$a[3] + $px[3] + $extras[3] + $dy - $self-> {rowSpan} + 1,
					$right,
					$a[3] - $px[1] + $dy);
			}
		}
		$canvas-> backColor( $self-> {indentCellBackColor});
		if ( $self-> {rowUnits}) {
			$canvas-> clear(
				$left, $a[3] - $px[1] + $extras[1] + 1,
				$right, $a[3]
			) if $px[1] > $dh;
			$canvas-> clear(
				$left, $a[1] + $dh,
				$right, $a[1] + $px[3] - 1
			) if $px[3] > $dh;
		} else {
			$canvas-> clear(
				$left, $a[3] - $px[1] + $extras[1] + 1 + $dy,
				$right, $a[3] + $dy
			) if $px[1] > $dh;
			$canvas-> clear(
				$left, $a[3] - $self-> {rowSpan} + $dy + $dh,
				$right, $a[3] - $self-> {rowSpan} + $dy + $dh + $px[3] - 1
			) if $px[3] > $dh;
		}
		$canvas-> backColor( $bk);
	}

	# and horizontal area
	if ( !$self-> {rowUnits} || $in[3] == 0) {
		$canvas-> clear( @a[0..2], $rowsDraw[-1][3] - 1 - $dh)
			if $rowsDraw[-1][3] > $a[1];
	} elsif ( $self-> {lastRowEmpty} ) {
		my $bottom = $a[1] + $px[3] + $extras[3];
		my $top = $a[1] + $px[3] + $self-> {lastRowTail} - $dh;
		my $bk = $canvas-> backColor;
		if ( $self-> {lastRowTail} > $dh) {
			if ( $self-> {colUnits}) {
				$canvas-> clear(
					$a[0] + $px[0], $bottom,
					$a[2] - $px[2] - $extras[2], $top
				);
			} else {
				$canvas-> clear(
					$a[0] + $px[0] - $dx, $bottom,
					$a[0] - $px[2] - $extras[2] - $dx - $dv + $self-> {colSpan}, $top
				);
			}
		}
		$canvas-> backColor( $self-> {indentCellBackColor});
		if ( $self-> {colUnits}) {
			$canvas-> clear(
				$a[0], $bottom,
				$a[0] + $px[0] - 1 - $extras[0], $top
			) if $px[0] > $dv;
			$canvas-> clear(
				$a[2] - $px[2] + 1, $bottom,
				$a[2] - $dv, $top
			) if $px[2] > $dv;
		} else {
			$canvas-> clear(
				$a[0] - $dx, $bottom,
				$a[0] + $px[0] - 1 - $extras[0] - $dx, $top
			) if $px[0] > $dv;
			$canvas-> clear(
				$a[0] - $px[2] + $self-> {colSpan} - $dx , $bottom,
				$a[0] - $dx - $dv + $self-> {colSpan}, $top
			) if $px[2] > $dv;
		}
		$canvas-> backColor( $bk);
	}

	# prepare indent grid line array
	my @grid;
	for ( @{$self-> {vGrid}}) {
		my $x = $$_[0] - $dx;
		for ( $i = 1; $i < @$_; $i += 2) {
			push @grid, $x, $$_[$i], $x, $$_[$i+1];
		}
	}
	for ( @{$self-> {hGrid}}) {
		my $y = $$_[0] + $dy;
		for ( $i = 1; $i < @$_; $i += 2) {
			push @grid, $$_[$i], $y, $$_[$i+1], $y;
		}
	}

	# remove clipped cells
	splice( @colsDraw, $clipCells[2] + 1);
	splice( @colsDraw,  0, $clipCells[0]);
	@colsDraw = grep { $$_[2] > 0 } @colsDraw;

	splice( @rowsDraw, $clipCells[1] + 1);
	splice( @rowsDraw,  0, $clipCells[3]);
	@rowsDraw = grep { $$_[2] > 0 } @rowsDraw;

	# adjust cells rectangles not to overhang the active area
	for ( @colsDraw) {
		$$_[3] = $a[0] if $$_[3] < $a[0];
		$$_[4] = $a[2] if $$_[4] > $a[2];
	}
	for ( @rowsDraw) {
		$$_[3] = $a[1] if $$_[3] < $a[1];
		$$_[4] = $a[3] if $$_[4] > $a[3];
	}

	$canvas-> color( $self-> {gridColor});
	$canvas-> lines( \@grid) if @grid;

	$self-> draw_cells( $canvas, \@colsDraw, \@rowsDraw, \@a);
}

#sub on_stringify
#{
#   my ( $self, $index, $sref) = @_;
#   $$sref = '';
#}

sub set_border_width
{
	my ( $self, $bw) = @_;
	my $obw = $self-> {borderWidth};
	$self-> SUPER::set_border_width( $bw);
	return if $obw == $self-> {borderWidth};
	$self-> reset;
	$self-> repaint;
}

sub set_h_scroll
{
	my ( $self, $hs) = @_;
	return if $hs == $self-> {hScroll};

	$self-> SUPER::set_h_scroll( $hs);

	if ( !($self-> {scrollTransaction} & 2)) {
		$self-> {scrollTransaction} |= 2;
		$self-> reset;
		$self-> {scrollTransaction} &= ~2;
	}
	$self-> repaint;
}

sub set_v_scroll
{
	my ( $self, $vs) = @_;
	return if $vs == $self-> {vScroll};

	$self-> SUPER::set_v_scroll( $vs);

	if ( !($self-> {scrollTransaction} & 1)) {
		$self-> {scrollTransaction} |= 1;
		$self-> reset;
		$self-> {scrollTransaction} &= ~1;
	}
	$self-> repaint;
}

sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction} & 1;

	$self-> {scrollTransaction} |= 1;
	$self-> {rowUnits} ?
		$self-> topCell( $scr-> value + $self-> {cellIndents}-> [1]) :
		$self-> dy( $scr-> value);
	$self-> {scrollTransaction} &= ~1;
}

sub HScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction} & 2;

	$self-> {scrollTransaction} |= 2;
	$self-> {colUnits} ?
		$self-> leftCell( $scr-> value + $self-> {cellIndents}-> [0]) :
		$self-> dx( $scr-> value);
	$self-> {scrollTransaction} &= ~2;
}

sub allowChangeCellHeight
{
	return $_[0]-> {constantCellHeight} ? 0 : $_[0]-> {allowChangeCellHeight} unless $#_;

	my ( $self, $h) = @_;
	$self-> {allowChangeCellHeight} = $h;
}

sub allowChangeCellWidth
{
	return $_[0]-> {constantCellWidth} ? 0 : $_[0]-> {allowChangeCellWidth} unless $#_;

	my ( $self, $w) = @_;
	$self-> {allowChangeCellWidth} = $w;
}

sub cellIndents
{
	return wantarray ? @{$_[0]-> {cellIndents}} : [@{$_[0]-> {cellIndents}}] unless $#_;

	my ( $self, @indents) = @_;
	@indents = @{$indents[0]} if ( scalar(@indents) == 1) && ( ref($indents[0]) eq 'ARRAY');
	for ( @indents) {
		$_ = 0 if $_ < 0;
	}
	if ( $indents[2] + $indents[0] > $self-> {columns}) {
		$indents[2] = $self-> {columns} - $indents[0];
		if ( $indents[2] < 0) {
			$indents[2] = 0;
			$indents[0] = $self-> {columns};
		}
	}
	if ( $indents[3] + $indents[1] > $self-> {columns}) {
		$indents[3] = $self-> {rows} - $indents[1];
		if ( $indents[3] < 0) {
			$indents[3] = 0;
			$indents[1] = $self-> {rows};
		}
	}

	$self-> {leftCell}          += $indents[0] - $self-> {cellIndents}-> [0];
	$self-> {topCell}           += $indents[1] - $self-> {cellIndents}-> [1];
	$self-> {focusedCell}-> [0]  += $indents[0] - $self-> {cellIndents}-> [0];
	$self-> {focusedCell}-> [1]  += $indents[1] - $self-> {cellIndents}-> [1];
	$self-> {cellIndents} = \@indents;

	$self-> reset;
	$self-> repaint;
}

sub clipCells
{
	return $_[0]-> {clipCells} unless $#_;
	$_[0]-> {clipCells} = $_[1];
}

sub colorIndex
{
	my ( $self, $index, $color) = @_;
	if ( $#_ < 2) {
		return $self-> {gridColor}           if $index == ci::Grid;
		return $self-> {indentCellColor}     if $index == ci::IndentCellFore;
		return $self-> {indentCellBackColor} if $index == ci::IndentCellBack;
		return $self-> SUPER::colorIndex( $index)
	} else {
		my $notify = 1;
		if ( $index == ci::Grid) {
			$self-> gridColor( $color);
		} elsif ( $index == ci::IndentCellFore) {
			$self-> indentCellColor( $color);
		} elsif ( $index == ci::IndentCellBack) {
			$self-> indentCellBackColor( $color);
		} else {
			$self-> SUPER::colorIndex( $index, $color);
			$notify = 0;
		}
		$self-> notify(q(ColorChanged), $index) if $notify;
	}
}

sub columns
{
	return $_[0]-> {columns} unless $#_;
	my ( $self, $c) = @_;
	my $lim = $self-> {cellIndents}-> [0] + $self-> {cellIndents}-> [2];
	$lim = 1 if $lim < 1;
	$c = $lim if $c < $lim;
	$self-> {columns} = $c;
	$self-> reset;
	my @f = $self-> focusedCell;
	$self-> focusedCell( $c - $self-> {cellIndents}-> [2] - 1, $f[1])
		if $f[0] >= $c - $self-> {cellIndents}-> [2];
	$self-> reset;
	$self-> repaint;
}

sub columnWidth
{
	my ( $self, $col, $width) = @_;
	if ( $#_ <= 1) {
		return $self-> {constantCellWidth} if $self-> {constantCellWidth};
		return $self-> {geometry_cache_column}-> {$col}
			if $self-> {cache_geometry_requests} &&
				exists $self-> {geometry_cache}-> {$col};
		my $ref = 0;
		$self-> notify(q(Measure), 0, $col, \$ref);
		$ref = 1 if $ref < 1;
		$self-> {geometry_cache_column}-> {$col} = $ref
			if $self-> {cache_geometry_requests};
		return $ref;
	} elsif ( !$self-> {constantCellWidth}) {
		$self-> notify(q(SetExtent), 0, $col, $width);
		$self-> reset;
		$self-> repaint;
	} else {
		$self-> constantCellWidth( $width);
	}
}

sub constantCellHeight
{
	return $_[0]-> {constantCellHeight} unless $#_;
	my ( $self, $h) = @_;
	return if !defined( $self-> {constantCellHeight}) && !defined $h;
	return if
		defined($self-> {constantCellHeight}) &&
		defined($h) &&
		$self-> {constantCellHeight} == $h;
	$h = 1 if defined $h && $h < 1;
	$self-> {constantCellHeight} = $h;
	$self-> reset;
	$self-> repaint;
}

sub constantCellWidth
{
	return $_[0]-> {constantCellWidth} unless $#_;
	my ( $self, $w) = @_;
	return if !defined( $self-> {constantCellWidth}) && !defined $w;
	return if
		defined($self-> {constantCellWidth}) &&
		defined($w) &&
		$self-> {constantCellWidth} == $w;
	$w = 1 if defined $w && $w < 1;
	$self-> {constantCellWidth} = $w;
	$self-> reset;
	$self-> repaint;
}

sub drawHGrid
{
	return $_[0]-> {drawHGrid} unless $#_;
	my ( $self, $dh) = @_;
	$dh = $dh ? 1 : 0;
	return if $dh == $self-> {drawHGrid};
	$self-> {drawHGrid} = $dh;
	$self-> reset;
	$self-> repaint;
}

sub drawVGrid
{
	return $_[0]-> {drawVGrid} unless $#_;
	my ( $self, $dv) = @_;
	$dv = $dv ? 1 : 0;
	return if $dv == $self-> {drawVGrid};
	$self-> {drawVGrid} = $dv;
	$self-> reset;
	$self-> repaint;
}

sub dx
{
	return $_[0]-> {dx} unless $#_;
	my ( $self, $dx) = @_;
	return if $self-> {colUnits};
	my @size = $self-> size;
	my @a  = $self-> get_active_area(0, @size);
	my $w  = $a[2] - $a[0];
	$dx = 0 if $dx < 0;
	$dx = $self-> {colSpan} - $w if $dx > $self-> {colSpan} - $w;
	my $delta = $self-> {dx} - $dx;
	$self-> {dx} = $dx;
	if ( $self-> {hScroll} && !($self-> {scrollTransaction} & 2)) {
		$self-> {scrollTransaction} |= 2;
		$self-> {hScrollBar}-> value($dx);
		$self-> {scrollTransaction} &= ~2;
	}
	$self-> scroll( $delta, 0, clipRect => \@a);
	my @info = $self-> get_screen_cell_info( $self-> focusedCell);
	$self-> invalidate_rect( @info[ gsci::V_RECT] ) if scalar @info;
}

sub dy
{
	return $_[0]-> {dy} unless $#_;
	my ( $self, $dy) = @_;
	return if $self-> {rowUnits};
	my @size = $self-> size;
	my @a  = $self-> get_active_area(0, @size);
	my $h  = $a[3] - $a[1];
	$dy = 0 if $dy < 0;
	$dy = $self-> {rowSpan} - $h if $dy > $self-> {rowSpan} - $h;
	my $delta = $dy - $self-> {dy};
	$self-> {dy} = $dy;
	if ( $self-> {vScroll} && !($self-> {scrollTransaction} & 1)) {
		$self-> {scrollTransaction} |= 1;
		$self-> {vScrollBar}-> value($dy);
		$self-> {scrollTransaction} &= ~1;
	}
	$self-> scroll( 0, $delta, clipRect => \@a);
	my @info = $self-> get_screen_cell_info( $self-> focusedCell);
	$self-> invalidate_rect( @info[ gsci::V_RECT ] ) if scalar @info;
}

sub focusedCell
{
	return @{$_[0]-> {focusedCell}} unless $#_;
	my ( $self, @f) = @_;
	@f = @{$f[0]} if ( scalar(@f) == 1) && ( ref($f[0]) eq 'ARRAY');

	my @in = @{$self-> {cellIndents}};
	my ( $c, $r) = ( $self-> {columns}, $self-> {rows});
	$f[0] = $in[0] if $f[0] < $in[0];
	$f[1] = $in[1] if $f[1] < $in[1];
	$f[0] = $c - $in[2] - 1 if $f[0] >= $c - $in[2];
	$f[1] = $r - $in[3] - 1 if $f[1] >= $r - $in[3];
	my @o = @{$self-> {focusedCell}};
	return if $o[0] == $f[0] && $o[1] == $f[1];

	$self-> notify(q(SelectCell), @f);
	my @old = $self-> get_screen_cell_info( @o);
	my @new = $self-> get_screen_cell_info( @f);
	@{$self-> {focusedCell}} = @f;

	if ( $new[gsci::V_FULL ]) {
		# the new cell is fully visible, need no scrolling
		$self-> invalidate_rect( @new[gsci::V_RECT]);
		$self-> invalidate_rect( @old[gsci::V_RECT]) if @old;
	} else {
		my @r = $self-> get_cell_area;
		my ( $x1, $y1, $x2, $y2) = (
			$self-> {leftCell}, $self-> {topCell},
			$self-> {leftCell} + $self-> {fullCols} - 1,
			$self-> {topCell}  + $self-> {fullRows} - 1
		);
		my ( $o, $t) = ( $x1, $y1);

		$self-> begin_paint_info unless $self-> {NoBulkPaintInfo};
		if ( $f[0] > $x2) {
			$o = $f[0];
			my $maxw = $r[2] - $r[0] + 1 - $self-> columnWidth( $o) - $self-> {drawVGrid};
			while ( 1) {
				$maxw -= $self-> columnWidth( $o - 1) + $self-> {drawVGrid};
				last if $maxw < 0;
				$o--;
			}
		} elsif ( $f[0] < $x1) {
			$o = $f[0];
		}
		if ( $f[1] > $y2) {
			$t = $f[1];
			my $maxh = $r[3] - $r[1] + 1 - $self-> rowHeight( $t) - $self-> {drawHGrid};
			while ( 1) {
				$maxh -= $self-> rowHeight( $t - 1) + $self-> {drawHGrid};
				last if $maxh < 0;
				$t--;
			}
		} elsif ( $f[1] < $y1) {
			$t = $f[1];
		}

		$self-> end_paint_info unless $self-> {NoBulkPaintInfo};
		$self-> leftCell( $o);
		$self-> topCell( $t);
		@old = $self-> get_screen_cell_info( @o);
		@new = $self-> get_screen_cell_info( @f);
		$self-> invalidate_rect( @new[gsci::V_RECT]) if @new;
		$self-> invalidate_rect( @old[gsci::V_RECT]) if @old;
	}
}

sub gridColor
{
	return $_[0]-> {gridColor} unless $#_;
	my ( $self, $gc) = @_;
	return if $gc == $self-> {gridColor};

	$self-> {gridColor} = $gc;
	$self-> repaint if $self-> {drawVGrid} || $self-> {drawHGrid};
}

sub gridGravity
{
	return $_[0]-> {gridGravity} unless $#_;

	my ( $self, $gg) = @_;
	$gg = 0 if $gg < 0;
	$self-> {gridGravity} = $gg;
}

sub indentCellBackColor
{
	return $_[0]-> {indentCellBackColor} unless $#_;
	my ( $self, $c) = @_;
	return if $c == $self-> {indentCellBackColor};

	$self-> {indentCellBackColor} = $c;
	$self-> repaint if grep { $_ > 0 } @{$self-> {cellIndents}};
}

sub indentCellColor
{
	return $_[0]-> {indentCellColor} unless $#_;
	my ( $self, $c) = @_;
	return if $c == $self-> {indentCellColor};

	$self-> {indentCellColor} = $c;
	$self-> repaint if grep { $_ > 0 } @{$self-> {cellIndents}};
}

sub leftCell
{
	return $_[0]-> {leftCell} unless $#_;

	my ( $self, $c) = @_;
	return if defined( $self-> {mouseTransaction}) && $self-> {mouseTransaction} == 2;

	$c = $self-> {cellIndents}-> [0] if $c < $self-> {cellIndents}-> [0];
	$c = $self-> {colMax} if $c > $self-> {colMax};
	return if $c == $self-> {leftCell};

	my ( $old, $unit, $span, $dv) = (
		$self-> {leftCell}, $self-> {colUnits}, $self-> {colSpan}, $self-> {drawVGrid});
	my @a = $self-> get_active_area( 0);
	my $width = $a[2] -
		$a[0] -
		$self-> {pixelCellIndents}-> [0] -
		$self-> {pixelCellIndents}-> [2];
	$self-> {leftCell} = $c;
	$self-> reset;

	# see if the geometry changed too much after the reset
	if ( $unit != $self-> {colUnits}) {
		$self-> invalidate_rect( @a);
		return;
	}
	# When units are pixels, no scrolling can be done, just effective repaints.
	if ( !$unit) {
		$a[0] += $self-> {pixelCellIndents}-> [0];
		$self-> invalidate_rect( @a);
		return;
	}

	# see if can do scrolling - calculate distance between
	# current and new x coordinates, not too far though
	my $w = 0;
	my $i = $old;
	$self-> begin_paint_info unless $self-> {NoBulkPaintInfo};
	if ( $i < $c) {
		while ( $w < $width && $i < $c) {
			$w += $self-> columnWidth( $i++) + $dv;
		}
	} else {
		while ( $w < $width && $i > $c) {
			$w += $self-> columnWidth( --$i) + $dv;
		}
	}
	$self-> end_paint_info unless $self-> {NoBulkPaintInfo};
	$a[0] += $self-> {pixelCellIndents}-> [0];
	$a[2] -= $self-> {pixelCellIndents}-> [2] + $dv;
	if ( $w < $width) {
		$w *= -1 if $old < $c;
		$self-> scroll( $w, 0, clipRect => \@a);
	} else {
		$self-> invalidate_rect( @a);
	}
}

sub multiSelect
{
	return $_[0]-> {multiSelect} unless $#_;
	my ( $self, $ms) = @_;
	return if $ms == $self-> {multiSelect};

	$self-> selection(-1,-1,-1,-1) if $self-> {multiSelect};
	$self-> {multiSelect} = $ms;
}

sub rows
{
	return $_[0]-> {rows} unless $#_;
	my ( $self, $r) = @_;
	my $lim = $self-> {cellIndents}-> [1] + $self-> {cellIndents}-> [3];
	$lim = 1 if $lim < 1;
	$r = $lim if $r < $lim;
	$self-> {rows} = $r;
	$self-> reset;
	my @f = $self-> focusedCell;
	$self-> focusedCell( $f[0], $r - $self-> {cellIndents}-> [3] - 1)
		if $f[1] >= $r - $self-> {cellIndents}-> [3];
	$self-> reset;
	$self-> repaint;
}

sub topCell
{
	return $_[0]-> {topCell} unless $#_;

	my ( $self, $c) = @_;
	return if defined( $self-> {mouseTransaction}) && $self-> {mouseTransaction} == 3;

	$c = $self-> {cellIndents}-> [1] if $c < $self-> {cellIndents}-> [1];
	$c = $self-> {rowMax} if $c > $self-> {rowMax};
	return if $c == $self-> {topCell};

	my ( $old, $unit, $span, $dh) = (
		$self-> {topCell}, $self-> {rowUnits}, $self-> {rowSpan}, $self-> {drawHGrid});
	my @a = $self-> get_active_area( 0);
	my $height = $a[3] -
		$a[1] -
		$self-> {pixelCellIndents}-> [3] -
		$self-> {pixelCellIndents}-> [1];
	$self-> {topCell} = $c;
	$self-> reset;

	# see if the geometry changed too much after the reset
	if ( $unit != $self-> {rowUnits}) {
		$self-> invalidate_rect( @a);
		return;
	}
	# When units are pixels, no scrolling can be done, just effective repaints.
	if ( !$unit) {
		$a[3] -= $self-> {pixelCellIndents}-> [1];
		$self-> invalidate_rect( @a);
		return;
	}

	# see if can do scrolling - calculate distance between
	# current and new x coordinates, not too far though
	my $h = 0;
	my $i = $old;
	$self-> cache_geometry_requests(1);
	if ( $i < $c) {
		while ( $h < $height && $i < $c) {
			$h += $self-> rowHeight( $i++) + $dh;
		}
	} else {
		while ( $h < $height && $i > $c) {
			$h += $self-> rowHeight( --$i) + $dh;
		}
	}
	$self-> cache_geometry_requests(0);
	$a[1] += $self-> {pixelCellIndents}-> [3] + $dh;
	$a[3] -= $self-> {pixelCellIndents}-> [1];
	if ( $h < $height) {
		$h *= -1 if $old > $c;
		$self-> scroll( 0, $h, clipRect => \@a);
	} else {
		$self-> invalidate_rect( @a);
	}
}

sub rowHeight
{
	my ( $self, $row, $height) = @_;
	if ( $#_ <= 1) {
		return $self-> {constantCellHeight} if $self-> {constantCellHeight};
		return $self-> {geometry_cache_row}-> {$row}
			if $self-> {cache_geometry_requests} &&
				exists $self-> {geometry_cache}-> {$row};

		my $ref = 0;
		$self-> notify(q(Measure), 1, $row, \$ref);
		$ref = 1 if $ref < 1;
		$self-> {geometry_cache_row}-> {$row} = $ref
			if $self-> {cache_geometry_requests};

		return $ref;
	} elsif ( !$self-> {constantCellHeight}) {
		$self-> notify(q(SetExtent), 1, $row, $height);
		$self-> reset;
		$self-> repaint;
	} else {
		$self-> constantCellHeight( $height);
	}
}

sub selection
{
	return $_[0]-> {multiSelect} ?
		@{$_[0]-> {selection}} :
		(@{$_[0]-> {focusedCell}}, @{$_[0]-> {focusedCell}})
			unless $#_;
	return unless $_[0]-> {multiSelect};

	my ( $self, $x1, $y1, $x2, $y2) = @_;
	( $x1, $x2) = ( $x2, $x1) if $x1 > $x2;
	( $y1, $y2) = ( $y2, $y1) if $y1 > $y2;

	my @in = @{$self-> {cellIndents}};
	my ( $c, $r) = ( $self-> {columns}, $self-> {rows});
	if ( $x1 < 0 || $y1 < 0 || $x2 < 0 || $y2 < 0) {
		$x1 = $y1 = $x2 = $y2 = -1;
	} else {
		$x1 = $in[0] if $x1 < $in[0];
		$x1 = $c - $in[2] - 1 if $x1 >= $c - $in[2];
		$x2 = $in[0] if $x2 < $in[0];
		$x2 = $c - $in[2] - 1 if $x2 >= $c - $in[2];
		$y1 = $in[1] if $y1 < $in[1];
		$y1 = $r - $in[3] - 1 if $y1 >= $r - $in[3];
		$y2 = $in[1] if $y2 < $in[1];
		$y2 = $r - $in[3] - 1 if $y2 >= $r - $in[3];
	}

	my ( $ox1, $oy1, $ox2, $oy2) = @{$self-> {selection}};
	return if $x1 == $ox1 && $y1 == $oy1 && $x2 == $ox2 && $y2 == $oy2;

	$self-> {selection} = [$x1, $y1, $x2, $y2];

	# union cell change
	$x1 = $ox1 if $x1 > $ox1;
	$x2 = $ox2 if $x2 < $ox2;
	$y1 = $oy1 if $y1 > $oy1;
	$y2 = $oy2 if $y2 < $oy2;

	# intersect with screen cells, leave if the result is empty
	$ox1 = $self-> {leftCell};
	$oy1 = $self-> {topCell};
	$ox2 = $ox1 + $self-> {visibleCols};
	$oy2 = $oy1 + $self-> {visibleRows};
	return if $x1 > $ox2 || $x2 < $ox1 || $y1 > $oy2 || $y2 < $oy1;
	$x1 = $ox1 if $x1 < $ox1;
	$x2 = $ox2 if $x2 > $ox2;
	$y1 = $oy1 if $y1 < $oy1;
	$y2 = $oy2 if $y2 > $oy2;

	# normalize
	( $x1, $x2) = ( $x2, $x1) if $x1 > $x2;
	( $y1, $y2) = ( $y2, $y1) if $y1 > $y2;

	# get pixel coordinates
	my @info1 = $self-> get_screen_cell_info( $x1, $y2);
	my @info2 = $self-> get_screen_cell_info( $x2, $y1);
	if ( @info1 && @info2) {
		$self-> invalidate_rect(
			@info1[gsci::V_LEFT,gsci::V_BOTTOM],
			@info2[gsci::V_RIGHT,gsci::V_TOP]
		);
	} else {
		$self-> repaint;
	}
}

package Prima::AbstractGrid;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractGridViewer);

sub draw_cells
{
	shift-> std_draw_text_cells(@_);
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> constantCellHeight( $self-> font-> height + 2 ) if
		$self-> constantCellHeight;
}

sub on_getrange
{
	my ( $self, $column, $index, $min, $max) = @_;
	$$min = $self-> font-> height + 2 unless $column;
}

sub on_measure
{
	my ( $self, $vertical, $index, $sref) = @_;
	if ( $vertical) {
		$$sref = $self-> font-> height + 2;
	} else {
		$$sref = 0;
		for ( my $i = 0; $i < $self-> {colMax}; $i++ ) {
			my $w = $self-> get_text_width( $self->get_cell_text($i, $index), 1);
			$$sref = $w if $$sref < $w;
		}
	}
}

package Prima::GridViewer;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractGridViewer);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		allowChangeCellHeight => 1,
		allowChangeCellWidth  => 1,
		cells                 => [['']],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {cells}      = [];
	$self-> {widths}     = [];
	$self-> {heights}    = [];
	my %profile = $self-> SUPER::init(@_);
	$self-> cells($profile{cells});
	return %profile;
}

sub columnWidth
{
	my ( $self, $col, $width) = @_;
	if ( $#_ <= 1) {
		unless ( defined $self-> {widths}-> [$col]) {
			if ( defined $self-> {constantCellWidth}) {
				$self-> {widths}-> [$col] = $self-> {constantCellWidth};
			} else {
				my $ref = 0;
				$self-> notify(q(Measure), 0, $col, \$ref);
				$ref = 1 if $ref < 1;
				$self-> {widths}-> [$col] = $ref;
			}
		}
		return $self-> {widths}-> [$col];
	} elsif ( !$self-> {constantCellWidth}) {
		$width = 1 if $width < 1;
		return if
			defined($self-> {widths}-> [$col]) &&
			$width == $self-> {widths}-> [$col];
		$self-> {widths}-> [$col] = $width;
		$self-> notify(q(SetExtent), 0, $col, $width);
		$self-> reset;
		$self-> repaint;
	} else {
		$self-> constantCellWidth( $width);
	}
}

sub rowHeight
{
	my ( $self, $row, $height) = @_;
	if ( $#_ <= 1) {
		unless ( defined $self-> {heights}-> [$row]) {
			if ( defined $self-> {constantCellHeight}) {
				$self-> {heights}-> [$row] = $self-> {constantCellHeight};
			} else {
				my $ref = 0;
				$self-> notify(q(Measure), 1, $row, \$ref);
				$ref = 1 if $ref < 1;
				$self-> {heights}-> [$row] = $ref;
			}
		}
		return $self-> {heights}-> [$row];
	} elsif ( !$self-> {constantCellHeight}) {
		$height = 1 if $height < 1;
		return if
			defined($self-> {heights}-> [$row]) &&
			$height == $self-> {heights}-> [$row];
		$self-> {heights}-> [$row] = $height;
		$self-> notify(q(SetExtent), 1, $row, $height);
		$self-> reset;
		$self-> repaint;
	} else {
		$self-> constantCellHeight( $height);
	}
}

sub columns
{
	return $_[0]-> {columns} unless $#_;
	$_[0]-> raise_ro('columns') if $_[0]-> alive != 2; # AbstractGrid does it inside init
}

sub rows
{
	return $_[0]-> {rows} unless $#_;
	$_[0]-> raise_ro('rows') if $_[0]-> alive != 2 # AbstractGrid does it inside init;
}

sub constantCellWidth
{
	return $_[0]-> {constantCellWidth} unless $#_;
	my ( $self, $w) = @_;
	$self-> {widths}  = [( $self-> {constantCellWidth}  ) x $self-> {columns}];
	$self-> SUPER::constantCellWidth( $w);
}

sub constantCellHeight
{
	return $_[0]-> {constantCellHeight} unless $#_;
	my ( $self, $h) = @_;
	$self-> {heights} = [( $self-> {constantCellHeight} ) x $self-> {rows}];
	$self-> SUPER::constantCellHeight( $h);
}

sub cell
{
	my ( $self, $x, $y, $data) = @_;
	my ( $r, $c) = ( $self-> {rows}, $self-> {columns});
	return if $x < 0 || $x >= $c || $y < 0 || $y >= $c;
	if ( $#_ <= 2) {
		return $self-> {cells}-> [$y]-> [$x];
	} else {
		$self-> {cells}-> [$y]-> [$x] = $data;
	}
}

sub cells
{
	return map { [ @$_ ] } @{$_[0]-> {cells}} unless $#_;
	my ( $self, @cells) = @_;
	@cells = @{$cells[0]} if ( scalar(@cells) == 1) && ( ref($cells[0]) eq 'ARRAY');
	$self-> {cells} = \@cells;
	$self-> SUPER::columns( scalar @{$cells[0]});
	$self-> SUPER::rows( scalar @cells);
	$self-> {widths}  = [( $self-> {constantCellWidth}  ) x $self-> {columns}];
	$self-> {heights} = [( $self-> {constantCellHeight} ) x $self-> {rows}];
}

sub add_column
{
	my $self = shift;
	$self-> insert_column( $self-> {columns}, @_);
}

sub add_columns
{
	my $self = shift;
	$self-> insert_columns( $self-> {columns}, @_);
}

sub add_row
{
	my $self = shift;
	$self-> insert_row( $self-> {rows}, @_);
}

sub add_rows
{
	my $self = shift;
	$self-> insert_rows( $self-> {rows}, @_);
}

sub delete_columns
{
	my ( $self, $column, $how_many) = @_;
	my $c = $self-> {columns};
	$column = $c if $column > $c;
	splice( @$_, $column, $how_many) for @{$self-> {cells}};
	splice( @{$self-> {widths}}, $column, $how_many);
	$self-> SUPER::columns( scalar @{$self-> {cells}-> [0]});
}

sub delete_rows
{
	my ( $self, $row, $how_many) = @_;
	my $r = $self-> {rows};
	$row = $r if $row > $r;
	splice( @{$self-> {cells}}, $row, $how_many);
	splice( @{$self-> {heights}}, $row, $how_many);
	$self-> SUPER::rows( scalar @{$self-> {cells}});
}

sub insert_column
{
	my ( $self, $column, @cells) = @_;
	my $c = $self-> {columns};
	$column = $c if $column > $c;
	my $i;
	my $lim = ( scalar(@cells) < $c) ? scalar(@cells) : $c;
	for ( $i = 0; $i < $lim; $i++) {
		$c = $self-> {cells}-> [$i];
		splice( @$c, $column, 0, $cells[$i]);
	}
	splice( @{$self-> {widths}}, $column, 0, $self-> {constantCellWidths});
	$self-> SUPER::columns( scalar @{$self-> {cells}-> [0]});
}

sub insert_columns
{
	my ( $self, $column, @cells) = @_;
	my $c = $self-> {columns};
	$column = $c if $column > $c;
	my $i;
	my $lim = ( scalar(@cells) < $c) ? scalar(@cells) : $c;
	for ( $i = 0; $i < $lim; $i++) {
		$c = $self-> {cells}-> [$i];
		splice( @$c, $column, 0, @{$cells[$i]});
	}
	splice( @{$self-> {widths}}, $column, 0, ( $self-> {constantCellWidths} ) x scalar(@cells));
	$self-> SUPER::columns( scalar @{$self-> {cells}-> [0]});
}

sub insert_row
{
	my ( $self, $row, @cells) = @_;
	my $r = $self-> {rows};
	$row = $r if $row > $r;
	splice( @{$self-> {cells}},   $row, 0, [@cells]);
	splice( @{$self-> {heights}}, $row, 0, $self-> {constantCellHeight});
	$self-> SUPER::rows( scalar @{$self-> {cells}});
}

sub insert_rows
{
	my ( $self, $row, @cells) = @_;
	my $r = $self-> {rows};
	$row = $r if $row > $r;
	splice( @{$self-> {cells}},   $row, 0, @cells);
	splice( @{$self-> {heights}}, $row, 0, ( $self-> {constantCellHeight} ) x scalar(@cells));
	$self-> SUPER::rows( scalar @{$self-> {cells}});
}

package Prima::Grid;
use vars qw(@ISA);
@ISA = qw(Prima::GridViewer);

sub draw_cells
{
	shift-> std_draw_text_cells(@_);
}

sub get_cell_text
{
	my ( $self, $col, $row) = @_;
	return $self-> {cells}-> [$row]-> [$col];
}

sub get_cell_alignment
{
	return (ta::Left, ta::Middle);
}

sub on_getrange
{
	my ( $self, $column, $index, $min, $max) = @_;
	$$min = $self-> font-> height + 2 unless $column;
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> constantCellHeight( $self-> font-> height + 2 ) if
		$self-> constantCellHeight;
}

sub on_measure
{
	my ( $self, $vertical, $index, $sref) = @_;
	if ( $vertical) {
		$$sref = $self-> font-> height + 2;
	} else {
		$$sref = 0;
		for ( @{$self-> {cells}}) {
			my $w = $self-> get_text_width( $$_[$index], 1);
			$$sref = $w if $$sref < $w;
		}
	}
}

sub on_stringify
{
	my ( $self, $col, $row, $sref) = @_;
	$$sref = $self-> {cells}-> [$row]-> [$col];
}

1;

=pod

=head1 NAME

Prima::Grids - grid widgets

=head2 SYNOPSIS

	use Prima qw(Grids Application);

	my $grid = Prima::Grid-> new(
		cells       => [
			[qw(1.First 1.Second 1.Third)],
			[qw(2.First 2.Second 2.Third)],
			[qw(3.First 3.Second 3.Third)],
		],
		onClick     => sub {
			print $_[0]-> get_cell_text( $_[0]-> focusedCell), " is selected\n";
		}
	);
	run Prima;

=for podview <img src="grid.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/grid.gif">

=head1 DESCRIPTION

The module provides classes for several abstraction layers for the representation
of grid widgets. The class hierarchy is as follows:

	AbstractGridViewer
		AbstractGrid
		GridViewer
			Grid

The root class, C<Prima::AbstractGridViewer>, provides a common interface, while
by itself it is not directly usable.  The main difference between classes is in
the way the cell data are stored. The simplest organization of a text-only
cell, provided by C<Prima::Grid>, stores data as a two-dimensional array of
text scalars. More elaborated storage and representation types are not
realized, and the programmer is urged to use the more abstract classes to
derive their own mechanisms.  To organize an item storage different from
C<Prima::Grid> it is usually enough to overload either the C<Stringify>,
C<Measure>, and C<DrawCell> events, or their method counterparts:
C<get_cell_text>, C<columnWidth>, C<rowHeight>, and C<draw_items>.

The grid widget is designed to contain cells of variable extents, of two types,
normal and indent. The indent rows and columns are displayed near to the widget
borders, and their cells are drawn with distinguished colors.  An example usage
for a bottom indent row is a sum row in a spreadsheet application; the top
indent row can be used for displaying columns' headers. The normal cells can be
selected by the user, scrolled, and selected. The cell selection can only
contain rectangular areas and therefore is operated with two integer pairs
at the beginning and the end of the selection.

The widget operates in two visual scrolling modes; when the space allows, the
scrollbars affect the leftmost and the topmost cell. When the widget is not
large enough to accommodate at least one cell and all indent cells, the layout
is scrolled pixel-wise. These modes are named 'cell' and 'pixel', after the
scrolling units.

The widget allows the interactive changing of cell widths and heights by
dragging the grid lines between the cells.

=head1 Prima::AbstractGridViewer

C<Prima::AbstractGridViewer>, the base for all grid widgets in the module,
provides the interface to generic grid browsing functionality, plus some
functionality for text-oriented grids. The class is not usable directly.

C<Prima::AbstractGridViewer> is a descendant of C<Prima::Widget::GroupScroller>,
and some of its properties are not described here.

=head2 Properties

=over

=item allowChangeCellHeight BOOLEAN

If 1, the user is allowed to change the vertical extents of cells by dragging the
horizontal grid lines. The prerequisites are: the lines must be set visible via
the C<drawHGrid> property, the C<constantCellHeight> property set to 0, and the changes
to the vertical extents can be recorded via the C<SetExtent> notification.

Default value: 0

=item allowChangeCellWidth BOOLEAN

If 1, the user is allowed to change the horizontal extents of cells by dragging the
horizontal grid lines. The prerequisites are: the lines must be set visible via
the C<drawVGrid> property, the C<constantCellWidth> property set to 0, and the changes
to the horizontal extents can be recorded via the C<SetExtent> notification.

Default value: 0

=item cellIndents X1, Y1, X2, Y2

Marks the marginal rows and columns as 'indent' cells. The indent cells are
drawn with another color pair ( see L<indentCellColor>, L<indentCellBackColor>
), and cannot be selected and scrolled. X1 and X2 correspond to the number of the
indent columns, and Y1 and Y2 to the number of the indent rows.

C<leftCell> and C<topCell> do not count the indent cells as the leftmost or topmost
visible cells; in other words, X1 and Y1 are minimal values for C<leftCell> and C<topCell>
properties.

Default value: 0,0,0,0

=item clipCells INTEGER

A three-state integer property that manages the way clipping is applied
when cells are drawn. Depending on the kind of graphic in cells, the clipping
may be necessary, or unnecessary.

If the value is 1, the clipping is applied for every column drawn, as the
default drawing routines proceeds column-wise. If the value is 2, the clipping
is applied for every cell. This setting reduces the drawing speed significantly.
If the value is 0, no clipping is applied.

This property is destined for custom-drawn grid widgets when it is the
developer's task to decide what kind of clipping suits better. Text grid
widgets, C<Prima::AbstractGrid> and C<Prima::Grid>, are safe with C<clipCells>
set to 1.

Default value: 1

=item columns INTEGER

Sets the number of columns, including the indent columns. The number of
columns must be larger than the number of indent columns.

Default value: 0.

=item columnWidth COLUMN [ WIDTH ]

A run-time property, selects the width of a column. To acquire or set the width,
C<Measure> and C<SetExtent> notifications can be invoked. The result of C<Measure>
may be cached internally using the C<cache_geometry_requests> method.

The width does not include the widths of eventual vertical grid lines.

If C<constantCellWidth> is defined, the property is used as its alias.

=item constantCellHeight HEIGHT

If defined, all rows have equal height, HEIGHT pixels. If C<undef>,
rows have different heights.

Default value: undef

=item constantCellWidth WIDTH

If defined, all rows have equal width, WIDTH pixels. If C<undef>,
columns have different widths.

Default value: undef

=item drawHGrid BOOLEAN

If 1, horizontal grid lines between cells are drawn with C<gridColor>.

Default value: 1

=item drawVGrid

If 1, vertical grid lines between cells are drawn with C<gridColor>.

Default value: 1

=item dx INTEGER

A run-time property. Selects horizontal offset in pixels of grid layout
in pixel mode.

=item dy INTEGER

A run-time property. Selects vertical offset in pixels of grid layout
in pixel mode.

=item focusedCell X, Y

Selects coordinates or the focused cell.

=item gridColor COLOR

Selects the color of the grid lines.

Default value: C<cl::Black> .

=item gridGravity INTEGER

The property selects the breadth of the area around the grid lines that react to
the grid-dragging mouse events. The minimal value of 0, marks only grid lines
themselves as the dragging areas but makes the operation inconvenient for the
user.  Larger values make the dragging more convenient, but increase the chance
that the user will not be able to select too narrow cells with the mouse.

Default value: 3

=item indentCellBackColor COLOR

Selects the background color of the indent cells.

Default value: C<cl::Gray> .

=item indentCellColor

Selects the foreground color of the indent cells.

Default value: C<cl::Gray> .

=item leftCell INTEGER

Selects the index of the leftmost visible normal cell.

=item multiSelect BOOLEAN

If 1, the normal cells in an arbitrary rectangular area can be marked
as selected ( see L<selection> ). If 0, only one cell at a time
can be selected.

Default value: 0

=item rows INTEGER

Sets the number of rows, including the indent rows. The number of
rows must be larger than the number of the indent rows.

Default value: 0.

=item topCell

Selects the index of the topmost visible normal cell.

=item rowHeight INTEGER

A run-time property, selects the height of a row. To acquire or set the height,
C<Measure> and C<SetExtent> notifications can be invoked. The result of C<Measure>
may be cached internally using the C<cache_geometry_requests> method.

The height does not include the heights of eventual horizontal grid lines.

If C<constantCellHeight> is defined, the property is used as its alias.

=item selection X1, Y1, X2, Y2

If C<multiSelect> is 1, manages the extent of a rectangular area that
contains selected cells. If no such area is present, the selection
is (-1,-1,-1,-1), and C<has_selection> returns 0 .

If C<multiSelect> is 0, in get-mode returns the focused cell, and ignores
the parameters in the set-mode.

=back

=head2 Methods

=over

=item cache_geometry_requests CACHE

If CACHE is 1, starts caching results of the C<Measure> notification, thus
making the subsequent C<columnWidth> and C<rowHeight> calls lighter. If CACHE
is 0, flushes the cache.

If a significant geometry change happens during the caching, the cache is not
updated automatically, so it is the caller's responsibility to flush the cache.

=item deselect_all

Removes the selection if C<multiSelect> is 1.

=item draw_cells CANVAS, COLUMNS, ROWS, AREA

A bulk draw routine, called from C<onPaint> to draw individual cells.
AREA is an array of four integers in inclusive-inclusive
coordinates of the widget inferior without borders and scrollbars
( the result of C<get_active_area(2)> call; see L<Prima::Widget::IntIndents/get_active_area> ).

COLUMNS and ROWS are structures that reflect the columns and rows of the cells
to be drawn. Each item in these corresponds to a column or row, and is an
array with the following layout:

	0: column or row index
	1: type; 0 - normal cell, 1 - indent cell
	2: visible cell breadth
	3: visible cell start
	4: visible cell end
	5: real cell start
	6: real cell end

The coordinates are in the inclusive-inclusive coordinate system and
do not include eventual grid space, nor gaps between the indent and
normal cells. By default, internal arrays C<{colsDraw}> and
C<{rowsDraw}> are passed as COLUMNS and ROWS parameters.

In the C<Prima::AbstractGrid> and C<Prima::Grid> classes <draw_cells> is overloaded to
transfer the call to C<std_draw_text_cells>, the text-oriented optimized routine.

=item draw_text_cells SCREEN_RECTANGLES, CELL_RECTANGLES, CELL_INDECES, FONT_HEIGHT

A bulk routine for drawing text cells, is called from C<std_draw_text_cells> .

SCREEN_RECTANGLES and CELL_RECTANGLES are arrays, where each item is a rectangle
with an exterior of a cell. SCREEN_RECTANGLES contains rectangles that cover the
visible area of the cell; CELL_RECTANGLES contains rectangles that span the cell extent
disregarding its eventual partial visibility. For example, a 100-pixel cell with
only its left half visible would contain corresponding arrays [150,150,200,250]
in SCREEN_RECTANGLES, and [150,150,250,250] in CELL_RECTANGLES.

CELL_INDECES contains arrays of the cell coordinates; each array item is an array of
integer pairs where item 0 is the column, and item 1 is the row of the cell.

FONT_HEIGHT is the current font height value, as C<draw_text_cells> is
mostly used for text operations and may require vertical text justification.

=item get_cell_area [ WIDTH, HEIGHT ]

Returns screen area in the inclusive-inclusive pixel coordinates. The area is used
to display normal cells. The extensions are related to the current size of a widget,
however, can be overridden by specifying WIDTH and HEIGHT.

=item get_cell_alignment COLUMN, ROW

Returns two C<ta::> constants for horizontal and vertical cell text alignment.
Since the class does not assume the item storage organization,
the values are queried via the C<GetAlignment> notification.

=item get_cell_text COLUMN, ROW

Returns the text string assigned to the cell in COLUMN and ROW.
Since the class does not assume the item storage organization,
the text is queried via the C<Stringify> notification.

=item get_range VERTICAL, INDEX

Returns a pair of integers, the minimal and maximal breadth of INDEXth column
or row in pixels. If VERTICAL is 1, the rows are queried; if 0, the columns.

The method calls the C<GetRange> notification.

=item get_screen_cell_info COLUMN, ROW

Returns information about the cell in COLUMN and ROW, if it is currently visible.
The returned parameters are indexed by the C<gsci::XXX> constants:

	gsci::COL_INDEX - visual column number where the cell is displayed
	gsci::ROW_INDEX - visual row number where the cell is displayed
	gsci::V_FULL    - cell is fully visible

	gsci::V_LEFT    - an inclusive-inclusive rectangle of the visible
	gsci::V_BOTTOM      part of the cell. These four indices are grouped
	gsci::V_RIGHT       under list constant, gsci::V_RECT.
	gsci::V_TOP

	gsci::LEFT      - an inclusive-inclusive rectangle of the cell, as if
	gsci::BOTTOM        it is fully visible. These four indices are grouped
	gsci::RIGHT         under list constant, gsci::RECT. If gsci::V_FULL
	gsci::TOP           is 1, these values are identical to those in gsci::V_RECT.

If the cell is not visible, returns an empty array.

=item has_selection

Returns a boolean value, indicating whether the grid contains a selection (1) or not (0).

=item point2cell X, Y, [ OMIT_GRID = 0 ]

Returns information about the point X, Y in widget coordinates. The method
returns two integers CX and CY with cell coordinates and an eventual HINTS hash
that contains more information about the pixel location. If OMIT_GRID is set to
1 but the pixel belongs to the grid, the pixels are treated as part of an
adjacent cell.  The call syntax is:

	($CX, $CY, %HINTS) = $self->point2cell( $X, $Y);

If the pixel lies within the cell boundaries by either coordinate, CX and/or CY
are correspondingly set to that cell column and/or row. When the pixel is outside
the cell space, CX and/or CY are set to -1.

HINTS may contain the following values:

=over

=item C<x> and C<y>

If 0, the coordinate lies within the boundaries of a cell.

If -1, the coordinate is on the left/top of the cell body.

If +1, the coordinate is on the right/bottom of the cell body, but within
the widget.

If +2, the coordinate is on the right/bottom of the cell body, but outside
the widget.

=item C<x_type> and C<y_type>

Present when C<x> or C<y> values are 0.

If 0, the cell is a normal cell.

If -1, the cell is a left or a top indent cell.

If +1, the cell is a right or a bottom indent cell.

=item C<x_grid> and C<y_grid>

If 1, the point is at a grid line. This case can only happen when OMIT_GRID is 0.
If C<allowChangeCellHeight> and/or C<allowChangeCellWidth> are set, treats also
C<gridGravity>-broad pixels strips on both sides of the line as the grid.

Also, values of C<x_left>/C<x_right> or C<y_bottom>/C<y_top> might be set.

=item C<x_left>/C<x_right> and C<y_bottom>/C<y_top>

Present together with C<x_grid> or C<y_grid>. Contain the indices of
the cells adjacent to the grid line.

=item C<x_gap> and C<y_gap>

If 1, the point is inside a gap between the last normal cell and the first
right/bottom indent cell.

=item C<normal>

If 1, the point lies within the boundaries of a normal cell.

=item C<indent>

If 1, the point lies within the boundaries of an indent cell.

=item C<grid>

If 1, the point is at a grid line.

=item C<exterior>

If 1, the point is in an inoperable area or outside the widget boundaries.

=back

=item redraw_cell X, Y

Repaints the cell with coordinates X and Y.

=item reset

Recalculates internal geometry variables.

=item select_all

Marks all cells as selected if C<multiSelect> is 1.

=item std_draw_text_cells CANVAS, COLUMNS, ROWS, AREA

An optimized bulk routine for text-oriented grid widgets. The optimization
is achieved under the assumption that each cell is drawn with two colors only
so that the color switching can be reduced.

The routine itself paints the cells' background and then calls C<draw_text_cells>
to draw the cells' content.

For the explanation of COLUMNS, ROWS, and AREA parameters see L<draw_cells> .

=back

=head2 Events

=over

=item DrawCell CANVAS, COLUMN, ROW, INDENT, @SCREEN_RECT, @CELL_RECT, SELECTED, FOCUSED, PRELIGHT

Called when a cell with COLUMN and ROW coordinates is to be drawn on CANVAS.
SCREEN_RECT is a cell rectangle in widget coordinates,
where the item is to be drawn. CELL_RECT is the same as SCREEN_RECT but calculated
as if the cell is fully visible.

SELECTED, FOCUSED, and PRELIGHT are boolean flags, if the cell must be drawn
correspondingly in selected, focused, and pre-lighted states.

=item GetAlignment COLUMN, ROW, HORIZONTAL_ALIGN_REF, VERTICAL_ALIGN_REF

Stores two text alignment C<ta::> constants, assigned to the cell with COLUMN and ROW coordinates,
into HORIZONTAL_ALIGN_REF and VERTICAL_ALIGN_REF scalar references.

=item GetRange VERTICAL, INDEX, MIN, MAX

Stores minimal and maximal breadth of INDEXth column ( VERTICAL = 0 ) or row ( VERTICAL = 1)
in the corresponding MIN and MAX scalar references.

=item Measure VERTICAL, INDEX, BREADTH

Stores breadth in pixels of the INDEXth column ( VERTICAL = 0 ) or row ( VERTICAL = 1)
into BREADTH scalar reference.

This notification by default may be called from within
the C<begin_paint_info/end_paint_info> brackets. To disable this feature
set the internal flag C<{NoBulkPaintInfo}> to 1.

=item SelectCell COLUMN, ROW

Called when a cell with COLUMN and ROW coordinates is focused.

=item SetExtent VERTICAL, INDEX, BREADTH

Reports breadth in pixels of the INDEXth column ( VERTICAL = 0 ) or row ( VERTICAL = 1),
as a response to the C<columnWidth> and C<rowHeight> calls.

=item Stringify COLUMN, ROW, TEXT_REF

Stores the text string, assigned to the cell with COLUMN and ROW coordinates, into the TEXT_REF
scalar reference.

=back

=head1 Prima::AbstractGrid

The same as its ascendant, C<Prima::AbstractGridViewer>, except that it
does not propagate the C<DrawItem> message, assuming that the items must be drawn
as text.

=head1 Prima::GridViewer

The class implements cell data and geometry storage mechanisms but leaves
the data format to the programmer. The cells are accessible via
the C<cells> property and several other helper routines.

The cell data are stored in an array, where each item corresponds to a row,
and contains an array of scalars, where each corresponds to a column. All
data managing routines, that accept two-dimensional arrays, assume that
the column arrays are of the same width.

For example, C<[[1,2,3]]]> is a valid one-row, three-column structure, and
C<[[1,2],[2,3],[3,4]]> is a valid three-row, two-column structure.
The structure C<[[1],[2,3],[3,4]]> is invalid, since its first row has
one column, while the others have two.

C<Prima::GridViewer> is derived from C<Prima::AbstractGridViewer>.

=head2 Properties

=over

=item allowChangeCellHeight

Default value: 1

=item allowChangeCellWidth

Default value: 1

=item cell COLUMN, ROW, [ DATA ]

Run-time property. Selects the data in the cell with COLUMN and ROW coordinates.

=item cells [ ARRAY ]

The property accepts or returns all cells as a two-dimensional
rectangular array or scalars.

=item columns INDEX

A read-only property; returns the number of columns.

=item rows INDEX

A read-only property; returns the number of rows.

=back

=head2 Methods

=over

=item add_column CELLS

Inserts a one-dimensional array of scalars to the end of columns.

=item add_columns CELLS

Inserts a two-dimensional array of scalars to the end of columns.

=item add_row CELLS

Inserts a one-dimensional array of scalars to the end of rows.

=item add_rows CELLS

Inserts a two-dimensional array of scalars to the end of rows.

=item delete_columns OFFSET, LENGTH

Removes LENGTH columns starting from OFFSET. Negative values
are accepted.

=item delete_rows OFFSET, LENGTH

Removes LENGTH rows starting from OFFSET. Negative values
are accepted.

=item insert_column OFFSET, CELLS

Inserts a one-dimensional array of scalars as column OFFSET.
Negative values are accepted.

=item insert_columns OFFSET, CELLS

Inserts a two-dimensional array of scalars in column OFFSET.
Negative values are accepted.

=item insert_row

Inserts a one-dimensional array of scalars as row OFFSET.
Negative values are accepted.

=item insert_rows

Inserts a two-dimensional array of scalars in row OFFSET.
Negative values are accepted.

=back

=head1 Prima::Grid

Descendant of C<Prima::GridViewer>, declares format of cell data as a single
text string.  Provides the standard text grid widget that has all the
functionality of its ascendants.

=head2 Methods

=over

=item get_cell_alignment COLUMN, ROW

Returns two C<ta::> constants for horizontal and vertical cell text alignment.
Since the item storage organization is implemented, does
so without calling the C<GetAlignment> notification.

=item get_cell_text COLUMN, ROW

Returns text string assigned to the cell in COLUMN and ROW.
Since the item storage organization is implemented, does
so without calling the C<Stringify> notification.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, F<examples/grid.pl>

=cut

