#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  Modifications by Anton Berezin <tobez@tobez.org>
package Prima::DetailedList;

use strict;
use warnings;
use Prima::Classes;
use Prima::Lists;
use Prima::Widget::Header;

use vars qw(@ISA);
@ISA = qw(Prima::ListViewer);

{
my %RNT = (
	%{Prima::ListViewer-> notification_types()},
	Sort => nt::Command,
);

sub notification_types { return \%RNT; }
}

my %hdrProps = (
	clickable   => 1,
	scalable    => 1,
	minTabWidth => 1,
);

for ( keys %hdrProps) {
	eval <<GENPROC;
sub $_ { return shift-> {hdr}-> $_(\@_); }
sub Prima::DetailList::DummyHeader::$_ {}
GENPROC
}

sub profile_default
{
	return {
		%{Prima::Widget::Header-> profile_default},
		%{$_[ 0]-> SUPER::profile_default},
		headerClass       => 'Prima::Widget::Header',
		headerProfile     => {},
		headerDelegations => [qw(MoveItem SizeItem SizeItems Click)],
		multiColumn       => 0,
		autoWidth         => 0,
		columns           => 0,
		widths            => [],
		headers           => [],
		aligns            => [],
		mainColumn        => 0,
	};
}


sub init
{
	my ( $self, %profile) = @_;
	$self-> {noHeader} = 1;
	$self-> {header} = bless {
		maxWidth => 0,
	}, q\Prima::DetailList::DummyHeader\;
	$self-> {$_} = 0 for qw( mainColumn);
	%profile = $self-> SUPER::init( %profile);

	my $hh = $self-> {headerInitHeight};
	delete $self-> {headerInitHeight};
	delete $self-> {noHeader};
	my $bw = $self-> borderWidth;
	my @sz = $self-> size;

	$self-> {header} = $self-> insert( $profile{headerClass} =>
		name     => 'Header',
		origin   => [ $bw, $sz[1] - $bw - $hh],
		size     => [ $sz[0] - $bw * 2, $hh],
		vertical => 0,
		growMode => gm::Ceiling,
		items    => $profile{headers},
		widths   => $profile{widths},
		delegations => $profile{headerDelegations},
		(map { $_ => $profile{$_}} keys %hdrProps),
		%{$profile{headerProfile}},
	);
	$self-> {header}-> send_to_back;

	my $x = $self-> {header}-> items;
	$self-> {umap} = [ 0 .. $#$x];
	$self-> $_( $profile{$_}) for qw( aligns columns mainColumn);

	if ( scalar @{$profile{widths}}) {
		$self-> {itemWidth} = $self-> {header}-> {maxWidth} - 1;
		$self-> reset_scrolls;
	} else {
		$self-> autowidths;
	}
	return %profile;
}

sub setup_indents
{
	$_[0]-> SUPER::setup_indents;
	$_[0]-> {headerInitHeight} = $_[0]-> font-> height + 8;
	$_[0]-> {indents}-> [ 3] += $_[0]-> {headerInitHeight};
}


sub set_v_scroll
{
	my ( $self, $s) = @_;
	$self-> SUPER::set_v_scroll( $s);
	return if $self-> {noHeader};
	my @a = $self-> get_active_area(2);
	$self-> {header}-> width( $a[0]);
}

sub set_offset
{
	my ( $self, $o) = @_;
	$self-> SUPER::set_offset( $o);
	$self-> { header}-> offset( $self-> {offset}) unless $self-> {noHeader};
}

sub columns
{
	return $_[0]-> {numColumns} unless $#_;
	my ( $self, $c) = @_;
	$c = 0 if $c < 0;
	return if defined $self-> {numColumns} && $self-> {numColumns} == $c;
	my $h    = $self-> {header};
	my @iec  = @{$h-> items};
	my @umap = @{$self-> {umap}};
	if ( scalar(@iec) > $c) {
		splice( @iec, $c);
		splice( @umap, $c);
	} elsif ( scalar(@iec) < $c) {
		push( @umap, (( undef ) x ( $c - scalar @iec)));
		push( @iec, (( '' ) x ( $c - scalar @iec)));
		my $i = 0; for ( @umap) { $_ = $i unless defined $_; $i++; }
	}
	$self-> {umap} = \@umap;
	$h-> items( \@iec);
	$self-> {numColumns} = $c;
	$self-> repaint;
}

sub autowidths
{
	my $self = $_[0];
	my $i;
	my @w = @{$self-> widths};
	my @header_w = $self-> {header}-> calc_autowidths;
	for ( $i = 0; $i < $self-> {numColumns}; $i++) {
		$self-> mainColumn( $i);
		$self-> recalc_widths;
		$w[ $i] = $self-> {maxWidth} + 5
			if $w[ $i] < $self-> {maxWidth} + 5;
		$w[$i] = $header_w[$i] if $w[$i] < $header_w[$i];
	}
	undef $self-> {widths};
	$self-> widths( \@w);
}

sub draw_items
{
	my ($self,$canvas) = (shift,shift);
	my @clrs = (
		$self-> color,
		$self-> backColor,
		$self-> colorIndex( ci::HiliteText),
		$self-> colorIndex( ci::Hilite)
	);
	my @clipRect = $canvas-> clipRect;
	my $cols   = $self-> {numColumns};

	my $xstart = $self-> {borderWidth} - 1;
	my ( $i, $ci, $xend);
	my @widths = @{ $self-> { header}-> widths };
	my $umap   = $self-> {umap}-> [0];
	my $o = $self-> {offset} ;

	$xend = $xstart - $o + 2;
	$xend += $_ + 2 for @widths;
	$canvas-> clear( $xend, @clipRect[1..3]) if $xend <= $clipRect[2];

	return if $cols == 0;

	my $iref   = \@_;
	my $rref   = $self-> {items};
	my $icount = scalar @_;

	my $drawVeilFoc = -1;
	my $x0d    = $self-> {header}-> {maxWidth} - 2;

	my (@normals, @selected, @p_normal, @p_selected);
	my ( $lastNormal, $lastSelected) = (undef, undef);

	# sorting items by index
	$iref = [ sort { $$a[0]<=>$$b[0] } @$iref];

	# calculating conjoint bars for normals / selected
	@normals = ();
	$lastNormal = undef;

	for ( $i = 0; $i < $icount; $i++)
	{
		my ( $itemIndex, $x, $y, $x2, $y2, $selected, $focusedItem, $prelight) = @{$$iref[$i]};
		$drawVeilFoc = $i if $focusedItem;
		if ( $prelight) {
			if ( $selected ) {
				push ( @p_selected, [ $x, $y, $x + $x0d, $y2]);
			} else {
				push ( @p_normal, [ $x, $y, $x + $x0d, $y2]);
			}

		} elsif ( $selected) {
			if ( defined $lastSelected && ( $y2 + 1 == $lastSelected)) {
				${$selected[-1]}[1] = $y;
			} else {
				push ( @selected, [ $x, $y, $x + $x0d, $y2]);
			}
			$lastSelected = $y;
		} else {
			if ( defined $lastNormal && ( $y2 + 1 == $lastNormal)) {
				${$normals[-1]}[1] = $y;
			} else {
				push ( @normals, [ $x, $y, $x + $x0d, $y2]);
			}
			$lastNormal = $y;
		}
	}

	$canvas-> backColor( $clrs[1]);
	$canvas-> clear( @$_) for @normals;
	$canvas-> backColor( $clrs[3]);
	$canvas-> clear( @$_) for @selected;
	if ( @p_normal ) {
		$self-> draw_item_background( $canvas, @$_, 1, $clrs[1]) for @p_normal;
	}
	if ( @p_selected ) {
		$self-> draw_item_background( $canvas, @$_, 1, $clrs[3]) for @p_selected;
	}

	# draw veil
	if ( $drawVeilFoc >= 0) {
		my ( $itemIndex, $x, $y, $x2, $y2) = @{$$iref[$drawVeilFoc]};
		$canvas-> rect_focus( $x + $o, $y, $x + $o + $x0d, $y2);
	}

	# texts
	my $lc = $clrs[0];
	my $txw = 1;
	for ( $ci = 0; $ci < $cols; $ci++) {
		$umap = $self-> {umap}-> [$ci];
		my $dx = 0;
		my $wx = $widths[ $ci] + 2;
		my $align = $self->{aligns}->[$ci] // $self->{align};
		if ( $xstart + $wx - $o >= $clipRect[0]) {
			$canvas-> clipRect(
				(( $xstart - $o) < $clipRect[0]) ? $clipRect[0] : $xstart - $o,
				$clipRect[1],
				(( $xstart + $wx - $o) > $clipRect[2]) ? $clipRect[2] : $xstart + $wx - $o,
				$clipRect[3]);
			for ( $i = 0; $i < $icount; $i++) {
				my ( $itemIndex, $x, $y, $x2, $y2, $selected, $focusedItem) =
					@{$$iref[$i]};
				my $c = $clrs[ $selected ? 2 : 0];
				$canvas-> color( $c), $lc = $c if $c != $lc;
				if ( $align == ta::Center) {
					my $iw = $canvas->get_text_width($rref-> [$itemIndex]-> [$umap]);
					$dx = ($iw < $wx) ? ($wx - $iw) / 2 : 0;
				} elsif ( $align == ta::Right ) {
					my $iw = $canvas->get_text_width($rref-> [$itemIndex]-> [$umap]);
					$dx = ($iw < $wx) ? $wx - $iw : 0;
				}
				$canvas-> text_shape_out( $rref-> [$itemIndex]-> [$umap], $x + $txw + $dx, $y);
			}
		}
		$xstart += $wx;
		$txw    += $wx;
		last if $xstart - $o >= $clipRect[2];
	}
}

sub item2rect
{
	my ( $self, $item, @size) = @_;
	my @a = $self-> get_active_area( 0, @size);
	my ($i,$ih) = ( $item - $self-> {topItem}, $self-> {itemHeight});
	return $a[0], $a[3] - $ih * ( $i + 1), $a[0] + $self-> {header}-> {maxWidth}, $a[3] - $ih * $i;
}

sub Header_SizeItem
{
	my ( $self, $header, $col, $oldw, $neww) = @_;
	my $xs = $self-> {borderWidth} - 1 - $self-> {offset};
	my $i = 0;
	my @widths = @{$self-> {header}-> widths};
	for ( @widths ) {
		last if $col == $i++;
		$xs += $_ + 2;
	}
	$xs += 3 + $oldw;
	my @sz = $self-> size;
	my @a = $self-> get_active_area( 0, @sz);
	$self-> scroll(
		$neww - $oldw, 0,
		confineRect => [ $xs, $a[1], $a[2] + abs( $neww - $oldw), $a[3]],
		clipRect    => \@a,
	);
	$self->invalidate_rect( $xs - $widths[$col], $a[1], $xs, $a[3])
		if ( $self->{aligns}->[$col] // $self->{align} ) != ta::Left;
	$self-> {itemWidth} = $self-> {header}-> {maxWidth} - 1;
	$self-> reset_scrolls if $self-> {hScroll} || $self-> {autoHScroll};
}

sub aligns {
	return shift-> {aligns} unless $#_;
	my $self = shift;
	$self-> {aligns} = shift;
}

sub widths {
	return shift-> { header}-> widths( @_) unless $#_;
	my $self = shift;
	$self-> {header}-> widths( @_);
}

sub headers {
	return shift-> { header}-> items( @_) unless $#_;
	my $self = shift;
	$self-> {header}-> items( @_);
	my $x = $self-> {header}-> items;
	$self-> {umap} = [ 0 .. $#$x];
	$self-> repaint;
}

sub mainColumn
{
	return $_[0]-> {mainColumn} unless $#_;
	my ( $self, $c) = @_;
	$c = 0 if $c < 0;
	$c = $self-> {numColumns} - 1 if $c >= $self-> {numColumns};
	$self-> {mainColumn} = $c;
}

sub Header_SizeItems
{
	$_[0]-> {itemWidth} = $_[0]-> {header}-> {maxWidth} - 1;
	$_[0]-> reset_scrolls;
	$_[0]-> repaint;
}

sub Header_MoveItem  {
	my ( $self, $hdr, $o, $p) = @_;
	splice( @{$self-> {umap}}, $p, 0, splice( @{$self-> {umap}}, $o, 1));
	$self-> repaint;
}

sub Header_Click
{
	my ( $self, $hdr, $id) = @_;
	$self-> mainColumn( $self-> {umap}-> [ $id]);
	$self-> sort( $self-> {mainColumn});
}

sub get_item_text
{
	my ( $self, $index, $sref) = @_;
	my $c = $self-> {mainColumn};
	$$sref = $self-> {items}-> [$index]-> [ $c];
}

sub on_fontchanged
{
	my $self = $_[0];
	$self-> setup_indents;
	$self-> {header}-> set(
		bottom => $self-> {header}-> top - $self-> {headerInitHeight},
		height => $self-> {headerInitHeight},
	);
	$self-> SUPER::on_fontchanged;
}

sub on_measureitem
{
	my ( $self, $index, $sref) = @_;
	my $c = $self-> {mainColumn};
	$$sref = $self-> get_text_width( $self-> {items}-> [$index]-> [ $c]);
}

sub on_stringify
{
	my ( $self, $index, $sref) = @_;
	my $c = $self-> {mainColumn};
	$$sref = $self-> {items}-> [$index]-> [ $c];
}

sub sort
{
	my ( $self, $c) = @_;
	my $dirSort;
	if ( defined $c) {
		return if $c < 0;
		if ( defined($self-> {lastSortCol}) && ( $self-> {lastSortCol} == $c)) {
			$dirSort = $self-> {lastSortDir} = ( $self-> {lastSortDir} ? 0 : 1);
		} else {
			$dirSort = 1;
			$self-> {lastSortDir} = 1;
			$self-> {lastSortCol} = $c;
		}
	}
	else {
		$self-> { lastSortCol} = 0 unless defined $self-> { lastSortCol};
		$c = $self-> { lastSortCol};
		$self-> { lastSortDir} = 0 unless defined $self-> { lastSortDir};
		$dirSort = $self-> { lastSortDir};
	}
	my $foci = undef;
	my %selected = map {
		$self->{items}->[$_] =>  $_
	} keys %{$self-> {selectedItems}}
		if $self-> {multiSelect};

	$foci = $self-> {items}-> [$self-> {focusedItem}] if $self-> {focusedItem} >= 0;
	$self-> notify(q(Sort), $c, $dirSort);
	$self-> repaint;

	return unless defined $foci; # do not select items either;
	                             # focused item should be < 0 only on empty lists
	my $i = 0;
	my $newfoc;
	my @newsel;
	for ( @{$self-> {items}}) {
		if ( $_ == $foci) {
			$newfoc = $i;
			last unless $self-> {multiSelect};
		}
		push @newsel, $i
			if $self-> {multiSelect} and exists $selected{ $_ };
		$i++;
	}
	$self-> focusedItem( $newfoc) if defined $newfoc;
	$self-> selectedItems( \@newsel) if $self-> {multiSelect};
}

sub on_sort
{
	my ( $self, $col, $dir) = @_;
	if ( $dir) {
		$self-> {items} = [
			sort { $$a[$col] cmp $$b[$col]}
			@{$self-> {items}}];
	} else {
		$self-> {items} = [
			sort { $$b[$col] cmp $$a[$col]}
			@{$self-> {items}}];
	}
	$self-> clear_event;
}

sub itemWidth {$_[0]-> {itemWidth};}
sub autoWidth { 0;}

1;

=pod

=head1 NAME

Prima::DetailedList - a multi-column list viewer with controlling
header widget.

=head1 SYNOPSIS

use Prima::DetailedList;

	use Prima qw(DetailedList Application);
	my $l = Prima::DetailedList->new(
		columns => 2,
		headers => [ 'Column 1', 'Column 2' ],
		items => [
			['Row 1, Col 1', 'Row 1, Col 2'],
			['Row 2, Col 1', 'Row 2, Col 2']
		],
	);
	$l-> sort(1);
	run Prima;

=for podview <img src="detailedlist.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/detailedlist.gif">

=head1 DESCRIPTION

Prima::DetailedList is a descendant of Prima::ListViewer, and as such provides
a certain level of abstraction. It overloads format of L<items> in order to
support multi-column ( 2D ) cell span. It also inserts L<Prima::Widget::Header> widget
on top of the list, so the user can interactively move, resize and sort the content
of the list. The sorting mechanism is realized inside the package; it is
activated by the mouse click on a header tab.

Since the class inherits Prima::ListViewer, some functionality, like 'item search by
key', or C<get_item_text> method can not operate on 2D lists. Therefore, L<mainColumn>
property is introduced, that selects the column representing all the data.

=head1 API

=head2 Events

=over

=item Sort COLUMN, DIRECTION

Called inside L<sort> method, to facilitate custom algorithms of sorting.
If the callback procedure is willing to sort by COLUMN index, then it must
call C<clear_event>, to signal the event flow stop. The DIRECTION is a boolean
flag, specifying whether the sorting must be performed is ascending ( 1 ) or
descending ( 0 ) order.

The callback procedure must operate on the internal storage of C<{items}>,
which is an array of arrays of scalars.

The default action is the literal sorting algorithm, where precedence is
arbitrated by C<cmp> operator ( see L<perlop/"Equality Operators"> ) .

=back

=head2 Properties

=over

=item aligns ARRAY

Array of C<ta::> align constants, where each defined the column alignment.
Where an item in the array is undef, it means that the value of the C<align> property must be used.

=item columns INTEGER

Governs the number of columns in L<items>. If set-called, and the new number
is different from the old number, both L<items> and L<headers> are restructured.

Default value: 0

=item headerClass

Assigns a header class.

Create-only property.

Default value: C<Prima::Widget::Header>

=item headerProfile HASH

Assigns hash of properties, passed to the header widget during the creation.

Create-only property.

=item headerDelegations ARRAY

Assigns a header widget list of delegated notifications.

Create-only property.

=item headers ARRAY

Array of strings, passed to the header widget as column titles.

=item items ARRAY

Array of arrays of scalars, of arbitrary kind. The default
behavior, however, assumes that the scalars are strings.
The data direction is from left to right and from top to bottom.

=item mainColumn INTEGER

Selects the column, responsible for representation of all the data.
As the user clicks the header tab, C<mainColumn> is automatically
changed to the corresponding column.

Default value: 0

=back

=head2 Methods

=over

=item sort [ COLUMN ]

Sorts items by the COLUMN index in ascending order. If COLUMN is not specified,
sorts by the last specified column, or by #0 if it is the first C<sort> invocation.

If COLUMN was specified, and the last specified column equals to COLUMN,
the sort direction is reversed.

The method does not perform sorting itself, but invokes L<Sort> notification,
so the sorting algorithms can be overloaded, or be applied differently to
the columns.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Lists>, L<Prima::Widget::Header>, F<examples/sheet.pl>

=cut
