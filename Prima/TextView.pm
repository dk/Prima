package Prima::TextView;

use strict;
use warnings;

use Prima;
use Prima::ScrollBar;
use Prima::Drawable::TextBlock;
use base qw(Prima::Widget Prima::Widget::MouseScroller Prima::Widget::GroupScroller);
__PACKAGE__->inherit_core_methods('Prima::Widget::GroupScroller');

use constant YMAX => 1000;

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		autoVScroll       => 0,
		colorMap        => [ $def-> {color}, $def-> {backColor} ],
		fontPalette     => [ {
			name     => $def-> {font}-> {name},
			encoding => '',
			pitch    => fp::Default,
		}],
		offset          => 0,
		paneWidth       => 0,
		paneHeight      => 0,
		paneSize        => [0,0],
		resolution      => [ $::application-> resolution ],
		topLine         => 0,
		scaleChildren   => 0,
		selectable      => 1,
		textDirection   => $::application->textDirection,
		textOutBaseline => 1,
		textRef         => '',
		vScroll         => 1,
		widgetClass     => wc::Edit,
		pointer         => cr::Text,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	if ( exists $p->{owner} && !exists $p->{font} && !exists $p->{fontPalette} &&
		(( $p->{ownerFont} // $default->{ownerFont} // 1 ) == 1 )) {
		$p-> {fontPalette}-> [0]-> {name} = $p->{owner}->font->name;
	}
	$self-> SUPER::profile_check_in( $p, $default);
	if ( exists( $p-> { paneSize})) {
		$p-> { paneWidth}  = $p-> { paneSize}-> [ 0];
		$p-> { paneHeight} = $p-> { paneSize}-> [ 1];
	}
	$p-> { text} = '' if exists( $p-> { textRef});
}

sub init
{
	my $self = shift;
	for ( qw( topLine scrollTransaction offset textDirection
		paneWidth paneHeight ))
		{ $self-> {$_} = 0; }
	my %profile = $self-> SUPER::init(@_);
	$self-> {paneSize} = [0,0];
	$self-> {colorMap} = [];
	$self-> {fontPalette} = [];
	$self-> {fontPaletteSize} = 0;
	$self-> {blocks} = [];
	$self-> {resolution} = [];
	$self-> {defaultFontSize} = $self-> font-> size;
	$self-> {selection}   = [ -1, -1, -1, -1];
	$self-> {selectionPaintMode} = 0;
	$self-> {ymap} = [];
	$self-> setup_indents;
	$self-> resolution( @{$profile{resolution}});
	for ( qw( colorMap fontPalette paneWidth paneHeight offset topLine textDirection text textRef))
		{ $self-> $_( $profile{ $_}); }
	return %profile;
}

sub reset_scrolls
{
	my $self = shift;
	my @sz = $self-> get_active_area( 2, @_);
	if ( $self-> {scrollTransaction} != 1) {
		if ( $self-> {autoVScroll}) {
			my $vs = ($self-> {paneHeight} > $sz[1]) ? 1 : 0;
			if ( $vs != $self-> {vScroll}) {
				$self-> vScroll( $vs);
				@sz = $self-> get_active_area( 2, @_);
			}
		}
		$self-> {vScrollBar}-> set(
			max      => $self-> {paneHeight} - $sz[1],
			pageStep => int($sz[1] * 0.9),
			step     => $self-> font-> height,
			whole    => $self-> {paneHeight},
			partial  => $sz[1],
			value    => $self-> {topLine},
		) if $self-> {vScroll};
	}
	if ( $self-> {scrollTransaction} != 2) {
		if ( $self-> {autoHScroll}) {
			my $hs = ($self-> {paneWidth} > $sz[0]) ? 1 : 0;
			if ( $hs != $self-> {hScroll}) {
				$self-> hScroll( $hs);
				@sz = $self-> get_active_area( 2, @_);
			}
		}
		$self-> {hScrollBar}-> set(
			max      => $self-> {paneWidth} - $sz[0],
			whole    => $self-> {paneWidth},
			value    => $self-> {offset},
			partial  => $sz[0],
			pageStep => int($sz[0] * 0.75),
		) if $self-> {hScroll};
	}
}

sub on_size
{
	my ( $self, $oldx, $oldy, $x, $y) = @_;
	$self-> reset_scrolls( $x, $y);
}

sub on_fontchanged
{
	my $f = $_[0]-> font;
	$_[0]-> {defaultFontSize}            = $f-> size;
	$_[0]-> {fontPalette}-> [0]-> {name} = $f-> name;
}

sub set
{
	my ( $self, %set) = @_;
	if ( exists $set{paneSize}) {
		$self-> paneSize( @{$set{paneSize}});
		delete $set{paneSize};
	}
	$self-> SUPER::set( %set);
}

sub text
{
	unless ($#_) {
		my $hugeScalarRef = $_[0]-> textRef;
		return $$hugeScalarRef;
	} else {
		my $s = $_[1];
		$_[0]-> textRef( \$s);
	}
}

sub textRef
{
	return $_[0]-> {text} unless $#_;
	$_[0]-> {text} = $_[1] if $_[1];
}

sub paneWidth
{
	return $_[0]-> {paneWidth} unless $#_;
	my ( $self, $pw) = @_;
	$pw = 0 if $pw < 0;
	return if $pw == $self-> {paneWidth};
	$self-> {paneWidth} = $pw;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub paneHeight
{
	return $_[0]-> {paneHeight} unless $#_;
	my ( $self, $ph) = @_;
	$ph = 0 if $ph < 0;
	return if $ph == $self-> {paneHeight};
	$self-> {paneHeight} = $ph;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub paneSize
{
	return $_[0]-> {paneWidth}, $_[0]-> {paneHeight} if $#_ < 2;
	my ( $self, $pw, $ph) = @_;
	$ph = 0 if $ph < 0;
	$pw = 0 if $pw < 0;
	return if $ph == $self-> {paneHeight} && $pw == $self-> {paneWidth};
	$self-> {paneWidth}  = $pw;
	$self-> {paneHeight} = $ph;
	$self-> reset_scrolls;
	$self-> repaint;
}

sub offset
{
	return $_[0]-> {offset} unless $#_;
	my ( $self, $offset) = @_;
	$offset = int($offset);
	my @sz = $self-> size;
	my @aa = $self-> get_active_area(2, @sz);
	my $pw = $self-> {paneWidth};
	$offset = $pw - $aa[0] if $offset > $pw - $aa[0];
	$offset = 0 if $offset < 0;
	return if $self-> {offset} == $offset;
	my $dt = $offset - $self-> {offset};
	$self-> {offset} = $offset;
	if ( $self-> {hScroll} && $self-> {scrollTransaction} != 2) {
		$self-> {scrollTransaction} = 2;
		$self-> {hScrollBar}-> value( $offset);
		$self-> {scrollTransaction} = 0;
	}
	$self-> scroll( -$dt, 0, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub resolution
{
	return @{$_[0]->{resolution}} unless $#_;
	my ( $self, $x, $y) = @_;
	die "Invalid resolution\n" if $x <= 0 or $y <= 0;
	@{$self-> {resolution}} = ( $x, $y);
}

sub textDirection
{
	return $_[0]-> {textDirection} unless $#_;
	my ( $self, $td ) = @_;
	$self-> {textDirection} = $td;
}

sub topLine
{
	return $_[0]-> {topLine} unless $#_;
	my ( $self, $top) = @_;
	$top = int($top);
	my @sz = $self-> size;
	my @aa = $self-> get_active_area(2, @sz);
	my $ph = $self-> {paneHeight};
	$top = $ph - $aa[1] if $top > $ph - $aa[1];
	$top = 0 if $top < 0;
	return if $self-> {topLine} == $top;
	my $dt = $top - $self-> {topLine};
	$self-> {topLine} = $top;
	if ( $self-> {vScroll} && $self-> {scrollTransaction} != 1) {
		$self-> {scrollTransaction} = 1;
		$self-> {vScrollBar}-> value( $top);
		$self-> {scrollTransaction} = 0;
	}
	$self-> scroll( 0, $dt, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub VScroll_Change
{
	my ( $self, $scr) = @_;
	return if $self-> {scrollTransaction};
	$self-> {scrollTransaction} = 1;
	$self-> topLine( $scr-> value);
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

sub colorMap
{
	return [ @{$_[0]-> {colorMap}}] unless $#_;
	my ( $self, $cm) = @_;
	$self-> {colorMap} = [@$cm];
	$self-> {colorMap}-> [1] = $self-> backColor if scalar @$cm < 2;
	$self-> {colorMap}-> [0] = $self-> color if scalar @$cm < 1;
	$self-> repaint;
}

sub fontPalette
{
	return [ @{$_[0]-> {fontPalette}}] unless $#_;
	my ( $self, $fm) = @_;
	$self-> {fontPalette} = [@$fm];
	$self-> {fontPalette}-> [0] = {
		name     => $self-> font-> name,
		encoding => '',
		pitch    => fp::Default,
	} if scalar @$fm < 1;
	$self-> {fontPaletteSize} = @$fm;
	$self-> repaint;
}

sub create_state
{
	my $self = $_[0];
	my $g = tb::block_create();
	$$g[ tb::BLK_FONT_SIZE] = $self-> {defaultFontSize};
	$$g[ tb::BLK_COLOR]     = tb::COLOR_INDEX;
	$$g[ tb::BLK_BACKCOLOR] = tb::BACKCOLOR_DEFAULT;
	return $g;
}

sub begin_paint_info
{
	my $self = shift;
	$self-> reset_state;
	return $self->SUPER::begin_paint_info;
}

sub end_paint_info
{
	my $self = shift;
	$self-> reset_state;
	return $self->SUPER::end_paint_info;
}

sub _hash { my $k = shift; join("\0", map { ($_, $k->{$_}) } sort keys %$k) }

sub reset_state { delete $_[0]->{currentFont} }

sub realize_state
{
	my ( $self, $canvas, $state, $mode) = @_;

	if ( $mode & tb::REALIZE_FONTS) {
		my $f = tb::realize_fonts($self-> {fontPalette}, $state);
		goto SKIP if
			exists $self->{currentFont} &&
			_hash($self->{currentFont}) eq _hash($f);
		$self->{currentFont} = $f;
		$canvas-> set_font( $f);
	SKIP:
	}

	return unless $mode & tb::REALIZE_COLORS;
	if ( $self-> {selectionPaintMode}) {
		$self-> selection_state( $canvas);
	} else {
		$canvas-> set( tb::realize_colors( $self-> {colorMap}, $state));
	}
}

sub recalc_ymap
{
	my ( $self, $from) = @_;
	# if $from is zero or not defined, clear the ymap; otherwise we append
	# to what was already calculated. This is optimized for *building* a
	# collection of blocks; if you need to change a collection of blocks,
	# you should always set $from to a false value.
	$self-> {ymap} = [] unless $from; # ok if $from == 0
	my $ymap = $self-> {ymap};
	my $blocks = $self-> {blocks};
	my ( $i, $lim) = ( defined($from) ? $from : 0, scalar(@{$blocks}));
	for ( ; $i < $lim; $i++) {
		my $block = $$blocks[$i];
		my $y1 = $block->[ tb::BLK_Y];
		my $y2 = $block->[ tb::BLK_HEIGHT] + $y1;
		for my $y ( int( $y1 / YMAX) .. int ( $y2 / YMAX)) {
			push @{$ymap-> [$y]}, $i;
		}
	}
}

sub block_walk_abort { shift->{blockWalk} = 1 }

sub block_walk_defaults
{
	my ( $self, %commands ) = @_;
	my $canvas = $commands{canvas} // $self;
	return (
		textPtr      => $self->{text},
		canvas       => $canvas,
		realize      => sub { $self-> realize_state($canvas, @_) },
		baseFontSize => $self-> {defaultFontSize},
		semaphore    => \ $self-> {blockWalk},
		resolution   => $self->{resolution},
		%commands
	);
}

sub block_walk
{
	my ( $self, $block, %commands ) = @_;
	local $self-> {blockWalk} = 0;
	return tb::walk( $block,
		semaphore  => \ $self-> {blockWalk},
		$self->block_walk_defaults(%commands),
	);
}

sub block_wrap
{
	my ( $self, $canvas, $b, $state, $width, %opt) = @_;
	return tb::block_wrap( $b,
		textPtr       => $self->{text},
		canvas        => $canvas,
		state         => $state,
		width         => $width,
		fontmap       => $self->{fontPalette},
		baseFontSize  => $self->{defaultFontSize},
		resolution    => $self->{resolution},
		wordBreak     => 1,
		textDirection => $self->{textDirection},
		%opt
	);
}

sub justify_interspace
{
	my ( $self, $canvas, $b, $width) = @_;
	return tb::justify_interspace( $b,
		textPtr       => $self->{text},
		canvas        => $canvas,
		width         => $width,
		fontmap       => $self->{fontPalette},
		baseFontSize  => $self->{defaultFontSize},
		resolution    => $self->{resolution},
	);
}

sub selection_state
{
	my ( $self, $canvas) = @_;
	$canvas-> color( $self-> hiliteColor);
	$canvas-> backColor( $self-> hiliteBackColor);
	$canvas-> textOpaque(0);
}

sub paint_selection
{
	my ( $self, $canvas, $block, $index, $x, $y, $sx1, $sx2) = @_;

	my $len = $self->get_block_text_length($index);
	$sx2 = $len - 1 if $sx2 < 0;

	my @state;
	my @xy = ($x,$y);
	local $self->{selectionPaintMode} = 0;

	my $draw_text = sub {
		my ( $glyphs, $x, $start, $end ) = @_;
		my $f = $canvas->get_font;
		my $w = $canvas->get_text_width($glyphs, 0, $start, $end - $start);
		$self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS);
		my $g = $glyphs->glyphs;
		$canvas->clear(
			$x, $xy[1] - $f->{descent},
			$x + $w - 1, $xy[1] + $f->{ascent} + $f->{externalLeading} - 1
		) if $self->{selectionPaintMode};
		$canvas-> text_out($glyphs, $x, $xy[1], $start, $end - $start);
		return $w;
	};

	$self-> block_walk( $block,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE_PENS | tb::TRACE_TEXT,
		canvas   => $canvas,
		position => \@xy,
		state    => \@state,
		text     => sub {
			my ($offset, $length, undef, $text) = @_;
			my ($vis_start, $vis_end) = (0,0);
			my $x = $xy[0];
			my $glyphs  = $self-> text_shape($text);
			my $indexes = $glyphs->indexes;
			$length = @{ $glyphs-> glyphs };
			my $selection_map = $glyphs-> selection_map_glyphs( map { $_ - $offset } ($sx1, $sx2));
			for ( my $i = 0; $i < $length; $i++) {
				if ( $selection_map->[$i] != $self->{selectionPaintMode} ) {
					if ($vis_end > $vis_start) {
						my $curr_index = $indexes->[$i-1];
						$i++, $vis_end++ while $curr_index == $indexes->[$i];
						$x += $draw_text->( $glyphs, $x, $vis_start, $vis_end );
					}
					$vis_start = $vis_end;
					$self->{selectionPaintMode} = $selection_map->[$i];
				}
 				$vis_end++;
			}
			$draw_text->( $glyphs, $x, $vis_start, $vis_end )
				if $vis_end > $vis_start;
			if ( $sx1 == $offset + $length ) {
				$self->{selectionPaintMode} = 1;
			} elsif ( $sx2 == $offset + $length - 1 ) {
				$self->{selectionPaintMode} = 0;
			}
		},
		code     => sub {
			my ( $code, $data ) = @_;
			$self-> realize_state( $canvas, \@state, tb::REALIZE_ALL);
			$code-> ( $self, $canvas, $block, \@state, @xy, $data);
		},
		transpose => sub {
			my ( $x, $y, $f) = @_;
			return if !($f & tb::X_EXTEND) || !$self->{selectionPaintMode} || $x == 0 || $y == 0;
			$self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS);
			$canvas->clear($xy[0], $xy[1] - $$block[ tb::BLK_APERTURE_Y], $xy[0] + $x - 1, $xy[1] + $y - $$block[ tb::BLK_APERTURE_Y] - 1);
		},
	);
}

sub on_paint
{
	my ( $self, $canvas) = @_;
	$self-> reset_state;
	my @size = $canvas-> size;
	unless ( $self-> enabled) {
		$self-> color( $self-> disabledColor);
		$self-> backColor( $self-> disabledBackColor);
	}
	my ( $t, $offset, @aa) = (
	$self-> { topLine}, $self-> { offset},
	$self-> get_active_area(1,@size));

	my @clipRect = $canvas-> clipRect;
	$self-> draw_border( $canvas, $self-> backColor, @size);

	my $bx = $self-> {blocks};
	my $lim = scalar @$bx;
	return unless $lim;

	my @cy = ( $aa[3] - $clipRect[3], $aa[3] - $clipRect[1]);
	$cy[0] = 0 if $cy[0] < 0;
	$cy[1] = $aa[3] - $aa[1] if $cy[1] > $aa[3] - $aa[1];
	$cy[$_] += $t for 0,1;

	$self-> clipRect( $self-> get_active_area( 1, @size));
	@clipRect = $self-> clipRect;
	my $i = 0;
	my $b;

	my ( $sx1, $sy1, $sx2, $sy2) = @{$self-> {selection}};

	my @visited;
	for my $ymap_i ( int( $cy[0] / YMAX) .. int( $cy[1] / YMAX)) {
		next unless $self-> {ymap}-> [$ymap_i];
		for my $j ( @{$self-> {ymap}-> [$ymap_i]}) {
			next if $visited[$j]++;
			$b = $$bx[$j];
			my ( $x, $y) = (
				$aa[0] - $offset + $$b[ tb::BLK_X],
				$aa[3] + $t - $$b[ tb::BLK_Y] - $$b[ tb::BLK_HEIGHT]
			);
			next if
				$x + $$b[ tb::BLK_WIDTH]  < $clipRect[0] || $x > $clipRect[2] ||
				$y + $$b[ tb::BLK_HEIGHT] < $clipRect[1] || $y > $clipRect[3] ||
				$$b[ tb::BLK_WIDTH] == 0 || $$b[ tb::BLK_HEIGHT] == 0;

			if ( $j == $sy1 && $j == $sy2 ) {
				# selection within one line
				$self->paint_selection( $canvas, $b, $j, $x, $y, $sx1, $sx2 - 1);
			} elsif ( $j == $sy1 ) {
				# upper selected part
				$self->paint_selection( $canvas, $b, $j, $x, $y, $sx1, -1);
			} elsif ( $j == $sy2 && $sx2 > 0 ) {
				# lower selected part
				$self->paint_selection( $canvas, $b, $j, $x, $y, 0, $sx2 - 1);
			} elsif ( $j > $sy1 && $j < $sy2) { # simple selection case
				$self-> {selectionPaintMode} = 1;
				$self-> selection_state( $canvas);
				$self-> block_draw( $canvas, $b, $x, $y);
				$self-> {selectionPaintMode} = 0;
			} else { # no selection case
				$self-> block_draw( $canvas, $b, $x, $y);
			}
		}
	}

	$self-> {selectionPaintMode} = 0;
}

sub block_draw
{
	my ( $self, $canvas, $b, $x, $y) = @_;

	my $ret = 1;
	$canvas-> clear( $x, $y, $x + $$b[ tb::BLK_WIDTH] - 1, $y + $$b[ tb::BLK_HEIGHT] - 1)
		if $self-> {selectionPaintMode};

	my @xy = ($x, $y);
	my @state;
	$self-> block_walk( $b,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE_PENS | tb::TRACE_TEXT,
		canvas   => $canvas,
		position => \@xy,
		state    => \@state,
		text     => sub {
			$self-> block_walk_abort( $ret = 0 ) unless $canvas-> text_shape_out($_[-1], @xy);
		},
		code     => sub {
			my ( $code, $data ) = @_;
			$code-> ( $self, $canvas, $b, \@state, @xy, $data);
		},
	);

	return $ret;
}

sub xy2info
{
	my ( $self, $x, $y) = @_;

	my $bx = $self-> {blocks};
	my ( $pw, $ph) = $self-> paneSize;
	$x = 0 if $x < 0;
	$x = $pw if $x > $pw;

	return (0,0) if $y < 0 || !scalar(@$bx) ;
	$x = $pw, $y = $ph if $y > $ph;

	my ( $b, $bid);

	my $xhint = 0;

	# find if there's a block that has $y in its inferior
	my $ymapix = int( $y / YMAX);
	if ( $self-> {ymap}-> [ $ymapix]) {
		my ( $minxdist, $bdist, $bdistid) = ( $self-> {paneWidth} * 2, undef, undef);
		for ( @{$self-> {ymap}-> [ $ymapix]}) {
			my $z = $$bx[$_];
			if (
				$y >= $$z[ tb::BLK_Y] &&
				$y < $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT] &&
				$$z[tb::BLK_TEXT_OFFSET] >= 0
			) {
				if (
					$x >= $$z[ tb::BLK_X] &&
					$x < $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH]
				) {
					$b = $z;
					$bid = $_;
					last;
				} elsif (
					abs($$z[ tb::BLK_X] - $x) < $minxdist ||
					abs($$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x) < $minxdist
				) {
					$minxdist = ( abs( $$z[ tb::BLK_X] - $x) < $minxdist) ?
						abs( $$z[ tb::BLK_X] - $x) :
						abs( $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x);
					$bdist = $z;
					$bdistid = $_;
				}
			}
		}
		if ( !$b && $bdist) {
			$b   = $bdist;
			$bid = $bdistid;
			$xhint = (( $$b[ tb::BLK_X] > $x) ? -1 : 1);
		}
	}

	# if still no block found, find the closest block down
	unless ( $b) {
		my $minydist = $self-> {paneHeight} * 2;
		my $ymax = scalar @{$self-> {ymap}};
		while ( $ymapix < $ymax) {
			if ( $self-> {ymap}-> [ $ymapix]) {
				for ( @{$self-> {ymap}-> [ $ymapix]}) {
					my $z = $$bx[$_];
					if (
						$minydist > $$z[ tb::BLK_Y] - $y &&
						$$z[ tb::BLK_Y] >= $y
					) {
						$minydist = $$z[ tb::BLK_Y] - $y;
						$b = $z;
						$bid = $_;
					}
				}
			}
			last if $b;
			$ymapix++;
		}
		$ymapix = int( $y / YMAX);
		$xhint = -1;
	}

	# if still no block found, assume EOT
	unless ( $b) {
		$b = $$bx[-1];
		$bid = scalar @{$bx} - 1;
		$xhint = 1;
	}

	if ( $xhint < 0) { # start of line
		return ( 0, $bid);
	} elsif ( $xhint > 0) { # end of line
		if ( $bid < ( scalar @{$bx} - 1)) {
			return (
				$$bx[ $bid + 1]-> [ tb::BLK_TEXT_OFFSET] - $$b[ tb::BLK_TEXT_OFFSET],
				$bid
			);
		} else {
			return ( length( ${$self-> {text}}) - $$b[ tb::BLK_TEXT_OFFSET], $bid);
		}
	}

	# find text offset
	my $ofs = 0;
	my @pos = ($$b[ tb::BLK_X] - $x,0);
	my $nontext_offset;

	$self-> block_walk( $b,
		position => \@pos,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE | tb::TRACE_PAINT_STATE | tb::TRACE_TEXT,
		text     => sub {
			my ( $offset, $length, $width, $text) = @_;
			my $npx = $pos[0] + $width;
			if ( $pos[0] > 0) {
				$ofs = $nontext_offset // $offset;
				$self-> block_walk_abort;
			} elsif ( $pos[0] <= 0 && $npx > 0) {
				my $glyphs = $self-> text_shape( $text );
				my $goffs  = $self-> text_wrap(
					$glyphs, -$pos[0],
					tw::ReturnFirstLineLength | tw::BreakSingle
				);
				my $indexes = $glyphs->indexes;
				$ofs = $offset + (( $goffs >= $#$indexes ) ? 
						($indexes-> [-1]) : # shouldn't happen
						($indexes->[$goffs] & ~to::RTL));
				$self-> block_walk_abort;
			} else {
				$ofs = $offset + $length;
			}
			undef $nontext_offset;
		},
		transpose => sub {
			my ( $x, $y, $f) = @_;
			# for cases where a non-text block covers a text range, f ex spaces in justify
			$nontext_offset = $ofs if $f & tb::X_EXTEND;
		},
	);

	return $ofs, $bid;
}

sub screen2point
{
	my ( $self, $x, $y, @size) = @_;

	@size = $self-> size unless @size;
	my @aa = $self-> get_active_area( 0, @size);

	$x -= $aa[0];
	$y  = $aa[3] - $y;
	$y += $self-> {topLine};
	$x += $self-> {offset};

	return $x, $y;
}

sub point2screen
{
	my ( $self, $x, $y, @size) = @_;

	@size = $self-> size unless @size;
	my @aa = $self-> get_active_area( 0, @size);

	$y -= $self-> {topLine};
	$x -= $self-> {offset};
	$x += $aa[0];
	$y  = $aa[3] - $y;

	return $x, $y;
}

sub text2xoffset
{
	my ( $self, $x, $bid) = @_;

	my $b = $self-> {blocks}-> [$bid];
	return 0 unless $b;
	return 0 if $x <= 0; # XXX

	my @pos = (0,0);

	$self-> block_walk( $b,
		position => \@pos,
		trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE | tb::TRACE_PAINT_STATE | tb::TRACE_TEXT,
		text     => sub {
			my ( $offset, $length, $width, $text) = @_;
			return if $x < $offset;

			if ( $x < $offset + $length ) {
				$pos[0] += $self-> get_text_width( substr( $text, 0, $x - $offset), 1) - $width;
				$self-> block_walk_abort;
			} elsif ( $x == $offset + $length ) {
				$self-> block_walk_abort;
			}
		},
	);

	return $pos[0];
}

sub get_block_text
{
	my ( $self, $block ) = @_;
	my $ptr = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $len = $self->get_block_text_length( $block );
	return substr( ${$self->{text}}, $ptr, $len);
}

sub get_block_text_length
{
	my ( $self, $block ) = @_;
	my $ptr1 = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $ptr2 = ( $block + 1 < @{$self-> {blocks}}) ?
		$self-> {blocks}-> [$block+1]-> [tb::BLK_TEXT_OFFSET] :
		length ${$self-> {text}};
	return $ptr2 - $ptr1;
}

sub info2text_offset
{
	my ( $self, $offset, $block) = @_;
	return length ${$self-> {text}} unless $block >= 0 && $offset >= 0;

	my $ptr = $self-> {blocks}-> [$block]-> [tb::BLK_TEXT_OFFSET];
	my $len = $self->get_block_text_length( $block );
	return $ptr + $offset;
}

sub text_offset2info
{
	my ( $self, $ofs) = @_;
	my $blk = $self-> text_offset2block( $ofs);
	return undef unless defined $blk;
	$ofs -= $self-> {blocks}-> [$blk]-> [ tb::BLK_TEXT_OFFSET];
	return $ofs, $blk;
}

sub info2xy
{
	my ( $self, $ofs, $blk) = @_;
	$blk = $self-> {blocks}-> [$blk];
	return undef unless defined $blk;
	return @$blk[ tb::BLK_X, tb::BLK_Y];
}

sub text_offset2block
{
	my ( $self, $ofs) = @_;

	my $bx = $self-> {blocks};
	my $end = length ${$self-> {text}};
	my $ret = 0;
	return undef if $ofs < 0 || $ofs >= $end;

	my ( $l, $r) = ( 0, scalar @$bx);
	while ( 1) {
		my $i = int(( $l + $r) / 2);
		my $j = $i + 1;
		last if $i == $ret;
		$ret = $i;
		$i-- while $i > 0     && $$bx[$i]->[tb::BLK_TEXT_OFFSET] < 0;
		$j++ while $j < $#$bx && $$bx[$j]->[tb::BLK_TEXT_OFFSET] < 0;

		my ($b1, $b2) = ( $$bx[$i], $$bx[$j]);

		last if $ofs == $$b1[ tb::BLK_TEXT_OFFSET];

		if ( $ofs > $$b1[ tb::BLK_TEXT_OFFSET]) {
			last if $ofs < $$b2[ tb::BLK_TEXT_OFFSET];
			$l = $j;
		} else {
			$r = $i;
		}
	}
	return $ret;
}


sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransaction};

	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	return if $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3];

	( $x, $y) = $self-> screen2point( $x, $y, @size);

	for my $obj ( @{$self-> {contents}}) {
		unless ( $obj-> on_mousedown( $self, $btn, $mod, $x, $y)) {
			$self-> clear_event;
			return;
		}
	}

	return if $btn != mb::Left;
	$self-> clear_event;

	my @xy = $self-> xy2info( $x, $y);
	my @sel = $self->selection;
	if (
		$self->has_selection &&
		!$self->{drag_transaction} && 
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
			actions    => dnd::Copy,
		);
		$self-> {drag_transaction} = 0;
		$self-> clear_selection if $act < 0;
	} else {
		$self-> {mouseTransaction} = 1;
		$self-> {mouseAnchor} = \@xy;
		$self-> clear_selection;

		$self-> capture(1);
	}
}

sub on_mouseclick
{
	my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
	return unless $dbl;
	return if $self-> {mouseTransaction};
	return if $btn != mb::Left;

	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
		if ( $self-> has_selection) {
			$self-> selection( -1, -1, -1, -1);
			my $cp = $::application-> bring('Primary');
			$cp-> text( '') if $cp;
		}
		return;
	}

	( $x, $y) = $self-> screen2point( $x, $y, @size);
	my ( $text_offset, $bid) = $self-> xy2info( $x, $y);
	my $ln = ( $bid + 1 == scalar @{$self-> {blocks}}) ?
		length ${$self-> {text}} : $self-> {blocks}-> [$bid+1]-> [tb::BLK_TEXT_OFFSET];
	$self-> selection( 0, $bid, $ln - $self-> {blocks}-> [$bid]-> [tb::BLK_TEXT_OFFSET], $bid);
	$self-> clear_event;

	my $cp = $::application-> bring('Primary');
	$cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	unless ( $self-> {mouseTransaction}) {
		my @size = $self->size;
		( $x, $y) = $self-> screen2point( $x, $y, @size);
		for my $obj ( @{$self-> {contents}}) {
			unless ( $obj-> on_mouseup( $self, $btn, $mod, $x, $y)) {
				$self-> clear_event;
				return;
			}
		}
		return;
	}


	return if $btn != mb::Left;

	$self-> capture(0);
	$self-> {mouseTransaction} = undef;
	$self-> clear_event;

	my $cp = $::application-> bring('Primary');
	$cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;

	unless ( $self-> {mouseTransaction}) {
		my @size = $self->size;
		( $x, $y) = $self-> screen2point( $x, $y, @size);
		for my $obj ( @{$self-> {contents}}) {
			unless ( $obj-> on_mousemove( $self, $mod, $x, $y)) {
				$self-> clear_event;
				return;
			}
		}
		return;
	}


	my @size = $self-> size;
	my @aa = $self-> get_active_area( 0, @size);
	if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore(0);
	} else {
		$self-> scroll_timer_stop;
	}

	my ( $nx, $ny) = $self-> screen2point( $x, $y, @size);
	my ( $text_offset, $bid) = $self-> xy2info( $nx, $ny);

	$self-> selection( @{$self-> {mouseAnchor}}, $text_offset, $bid);

	if ( $x < $aa[0] || $x >= $aa[2]) {
		my $px = $self-> {paneWidth} / 8;
		$px = 5 if $px < 5;
		$px *= -1 if $x < $aa[0];
		$self-> offset( $self-> {offset} + $px);
	}
	if ( $y < $aa[1] || $y >= $aa[3]) {
		my $py = $self-> font-> height;
		$py = 5 if $py < 5;
		$py *= -1 if $y >= $aa[3];
		$self-> topLine( $self-> {topLine} + $py);
	}
}


