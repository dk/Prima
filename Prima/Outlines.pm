# contains:
#   OutlineViewer
#   StringOutline
#   Outline
#   DirectoryOutline
package Prima::Outlines;
our %images;
our $default_style = 'plusminus';
use strict;
use warnings;
use Cwd;
use Prima qw(StdBitmap);

sub init_image_plusminus
{
	return if $images{plusminus};

	my $i = 0;
	my $uis = $::application->uiScaling;
	my $iw = int( $uis * 11 + .5);
	$iw++ unless $iw % 2;
	my @imageSize = ($iw, $iw);
	my $xd = int(2 * $uis + .5);
	my $lw = int( $uis + .5);
	$lw-- if $lw > 1 && $lw % 2;
	my @c = map { int ( $_ / 2 ) } @imageSize;

	my @images;
	for my $i (0,1) {
		$images[$i] = Prima::DeviceBitmap->new(
			size      => \@imageSize,
			backColor => cl::White,
			color     => cl::Gray,
		);
		$images[$i]->clear;
		$images[$i]->lineWidth($lw);
		$images[$i]->lineEnd(le::Square);
		$images[$i]->rectangle(0,0,$imageSize[0]-1,$imageSize[1]-1);
		$images[$i]->color(cl::Black);
		$images[$i]->line( $xd, $c[1], $imageSize[0]-$xd-1, $c[1] );
	}
	$images[1]->line( $c[0], $xd, $c[0], $imageSize[1]-$xd-1);

	$images{plusminus}->{expanded}  = $images[0];
	$images{plusminus}->{collapsed} = $images[1];
}

sub init_image_triangle
{
	return if $images{plusminus};

	my $i = 0;
	my $uis = $::application->uiScaling;
	my $iw = int( $uis * 11 + .5);
	$iw++ unless $iw % 2;
	my @imageSize = ($iw, $iw);
	my $lw = $::application->can_draw_alpha ? $uis : int($uis);
	my @c2 = map { int ( $_ / 2 ) } @imageSize;
	my $lw2 = $lw * 2;

	my @images;
	for my $i (0,1) {
		$images[$i] = Prima::DeviceBitmap->new(
			antialias => 1,
			size      => \@imageSize,
			backColor => cl::Back | wc::ListBox,
			color     => cl::Fore | wc::ListBox,
		);
		$images[$i]->clear;
		$images[$i]->lineWidth($lw);
		$images[$i]->lineJoin(lj::Miter);
		$images[$i]->translate(0, $imageSize[1]*0.25) if $i;
		$images[$i]->polyline( [
			$i ? (
				$lw2, $c2[1], $imageSize[0]-1-$lw2, $c2[1],
				$c2[0], $lw2, $lw2, $c2[1]
			) : (
				$c2[0], $lw2, $c2[0], $imageSize[1]-1-$lw2,
				$imageSize[0]-1-$lw2, $c2[1], $c2[0], $lw2
			)
		]);
	}

	$images{triangle}->{collapsed} = $images[0];
	$images{triangle}->{expanded}  = $images[1];
}

sub icon
{
	shift if @_ % 2;
	my ( %opt ) = @_;

	my $type  = $opt{type} // 'collapsed';
	my $style = $opt{style} // 'default';

	$style = $default_style if $style eq 'default';
	my $meth = 'init_image_' . $style;
	$meth = 'init_image_plusminus' unless __PACKAGE__->can($meth);
	__PACKAGE__->$meth();

	return $images{$style}->{$type};
}

package Prima::OutlineViewer;
use base qw(
	Prima::Widget
	Prima::Widget::Fader
	Prima::Widget::MouseScroller
	Prima::Widget::GroupScroller
	Prima::Widget::ListBoxUtils
);
__PACKAGE__->inherit_core_methods('Prima::Widget::GroupScroller');

use constant DATA     => 0;
use constant DOWN     => 1;
use constant EXPANDED => 2;
use constant WIDTH    => 3;
use constant SELECTED => 4;

# node record:
#  user fields:
#  0 : item text of ID
#  1 : node subreference ( undef if none)
#  2 : expanded flag
#  private fields
#  3 : item width
#  4 : selected flag

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	%{Prima::Widget::Fader-> notification_types()},
	SelectItem  => nt::Default,
	DrawItem    => nt::Action,
	Stringify   => nt::Action,
	MeasureItem => nt::Action,
	Expand      => nt::Action,
	DragItem    => nt::Default,
);


sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		autoHeight     => 1,
		extendedSelect => 0,
		draggable      => 1,
		drawLines      => undef,
		focusedItem    => -1,
		iconCollapsed  => undef,
		iconExpanded   => undef,
		iconStyle      => 'default',
		indent         => int( 12 * $::application->uiScaling + .5),
		itemHeight     => $def-> {font}-> {height},
		items          => [],
		multiSelect    => 0,
		topItem        => 0,
		offset         => 0,
		scaleChildren  => 0,
		selectable     => 1,
		showItemHint   => 1,
		widgetClass    => wc::ListBox,
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
	$p-> { autoHeight} = 0 if
		exists $p-> { itemHeight}
		&& !exists $p-> {autoHeight};
	$p-> {multiSelect} = 1 if
		exists $p-> { extendedSelect}
		&& $p-> {extendedSelect}
		&& !exists $p-> {multiSelect};
}

use constant STACK_FRAME => 64;

sub init
{
	my $self = shift;
	for ( qw( topItem focusedItem))
		{ $self-> {$_} = -1; }
	for ( qw( scrollTransaction dx dy 
		offset count autoHeight multiSelect extendedSelect
		rows maxWidth hintActive showItemHint draggable))
		{ $self-> {$_} = 0; }
	for ( qw( itemHeight indent))
		{ $self-> {$_} = 1; }
	$self-> {items}      = [];
	$self-> {iconStyle}  = 'default';
	my %profile = $self-> SUPER::init(@_);
	$self-> setup_indents;
	for ( qw( offset itemHeight autoHeight 
		indent items focusedItem topItem showItemHint draggable multiSelect extendedSelect
		iconStyle skin iconCollapsed iconExpanded drawLines
	))
		{ $self-> $_( $profile{ $_}); }
	$self-> reset;
	$self-> reset_scrolls;
	return %profile;
}

# iterates throughout the item tree, calling given sub for each item.
# sub's parameters are:
# 0 - current item record pointer
# 1 - parent item record pointer, undef if top-level
# 2 - index of the current item into $parent->[1] array
# 3 - index of the current item into items
# 4 - level of the item ( 0 is topmost)
# 5 - boolean, whether the current item is last item (e.g.$parent->[1]->[-1] == $parent->[1]->[$_[5]]).
# 6 - index of the current item if visible; undef otherwise. Equal to [3] if $full is 0.
#
# $full - if 0, iterates only expanded ( visible) items, if 1 - all items into the tree

sub iterate
{
	my ( $self, $sub, $full) = @_;
	my $position = 0;
	my $visible = 1;
	my $visual_position = 0;
	my $traverse;
	$traverse = sub {
		my ( $current, $parent, $index, $level, $lastChild) = @_;
		return $current if $sub-> ( $current, $parent, $index, $position, $level,
			$lastChild, $visible ? $visual_position : undef);
		$position++;
		$level++;
		$visual_position++ if $visible;
		if ( $current-> [DOWN] && ( $full || $current-> [EXPANDED])) {
			my $c = scalar @{$current-> [DOWN]};
			my $i = 0;
			my $dive;
			if ( $visible && $full && !$current-> [EXPANDED]) {
				$visible = 0;
				$dive = 1;
			}
			for ( @{$current-> [DOWN]}) {
				my $ret = $traverse-> ( $_, $current, $i++, $level, --$c ? 0 : 1);
				return $ret if $ret;
			}
			$visible = 1 if $dive;
		};
		0;
	};
	my $c = scalar @{$self-> {items}};
	my $i = 0;
	for ( @{$self-> {items}}) {
		my $ret = $traverse-> ( $_, undef, $i++, 0, --$c ? 0 : 1);
		undef $traverse, return $ret if $ret;
	}
	undef $traverse;
}

sub adjust
{
	my ( $self, $index, $action) = @_;
	return unless defined $index;
	my ($node, $lev) = $self-> get_item( $index);
	return unless $node;
	return unless $node-> [DOWN];
	return if $node-> [EXPANDED] == $action;
	$self-> notify(q(Expand), $node, $action);
	$node-> [EXPANDED] = $action;
	my $c = $self-> {count};
	my $f = $self-> {focusedItem};
	$self-> reset_tree;

	my ( $ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);
	$self-> scroll(
		0, ( $c - $self-> {count}) * $ih,
		clipRect => [ @a[0..2], $a[3] - $ih * ( $index - $self-> {topItem} + 1)]
	);
	$self-> invalidate_rect(
		$a[0], $a[3] - ( $index - $self-> {topItem} + 1) * $ih,
		$a[2], $a[3] - ( $index - $self-> {topItem}) * $ih
	);
	$self-> {doingExpand} = 1;
	if ( $c > $self-> {count} && $f > $index) {
		if ( $f <= $index + $c - $self-> {count}) {
			$self-> focusedItem( $index);
		} else {
			$self-> focusedItem( $f + $self-> {count} - $c);
		}
	} elsif ( $c < $self-> {count} && $f > $index) {
		$self-> focusedItem( $f + $self-> {count} - $c);
	}
	$self-> {doingExpand} = 0;
	my ($ix,$l) = $self-> get_item( $self-> focusedItem);

	$self-> update_tree;

	$self-> reset_scrolls;
	$self-> offset( $self-> {offset} + $self-> {indent})
		if $action && $c != $self-> {count};
}

