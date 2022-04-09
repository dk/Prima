package Prima::DetailedOutline;

use strict;
use warnings;
use Prima qw(Outlines DetailedList);

use vars qw(@ISA @images @imageSize);
@ISA = qw(Prima::OutlineViewer Prima::DetailedList);

{
	my %RNT = (
	%{ Prima::OutlineViewer->notification_types() },
	Sort => nt::Command,
	);

	sub notification_types { return \%RNT; }
}


my %hdrProps = (
	clickable   => 1,
	scalable    => 1,
	dragable    => 1,
	minTabWidth => 1,
);

for ( keys %hdrProps) {
	eval <<GENPROC;
	sub $_ { return shift->{header}->$_(\@_); }
	sub Prima::DetailOutline::DummyHeader::$_ {}
GENPROC
}

sub profile_default {
	return {
 		%{Prima::Header->profile_default},
		%{$_[ 0]-> SUPER::profile_default},
		autoRecalc        => 1,
		autoHScroll       => 1,
		hScroll           => 0,
		headerClass       => 'Prima::Header',
		headerProfile     => {},
		headerDelegations => [qw(MoveItem SizeItem SizeItems Click)],
		multiColumn       => 0,
		autoWidth         => 0,
		columns           => 0,
		widths            => [],
		headers           => [],
		mainColumn        => 0,
	};
}