sub on_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	$z *= 3;
	$z *= $self-> font-> height + $self-> font-> externalLeading unless $mod & km::Ctrl;
	my $newTop = $self-> {topLine} - $z;
	$self-> topLine( $newTop > $self-> {paneHeight} ? $self-> {paneHeight} : $newTop);
	$self-> clear_event;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod, $repeat) = @_;

	$mod &= km::Alt|km::Ctrl|km::Shift;
	return if $mod & km::Alt;

	if ( grep { $key == $_ } (
		kb::Up, kb::Down, kb::Left, kb::Right,
		kb::Space, kb::PgDn, kb::PgUp, kb::Home, kb::End
	)) {
		my ( $dx, $dy) = (0,0);
		if ( $key == kb::Up || $key == kb::Down) {
			$dy = $self-> font-> height;
			$dy = 5 if $dy < 5;
			$dy *= $repeat;
			$dy = -$dy if $key == kb::Up;
		} elsif ( $key == kb::Left || $key == kb::Right) {
			$dx = $self-> {paneWidth} / 8;
			$dx = 5 if $dx < 5;
			$dx *= $repeat;
			$dx = -$dx if $key == kb::Left;
		} elsif ( $key == kb::PgUp || $key == kb::PgDn || $key == kb::Space) {
			my @aa = $self-> get_active_area(0);
			$dy = ( $aa[3] - $aa[1]) * 0.9;
			$dy = 5 if $dy < 5;
			$dy *= $repeat;
			$dy = -$dy if $key == kb::PgUp;
		}

		$dx += $self-> {offset};
		$dy += $self-> {topLine};

		if ( $key == kb::Home) {
			$dy = 0;
		} elsif ( $key == kb::End) {
			$dy = $self-> {paneHeight};
		}
		$self-> offset( $dx);
		$self-> topLine( $dy);
		$self-> clear_event;
	}

	if (((( $key == kb::Insert) && ( $mod & km::Ctrl)) ||
		chr($code & 0xff) eq "\cC") && $self-> has_selection)
	{
		$self-> copy;
		$self-> clear_event;
	}
}