sub expand_all
{
	my ( $self, $node) = @_;
	$node = [ 0, $self-> {items}, 1] unless $node;
	$self-> {expandAll}++;
	if ( $node-> [DOWN]) {
		#  - light version of adjust
		unless ( $node-> [EXPANDED]) {
			$node-> [EXPANDED] = 1;
			$self-> notify(q(Expand), $node, 1);
		}
		$self-> expand_all( $_) for @{$node-> [DOWN]};
	};
	return if --$self-> {expandAll};
	delete $self-> {expandAll};
	$self-> reset_tree;
	$self-> update_tree;
	$self-> repaint;
	$self-> reset_scrolls;
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	my @size   = $canvas-> size;
	my @clr    = $self-> enabled ?
	( $self-> color, $self-> backColor) :
	( $self-> disabledColor, $self-> disabledBackColor);
	my ( $ih, $iw, $lines, $indent, $foc, @a) = (
		$self-> { itemHeight}, $self-> { maxWidth}, $self->{drawLines},
		$self-> {indent}, $self-> {focusedItem}, $self-> get_active_area( 1, @size));
	my @images = @{ $self }{qw(iconExpanded iconCollapsed) };
	my $i;
	my $j;
	my $locWidth = $a[2] - $a[0] + 1;
	my @clipRect = $canvas-> clipRect;
	if (
		$clipRect[0] > $a[0] &&
		$clipRect[1] > $a[1] &&
		$clipRect[2] < $a[2] &&
		$clipRect[3] < $a[3]
	) {
		$canvas-> clipRect( @a);
		$canvas-> color( $clr[1]);
		$canvas-> bar( 0, 0, @size);
	} else {
		$self-> draw_border( $canvas, $clr[1], @size);
		$canvas-> clipRect( @a);
	}

	my ( $topItem, $rows) = ( $self-> {topItem}, $self-> {rows});
	my $lastItem  = $topItem + $rows + 1;
	my $timin = $topItem;
	$timin    += int(( $a[3] - $clipRect[3]) / $ih) if $clipRect[3] < $a[3];

	if ( $clipRect[1] >= $a[1]) {
		my $y = $a[3] - $clipRect[1] + 1;
		$lastItem = $topItem + int($y / $ih) + 1;
	}
	$lastItem     = $self-> {count} - 1 if $lastItem > $self-> {count} - 1;
	my $firstY    = $a[3] + 1 + $ih * $topItem;
	my $lineY     = $a[3] + 1 - $ih * ( 1 + $timin - $topItem);
	my $dyim      = int(( $ih - $self->{imageSize}->[1]) / 2) + 1;
	my $dxim      = int( $self->{imageSize}->[0] / 2);

# drawing lines
	my @lines;
	my @marks;
	my @texts;

	my $deltax = - $self-> {offset} + ($indent/2) + $a[0];
	$canvas-> set(
		fillPattern => fp::SimpleDots,
		color       => cl::White,
		backColor   => cl::Black,
		rop2        => rop::CopyPut,
	) if $lines;

	my ($array, $idx, $lim, $level) = ([['root'],$self-> {items}], 0, scalar @{$self-> {items}}, 0);
	my @stack;
	my $position = 0;
	my $prelight = $self->{prelight} // -1;

# preparing stack
	$i = int(( $timin + 1) / STACK_FRAME) * STACK_FRAME - 1;
#   $i = int( $timin / STACK_FRAME) * STACK_FRAME - 1;
	if ( $i >= 0) {
#  if ( $i > 0) {
		$position = $i;
		$j = int(( $timin + 1) / STACK_FRAME) - 1;
#     $j = int( $timin / STACK_FRAME) - 1;
		$i = $self-> {stackFrames}-> [$j];
		if ( $i) {
			my $k;
			for ( $k = 0; $k < scalar @{$i} - 1; $k++) {
				$idx   = $i-> [$k] + 1;
				$lim   = scalar @{$array-> [DOWN]};
				push( @stack, [ $array, $idx, $lim]);
				$array = $array-> [1]-> [$idx - 1];
			}
			$idx   = $$i[$k];
			$lim   = scalar @{$array-> [DOWN]};
			$level = scalar @$i - 1;
			$i = $self-> {lineDefs}-> [$j];
			$lines[$k] = $$i[$k] while $k--;
		}
	}

# following loop is recursive call turned inside-out -
# so we can manipulate with stack
	if ( $position <= $lastItem) {
	while (1) {
		my $node      = $array-> [DOWN]-> [$idx++];
		my $lastChild = $idx == $lim;

		# outlining part
		my $l = int(( $level + 0.5) * $indent) + $deltax;
		if ( $lastChild) {
			if ( defined $lines[ $level]) {
				$canvas-> bar(
					$l, $firstY - $ih * $lines[ $level],
					$l, $firstY - $ih * ( $position + 0.5)
				) if $lines and $position >= $timin;
				$lines[ $level] = undef;
			} elsif ( $position > 0) {
				# first and last
				$canvas-> bar(
					$l, $firstY - $ih * ( $position - 0.5),
					$l, $firstY - $ih * ( $position + 0.5)
				) if $lines;
			}
		} elsif ( !defined $lines[$level]) {
			$lines[$level] = $position ? $position - 0.5 : 0.5;
		}
		if ( $position >= $timin) {
			$canvas-> bar( $l + 1, $lineY + $ih/2, $l + $indent - 1, $lineY + $ih/2)
				if $lines;
			if ( defined $node-> [DOWN]) {
				my $i = $images[($node-> [EXPANDED] == 0) ? 1 : 0];
				push( @marks, [$l - $dxim, $lineY + $dyim, $i]) if $i;
			};
			push ( @texts, [ $node, $l + $indent * 1.5, $lineY,
				$l + $indent * 1.5 + $node-> [WIDTH] - 1, $lineY + $ih - 1,
				$position,
				$self-> {multiSelect} ? $node-> [SELECTED] : ($foc == $position),
				$foc == $position,
				$prelight == $position,
			]);
			$lineY -= $ih;
		}
		last if $position >= $lastItem;

		# recursive part
		$position++;

		if ( $node-> [DOWN] && $node-> [EXPANDED] && scalar @{$node-> [DOWN]}) {
			$level++;
			push ( @stack, [ $array, $idx, $lim]);
			$idx   = 0;
			$array = $node;
			$lim   = scalar @{$node-> [DOWN]};
			next;
		}
		while ( $lastChild) {
			last unless $level--;
			( $array, $idx, $lim) = @{pop @stack};
			$lastChild = $idx == $lim;
		}
	}}

# drawing line ends
	if ( $lines ) {
		$i = 0;
		for ( @lines) {
			$i++;
			next unless defined $_;
			my $l = ( $i - 0.5) * $indent + $deltax;;
			$canvas-> bar( $l, $firstY - $ih * $_, $l, 0);
		}
		$canvas-> set(
			fillPattern => fp::Solid,
			color       => $clr[0],
			backColor   => $clr[1],
			rop2        => rop::NoOper,
		);
	}

#
	$canvas-> put_image( @$_) for @marks;
	$self-> draw_items( $canvas, \@texts);
}

sub on_size
{
	my $self = $_[0];
	$self-> reset;
	$self-> reset_scrolls;
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> itemHeight( $self-> font-> height), $self-> {autoHeight} = 1
		if $self-> { autoHeight};
	$self-> calibrate;
}

sub point2item
{
	my ( $self, $y, $h) = @_;
	my $i = $self-> {indents};
	$h = $self-> height unless defined $h;
	return $self-> {topItem} - 1 if $y >= $h - $$i[3];
	return $self-> {topItem} + $self-> {rows} if $y <= $$i[1];
	$y = $h - $y - $$i[3];
	return $self-> {topItem} + int( $y / $self-> {itemHeight});
}


sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	my $bw = $self-> { borderWidth};
	my @size = $self-> size;
	$self-> clear_event;
	my ($o,$i,@a) = ( $self-> {offset}, $self-> {indent}, $self-> get_active_area(0, @size));
	return if $btn != mb::Left;
	return if
		defined $self-> {mouseTransaction} ||
		$y < $a[1] ||
		$y >= $a[3] ||
		$x < $a[0] ||
		$x >= $a[2];

	my $item   = $self-> point2item( $y, $size[1]);
	my ( $rec, $lev) = $self-> get_item( $item);

	if (
		$rec &&
		( $x >= ( 1 + $lev) * $i + $a[0] - $o - $self->{imageSize}->[0] / 2) &&
		( $x <  ( 1 + $lev) * $i + $a[0] - $o + $self->{imageSize}->[0] / 2)
	) {
		$self-> adjust( $item, $rec-> [2] ? 0 : 1) if $rec-> [1];
		return;
	}

	my $foc = $item >= 0 ? $item : 0;
	if ( $self-> {multiSelect}) {
		if ( $self-> {extendedSelect}) {
			if ($mod & km::Shift) {
				my $foc = $self-> focusedItem;
				return $self-> selectedItems(( $foc < $item) ? [$foc..$item] : [ $item..$foc]);
			} elsif ( $mod & km::Ctrl) {
				return $self-> toggle_item( $item);
			}
			$self-> {anchor} = $item;
			$self-> selectedItems([$foc]);
		} elsif ( $mod & (km::Ctrl||km::Shift)) {
			return $self-> toggle_item( $item);
		}
	}

	$self-> {mouseTransaction} =
		(( $mod & ( km::Alt | ($self-> {multiSelect} ? 0 : km::Ctrl))) && $self-> {draggable}) ? 2 : 1;
	$self-> focusedItem( $item >= 0 ? $item : 0);
	$self-> {mouseTransaction} = 1 if $self-> focusedItem < 0;
	if ( $self-> {mouseTransaction} == 2) {
		$self-> {dragItem} = $self-> focusedItem;
		$self-> {mousePtr} = $self-> pointer;
		$self-> pointer( cr::Move);
	}
	$self-> capture(1);
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	$self-> clear_event;
	return if $btn != mb::Left || !$dbl;
	my $bw = $self-> { borderWidth};
	my @size = $self-> size;
	my $item   = $self-> point2item( $y, $size[1]);
	my ($o,$i) = ( $self-> {offset}, $self-> {indent});
	my ( $rec, $lev) = $self-> get_item( $item);
	if (
		$rec &&
		( $x >= ( 1 + $lev) * $i + $self-> {indents}-> [0] - $o - $self->{imageSize}->[0] / 2) &&
		( $x <  ( 1 + $lev) * $i + $self-> {indents}-> [0] - $o + $self->{imageSize}->[0] / 2)
	) {
		$self-> adjust( $item, $rec-> [EXPANDED] ? 0 : 1) if $rec-> [DOWN];
		return;
	}
	$self-> notify( q(Click)) if $self-> {count};
}