sub init {
	my ( $self, %profile) = @_;
	$self->{noHeader} = 1;
	$self->{header} = bless {}, q\Prima::DetailOutline::DummyHeader\;
	$self->{$_} = 0 for qw(mainColumn);
	%profile = $self-> SUPER::init(%profile);

	my $hh = $self-> {headerInitHeight};
	delete $self-> {headerInitHeight};
	delete $self-> {noHeader};
	my $bw = $self-> borderWidth;
	my @sz = $self-> size;

	$self-> {header} = $self-> insert($profile{headerClass} =>
		name     => 'Header',
		origin   => [ $bw, $sz[1] - $bw - $hh],
		size     => [ $sz[0] - $bw * 2 + 1, $hh],
		vertical => 0,
		growMode => gm::Ceiling,
		items    => $profile{headers},
		widths   => $profile{widths},
		delegations => $profile{headerDelegations},
		(map { $_ => $profile{$_}} keys %hdrProps),
		%{$profile{headerProfile}},
	);
	$self-> {header}-> send_to_back;
	my $x = $self->{header}->items;
	$self->{umap} = [ 0 .. $#$x];
	$self->$_( $profile{$_}) for qw(autoRecalc columns mainColumn);
	$self->autowidths unless scalar @{$profile{widths}};
	$self->{recalc} = 1 if $profile{autoRecalc};
	$self->{align} = ta::Left;
	return %profile;
}

sub setup_indents {
	$_[0]->SUPER::setup_indents;
	$_[0]->{headerInitHeight} = $_[0]->font-> height + 8;
	$_[0]->{indents}->[ 3] += $_[0]->{headerInitHeight};
}

sub on_paint {
	my $self = shift;
	if (defined $self->{recalc} and $self->{recalc}) {
		delete $self->{recalc};
		$self->widths([ (0) x $self->{numColumns} ]);
		$self->autowidths;
	}
	$self->SUPER::on_paint(@_);
}

sub draw_items
{
	my ($self, $canvas, $paintStruc) = @_;
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
	my $umap   = $self-> {umap}->[0];
	my $o = $self-> {offset} ;

	# we altered this a bit so it clears everything after
	# the firts column instead of everything after all columns
	# this way is the outline images have gone over bounds,
	# we can get rid of the excess
	$xend = $xstart - $o + 2;
	$xend += $widths[0] + 1;
	$canvas-> clear( $xend, @clipRect[1..3]) if $xend <= $clipRect[2];

	return if $cols == 0;

	my $icount = scalar @$paintStruc;
	my $extent = $#widths * 2 + 3;
	for (@widths) { $extent += $_ }
	$canvas->backColor($clrs[3]);
	for ( $i = 0; $i < $icount; $i++) {
		my ($node, $x, $y, $x2, $y2, $position, $selected, $focused, $prelight) = @{$$paintStruc[$i]};
		next unless $prelight || $selected;
		$x = $xend + 1 if ($xend < $x);
		$self->draw_item_background( $canvas, $x, $y, $extent, $y2, $prelight, $selected ? $clrs[3] : $clrs[1]);
	}
	$canvas->backColor($clrs[0]);

	# texts
	my $lc = $clrs[0];
	my $txw = 1;
	for ( $ci = 0; $ci < $cols; $ci++) {
		$umap = $self-> {umap}->[$ci];
		my $wx = $widths[ $ci] + 2;
		if ( $xstart + $wx - $o >= $clipRect[0]) {
			$canvas-> clipRect(
				(( $xstart - $o) < $clipRect[0]) ? $clipRect[0] : $xstart - $o,
				$clipRect[1],
				(( $xstart + $wx - $o) > $clipRect[2]) ? $clipRect[2] : $xstart + $wx - $o,
				$clipRect[3]);
			for ( $i = 0; $i < $icount; $i++) {
				my ( $node, $x, $y, $x2, $y2, $position, $focused) = @{$$paintStruc[$i]};
				# the x passed in paintStruc is adjusted for the outline marks
				# we need to lose that adjustment for everything but the first column
				$x = 2 - $o if ($ci);
				my $c = $clrs[ $focused ? 2 : 0];
				$canvas-> color( $c), $lc = $c if $c != $lc;
				$canvas->text_shape_out($node->[0]->[$ci], $x+$txw, $y);
			}
		}
		$xstart += $wx;
		$txw    += $wx;
		last if $xstart - $o >= $clipRect[2];
	}
}

sub on_measureitem {
	my ($self, $node, $level, $result) = @_;
	my $c = $self->{mainColumn};
	my $txt = defined($node->[0]->[$c]) ? $node->[0]->[$c] : '';
	$$result = $self->get_text_width($txt);

	# since the text of the first item is offset,
	# we need to get the offset and add it to the width
	unless ($c) {
		my @size = $self->size;
		my @a = $self->get_active_area(1, @size);
		my $indent = $self->{indent};
		my $deltax = - $self->{offset} + ($indent/2) + $a[0];
		$$result += int(($level + 0.5) * $indent) + $deltax;
		$$result += $indent * 1.5;
	}
}

sub on_stringify {
	my ($self, $node, $sref) = @_;
	$$sref = $node->[0]->[$self->{mainColumn}];
}

sub recalc_widths {
	my $self = $_[0];
	my @w;
	my $maxWidth = 0;
	my $i = 0;
	my ( $notifier, @notifyParms) = $self-> get_notify_sub(q(MeasureItem));
	$self-> push_event;
	$self-> begin_paint_info;
	while (my ($node, $lev) = $self->get_item($i)) {
		my $iw = 0;
		$notifier->( @notifyParms, $node, $lev, \$iw);
		$maxWidth = $iw if $maxWidth < $iw;
		push ( @w, $iw);
		$i++;
	}
	$self-> end_paint_info;
	$self-> pop_event;
	$self->{widths} = [@w];
	$self->{maxWidth} = $maxWidth;
}

sub reset_scrolls {
	my $self = $_[0];
	$self-> makehint(0);
	if ( $self-> {scrollTransaction} != 1) {
		$self-> vScroll( $self-> {rows} < $self-> {count} ) if $self-> {autoVScroll};
		$self-> {vScrollBar}-> set(
			max      => $self-> {count} - $self->{rows},
			pageStep => $self-> {rows},
			whole    => $self-> {count},
			partial  => $self-> {rows},
			value    => $self-> {topItem},
		) if $self-> {vScroll};
	}

	if ( $self->{scrollTransaction} != 2) {
		my @sz = $self-> get_active_area( 2);
		my @widths = @{ $self->{header}->{widths} or [] };
		my $iw = $#widths * 2;
		for (@widths) { $iw += $_ }
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

sub set_offset {
	my ( $self, $offset) = @_;
	my @widths = @{ $self->{header}->{widths} or [] };
	my $iw = $#widths * 2;
	for (@widths) { $iw += $_ }
	my @a = $self-> get_active_area;

	my $lc = $a[2] - $a[0];
	if ( $iw > $lc) {
		$offset = $iw - $lc if $offset > $iw - $lc;
		$offset = 0 if $offset < 0;
	}
	else {
		$offset = 0;
	}
	return if $self->{offset} == $offset;
	my $oldOfs = $self->{offset};
	$self->{offset} = $offset;
	$self->{header}->offset($self->{offset}) unless $self->{noHeader};
	if ( $self->{hScroll} && $self->{scrollTransaction} != 2) {
		$self->{scrollTransaction} = 2;
		$self-> {hScrollBar}-> value( $offset);
		$self->{scrollTransaction} = 0;
	}
	$self-> makehint(0);
	$self-> scroll( $oldOfs - $offset, 0,
		clipRect => \@a);
}

sub set_auto_recalc {
	$_[0]->{autoRecalc} = $_[1];
}

sub Header_MoveItem {
	my ($self, $header, $old, $new) = @_;
	my $sub = sub {
		my ($current, $parent, $index, $level, $lastChild) = @_;
		my $texts = $current->[0];
		splice(@$texts, $new, 0, splice(@$texts, $old, 1));
	};
	$self->iterate($sub,1);

	$self->repaint;
}

sub on_sort {
	my ($self, $col, $dir) = @_;
	$self->item_sort($self->items, $col, $dir);
	$self->clear_event;
}

sub item_sort {
	my ($self, $items, $col, $dir) = @_;
	@$items = sort { $a->[0][$col] cmp $b->[0][$col] } @$items;
	unless ($dir) { @$items = reverse @$items }
	for my $i (@$items) {
		if (defined $i->[1]) { $self->item_sort($i->[1], $col, $dir) }
	}
	$self->reset_item_cache;
}

sub on_expand {
	my ($self, $node, $action) = @_;
	return unless $self->autoRecalc;
	$self->{recalc} = 1;
	$self->repaint;
}

sub autoRecalc {($#_)?$_[0]->set_auto_recalc ($_[1]):return $_[0]->{autoRecalc} }

1;

=pod

=head1 NAME

Prima::DetailedOutline - a multi-column outline viewer with controlling
header widget.

=head1 SYNOPSIS

  use Prima qw(DetailedOutline Application);

  my $l = Prima::DetailedOutline->new(
        columns => 2,
        headers => [ 'Column 1', 'Column 2' ],
	size    => [200, 100],
        items => [
              [ ['Item 1, Col 1', 'Item 1, Col 2'], [
                    [ ['Item 1-1, Col 1', 'Item 1-1, Col 2'] ],
                    [ ['Item 1-2, Col 1', 'Item 1-2, Col 2'], [
                          [ ['Item 1-2-1, Col 1', 'Item 1-2-1, Col 2'] ],
                    ] ],
              ] ],
              [ ['Item 2, Col 1', 'Item 2, Col 2'], [
                    [ ['Item 2-1, Col 1', 'Item 2-1, Col 2'] ],
              ] ],
        ],
  );
  $l-> sort(1);
  run Prima;

=for podview <img src="detailedoutline.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/detailedoutline.gif">

=head1 DESCRIPTION

Prima::DetailedOutline combines the functionality of Prima::OutlineViewer
and Prima::DetailedList.

=head1 API

This class inherits all the properties, methods, and events of Prima::OutlineViewer
(primary ancestor) and Prima::DetailedList (secondary ancestor).  One new property
is introduced, and one property is different enough to warrant mention.

=head2 Methods

=over

=item items ARRAY

Each item is represented by an arrayref with either one or two elements.  The
first element is the item data, an arrayref of text strings to display.  The
second element, if present, is an arrayref of child items.

When using the node functionality inherited from Prima::OutlineViewer, the
item data (that is, the arrayref of text strings) is the first element of the
node.

=item autoRecalc BOOLEAN

If this is set to a true value, the column widths will be automatically recalculated
(via C<autowidths>) whenever a node is expanded or collapsed.

=back

=head1 COPYRIGHT

Copyright 2003 Teo Sankaro

This program is distributed under the BSD License.
(Although a credit would be nice.)

=head1 AUTHOR

Teo Sankaro, E<lt>teo_sankaro@hotmail.comE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Outlines>, L<Prima::DetailedList>

=cut