sub has_selection
{
	return ( grep { $_ != -1 } @{$_[0]-> {selection}} ) ? 1 : 0;
}

sub selection
{
	return @{$_[0]-> {selection}} unless $#_;
	my ( $self, $sx1, $sy1, $sx2, $sy2) = @_;
	($sx1, $sy1, $sx2, $sy2) = ( $sx2, $sy2, $sx1, $sy1 ) if $sy1 > $sy2;

	$sy1 = 0 if $sy1 < 0;
	$sy2 = 0 if $sy2 < 0;
	my $lim = scalar @{$self-> {blocks}} - 1;
	$sy1 = $lim if $sy1 > $lim;
	$sy2 = $lim if $sy2 > $lim;

	my $empty = ! $self-> has_selection;
	my ( $osx1, $osy1, $osx2, $osy2) = @{$self-> {selection}};
	my ( $y1, $y2) = (0,0);
	my ( @old, @new);

	unless ( grep { $_ != -1 } $sx1, $sy1, $sx2, $sy2 ) { # new empty selection
	EMPTY:
		return if $empty;
		$y1 = $osy1;
		$y2 = $osy2;
		if ( $y1 == $y2) {
			@old = ( $osx1, $osx2 - 1 );
			@new = ();
		}
	} else {
		( $sy1, $sy2, $sx1, $sx2) = ( $sy2, $sy1, $sx2, $sx1) if $sy2 < $sy1;
		( $sx1, $sx2) = ( $sx2, $sx1) if $sy2 == $sy1 && $sx2 < $sx1;
		( $sx1, $sx2, $sy1, $sy2) = ( -1, -1, -1, -1), goto EMPTY
			if $sy1 == $sy2 && $sx1 == $sx2;
		if ( $empty) {
			$y1 = $sy1;
			$y2 = $sy2;
			if ( $y1 == $y2) {
				@new = ( $sx1, $sx2 - 1 );
				@old = ();
			}
		} else {
			if ( $sy1 == $osy1 && $sx1 == $osx1) {
				return if $sy2 == $osy2 && $sx2 == $osx2;
				$y1 = $sy2;
				$y2 = $osy2;
				if ( $sy2 == $osy2) {
					@old = ( 0, $osx2 - 1 );
					@new = ( 0, $sx2  - 1 );
				}
			} elsif ( $sy2 == $osy2 && $sx2 == $osx2) {
				$y1 = $sy1;
				$y2 = $osy1;
				if ( $sy1 == $osy1) {
					@old = ( $osx1, -1);
					@new = ( $sx1,  -1);
				}
			} else {
				$y1 = ( $sy1 < $osy1) ? $sy1 : $osy1;
				$y2 = ( $sy2 > $osy2) ? $sy2 : $osy2;
				if ( $sy1 == $sy2 && $osy1 == $osy2 && $sy2 == $osy1) {
					@old = ( $osx1, $osx2 - 1 );
					@new = ( $sx1,  $sx2  - 1 );
				}
			}
			( $y1, $y2) = ( $y2, $y1) if $y2 < $y1;
		}
	}

	my $bx = $self-> {blocks};
	my @clipRect;
	my @size = $self-> size;
	my @aa   = $self-> get_active_area( 0, @size);
	my @invalid_rects;

	my $b = $$bx[ $y1];
	my @a = ( $$b[ tb::BLK_X], $$b[tb::BLK_Y], $$b[ tb::BLK_X], $$b[ tb::BLK_Y]);
	for ( $y1 .. $y2) {
		my $z = $$bx[ $_];
		my @b = ( $$z[ tb::BLK_X], $$z[tb::BLK_Y], $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH], $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT]);
		for ( 0, 1) { $a[$_] = $b[$_] if $a[$_] > $b[$_] }
		for ( 2, 3) { $a[$_] = $b[$_] if $a[$_] < $b[$_] }
	}
	$clipRect[0] = $aa[0] - $self-> {offset}  + $a[0];
	$clipRect[1] = $aa[3] + $self-> {topLine} - $a[1] - 1;
	$clipRect[2] = $aa[0] - $self-> {offset}  + $a[2];
	$clipRect[3] = $aa[3] + $self-> {topLine} - $a[3] - 1;

	for ( 0, 1) {
		@clipRect[$_,$_+2] = @clipRect[$_+2,$_]
			if $clipRect[$_] > $clipRect[$_+2];
		$clipRect[$_] = $aa[$_] if $clipRect[$_] < $aa[$_];
		$clipRect[$_+2] = $aa[$_+2] if $clipRect[$_+2] > $aa[$_+2];
	}

	if ( $y1 == $y2 ) {
		if ( grep { $_ == -1 } @old, @new ) {
			my $bx   = $self->{blocks};
			my $last = (( $y1 < (@{$bx} - 1)) ?
				$$bx[$y1 + 1]-> [ tb::BLK_TEXT_OFFSET] :
				length( ${$self-> {text}})
			) - $$b[ tb::BLK_TEXT_OFFSET];
			$old[1] = $last if @old && $old[1] == -1;
			$new[1] = $last if @new && $new[1] == -1;
		}
		my @xy = ( $aa[0] - $self->{offset} + $$b[ tb::BLK_X], 0 );
		my @cr;
		my $calc = sub {
			my ( $offset, $glyphs ) = @_;
			return $offset ? $self->get_text_width($glyphs, 0, 0, $offset) : 0;
		};
		$self-> begin_paint_info;
		my ($last_offset, @nontext);

		$self-> block_walk( $b,
			trace    => tb::TRACE_GEOMETRY | tb::TRACE_REALIZE_PENS | tb::TRACE_TEXT,
			canvas   => $self,
			position => \@xy,
			text     => sub {
				my ($offset, $length, undef, $text) = @_;
				if ( @nontext ) {
					my ( $s1, $s2 ) = ($last_offset, $offset - 1);
					my $old_selected = (@old && $s1 >= $old[0] && $s2 <= $old[1]) ? 1 : 0;
					my $new_selected = (@new && $s1 >= $new[0] && $s2 <= $new[1]) ? 1 : 0;
					if ( $old_selected != $new_selected ) {
						$cr[0] //= $nontext[0];
						$cr[1] = $nontext[2];
					}
					undef @nontext;
				}
				$last_offset = $offset + $length;
				my $glyphs  = $self-> text_shape($text);
				my $oldmap = @old ? $glyphs-> selection_chunks_glyphs( map { $_ - $offset } @old) : [];
				my $newmap = @new ? $glyphs-> selection_chunks_glyphs( map { $_ - $offset } @new) : [];
				my $diff   = $glyphs->selection_diff($oldmap, $newmap);
				return unless @$diff;
				$cr[0] //= $xy[0] + $calc->($diff->[0], $glyphs);
				my $end = 0; $end += $_ for @$diff;
				$cr[1] = $xy[0] + $calc->($end, $glyphs);
			},
			transpose => sub {
				my ( $x, $y, $f ) = @_;
				return unless $f & tb::X_EXTEND;
				return unless defined $last_offset;
				@nontext = (@xy, $xy[0] + $x, $xy[1] + $y);
			},
		);
		$self-> end_paint_info;
		if ( @cr && $cr[0] != $cr[1] ) {
			$cr[1]++;
			$clipRect[0] = $cr[0] if $clipRect[0] < $cr[0];
			$clipRect[2] = $cr[1] if $clipRect[2] > $cr[1];
		}
	}

	push @invalid_rects, \@clipRect;

	my @cpr = $self-> get_invalid_rect;
	if ( $cpr[0] != $cpr[2] || $cpr[1] != $cpr[3]) {
		for my $cr ( @invalid_rects ) {
			for ( 0,1) {
				$cr->[$_] = $cpr[$_]     if $cr->[$_] > $cpr[$_];
				$cr->[$_+2] = $cpr[$_+2] if $cr->[$_+2] < $cpr[$_+2];
			}
		}
	}
	$self-> {selection} = [ $sx1, $sy1, $sx2, $sy2 ];
	$self->invalidate_rect(@$_) for @invalid_rects;
}