sub makehint
{
	my ( $self, $show, $itemid) = @_;
	return if !$show && !$self-> {hintActive};
	if ( !$show) {
		$self-> {hinter}-> hide;
		$self-> {hintActive} = 0;
		return;
	}
	return if defined $self-> {unsuccessfullId} && $self-> {unsuccessfullId} == $itemid;

	return unless $self-> {showItemHint};

	my ( $item, $lev) = $self-> get_item( $itemid);
	unless ( $item) {
		$self-> makehint(0);
		return;
	}

	my $w = $self-> get_item_width( $item);
	my @a = $self-> get_active_area;
	my $ofs = ( $lev + 2.5) * $self-> {indent} - $self-> {offset} + $self-> {indents}-> [0];

	if ( $w + $ofs <= $a[2]) {
		$self-> makehint(0);
		return;
	}

	$self-> {unsuccessfullId} = undef;

	unless ( $self-> {hinter}) {
		$self-> {hinter} = $self-> insert( Widget =>
			clipOwner      => 0,
			selectable     => 0,
			ownerColor     => 1,
			ownerBackColor => 1,
			ownerFont      => 1,
			visible        => 0,
			height         => $self-> {itemHeight},
			name           => 'Hinter',
			delegations    => [qw(Paint MouseDown MouseLeave)],
		);
	}
	$self-> {hintActive} = 1;
	$self-> {hinter}-> {id} = $itemid;
	$self-> {hinter}-> {node} = $item;
	my @org = $self-> client_to_screen(0,0);
	$self-> {hinter}-> set(
		origin  => [
			$org[0] + $ofs - 2,
			$org[1] + $self-> height - $self-> {indents}-> [3] -
				$self-> {itemHeight} * ( $itemid - $self-> {topItem} + 1),
		],
		width   => $w + 4,
		text    => $self-> get_item_text( $item),
		visible => 1,
	);
	$self-> {hinter}-> bring_to_front;
	$self-> {hinter}-> repaint;
}

sub Hinter_Paint
{
	my ( $owner, $self, $canvas) = @_;
	my $c = $self-> color;
	$canvas-> color( $self-> backColor);
	my @sz = $canvas-> size;
	$canvas-> bar( 0, 0, @sz);
	$canvas-> color( $c);
	$canvas-> rectangle( 0, 0, $sz[0] - 1, $sz[1] - 1);
	my @rec = ([ $self-> {node}, 2, 0,
		$sz[0] - 3, $sz[1] - 1, 0, 0
	]);
	$owner-> draw_items( $canvas, \@rec);
}

sub Hinter_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	$owner-> makehint(0);
	my @ofs = $owner-> screen_to_client( $self-> client_to_screen( $x, $y));
	$owner-> mouse_down( $btn, $mod, @ofs);
	$owner-> {unsuccessfullId} = $self-> {id};
}

sub Hinter_MouseLeave
{
	$_[0]-> makehint(0);
}

sub update_prelight
{
	my ( $self, $x, $y ) = @_;

	return delete $self->{prelight} if $self->{mouseTransaction};
	return unless $self->enabled;

	my @size = $self-> size;
	my @a    = $self-> get_active_area( 0, @size);

	my $item   = $self-> point2item( $y, $size[1]);
	my ( $rec, $lev) = $self-> get_item( $item);
	if (
		!$rec ||
		( $x < -$self-> {offset} + ($lev + 2) * $self-> {indent} + $self-> {indents}-> [0])
	) {
		$self-> makehint( 0);
	} elsif (( $y >= $a[3]) || ( $y <= $a[1] + $self-> {itemHeight} / 2)) {
		$self-> makehint( 0);
	} else {
		$y = $a[3] - $y;
		$item = $self-> {topItem} + int( $y / $self-> {itemHeight});
	}
	undef $item if $item >= $self->{count};

	if ( $self-> {showItemHint} ) {
		if ( defined $item ) {
			$self-> makehint( 1, $self-> {topItem} + int( $y / $self-> {itemHeight}));
		} else {
			$self-> makehint( 0);
		}
	}

	if (( $self->{prelight} // -1) != ( $item // -1)) {
		my @redraw = (
			$self->{prelight} // (),
			$item // ()
		);
		if ( defined $item ) {
			$self-> fader_in_mouse_enter unless defined $self->{prelight};
			$self->{prelight} = $item;
		} else {
			$self-> fader_out_mouse_leave;
		}
		$self->redraw_items( @redraw );
	}
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self->update_prelight($x,$y);
	return unless $self-> {mouseTransaction};

	my @size = $self-> size;
	my @a    = $self-> get_active_area( 0, @size);

	my $item = $self-> point2item( $y, $size[1]);
	if ( $y >= $a[3] || $y < $a[1] || $x >= $a[2] || $x < $a[0])
	{
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
	} else {
		$self-> scroll_timer_stop;
	}

	if ( $self-> {multiSelect} && $self-> {extendedSelect} && exists $self-> {anchor})
	{
		my ( $a, $b, $c) = ( $self-> {anchor}, $item, $self-> {focusedItem});
		my $globSelect = 0;
		if (( $b <= $a && $c > $a) || ( $b >= $a && $c < $a)) {
			$globSelect = 1
		} elsif ( $b > $a) {
			if ( $c < $b) { $self-> add_selection([$c + 1..$b], 1) }
			elsif ( $c > $b) { $self-> add_selection([$b + 1..$c], 0) }
			else { $globSelect = 1 }
		} elsif ( $b < $a) {
			if ( $c < $b) { $self-> add_selection([$c..$b], 0) }
			elsif ( $c > $b) { $self-> add_selection([$b..$c], 1) }
			else { $globSelect = 1 }
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
		@dragnotify = ($self-> {dragItem}, $fci) unless $fci == $self-> {dragItem};
	}
	delete $self-> {mouseTransaction};
	delete $self-> {mouseHorizontal};

	$self-> capture(0);
	$self-> clear_event;
	$self-> notify(q(DragItem), @dragnotify) if @dragnotify;
}

sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$z *= $self-> {rows} if $mod & km::Ctrl;
	my $newTop = $self-> topItem - $z;
	my $maxTop = $self-> {count} - $self-> {rows};
	$self-> topItem( $newTop > $maxTop ? $maxTop : $newTop);
	$self-> update_prelight($x,$y);
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

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }

sub on_leave
{
	my $self = $_[0];
	if ( $self-> {mouseTransaction})  {
		$self-> capture(0) if $self-> {mouseTransaction};
		$self-> {mouseTransaction} = undef;
	}
}


sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	return if $mod & km::DeadKey;

	$mod &= ( km::Shift|km::Ctrl|km::Alt);
	$self-> notify(q(MouseUp),0,0,0) if defined $self-> {mouseTransaction};

	return unless $self-> {count};

	if (
		( $key == kb::NoKey) &&
		( $code >= ord(' '))
	) {
		if ( chr($code) eq '+') {
			$self-> adjust( $self-> {focusedItem}, 1);
			$self-> clear_event;
			return;
		} elsif ( chr($code) eq '-') {
			my ( $item, $lev) = $self-> get_item( $self-> {focusedItem});
			if ( $item-> [DOWN] && $item-> [EXPANDED]) {
				$self-> adjust( $self-> {focusedItem}, 0);
				$self-> clear_event;
				return;
			} elsif ( $lev > 0) {
				my $i = $self-> {focusedItem};
				my ( $par, $parlev) = ( $item, $lev);
				( $par, $parlev) = $self-> get_item( --$i) while $parlev != $lev - 1;
				$self-> adjust( $i, 0);
				$self-> clear_event;
				return;
			}
		}

		if (!($mod & (km::Ctrl|km::Alt))) {
			my $i;
			my ( $c, $hit, $items) = ( lc chr $code, undef, $self-> {items});
			for ( $i = $self-> {focusedItem} + 1; $i < $self-> {count}; $i++)
			{
				my $fc = substr( $self-> get_index_text($i), 0, 1);
				next unless defined $fc;
				$hit = $i, last if lc $fc eq $c;
			}
			unless ( defined $hit) {
				for ( $i = 0; $i < $self-> {focusedItem}; $i++)  {
					my $fc = substr( $self-> get_index_text($i), 0, 1);
					next unless defined $fc;
					$hit = $i, last if lc $fc eq $c;
				}
			}
			if ( defined $hit)  {
				$self-> focusedItem( $hit);
				$self-> clear_event;
				return;
			}
		}
		return;
	}

	if ( scalar grep { $key == $_ } (
		kb::Left,kb::Right,kb::Up,kb::Down,kb::Home,kb::End,kb::PgUp,kb::PgDn
	)) {
		my $doSelect = 0;
		my $newItem = $self-> {focusedItem};
		if (
			$mod == 0 ||
			(
				( $mod & km::Shift) &&
				$self-> {multiSelect} &&
				$self-> { extendedSelect}
			)
		) {
			my $pgStep  = $self-> {rows} - 1;
			$pgStep = 1 if $pgStep <= 0;
			if ( $key == kb::Up)   { $newItem--; };
			if ( $key == kb::Down) { $newItem++; };
			if ( $key == kb::Home) { $newItem = $self-> {topItem} };
			if ( $key == kb::End)  { $newItem = $self-> {topItem} + $pgStep; };
			if ( $key == kb::PgDn) { $newItem += $pgStep };
			if ( $key == kb::PgUp) { $newItem -= $pgStep};
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
				defined $self-> {anchor} ? $self-> {anchor} : $self-> {focusedItem},
				$newItem
			);
			( $a, $b) = ( $b, $a) if $a > $b;
			$self-> selectedItems([$a..$b]);
			$self-> {anchor} = $self-> {focusedItem} unless defined $self-> {anchor};
		} else {
			$self-> selectedItems([$self-> focusedItem]) if exists $self-> {anchor};
			delete $self-> {anchor};
		}

		$self-> offset(
			$self-> {offset} +
				$self-> {indent} * (( $key == kb::Left) ? -1 : 1
			)) if $key == kb::Left || $key == kb::Right;
		$self-> focusedItem( $newItem >= 0 ? $newItem : 0);
		$self-> clear_event;
		return;
	}

	if ( $mod == 0 && $key == kb::Enter)  {
		$self-> adjust( $self-> {focusedItem}, 1);
		$self-> clear_event;
		return;
	}
}