sub clear_selection { shift->selection(-1,-1,-1,-1) }

sub get_selected_text
{
	my $self = $_[0];
	return unless $self-> has_selection;
	my ( $sx1, $sy1, $sx2, $sy2) = $self-> selection;
	my ( $a1, $a2) = (
		$self-> info2text_offset( $sx1    , $sy1 ),
		$self-> info2text_offset( $sx2 - 1, $sy2 ),
	);
	($a1, $a2) = ($a2, $a1) if $a1 > $a2;
	return substr( ${$self-> {text}}, $a1, $a2 - $a1 + 1);
}

sub copy
{
	my $self = $_[0];
	my $text = $self-> get_selected_text;
	$::application-> Clipboard-> store( 'Text', $text) if defined $text;
}

sub clear_all
{
	my $self = $_[0];
	$self-> selection(-1,-1,-1,-1);
	$self-> {blocks} = [];
	$self-> paneSize( 0, 0);
	$self-> text('');
}

1;

__END__

=pod

=head1 NAME

Prima::TextView - rich text browser widget

=head1 SYNOPSIS

 use strict;
 use warnings;
 use Prima qw(TextView Application);

 my $w = Prima::MainWindow-> create(
     name => 'TextView example',
 );

 my $t = $w->insert(TextView =>
     text     => 'Hello from TextView!',
     pack     => { expand => 1, fill => 'both' },
 );

 # Create a single block that renders all the text using the default font
 my $tb = tb::block_create();
 my $text_width_px = $t->get_text_width($t->text);
 my $font_height_px = $t->font->height;
 $tb->[tb::BLK_WIDTH]  = $text_width_px;
 $tb->[tb::BLK_HEIGHT] = $font_height_px;
 $tb->[tb::BLK_BACKCOLOR] = cl::Back;
 $tb->[tb::BLK_FONT_SIZE] = int($font_height_px) + tb::F_HEIGHT;
 # Add an operation that draws the text:
 push @$tb, tb::text(0, length($t->text), $text_width_px);

 # Set the markup block(s) and recalculate the ymap
 $t->{blocks} = [$tb];
 $t->recalc_ymap;

 # Additional step needed for horizontal scroll as well as per-character
 # selection:
 $t->paneSize($text_width_px, $font_height_px);

 run Prima;

=for podview <img src="textview.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/textview.gif">

=head1 DESCRIPTION

Prima::TextView accepts blocks of formatted text, and provides
basic functionality - scrolling and user selection. The text strings
are stored as one large text chunk, available by the C<::text> and C<::textRef> properties.
A block of a formatted text is an array with fixed-length header and
the following instructions.

A special package C<tb::> provides the block constants and simple functions
for text block access.

=head2 Capabilities

Prima::TextView is mainly the text block functions and helpers. It provides
function for wrapping text block, calculating block dimensions, drawing
and converting coordinates from (X,Y) to a block position. Prima::TextView
is centered around the text functionality, and although any custom graphic of
arbitrary complexity can be embedded in a text block, the internal coordinate
system is used ( TEXT_OFFSET, BLOCK ), where TEXT_OFFSET is a text offset from
the beginning of a block and BLOCK is an index of a block.

The functionality does not imply any text layout - this is up to the class
descendants, they must provide they own layout policy. The only policy
Prima::TextView requires is that blocks' BLK_TEXT_OFFSET field must be
strictly increasing, and the block text chunks must not overlap. The text gaps
are allowed though.

A text block basic drawing function includes change of color, backColor and font,
and the painting of text strings. Other types of graphics can be achieved by
supplying custom code.