sub reset
{
	my $self = $_[0];
	my @size = $self-> get_active_area( 2);
	$self-> makehint(0);
	my $ih   = $self-> {itemHeight};
	$self-> {rows}  = int( $size[1] / $ih);
	$self-> {rows}  = 0 if $self-> {rows} < 0;
	$self-> {yedge} = ( $size[1] - $self-> {rows} * $ih) ? 1 : 0;
}

sub reset_scrolls
{
	my $self = $_[0];
	$self-> makehint(0);
	if ( $self-> {scrollTransaction} != 1) {
		$self-> vScroll( $self-> {rows} < $self-> {count} ) if $self-> {autoVScroll};
		$self-> {vScrollBar}-> set(
			max      => $self-> {count} - $self-> {rows},
			pageStep => $self-> {rows},
			whole    => $self-> {count},
			partial  => $self-> {rows},
			value    => $self-> {topItem},
		) if $self-> {vScroll};
	}

	if ( $self-> {scrollTransaction} != 2) {
		my @sz = $self-> get_active_area( 2);
		my $iw = $self-> {maxWidth};
		if ( $self-> {autoHScroll}) {
			my $hs = ($sz[0] < $iw) ? 1 : 0;
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

sub reset_item_cache
{
	my ( $self, $i) = ( $_[0], 0);
	$self-> {stackFrames} = [];
	$self-> {lineDefs}    = [];
	my @stack;
	my @lines;
	my $traverse;

	$traverse = sub {
		my ( $node, $level, $lastChild) = @_;
		$lines[ $level] = $lastChild ? undef : ( $i ? $i - 0.5 : 0.5);
		if (( $i % STACK_FRAME) == STACK_FRAME - 1) {
			push( @{$self-> {stackFrames}}, [@stack[0..$level]]);
			push( @{$self-> {lineDefs}},    [@lines[0..$level]]);
		}
		$i++;
		$level++;
		if ( $node-> [DOWN] && $node-> [EXPANDED]) {
			$stack[$level] = 0;
			my $c = @{$node-> [DOWN]};
			for ( @{$node-> [DOWN]}) {
				$traverse-> ( $_, $level, --$c ? 0 : 1);
				$stack[$level]++;
			}
		}
	};

	$stack[0] = 0;
	my $c = @{$self-> {items}};
	for (@{$self-> {items}}) {
		$traverse-> ( $_, 0, --$c ? 0 : 1);
		$stack[0]++;
	}
	undef $traverse;

	$self-> {count} = $i;

}

sub reset_tree
{
	my $self = shift;
	$self-> makehint(0);
	$self-> reset_item_cache;
	my $fullc = $self-> {fullCalibrate};
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
	my $maxWidth = 0;
	my $indent = $self-> {indent};
	$self-> push_event;
	$self-> begin_paint_info;
	$self-> iterate( sub {
		my ( $current, $parent, $index, $position, $level, $visibility) = @_;
		my $iw = $fullc ? undef : $current-> [WIDTH];
		unless ( defined $iw) {
			$notifier-> ( @notifyParms, $current, $level, \$iw);
			$current-> [WIDTH] = $iw;
		}
		my $iwc = $iw + ( 2.5 + $level) * $indent;
		$maxWidth = $iwc if $maxWidth < $iwc;
		return 0;
	});
	$self-> end_paint_info;
	$self-> pop_event;
	$self-> {maxWidth} = $maxWidth;
}

sub calibrate
{
	my $self = $_[0];
	$self-> {fullCalibrate} = 1;
	$self-> reset_tree;
	delete $self-> {fullCalibrate};
	$self-> update_tree;
}

sub update_tree
{
	my $self = $_[0];
	$self-> topItem( $self-> {topItem});
	$self-> offset( $self-> {offset});
}

sub redraw_items
{
	my ( $self, @items ) = @_;
	my @a  = $self-> get_active_area;
	my $ih = $self-> {itemHeight};

	my %match;
	my $buffered = $self->buffered;
	$self->buffered(1) unless $buffered;
	for my $i ( @items ) {
		next unless $i >= 0 && $i >= $self-> {topItem} && $i <= $self-> {topItem} + $self-> {rows};
		next if $match{$i}++;
		$i -= $self->{topItem};
		$self-> invalidate_rect(
			$a[0], $a[3] - ( $i + 1) * $ih,
			$a[2], $a[3] - $i * $ih
		);
	}
	unless ($buffered) {
		$self->update_view;
		$self->buffered(0);
	}
}

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(DrawItem));
	$self-> push_event;
	for ( @$paintStruc) { $notifier-> ( @notifyParms, $canvas, @$_); }
	$self-> pop_event;
}

sub set_auto_height
{
	my ( $self, $auto) = @_;
	$self-> itemHeight( $self-> font-> height) if $auto;
	$self-> {autoHeight} = $auto;
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
		if $self-> {multiSelect} && $self-> {extendedSelect} && ! exists $self-> {anchor};
	$self-> notify(q(SelectItem), [[$foc, undef, 1]]) if $foc >= 0;
	return if $self-> {doingExpand};

	my $topSet = undef;
	if ( $foc >= 0) {
		my $rows = $self-> {rows} ? $self-> {rows} : 1;
		if ( $foc < $self-> {topItem}) {
			$topSet = $foc;
		} elsif ( $foc >= $self-> {topItem} + $rows) {
			$topSet = $foc - $rows + 1;
		}
	}
	( $oldFoc, $foc) = ( $foc, $oldFoc) if $foc > $oldFoc;
	my @a  = $self-> get_active_area;
	my $ih = $self-> {itemHeight};
	my $lastItem = $self-> {topItem} + $self-> {rows};
	$self-> redraw_items($oldFoc, $foc);
	$self-> topItem( $topSet) if defined $topSet;
}

sub set_indent
{
	my ( $self, $i) = @_;
	return if $i == $self-> {indent};
	$i = 1 if $i < 1;
	$self-> {indent} = $i;
	$self-> calibrate;
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
	$self-> {hinter}-> height( $ih) if $self-> {hinter};
}

sub validate_items
{
	my ( $self, $items) = @_;
	my $traverse;
	$traverse = sub {
		my $current  = $_[0];
		my $spliceTo = 3;
		if ( ref $current-> [DOWN] eq 'ARRAY') {
			$traverse-> ( $_) for @{$current-> [DOWN]};
			$current-> [EXPANDED] = 0 unless defined $current-> [EXPANDED];
		} else {
			$spliceTo = 1;
		}
		splice( @$current, $spliceTo);
	};
	$traverse-> ( $items);
	undef $traverse;
}

sub set_items
{
	my ( $self, $items) = @_;
	$items = [] unless defined $items;
	$self-> validate_items( [ 0, $items]);
	$self-> {items} = $items;
	$self-> reset_tree;
	$self-> update_tree;
	$self-> repaint;
	$self-> reset_scrolls;
}

sub insert_items
{
	my ( $self, $where, $at, @items) = @_;
	return unless scalar @items;

	my $forceReset = 0;
	$where = [0, $self-> {items}], $forceReset = 1 unless $where;
	$self-> validate_items( $_) for @items;
	return unless $where-> [DOWN];

	my $ch = scalar @{$where-> [DOWN]};
	$at = 0 if $at < 0;
	$at = $ch if $at > $ch;

	my ( $x, $l) = $self-> get_index( $where);
	splice( @{$where-> [DOWN]}, $at, 0, @items);
	return if $x < 0 && !$forceReset;

	$self-> reset_tree;
	$self-> update_tree;
	$self-> repaint;
	$self-> reset_scrolls;
}

sub delete_items
{
	my ( $self, $where, $at, $amount) = @_;
	$where = [0, $self-> {items}] unless $where;
	return unless $where-> [DOWN];

	my ( $x, $l) = $self-> get_index( $where);
	$at = 0 unless defined $at;

	$amount = scalar @{$where-> [DOWN]} unless defined $amount;
	splice( @{$where-> [DOWN]}, $at, $amount);
	return if $x < 0;

	my $f = $self-> {focusedItem};
	$self-> focusedItem( -1) if $f >= $x && $f < $x + $amount;

	$self-> reset_tree;
	$self-> update_tree;
	$self-> repaint;
	$self-> reset_scrolls;
}

sub delete_item
{
	my ( $self, $item) = @_;
	return unless $item;
	my ( $x, $l) = $self-> get_index( $item);

	my ( $parent, $offset) = $self-> get_item_parent( $item);
	if ( defined $parent) {
		splice( @{$parent-> [DOWN]}, $offset, 1);
	} else {
		splice( @{$self-> {items}}, $offset, 1) if defined $offset;
	}

	if ( $x >= 0) {
		$self-> reset_tree;
		$self-> update_tree;
		$self-> focusedItem( -1) if $x == $self-> {focusedItem};
		$self-> repaint;
		$self-> reset_scrolls;
	}
}

sub get_item_parent
{
	my ( $self, $item) = @_;
	my $parent;
	my $offset;
	return unless $item;

	$self-> iterate( sub {
		my ($cur,$par,$idx) = @_;
		$parent = $par, $offset = $idx, return 1 if $cur == $item;
	}, 1);
	return $parent, $offset;
}

sub set_multi_select
{
	my ( $self, $ms) = @_;
	return if $ms == $self-> {multiSelect};

	unless ( $self-> {multiSelect} = $ms) {
		$self-> deselect_all(1);
		$self-> repaint;
	} else {
		$self-> selectedItems([$self-> focusedItem]);
	}
}

sub set_offset
{
	my ( $self, $offset) = @_;
	my ( $iw, @a) = ($self-> {maxWidth}, $self-> get_active_area);

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

	if ( $self-> {hScroll} && $self-> {scrollTransaction} != 2) {
		$self-> {scrollTransaction} = 2;
		$self-> {hScrollBar}-> value( $offset);
		$self-> {scrollTransaction} = 0;
	}

	$self-> makehint(0);
	$self-> scroll( $oldOfs - $offset, 0, clipRect => \@a);
}

sub set_top_item
{
	my ( $self, $topItem) = @_;
	$topItem = 0 if $topItem < 0;   # first validation
	$topItem = $self-> {count} - 1 if $topItem >= $self-> {count};
	$topItem = 0 if $topItem < 0;   # count = 0 case
	return if $topItem == $self-> {topItem};

	my $oldTop = $self-> {topItem};
	$self-> {topItem} = $topItem;
	my ($ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);
	$self-> makehint(0);

	if ( $self-> {scrollTransaction} != 1 && $self-> {vScroll}) {
		$self-> {scrollTransaction} = 1;
		$self-> {vScrollBar}-> value( $topItem);
		$self-> {scrollTransaction} = 0;
	}

	$self-> scroll( 0, ($topItem - $oldTop) * $ih, clipRect => \@a);
}


sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 1;
	$self-> topItem( $scr-> value);
	$self-> {scrollTransaction} = 0;
}