=over

=item block_draw CANVAS, BLOCK, X, Y

The C<block_draw> draws BLOCK onto CANVAS in screen coordinates (X,Y). It can
be used not only inside begin_paint/end_paint brackets; CANVAS can be an
arbitrary C<Prima::Drawable> descendant.

=item block_walk BLOCK, %OPTIONS

Cycles through block opcodes, calls supplied callbacks on each.

=back

=head2 Coordinate system methods

Prima::TextView employs two its own coordinate systems:
(X,Y)-document and (TEXT_OFFSET,BLOCK)-block.

The document coordinate system is isometric and measured in pixels. Its origin is located
into the imaginary point of the beginning of the document ( not of the first block! ),
in the upper-left pixel. X increases to the right, Y increases down.
The block header values BLK_X and BLK_Y are in document coordinates, and
the widget's pane extents ( regulated by C<::paneSize>, C<::paneWidth> and
C<::paneHeight> properties ) are also in document coordinates.

The block coordinate system in an-isometric - its second axis, BLOCK, is an index
of a text block in the widget's blocks storage, C<$self-E<gt>{blocks}>, and
its first axis, TEXT_OFFSET is a text offset from the beginning of the block.

Below different coordinate system converters are described

=over

=item screen2point, point2screen X, Y

C<screen2point> accepts (X,Y) in the screen coordinates ( O is a lower left
widget corner ), returns (X,Y) in document coordinates ( O is upper left corner
of a document ).  C<point2screen> does the reverse.

=item xy2info X, Y

Accepts (X,Y) is document coordinates, returns (TEXT_OFFSET,BLOCK) coordinates,
where TEXT_OFFSET is text offset from the beginning of a block ( not related
to the big text chunk ) , and BLOCK is an index of a block.

=item info2xy TEXT_OFFSET, BLOCK

Accepts (TEXT_OFFSET,BLOCK) coordinates, and returns (X,Y) in document coordinates
of a block.

=item text2xoffset TEXT_OFFSET, BLOCK

Returns X coordinate where TEXT_OFFSET begins in a BLOCK index.

=item info2text_offset

Accepts (TEXT_OFFSET,BLOCK) coordinates and returns the text offset
with regard to the big text chunk.

=item text_offset2info TEXT_OFFSET

Accepts big text offset and returns (TEXT_OFFSET,BLOCK) coordinates

=item text_offset2block TEXT_OFFSET

Accepts big text offset and returns BLOCK coordinate.

=back

=head2 Text selection

The text selection is performed automatically when the user selects a text
region with a mouse. The selection is stored in (TEXT_OFFSET,BLOCK)
coordinate pair, and is accessible via the C<::selection> property.
If its value is assigned to (-1,-1,-1,-1) this indicates that there is
no selection. For convenience the C<has_selection> method is introduced.