sub HScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 2;
	$self-> {multiColumn} ?
		$self-> topItem( $scr-> value) :
		$self-> offset( $scr-> value);
	$self-> {scrollTransaction} = 0;
}


sub reset_indents
{
	my $self = $_[0];
	$self-> reset;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub showItemHint
{
	return $_[0]-> {showItemHint} unless $#_;
	my ( $self, $sh) = @_;
	return if $sh == $self-> {showItemHint};
	$self-> {showItemHint} = $sh;
	$self-> makehint(0) if !$sh && $self-> {hintActive};
}

sub draggable
{
	return $_[0]-> {draggable} unless $#_;
	$_[0]-> {draggable} = $_[1];
}

sub drawLines
{
	return $_[0]-> {drawLines} unless $#_;
	return unless defined $_[1];
	$_[0]-> {drawLines} = $_[1];
	$_[0]-> repaint;
}

sub iconCollapsed
{
	return $_[0]-> {iconCollapsed} unless $#_;
	return unless $_[1];
	$_[0]-> {iconCollapsed} = $_[1];
	$_[0]-> {imageSize} = [$_[1]->size];
	$_[0]-> repaint;
}

sub iconExpanded
{
	return $_[0]-> {iconExpanded} unless $#_;
	return unless $_[1];
	$_[0]-> {iconExpanded} = $_[1];
	$_[0]-> repaint;
}

sub iconStyle
{
	return $_[0]-> {iconStyle} unless $#_;
	my ( $self, $style ) = @_;
	$self->iconCollapsed( Prima::Outlines->icon( type => 'collapsed', style => $style) );
	$self->iconExpanded(  Prima::Outlines->icon( type => 'expanded' , style => $style) );
	$self->drawLines( $style ne 'triangle' );
}

sub skin
{
	return $_[0]->SUPER::skin unless $#_;
	my $self = shift;
	$self->SUPER::skin($_[1]);
	my $skin = $self->SUPER::skin;
	if ($skin eq 'flat') {
		$self->iconStyle('triangle');
	} elsif ( $skin eq 'classic') {
		$self->iconStyle('plusminus');
	}
	$self->repaint;
}

sub get_index
{
	my ( $self, $item) = @_;
	return -1, undef unless $item;
	my $lev;
	my $rec = -1;
	$self-> iterate( sub {
		my ( $current, $parent, $index, $position, $level, $lastChild, $visibility) = @_;
		$lev = $level, $rec = $position, return 1 if $current == $item;
	});
	return $rec, $lev;
}


sub get_item
{
	my ( $self, $item) = @_;
	return if $item < 0 || $item >= $self-> {count};

	my ($array, $idx, $lim, $level) = ([['root'],$self-> {items}], 0, scalar @{$self-> {items}}, 0);
	my $i = int(( $item + 1) / STACK_FRAME) * STACK_FRAME - 1;
	my $position = 0;
	my @stack;
	if ( $i >= 0) {
		$position = $i;
		$i = $self-> {stackFrames}-> [int( $item + 1) / STACK_FRAME - 1];
		if ( $i) {
			my $k;
			for ( $k = 0; $k < scalar @{$i} - 1; $k++) {
				$idx   = $i-> [$k] + 1;
				$lim   = scalar @{$array-> [DOWN]};
				push( @stack, [ $array, $idx, $lim]);
				$array = $array-> [DOWN]-> [$idx - 1];
			}
			$idx   = $$i[$k];
			$lim   = scalar @{$array-> [DOWN]};
			$level = scalar @$i - 1;
		}
	}

	die "Internal error\n" if $position > $item;
	while (1) {
		my $node      = $array-> [DOWN]-> [$idx++];
		my $lastChild = $idx == $lim;
		return $node, $level if $position == $item;
		$position++;
		if ( $node-> [DOWN] && $node-> [EXPANDED] && scalar @{$node-> [DOWN]}) {
			$level++;
			push ( @stack, [ $array, $idx, $lim]);
			$idx   = 0;
			$array = $node;
			$lim   = scalar @{$node-> [DOWN]};
			next;
		}
		while ( $lastChild) {
			last unless $level--;
			( $array, $idx, $lim) = @{pop @stack};
			$lastChild = $idx == $lim;
		}
	}
}

sub get_item_text
{
	my ( $self, $item) = @_;
	my $txt = '';
	$self-> notify(q(Stringify), $item, \$txt);
	return $txt;
}

sub get_item_width
{
	return $_[1]-> [WIDTH];
}

sub get_index_text
{
	my ( $self, $index) = @_;
	my $txt = '';
	my ( $node, $lev) = $self-> get_item( $index);
	$self-> notify(q(Stringify), $node, \$txt);
	return $txt;
}

sub get_index_width
{
	my ( $self, $index) = @_;
	my ( $node, $lev) = $self-> get_item( $index);
	return $node-> [WIDTH];
}

sub on_drawitem
{
#	my ( $self, $canvas, $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @_;
}

sub on_measureitem
{
#	my ( $self, $node, $level, $result) = @_;
}

sub on_stringify
{
#	my ( $self, $node, $result) = @_;
}

sub on_selectitem
{
#	my ( $self, $index_array, $flag) = @_;
}

sub on_expand
{
#	my ( $self, $node, $action) = @_;
}

sub on_dragitem
{
	my ( $self, $from, $to) = @_;
	my ( $fx, $fl) = $self-> get_item( $from);
	my ( $tx, $tl) = $self-> get_item( $to);
	my ( $fpx, $fpo) = $self-> get_item_parent( $fx);
	return unless $fx && $tx;
	my $found_inv = 0;

	my $traverse;
	$traverse = sub {
		my $current = $_[0];
		$found_inv = 1, return if $current == $tx;
		if ( $current-> [DOWN] && $current-> [EXPANDED]) {
			my $c = scalar @{$current-> [DOWN]};
			for ( @{$current-> [DOWN]}) {
				my $ret = $traverse-> ( $_);
				return $ret if $ret;
			}
		}
	};
	$traverse-> ( $fx);
	undef $traverse;
	return if $found_inv;


	if ( $fpx) {
		splice( @{$fpx-> [DOWN]}, $fpo, 1);
	} else {
		splice( @{$self-> {items}}, $fpo, 1);
	}
	unless ( $tx-> [DOWN]) {
		$tx-> [DOWN] = [$fx];
		$tx-> [EXPANDED] = 1;
	} else {
		splice( @{$tx-> [DOWN]}, 0, 0, $fx);
	}
	$self-> reset_tree;
	$self-> update_tree;
	$self-> repaint;
	$self-> clear_event;
}

sub is_selected
{
	my ( $self, $index, $item, $sel) = @_;
	unless ( defined $item) {
		my ($node, $lev) = $self-> get_item( $index);
		return 0 unless $node;
		$item = $node;
	}
	return $item-> [SELECTED];
}

sub set_item_selected
{
	my ( $self, $index, $item, $sel) = @_;
	return unless $self-> {multiSelect};
	unless ( defined $item) {
		my ($node, $lev) = $self-> get_item( $index);
		return unless $node;
		$item = $node;
	}
	$sel ||= 0;
	return if $sel == ( $item-> [SELECTED] ? 1 : 0);
	$item-> [SELECTED] = $sel;

	if ( !defined $index) {
		my ( $x, $lev) = $self-> get_index( $item);
		if ( $x < 0) {
			$self-> notify(q(SelectItem), [[ undef, $item, $sel ]]);
			return 0;
		}
		$index = $x;
	}
	$self-> notify(q(SelectItem), [[ $index, $item, $sel]]);
	my ( $ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);
	$self-> invalidate_rect(
		$a[0], $a[3] - ( $index - $self-> {topItem} + 1) * $ih,
		$a[2], $a[3] - ( $index - $self-> {topItem}) * $ih
	);
}

sub select_all
{
	my ( $self, $full) = @_;
	$self-> iterate( sub { $_[0]-> [SELECTED] = 1; 0 }, $full);
	$self-> repaint;
}

sub deselect_all
{
	my ( $self, $full) = @_;
	$self-> iterate( sub { $_[0]-> [SELECTED] = 0 }, $full);
	$self-> repaint;
}

sub add_selection
{
	my ( $self, $array, $flag) = @_;
	return unless $self-> {multiSelect};
	my %items = map { $_ => 1 } @$array;
	$flag ||= 0;
	my ( $ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);
	my @sel;

	$self-> iterate( sub {
		my ( $current, $parent, $index, $position, $level, $lastChild) = @_;
		return 0 unless $items{$position};
		return 0 if $flag == ($current-> [SELECTED] ? 1 : 0);
		$current-> [SELECTED] = $flag;
		push @sel, [ $position, $current, 1];
		$self-> invalidate_rect(
			$a[0], $a[3] - ( $position - $self-> {topItem} + 1) * $ih,
			$a[2], $a[3] - ( $position - $self-> {topItem}) * $ih
		);
		0;
	});
	$self-> notify(q(SelectItem), \@sel) if @sel;
}

sub get_selected_items
{
	my $self = $_[0];
	my @ret;
	$self-> iterate( sub { push @ret, $_[3] if $_[0]-> [SELECTED]; 0 });
	return @ret;
}

sub set_selection
{
	my ( $self, $array, $flag) = @_;
	return unless $self-> {multiSelect};
	my %items = map { $_ => 1 } @$array;
	$flag ||= 0;
	my ( $ih, @a) = ( $self-> {itemHeight}, $self-> get_active_area);
	my @sel;

	$self-> iterate( sub {
		my ( $current, $parent, $index, $position, $level, $lastChild, $visibility) = @_;
		if ( defined $visibility) {
			my $new_val = $items{$visibility} ? $flag : !$flag;
			return 0 if $new_val == ($current-> [SELECTED] ? 1 : 0);
			$current-> [SELECTED] = $new_val;
			push @sel, [ $visibility, $current, $new_val];
			$self-> invalidate_rect(
				$a[0], $a[3] - ( $visibility - $self-> {topItem} + 1) * $ih,
				$a[2], $a[3] - ( $visibility - $self-> {topItem}) * $ih
			);
		} elsif ( $flag != ( $current-> [SELECTED] ? 1 : 0)) {
			$current-> [SELECTED] = $flag;
			push @sel, [ undef, $current, $flag];
		};
		0;
	}, 1);

	$self-> notify(q(SelectItem), \@sel) if @sel;
}

sub toggle_item
{
	my ( $self, $index, $item) = @_;
	unless ( defined $item) {
		my ($node, $lev) = $self-> get_item( $index);
		return unless $node;
		$item = $node;
	}
	$self-> set_item_selected( $index, $item, $item-> [SELECTED] ? 0 : 1);
}

sub select_item   {  $_[0]-> set_item_selected( $_[1], $_[2], 1); }
sub unselect_item {  $_[0]-> set_item_selected( $_[1], $_[2], 0); }

sub autoHeight    {($#_)?$_[0]-> set_auto_height    ($_[1]):return $_[0]-> {autoHeight}     }
sub extendedSelect{($#_)?$_[0]-> set_extended_select($_[1]):return $_[0]-> {extendedSelect} }
sub focusedItem   {($#_)?$_[0]-> set_focused_item   ($_[1]):return $_[0]-> {focusedItem}    }
sub indent        {($#_)?$_[0]-> set_indent( $_[1])        :return $_[0]-> {indent}         }
sub items         {($#_)?$_[0]-> set_items( $_[1])         :return $_[0]-> {items}          }
sub itemHeight    {($#_)?$_[0]-> set_item_height    ($_[1]):return $_[0]-> {itemHeight}     }
sub multiSelect   {($#_)?$_[0]-> set_multi_select   ($_[1]):return $_[0]-> {multiSelect}    }
sub offset        {($#_)?$_[0]-> set_offset         ($_[1]):return $_[0]-> {offset}         }
sub selectedItems {($#_)?$_[0]-> set_selection      ($_[1],1):return $_[0]-> get_selected_items}
sub topItem       {($#_)?$_[0]-> set_top_item       ($_[1]):return $_[0]-> {topItem}        }

package Prima::StringOutline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
	for ( @$paintStruc) {
		my ( $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @$_;
		if ( $selected || $prelight) {
			my $bc = $self->backColor;
			$self-> draw_item_background($canvas,
				$left, $bottom, $right, $top, $prelight,
				$selected ? $self-> hiliteBackColor : $self-> backColor
			);
			$self->backColor($bc);
			my $c = $canvas-> color;
			$canvas-> color( $selected ? $self-> hiliteColor : $c );
			$canvas-> text_shape_out( $node-> [0], $left, $bottom);
			$canvas-> color( $c);
		} else {
			$canvas-> text_shape_out( $node-> [0], $left, $bottom);
		}
		$canvas-> rect_focus( $left, $bottom, $right, $top) if $focused;
	}
}

sub on_measureitem
{
	my ( $self, $node, $level, $result) = @_;
	$$result = $self-> get_text_width( $node-> [0]);
}

sub on_stringify
{
	my ( $self, $node, $result) = @_;
	$$result = $node-> [0];
}

package Prima::Outline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
	for ( @$paintStruc) {
		my ( $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @$_;
		if ( $selected || $prelight ) {
			my $bc = $self->backColor;
			$self-> draw_item_background($canvas,
				$left, $bottom, $right, $top, $prelight,
				$selected ? $self-> hiliteBackColor : $bc,
			);
			$self->backColor($bc);

			my $c  = $canvas-> color;
			$canvas-> color( $selected ? $self-> hiliteColor : $c );
			$canvas-> text_shape_out( $node-> [0]-> [0], $left, $bottom);
			$canvas-> color( $c);
		} else {
			$canvas-> text_shape_out( $node-> [0]-> [0], $left, $bottom);
		}
		$canvas-> rect_focus( $left, $bottom, $right, $top) if $focused;
	}
}

sub on_measureitem
{
	my ( $self, $node, $level, $result) = @_;
	$$result = $self-> get_text_width( $node-> [0]-> [0]);
}

sub on_stringify
{
	my ( $self, $node, $result) = @_;
	$$result = $node-> [0]-> [0];
}

package Prima::DirectoryOutline;
use vars qw(@ISA);
@ISA = qw(Prima::OutlineViewer);

# node[0]:
#  0 : node text
#  1 : parent path, '' if none
#  2 : icon width
#  3 : drive icon, only for roots

my $unix = Prima::Application-> get_system_info-> {apc} == apc::Unix || $^O =~ /cygwin/;
my @images;
my @drvImages;

unless ($^C) {
	my $i = 0;
	my @idx = (  sbmp::SFolderOpened, sbmp::SFolderClosed);
	$images[ $i++] = Prima::StdBitmap::icon( $_) for @idx;
	unless ( $unix) {
		$i = 0;
		for (
			sbmp::DriveFloppy, sbmp::DriveHDD,    sbmp::DriveNetwork,
			sbmp::DriveCDROM,  sbmp::DriveMemory, sbmp::DriveUnknown
		) {
			$drvImages[ $i++] = Prima::StdBitmap::icon($_);
		}
	}
}

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		path           => '',
		draggable      => 0,
		openedGlyphs   => 1,
		closedGlyphs   => 1,
		openedIcon     => undef,
		closedIcon     => undef,
		showDotDirs    => 0,
	}
}

sub init_tree
{
	my $self = $_[0];
	my @tree;
	if ( $unix) {
		push ( @tree, [[ '/', ''], [], 0]);
	} else {
		my @drv = split( ' ', Prima::Utils::query_drives_map('A:'));
		for ( @drv) {
			my $type = Prima::Utils::query_drive_type($_);
			push ( @tree, [[ $_, ''], [], 0]);
		}
	}
	$self-> items( \@tree);
}

sub init
{
	my $self = shift;
	my %profile = @_;
	$profile{items} = [];
	%profile = $self-> SUPER::init( %profile);
	for ( qw( files filesStat items))             { $self-> {$_} = []; }
	for ( qw( openedIcon closedIcon openedGlyphs closedGlyphs indent showDotDirs))
		{ $self-> {$_} = $profile{$_}}
	$self-> {openedIcon} = $images[0] unless $self-> {openedIcon};
	$self-> {closedIcon} = $images[1] unless $self-> {closedIcon};
	$self-> {fontHeight} = $self-> font-> height;
	$self-> recalc_icons;
	$self-> init_tree;
	$self-> {cPath} = $profile{path};
	return %profile;
}

sub on_create
{
	my $self = $_[0];
	# path could invoke adjust(), thus calling notify(), which
	# fails until init() ends.
	$self-> path( $self-> {cPath}) if length $self-> {cPath};
}

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
	for ( @$paintStruc) {
		my ( $node, $left, $bottom, $right, $top, $position, $selected, $focused, $prelight) = @$_;
		my $c;
		my $dw = length $node-> [0]-> [1] ?
			$self-> {iconSizes}-> [0] :
			$node-> [0]-> [2];
		if ( $selected || $prelight) {
			$c = $canvas-> color;
			my $bc = $self->backColor;
			$self-> draw_item_background($canvas,
				$left - $self-> {indent} / 4, $bottom, $right, $top, $prelight,
				$selected ? $self-> hiliteBackColor : $self-> backColor
			);
			$self->backColor($bc);
			$canvas-> color( $selected ? $self-> hiliteColor : $c );
		}
		my $icon = (length( $node-> [0]-> [1]) || $unix) ?
			( $node-> [2] ? $self-> {openedIcon} : $self-> {closedIcon}) :
			$node-> [0]-> [3];
		$canvas-> put_image(
			$left - $self-> {indent} / 4,
			int($bottom + ( $self-> {itemHeight} - $self-> {iconSizes}-> [1]) / 2),
			$icon
		);
		$canvas-> text_shape_out(
			$node-> [0]-> [0],
			$left + $dw,
			int($bottom + ( $self-> {itemHeight} - $self-> {fontHeight}) / 2)
		);
		$canvas-> color( $c) if $selected || $prelight;
		$canvas-> rect_focus( $left - $self-> {indent} / 4, $bottom, $right, $top)
			if $focused;
	}
}

sub recalc_icons
{
	my $self = $_[0];
	my $hei = $self-> font-> height + 2;
	my ( $o, $c) = (
		$self-> {openedIcon} ? $self-> {openedIcon}-> height : 0,
		$self-> {closedIcon} ? $self-> {closedIcon}-> height : 0
	);
	my ( $ow, $cw) = (
		$self-> {openedIcon} ? ($self-> {openedIcon}-> width / $self-> {openedGlyphs}): 0,
		$self-> {closedIcon} ? ($self-> {closedIcon}-> width / $self-> {closedGlyphs}): 0
	);
	$hei = $o if $hei < $o;
	$hei = $c if $hei < $c;
	unless ( $unix) {
		for ( @drvImages) {
			next unless defined $_;
			my @s = $_-> size;
			$hei = $s[1] + 2 if $hei < $s[1] + 2;
		}
	}
	$self-> itemHeight( $hei);
	my ( $mw, $mh) = ( $ow, $o);
	$mw = $cw if $mw < $cw;
	$mh = $c  if $mh < $c;
	$self-> {iconSizes} = [ $mw, $mh];
}

sub on_fontchanged
{
	my $self = shift;
	$self-> recalc_icons;
	$self-> {fontHeight} = $self-> font-> height;
	$self-> SUPER::on_fontchanged(@_);
}

sub on_measureitem
{
	my ( $self, $node, $level, $result) = @_;
	my $tw = $self-> get_text_width( $node-> [0]-> [0]) + $self-> {indent} / 4;

	unless ( length $node-> [0]-> [1]) { #i.e. root
		if ( $unix) {
			$node-> [0]-> [2] = $self-> {iconSizes}-> [0];
		} else {
			my $dt = Prima::Utils::query_drive_type($node-> [0]-> [0]) - dt::Floppy;
			$node-> [0]-> [2] = $drvImages[$dt] ? $drvImages[$dt]-> width : 0;
			$node-> [0]-> [3] = $drvImages[$dt];
		}
		$tw += $node-> [0]-> [2];
	} else {
		$tw += $self-> {iconSizes}-> [0];
	}
	$$result = $tw;
}

sub on_stringify
{
	my ( $self, $node, $result) = @_;
	$$result = $node-> [0]-> [0];
}

sub get_directory_tree
{
	my ( $self, $path) = @_;
	my @fs = Prima::Utils::getdir( $path);
	return [] unless scalar @fs;
	my $oldPointer = $::application-> pointer;
	$::application-> pointer( cr::Wait);
	my $i;
	my @fs1;
	my @fs2;
	for ( $i = 0; $i < scalar @fs; $i += 2) {
		push( @fs1, $fs[ $i]);
		push( @fs2, $fs[ $i + 1]);
	}

	$self-> {files}     = \@fs1;
	$self-> {filesStat} = \@fs2;
	my @d;
	if ( $self-> {showDotDirs}) {
		@d   = grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
		push @d, grep { -d "$path/$_" } $self-> files( 'lnk');
	} else {
		@d = grep { !/\./ } $self-> files( 'dir');
		push @d, grep { !/\./ && -d "$path/$_" } $self-> files( 'lnk');
	}
	@d = sort @d;
	my $ind = 0;
	my @lb;
	for (@d)  {
		my $pathp = "$path/$_";
		@fs = Prima::Utils::getdir( "$path/$_");
		@fs1 = ();
		@fs2 = ();
		for ( $i = 0; $i < scalar @fs; $i += 2) {
			push( @fs1, $fs[ $i]);
			push( @fs2, $fs[ $i + 1]);
		}
		$self-> {files}     = \@fs1;
		$self-> {filesStat} = \@fs2;
		my @dd;
		if ( $self-> {showDotDirs}) {
			@dd   = grep { $_ ne '.' && $_ ne '..' } $self-> files( 'dir');
			push @dd, grep { -d "$pathp/$_" } $self-> files( 'lnk');
		} else {
			@dd = grep { !/\./ } $self-> files( 'dir');
			push @dd, grep { !/\./ && -d "$pathp/$_" } $self-> files( 'lnk');
		}
		push @lb, [[ $_, $path . ( $path eq '/' ? '' : '/')], scalar @dd ? [] : undef, 0];
	}
	$::application-> pointer( $oldPointer);
	return \@lb;
}

sub files {
	my ( $fn, $fs) = ( $_[0]-> {files}, $_[0]-> {filesStat});
	return wantarray ? @$fn : $fn unless ($#_);
	my @f;
	for ( my $i = 0; $i < scalar @$fn; $i++)
	{
		push ( @f, $$fn[$i]) if $$fs[$i] eq $_[1];
	}
	return wantarray ? @f : \@f;
}

sub on_expand
{
	my ( $self, $node, $action) = @_;
	return unless $action;
	my $x = $self-> get_directory_tree( $node-> [0]-> [1].$node-> [0]-> [0]);
	$node-> [1] = $x;
	# another valid way of doing the same -
	# $self-> delete_items( $node);
	# $self-> insert_items( $node, 0, @$x); but since on_expand is never called directly,
	# adjust() will call necessary update functions for us.
}

sub path
{
	my $self = $_[0];
	unless ( $#_) {
		my ( $n, $l) = $self-> get_item( $self-> focusedItem);
		return '' unless $n;
		return $n-> [0]-> [1].$n-> [0]-> [0];
	}
	my $p = $_[1];
	$p =~ s{^([^\\\/]*[\\\/][^\\\/]*)[\\\/]$}{$1};
	unless ( scalar( stat $p)) {
		$p = "";
	} else {
		$p = eval { Cwd::abs_path($p) };
		$p = "." if $@;
		$p = "" unless -d $p;
		$p = '' if !$self-> {showDotDirs} && $p =~ /\./;
		$p .= '/' unless $p =~ m{[/\\]$};
	}

	$self-> {path} = $p;
	if ( $p eq '/') {
		$self-> focusedItem(0);
		return;
	}

	$p = lc $p unless $unix;
	my @ups = split /[\/\\]/, $p;
	my $root;
	if ( $unix) {
		shift @ups if $ups[0] eq '';
		$root = $self-> {items}-> [0];
	} else {
		my $lr = shift @ups;
		for ( @{$self-> {items}}) {
			my $drive = lc $_-> [0]-> [0];
			$root = $_, last if $lr eq $drive;
		}
		return unless defined $root;
	}

	UPS: for ( @ups) {
		last UPS unless defined $root-> [1];
		my $subdir = $_;
		unless ( $root-> [2]) {
			my ( $idx, $lev) = $self-> get_index( $root);
			$self-> adjust( $idx, 1);
		}
		BRANCH: for ( @{$root-> [1]}) {
			next unless lc($_-> [0]-> [0]) eq lc($subdir);
			$root = $_;
			last BRANCH;
		}
	}

	my ( $idx, $lev) = $self-> get_index( $root);
	$self-> focusedItem( $idx);
	$self-> adjust( $idx, 1);
	$self-> topItem( $idx);
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

sub showDotDirs
{
	return $_[0]-> {showDotDirs} unless $#_;
	my $p = $_[0]-> path;
	$_[0]-> {showDotDirs} = $_[1];
	$_[0]-> init_tree;
	$_[0]-> {path} = '';
	$_[0]-> path($p);
}

1;

=pod

=head1 NAME

Prima::Outlines - tree view widgets

=head1 SYNOPSIS

	use Prima qw(Outlines Application);
	my $outline = Prima::StringOutline-> new(
		items => [
			[ 'Simple item' ],
			[ 'Embedded items', [['#1'], ['#2']]],
		],
	);
	$outline-> expand_all;
	Prima->run;

=for podview <img src="outline.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/outline.gif">

	my $outline = Prima::StringOutline-> new(
		iconStyle => 'triangle',
		...
	);

=for podview <img src="outline2.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/outline2.gif">

=head1 DESCRIPTION

The module provides a set of widget classes designed to display tree-like
structures. C<Prima::OutlineViewer> presents a generic class that contains the
basic functionality and defines the interface for the class descendants, which
are C<Prima::StringOutline>, C<Prima::Outline>, and C<Prima::DirectoryOutline>.

=head1 Prima::OutlineViewer

Presents a generic interface for browsing tree-like lists.  Each node in a
linked list represents an item.  The format of the node is predefined, and is
an anonymous array with the following definitions of its indices:

=over

=item #0

Item id in an unspecified format. The simplest implementation,
C<Prima::StringOutline>, treats the scalar as a text string. The more complex
classes store references to arrays or hashes here. See the C<items> article of a
concrete class for the format of the node record.

=item #1

Reference to a child node. C<undef> if there is none.

=item #2

A boolean flag, which selects if the node is to be shown as expanded, e.g.
all of its immediate children are visible.

=item #3

Width of an item in pixels.

=back

The indices above 3 should not be used because eventual changes to the
implementation of the class may use these. The general idea is that the data at
index #0 should be self-sufficient to define an item.

To support a custom format of the node the following notifications should be
overloaded: C<DrawItem>, C<MeasureItem>, and C<Stringify>. Since C<DrawItem> is
called for every item, a gross method C<draw_items> can be overloaded instead.
See also L<Prima::StringOutline> and L<Prima::Outline>.

The class employs two ways to address an item, index-wise and item-wise. The
index-wise counts only the visible ( non-expanded ) items and is represented
by an integer index.  The item-wise addressing cannot be expressed by an
integer index, and the full node structure is used as a reference. It is
important to use a valid reference here since the class does not always
perform the check if the node belongs to the internal node list due to speed
reasons.

C<Prima::OutlineViewer> is a descendant of C<Prima::Widget::GroupScroller> and
C<Prima::Widget::MouseScroller>, so some of their properties and methods are not
described here.

The class is not usable directly.

=head2 Properties

=over

=item autoHeight INTEGER

If set to 1, changes C<itemHeight> automatically according to the widget font height.
If 0, does not influence anything.  When C<itemHeight> is set explicitly,
changes value to 0.

Default value: 1

=item draggable BOOLEAN

If 1, allows the items to be dragged interactively by pressing the Control key
together with the left mouse button. If 0, item dragging is disabled.

Default value: 1

=item drawLines BOOLEAN

If 1, draws dotted tree lines left to the items.

Default value: 1

=item extendedSelect BOOLEAN

Manages the way the user selects multiple items and is only actual
when C<multiSelect> is 1. If 0, the user must click each item
to mark it as selected. If 1, the user can drag the mouse
or use the C<Shift> key plus arrow keys to perform range selection;
the C<Control> key can be used to select individual items.

Default value: 0

=item focusedItem INTEGER

Selects the focused item index. If -1, no item is focused.
It is mostly a run-time property, however, it can be set
during the widget creation stage given that the item list is
accessible at this stage as well.

=item iconCollapsed ICON

Sets the image that is to be displayed when a tree branch is collapsed

=item iconExpanded ICON

Sets the image that is to be displayed when a tree branch is expanded

=item iconStyle STYLE

Sets visual style, one of: C<default>, C<plusminus>, or C<triangle>.

The default style is set in C<$Prima::Outlines::default_style> and is currently
'plusminus', however, it can be overridden by C<skin>. The default style for
the current default skin C<flat> is 'triangle'.

=item indent INTEGER

Width in pixels of the indent between item levels.

Default value: 12

=item itemHeight INTEGER

Selects the height of the items in pixels. Since the outline classes do not
support items with various heights, changes to this property affect all items.

Default value: default font height

=item items ARRAY

Provides access to the items as an anonymous array. The format of an item is
described in the opening article ( see L<Prima::OutlineViewer> ).

Default value: []

=item multiSelect BOOLEAN

If 0, the user can only select one item, which is also reported by
the C<focusedItem> property. If 1, the user can select more than one item.
In this case, the C<focusedItem>'th item is not necessarily selected.
To access the selected item list, use the C<selectedItems> property.

Default value: 0

=item offset INTEGER

Horizontal offset of the item list in pixels.

=item selectedItems ARRAY

ARRAY is an array of integer indices of the selected items. Note, that these
are the items visible on the screen only. The property doesn't handle the
selection status of the collapsed items.

The widget keeps the selection status on each node, visible and invisible (
e.g. the node is invisible if its parent node is collapsed). However, C<selectedItems>
accounts for the visible nodes only; to manipulate the node status or both visible
and invisible nodes, use C<select_item>, C<unselect_item>, and C<toggle_item> methods.

=item showItemHint BOOLEAN

If 1, allows activation of a hint label when the mouse pointer is hovered above
an item that does not fit horizontally into the widget inferiors. If 0,
the hint is never shown.

See also: L<makehint>.

Default value: 1

=item topItem INTEGER

Selects the first item drawn.

=back

=head2 Methods

=over

=item add_selection ARRAY, FLAG

Sets item indices from ARRAY in selected or deselected state, depending on the FLAG
value, correspondingly 1 or 0.

Note, that these are the items visible on the screen only. The method doesn't
handle the selection status of the collapsed items.

Only for the multi-select mode.

=item adjust INDEX, EXPAND

Performs expansion ( 1 ) or collapse ( 0 ) of the INDEXth item, depending on the EXPAND
boolean flag value.

=item calibrate

Recalculates the node tree and item dimensions.
Used internally.

=item delete_items [ NODE = undef, OFFSET = 0, LENGTH = undef ]

Deletes LENGTH children items of NODE at OFFSET.
If NODE is C<undef>, the root node is assumed. If LENGTH
is C<undef>, all items after OFFSET are deleted.

=item delete_item NODE

Deletes NODE from the item list.

=item deselect_all

Removes selection from all items.

Only for multi-select mode.

=item draw_items CANVAS, PAINT_DATA

Called from within the C<Paint> notification to draw items. The default
behavior is to call the C<DrawItem> notification for every visible item. PAINT_DATA
is an array of arrays, where each consists of the parameters passed to
the C<DrawItem> notification.

This method is overridden in some descendant classes,
to increase the speed of the drawing routine.

See L<DrawItem> for PAINT_DATA parameters description.

=item get_index NODE

Traverses all items for NODE and finds if it is visible.  If it is, returns two
integers: the first is the item index and the second is the item depth level.
If the node is not visible, C<-1, undef> is returned.

=item get_index_text INDEX

Returns the text string associated with the INDEXth item.
Since the class does not assume the item storage organization,
the text is queried via the C<Stringify> notification.

=item get_index_width INDEX

Returns the width in pixels of the INDEXth item, which is a
cached result of the C<MeasureItem> notification, stored
under index #3 in a node.

=item get_item INDEX

Returns two scalars corresponding to the INDEXth item: the node reference and
its depth level. If INDEX is outside the list boundaries, an empty array is
returned.

=item get_item_parent NODE

Returns two scalars, corresponding to the NODE: its parent node reference and
offset of the NODE in the parent's immediate children list.

=item get_item_text NODE

Returns the text string associated with the NODE.
Since the class does not assume the item storage organization,
the text is queried via the C<Stringify> notification.

=item get_item_width NODE

Returns width in pixels of the INDEXth item, which is a
cached result of the C<MeasureItem> notification, stored
under index #3 in a node.

=item expand_all [ NODE = undef ].

Expands all nodes under NODE. If NODE is C<undef> the root node is assumed. If
the tree is large, the execution can take a significant amount of time.

=item insert_items NODE, OFFSET, @ITEMS

Inserts one or more ITEMS under NODE with OFFSET.
If NODE is C<undef>, the root node is assumed.

=item iterate ACTION, FULL

Traverses the item tree and calls the ACTION subroutine
on each node. If the FULL boolean flag is 1, all nodes
are traversed. If 0, only the expanded nodes are traversed.

ACTION subroutine is called with the following parameters:

=over

=item #0

Node reference

=item #1

Parent node reference; if C<undef>, the node is the root.

=item #2

Node offset in the parent item list.

=item #3

Node index.

=item #4

Node depth level. 0 means the root node.

=item #5

A boolean flag, set to 1 if the node is the last child in the parent node list,
set to 0 otherwise.

=item #6

Visibility index. When C<iterate> is called with C<FULL = 1>, the index is
the item index as seen on the screen. If the item is not visible, the index
is C<undef>.

When C<iterate> is called with C<FULL = 1>, the index is always the same
as C<node index>.

=back

=item is_selected INDEX, ITEM

Returns 1 if an item is selected, 0 if it is not.

The method can address the item either directly ( ITEM ) or
by its INDEX in the screen position.

=item makehint SHOW, INDEX

Controls hint label of the INDEXth item. If a boolean flag SHOW is set to 1,
the C<showItemHint> property is 1, and the item index does not fit horizontally
in the widget inferiors, then the hint label is shown.  By default, the label is
removed automatically as soon as the user moves the mouse pointer away from the
item. If SHOW is set to 0, the hint label is hidden immediately.

=item point2item Y, [ HEIGHT ]

Returns the index of an item that occupies the horizontal axis at Y in the
widget coordinates.  If HEIGHT is specified, it must be the widget height; if
it is not, the value is fetched by calling C<Prima::Widget::height>.  If the
value is known, passing it to C<point2item> thus achieves some speed-up.

=item select_all

Selects all items.

Only for multi-select mode.

=item set_item_selected INDEX, ITEM, FLAG

Sets the selection flag of an item.
If FLAG is 1, the item is selected. If 0, it is deselected.

The method can address the item either directly ( ITEM ) or
by its INDEX. Only for the multi-select mode.

=item select_item INDEX, ITEM

Selects an item.

The method can address the item either directly ( ITEM ) or
by its INDEX. Only for the multi-select mode.

=item toggle_item INDEX, ITEM

Toggles selection of an item.

The method can address the item either directly ( ITEM ) or
by its INDEX. Only for the multi-select mode.

=item unselect_item INDEX, ITEM

Deselects an item.

The method can address the item either directly ( ITEM ) or
by its INDEX. Only for the multi-select mode.

=item validate_items ITEMS

Traverses an array of ITEMS and changes every node so that eventual scalars
above index #3 are deleted.  Also adds default values to a node if it
contains less than 3 scalars.

=back

=head2 Events

=over

=item Expand NODE, EXPAND

Called when NODE is expanded ( 1 ) or collapsed ( 0 ).
The EXPAND boolean flag reflects the action taken.

=item DragItem OLD_INDEX, NEW_INDEX

Called when the user finishes the drag of an item
from OLD_INDEX to NEW_INDEX position. The default action
rearranges the item list according to the dragging action.

=item DrawItem CANVAS, NODE, X1, Y1, X2, Y2, INDEX, SELECTED, FOCUSED, PRELIGHT

Called when the INDEXth item contained in NODE is to be drawn on CANVAS. X1,
Y1, X2, Y2 coordinates define the exterior rectangle of the item in widget
coordinates. SELECTED, FOCUSED, and PRELIGHT boolean flags are set to 1 if the
item is selected, focused, or pre-lighted, respectively; 0 otherwise.

=item MeasureItem NODE, LEVEL, REF

Stores the width of the NODE item in pixels into the REF scalar reference. LEVEL is the
node depth as returned by C<get_item> for the reference. This notification must
be called from within the C<begin_paint_info/end_paint_info> block.

=item SelectItem [[INDEX, ITEM, SELECTED], [INDEX, ITEM, SELECTED], ...]

Called when an item gets selected or deselected. The array passed contains a
set of arrays for each item where each contains either an integer INDEX or the
ITEM, or both.  In case the INDEX is undef, the item is invisible; if the ITEM
is undef, then the caller didn't bother to call C<get_item> for speed
reasons, and the receiver should call this function. The SELECTED flag contains
the new value of the item.

=item Stringify NODE, TEXT_REF

Stores text string associated with the NODE item into the TEXT_REF
scalar reference.

=back

=head1 Prima::StringOutline

A descendant of the C<Prima::OutlineViewer> class, provides a standard
single-text-item widget. The items can be defined by supplying a text as the
first scalar in the node array structure:

$string_outline-> items([ 'String', [ 'Descendant' ]]);

=head1 Prima::Outline

A variant of C<Prima::StringOutline>, with the only difference
that the text is stored not in the first scalar in a node but
as a first scalar in an anonymous array, which in turn is
the first node scalar. The class does not define either format or
the number of scalars in the array, and as such presents a half-abstract
class.

=head1 Prima::DirectoryOutline

Provides a standard widget with the item tree mapped to the directory
structure, so that each item is mapped to a directory. Depending on the type
of the host OS, there is either a single root directory ( unix ), or
one or more disk drive root items ( win32 ).

The node format is defined as follows:

=over

=item #0

Directory name, string.

=item #1

Parent path; an empty string for the root items.

=item #2

Icon width in pixels, integer.

=item #3

Drive icon; defined only for the root items under Windows
to reflect the drive type ( hard, floppy, etc ).

=back

=head2 Properties

=over

=item closedGlyphs INTEGER

The number of horizontal equal-width images in the C<closedIcon> property.

Default value: 1

=item closedIcon ICON

Provides an icon representation for the collapsed items.

=item openedGlyphs INTEGER

The number of horizontal equal-width images in the C<openedIcon> property.

Default value: 1

=item openedIcon OBJECT

Provides an icon representation for the expanded items.

=item path STRING

Runtime-only property. Selects the current file system path.

=item showDotDirs BOOLEAN
Selects if the directories with the first dot character
are shown in the tree view. The treatment of the dot-prefixed names
as hidden is traditional to unix and is of doubtful use under
Windows.

Default value: 0

=back

=head2 Methods

=over

=item files [ FILE_TYPE ]

If the FILE_TYPE value is not specified, the list of all files in the
current directory is returned. If FILE_TYPE is given, only the files
of the types are returned. The FILE_TYPE is a string, one of those
returned by C<Prima::Utils::getdir> ( see L<Prima::Utils/getdir> ).

=item get_directory_tree PATH

Reads the file structure under PATH and returns a newly created hierarchy
structure in the class node format. If the C<showDotDirs> property value is 0,
the dot-prefixed names are not included.

Used internally inside the C<Expand> notification.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, <examples/outline.pl>.

=cut