Also, C<get_selected_text> returns the text within the selection
(or undef with no selection ), and C<copy> copies automatically
the selected text into the clipboard. The latter action is bound to
C<Ctrl+Insert> key combination.

A block with TEXT_OFFSET set to -1 will be treated as not containing any text,
and therefore will not be able to get selected.

=head2 Event rectangles

Partly as an option for future development, partly as a hack a
concept of 'event rectangles' was introduced. Currently, C<{contents}>
private variable points to an array of objects, equipped with
C<on_mousedown>, C<on_mousemove>, and C<on_mouseup> methods. These
are called within the widget's mouse events, so the overloaded classes
can define the interactive content without overloading the actual
mouse events ( which is although easy but is dependent on Prima::TextView
own mouse reactions ).

As an example L<Prima::PodView> uses the event rectangles to catch
the mouse events over the document links. Theoretically, every 'content'
is to be bound with a separate logical layer; when the concept was designed,
a html-browser was in mind, so such layers can be thought as
( in the html world ) links, image maps, layers, external widgets.

Currently, C<Prima::TextView::EventRectangles> class is provided
for such usage. Its property C<::rectangles> contains an array of
rectangles, and the C<contains> method returns an integer value, whether
the passed coordinates are inside one of its rectangles or not; in the first
case it is the rectangle index.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable::TextBlock>, L<Prima::PodView>, F<examples/mouse_tale.pl>.

=cut
