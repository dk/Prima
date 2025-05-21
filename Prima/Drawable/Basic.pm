package Prima::Drawable::Basic; # for metacpan
package Prima::Drawable;

use strict;
use warnings;
use Prima;

sub reset_matrix { shift->set_matrix( $Prima::matrix::identity ) }

sub rect3d
{
	my ( $self, $x, $y, $x1, $y1, $width, $lColor, $rColor, $backColor) = @_;
	my $c = $self-> color;
	$_ = int($_) for $x1, $y1, $x, $y, $width;
	if ( defined $backColor)
	{
		if ( ref($backColor)) {
			$backColor->clone(canvas => $self)->bar($x + $width, $y + $width, $x1 - $width, $y1 - $width);
		} elsif ( $backColor == cl::Back) {
			$self-> clear( $x + $width, $y + $width, $x1 - $width, $y1 - $width);
		} else {
			$self-> color( $backColor);
			$self-> bar( $x + $width, $y + $width, $x1 - $width, $y1 - $width);
		}
	}
	$lColor = $rColor = cl::Black if $self-> get_bpp == 1;
	$self-> color( $c), return if $width <= 0;
	$self-> color( $lColor);
	$width = ( $y1 - $y) / 2 if $width > ( $y1 - $y) / 2;
	$width = ( $x1 - $x) / 2 if $width > ( $x1 - $x) / 2;
	$self-> lineWidth( 0);
	my $i;
	for ( $i = 0; $i < $width; $i++) {
		$self-> line( $x + $i, $y + $i, $x + $i, $y1 - $i);
		$self-> line( $x + $i + 1, $y1 - $i, $x1 - $i, $y1 - $i);
	}
	$self-> color( $rColor);
	for ( $i = 0; $i < $width; $i++) {
		$self-> line( $x1 - $i, $y + $i, $x1 - $i, $y1 - $i);
		$self-> line( $x + $i + 1, $y + $i, $x1 - $i, $y + $i);
	}
	$self-> color( $c);
}

sub rect_solid
{
	my ( $canvas, $x1, $y1, $x2, $y2, $lw, $fg) = @_;
	($x1, $x2) = ($x2, $x1) if $x2 < $x1;
	($y1, $y2) = ($y2, $y1) if $y2 < $y1;
	$lw //= 1;
	return if $lw < 1;
	$canvas-> graphic_context( sub {
		$canvas->antialias(0);
		$canvas->fillPattern(fp::Solid);
		$canvas-> color( $fg ) if defined $fg;
		if ( $x1 + $lw * 2 > $x2 || $y1 + $lw * 2 > $y2 ) {
			$canvas-> bar( $x1, $y1, $x2, $y2 );
		} elsif ( $lw == 1 ) {
			$canvas-> lineWidth(0);
			$canvas-> linePattern(lp::Solid);
			$canvas-> rectangle( $x1, $y1, $x2, $y2);
		} else {
			$lw--;
			$canvas-> bars([
				$x1, $y1, $x1 + $lw, $y2,
				$x2 - $lw, $y1, $x2, $y2,
				$x1 + $lw + 1, $y1, $x2 - $lw - 1, $y1 + $lw,
				$x1 + $lw + 1, $y2 - $lw, $x2 - $lw - 1, $y2
			]);
		}
	});
}

sub rect_fill
{
	my ( $canvas, $x1, $y1, $x2, $y2, $lw, $fg, $bg) = @_;
	($x1, $x2) = ($x2, $x1) if $x2 < $x1;
	($y1, $y2) = ($y2, $y1) if $y2 < $y1;
	$lw //= 1;
	$canvas-> graphic_context( sub {
		$canvas->antialias(0);
		if ( $lw <= 0 ) {
			$canvas-> color( $bg ) if defined $bg;
			$canvas-> fillPattern(fp::Solid);
			$canvas-> bar( $x1, $y1, $x2, $y2 );
		} elsif ( $x1 + $lw * 2 > $x2 || $y1 + $lw * 2 > $y2 ) {
			$canvas-> color( $fg ) if defined $fg;
			$canvas-> bar( $x1, $y1, $x2, $y2 );
		} elsif ( $lw == 1 ) {
			$bg //= $canvas->backColor;
			$canvas-> color( $fg ) if defined $fg;
			$canvas-> lineWidth(0);
			$canvas-> linePattern(lp::Solid);
			$canvas-> rectangle( $x1, $y1, $x2, $y2);
			$canvas-> color( $bg );
			$canvas-> bar( $x1 + 1, $y1 + 1, $x2 - 1, $y2 - 1);
		} else {
			$fg //= $canvas->color;
			$canvas-> color( $bg ) if defined $bg;
			$canvas-> bar( $x1 + $lw, $y1 + $lw, $x2 - $lw, $y2 - $lw);
			$lw--;
			$canvas-> color( $fg );
			$canvas-> fillPattern(fp::Solid);
			$canvas-> bars([
				$x1, $y1, $x1 + $lw, $y2,
				$x2 - $lw, $y1, $x2, $y2,
				$x1 + $lw + 1, $y1, $x2 - $lw - 1, $y1 + $lw,
				$x1 + $lw + 1, $y2 - $lw, $x2 - $lw - 1, $y2
			]);
		}
	});
}

sub rect_focus
{
	my ( $canvas, $x, $y, $x1, $y1, $width) = @_;
	( $x, $x1) = ( $x1, $x) if $x > $x1;
	( $y, $y1) = ( $y1, $y) if $y > $y1;

	$width = 1 if !defined $width || $width < 1;

	return unless $canvas-> graphic_context_push;
	$canvas-> set(
		fillPattern => fp::SimpleDots,
		rop2        => rop::CopyPut,
		color       => cl::White,
		backColor   => cl::Black,
		antialias   => 0,
		alpha       => 255,
	);

	if ( $width * 2 >= $x1 - $x or $width * 2 >= $y1 - $y) {
		$canvas-> bar( $x, $y, $x1, $y1);
	} else {
		$width -= 1;
		$canvas-> bars([
			$x, $y, $x1, $y + $width,
			$x, $y1 - $width, $x1, $y1,
			$x, $y + $width + 1, $x + $width, $y1 - $width - 1,
			$x1 - $width, $y + $width + 1, $x1, $y1 - $width - 1
		]);
	}

	$canvas-> graphic_context_pop;
}

sub draw_text
{
	my ( $canvas, $string, $x, $y, $x2, $y2, $flags, $tabIndent) = @_;

	$flags     = dt::Default unless defined $flags;
	$tabIndent = 1 if !defined( $tabIndent) || $tabIndent < 0;

	$x2 //= $x + 1;
	$y2 //= $y + 1;

	$x2 = int( $x2);
	$x  = int( $x);
	$y2 = int( $y2);
	$y  = int( $y);

	my ( $w, $h) = ( $x2 - $x + 1, $y2 - $y + 1);

	return 0 if $w <= 0 || $h <= 0;

	my $twFlags = tw::ReturnLines | tw::ReturnGlyphs |
		(( $flags & dt::DrawMnemonic  ) ? ( tw::CalcMnemonic | tw::CollapseTilde) : 0) |
		(( $flags & dt::DrawSingleChar) ? 0 : tw::BreakSingle ) |
		(( $flags & dt::NewLineBreak  ) ? tw::NewLineBreak : 0) |
		(( $flags & dt::SpaceBreak    ) ? tw::SpaceBreak   : 0) |
		(( $flags & dt::WordBreak     ) ? tw::WordBreak    : 0) |
		(( $flags & dt::ExpandTabs    ) ? ( tw::ExpandTabs | tw::CalcTabs) : 0)
	;

	my @lines = @{$canvas-> text_wrap_shape( $string,
		( $flags & dt::NoWordWrap) ? undef : $w,
		options => $twFlags, tabs => $tabIndent,
		(( $flags & dt::Underlined) ? ( skip_if_simple => 0 ) : ()),
	)};

	my $tildes;
	$tildes = pop @lines if $flags & dt::DrawMnemonic;

	return 0 unless scalar @lines;

	my @clipSave;
	my $fh = $canvas-> font-> height +
		(( $flags & dt::UseExternalLeading) ?
			$canvas-> font-> externalLeading :
			0
		);
	my ( $linesToDraw, $retVal);
	my $valign = $flags & 0xC;

	if ( $flags & dt::QueryHeight) {
		$linesToDraw = scalar @lines;
		$h = $retVal = $linesToDraw * $fh;
	} else {
		$linesToDraw = int( $retVal = ( $h / $fh));
		$linesToDraw++
			if (( $h % $fh) > 0) and ( $flags & dt::DrawPartial);
		$valign      = dt::Top
			if $linesToDraw < scalar @lines;
		$linesToDraw = $retVal = scalar @lines
			if $linesToDraw > scalar @lines;
	}

	if ( $flags & dt::UseClip) {
		@clipSave = $canvas-> clipRect;
		$canvas-> clipRect( $x, $y, $x + $w, $y + $h);
	}

	if ( $valign == dt::Top) {
		$y = $y2;
	} elsif ( $valign == dt::VCenter) {
		$y = $y2 - int(( $h - $linesToDraw * $fh) / 2);
	} else {
		$y += $linesToDraw * $fh;
	}

	my ( $starty, $align) = ( $y, $flags & 0x3);

	my $need_gc;
	if ( $flags & (dt::DrawMnemonic|dt::Underlined)) {
		if ( $canvas->graphic_context_push ) {
			my $lw = $canvas->font->underlineThickness;
			$lw = 1.0 if $lw < 1.0;
			$canvas->lineWidth($lw);
			$need_gc = 1;
		}
	}

	for my $l ( @lines) {
		last unless $linesToDraw--;
		my $xx;
		if ( $align == dt::Left) {
			$xx = $x;
		} elsif ( $align == dt::Center) {
			$xx = $x + int(( $w - $canvas-> get_text_width( $l)) / 2);
		} else {
			$xx = $x2 - $canvas-> get_text_width( $l);
		}
		$y -= $fh;
		$canvas-> text_out( $l, $xx, $y);
		$canvas-> draw_underline($l, $xx, $y)
			if $flags & dt::Underlined;
	}

	if (( $flags & dt::DrawMnemonic) and ( defined $tildes-> {tildeLine})) {
		my $tl = $tildes-> {tildeLine};
		my $xx = $x;
		if ( $align == dt::Center) {
			$xx = $x + int(( $w - $canvas-> get_text_width( $lines[ $tl])) / 2);
		} elsif ( $align == dt::Right) {
			$xx = $x2 - $canvas-> get_text_width( $lines[ $tl]);
		}
		$tl++;
		$canvas->linePattern(lp::Solid);
		$canvas->lineEnd(le::Square);
		$canvas-> line(
			$xx + $tildes-> {tildeStart}, $starty - $fh * $tl,
			$xx + $tildes-> {tildeEnd}  , $starty - $fh * $tl
		);
	}

	$canvas->graphic_context_pop if $need_gc;

	$canvas-> clipRect( @clipSave) if $flags & dt::UseClip;

	return $retVal;
}

sub draw_underline
{
	my ( $canvas, $string, $x, $y, %opt) = @_;

	$string = $canvas->text_shape($string, %opt, skip_if_simple => 0)
		unless ref $string;

	my $r = $canvas->render_underline($string, $x, $y);
	if ( $r ) {
		$canvas->lines($r);
	} elsif ( $r = $canvas->get_text_box($string)) {
		$canvas->line(
			$r->[2] + $x,
			$r->[3] + $y,
			$r->[6] + $x,
			$r->[7] + $y,
		);
	}
}

sub prelight_color
{
	my ( $self, $color, $coeff ) = @_;
	$coeff //= 1.05;
	return 0 if $coeff <= 0;
	$color = $self->map_color($color) if $color & cl::SysFlag;
	my $blend = cl::blend( $color, $self->map_color($self->hiliteBackColor), 0.25 );
	return $blend if cl::distance( $blend, $color ) > 30;
	if (
		((cl::distance(cl::White, $color) < 30) && $coeff > 1) ||
		((cl::distance(cl::Black, $color) < 30) && $coeff < 1)
	) {
		$coeff = 1/$coeff;
	}
	$coeff = ($coeff - 1) * 256;
	my @channels = cl::to_rgb($color);
	for (@channels) {
		my $amp = ( 256 - $_ ) / 8;
		$amp = -$amp if $coeff < 0;
		$_ += $coeff + $amp;
		$_ = 255 if $_ > 255;
		$_ = 0   if $_ < 0;
	}
	return cl::from_rgb(@channels);
}

sub text_split_lines
{
	my ($self, $text) = @_;
	return ref($text) ?
		@{ $self-> text_wrap( $text, -1, tw::NewLineBreak ) } :
		split "\n", $text;
}

sub new_path
{
	require Prima::Drawable::Path;
	return Prima::Drawable::Path->new(@_);
}

sub new_gradient
{
	require Prima::Drawable::Gradient;
	return Prima::Drawable::Gradient->new(@_);
}

sub new_aa_surface
{
	require Prima::Drawable::Antialias;
	return Prima::Drawable::Antialias->new(@_);
}

sub new_glyph_obj
{
	shift;
	require Prima::Drawable::Glyphs;
	return Prima::Drawable::Glyphs->new(@_);
}

sub stroke_img_primitive
{
	my ( $self, $request ) = (shift, shift);
	return 1 if $self->rop == rop::NoOper;
	return 1 if $self->linePattern eq lp::Null && $self->rop2 == rop::NoOper;

	my $single_pixel = $self->lineWidth < 1.5;
	my $path = $self->new_path( subpixel => !$single_pixel);
	my $matrix = $self-> get_matrix;
	$path->matrix( $matrix );
	$path->$request(@_);
	my $ok = 1;

	return $self->graphic_context( sub {
		# paths produce floating point coordinates and line end arcs,
		# here we need internal pixel-wise plotting
		$self-> reset_matrix;
		$self-> lineWidth(0);
		return $path->stroke;
	}) if $single_pixel;

	my %widen;
	my $method;
	if ($self->linePattern eq lp::Null) {
		$widen{linePattern} = lp::Solid;
		$method = 'clear';
	} else {
		$method = 'bar';
	}

	my $region2 = $self->region;
	my $path2   = $path->widen(%widen);
	my $region1 = $path2->region(fm::Winding | fm::Overlay);
	my @rect    = $region1->rect;

	return unless $self->graphic_context_push;
	$self->fillPattern(fp::Solid);
	$self->reset_matrix;
	if ( $self-> rop2 == rop::CopyPut && $self->linePattern ne lp::Solid && $self->linePattern ne lp::Null ) {
		my $c = $self->color;
		$self->color($self->backColor);
		my $path3 = $path->widen( linePattern => lp::Solid );
		my $region3 = $path3->region;
		$region3->combine( $region1, rgnop::Diff);
		$region3->combine($region2, rgnop::Intersect) if $region2;
		$self->region($region3);
		$ok = $self->bar(@rect);
		$self->color($c);
	}

	$region1->combine($region2, rgnop::Intersect) if $region2;
	$self->region($region1);
	$ok &&= $self->$method(@rect);
	$self->graphic_context_pop;
	return $ok;
}

sub stroke_imgaa_primitive
{
	my ( $self, $request ) = (shift, shift);
	return 1 if $self->rop == rop::NoOper;
	my $lp = $self->linePattern;
	return 1 if $lp eq lp::Null && $self->rop2 == rop::NoOper;

	my $path = $self->new_path( subpixel => 1 );
	$path->$request(@_);
	my $path2 = $path->widen(
		linePattern => ( $lp eq lp::Null) ? lp::Solid : $lp
	);

	return unless $self->graphic_context_push;

	$self->fillPattern(fp::Solid);
	$self->fillMode(fm::Winding | fm::Overlay);
	if ( $self-> rop2 == rop::CopyPut && $self->linePattern ne lp::Solid && $self->linePattern ne lp::Null ) {
		my $c = $self->color;
		$self->color($self->backColor);
		my $path3 = $path->widen( linePattern => lp::Solid);
		$path3->fill;
		$self->color($c);
	}

	$self->color($self->backColor) if $lp eq lp::Null;
	my $ok = $path2->fill;
	$self->graphic_context_pop;
	return $ok;
}

sub execute_img_primitive
{
	my ( $self, $request ) = (shift, shift);
	my $ps = $self->get_paint_state;
	return $self->stroke_primitive($request, @_) if $ps == ps::Enabled;
	return 1 if $ps == ps::Information;

	my $suffix = $self->antialias ? 'aa' : '';
	my $act .= 'stroke_img' . $suffix . '_primitive';
	return $self->$act($request, @_);
}

sub stroke_primitive
{
	my ( $self, $request ) = (shift, shift);
	return 1 if $self->rop == rop::NoOper;
	my $lp = $self->linePattern;
	return 1 if $lp eq lp::Null && $self->rop2 == rop::NoOper;

	my $single_pixel = !$self->antialias && $self->alpha == 255 && $self->lineWidth < 1.5;
	my $path = $self->new_path( subpixel => !$single_pixel);
	$path->matrix( $self-> matrix );
	$path->$request(@_);

	return $self->graphic_context( sub {
		$self-> lineWidth(0);
		$self-> matrix-> identity;
		return $path->stroke;
	} ) if $single_pixel;

	my $path2 = $path->widen(
		linePattern => ( $lp eq lp::Null) ? lp::Solid : $lp,
		(!$self->antialias && $self->lineWidth == 0) ? (lineWidth => 1) : (),
	);
	$path2->subpixel( $self-> antialias );
	return unless $self->graphic_context_push;
	$self-> matrix-> identity;
	$self->fillPattern(fp::Solid);
	$self->fillMode(fm::Winding | fm::Overlay);
	if ( $self-> rop2 == rop::CopyPut && $self->linePattern ne lp::Solid && $self->linePattern ne lp::Null ) {
		my $c = $self->color;
		$self->color($self->backColor);
		my $path3 = $path->widen(
			linePattern => lp::Solid,
			(!$self->antialias && $self->lineWidth == 0) ? (lineWidth => 1) : (),
		);
		$path3->subpixel( $self-> antialias );
		$path3->fill;
		$self->color($c);
	}
	$self->color($self->backColor) if $lp eq lp::Null;
	my $ok = $path2->fill;
	$self->graphic_context_pop;
	return $ok;
}

sub fill_primitive
{
	my ( $self, $request, @p ) = @_;
	my $path = $self->new_path;
	$path->$request(@p);
	return $path->fill;
}

sub text_shape_out
{
	my ( $self, $text, $x, $y, $rtl) = @_;
	my %flags = (skip_if_simple => 1);
	$flags{rtl} = $rtl if defined $rtl;
	if ( my $glyphs = $self->text_shape($text, %flags)) {
		$text = $glyphs;
	}
	return $self->text_out( $text, $x, $y);
}

sub get_text_shape_width
{
	my ( $self, $text, $flags) = @_;
	Carp::confess unless defined $text;
	my %flags = (skip_if_simple => 1);
	$flags{rtl} = $flags & to::RTL if defined $flags;
	if ( my $glyphs = $self->text_shape($text, %flags)) {
		$text = $glyphs;
	}
	return $self->get_text_width( $text, $flags // 0);
}

sub text_wrap_shape
{
	my ( $self, $text, $width, %opt) = @_;

	my $opt    = delete($opt{options}) // tw::Default;

	my $shaped;
	if ( $opt & ( tw::NewLineBreak | tw::ExpandTabs | tw::SpaceBreak )) {
		# looking up a newline glyph can be expensive, and unnecessary here
		my $t = $text;
		$t =~ s/[\n\r]/ /gs if $opt & tw::NewLineBreak;
		$t =~ s/\t/ /s if $opt & ( tw::ExpandTabs | tw::SpaceBreak );
		$shaped = $self-> text_shape( $t, %opt );
	} else {
		$shaped = $self-> text_shape( $text, %opt );
	}
	return $self->text_wrap( $text, $width // -1, $opt & ~tw::ReturnGlyphs, delete($opt{tabs}) // 8) unless $shaped;
	my $ret = $self-> text_wrap( $text, $width // -1, $opt, delete($opt{tabs}) // 8, 0, -1, $shaped);

	if (( my $justify = delete $opt{justify} ) && $ret && @$ret ) {
		if (
			$justify->{kashida} &&
			!($opt & tw::ReturnChunks) &&
			$text =~ /[\x{600}-\x{6ff}]/
		) {
			my $last = @$ret - ($opt & (tw::CalcMnemonic | tw::CollapseTilde)) ? -2 : -1;
			for ( my $i = 0; $i < $last; $i++) {
				if ( $opt & tw::ReturnGlyphs ) {
					$$ret[$i]->justify_arabic($self, $text, $width, %opt, %$justify);
				} elsif ( my $tx = $self->text_shape( $$ret[$i], %opt)) {
					my $text = $tx->justify_arabic($self, $$ret[$i], $width, %opt, %$justify, as_text => 1);
					$$ret[$i] = $text if defined $text;
				}
			}
		}

		if (
			($justify->{letter} || $justify->{word}) &&
			!($opt & tw::ReturnChunks)
		) {
			# do not justify last (or the only) line
			my $last = @$ret - ($opt & (tw::CalcMnemonic | tw::CollapseTilde)) ? -3 : -2;
			for ( my $i = 0; $i < $last; $i++) {
				if ( $opt & tw::ReturnGlyphs ) {
					$$ret[$i]->justify_interspace($self, $text, $width, %opt, %$justify);
				} elsif ( my $tx = $self->text_shape( $$ret[$i], %opt)) {
					my $text = $tx->justify_interspace($self, $$ret[$i], $width, %opt, %$justify, as_text => 1);
					$$ret[$i] = $text if defined $text;
				}
			}
		}
	}

	return $ret;
}

sub render_pattern
{
	my ( $package, $handle, %opt ) = @_;

	# parse arguments
	my $matrix = $opt{matrix};
	my @margin = defined($opt{margin}) ?
		( ref($opt{margin}) ? @{$opt{margin}} : (($opt{margin}) x 2)) :
		( 0,0 );

	return unless $matrix || $margin[0] != 0 || $margin[1] != 0;
	if ( $margin[0] < 0 || $margin[1] < 0 ) {
		warn "render_pattern: bad margin";
		return;
	}

	# non-image
	if ( !ref($handle) || ref($handle) eq 'ARRAY') {
		my $p = Prima::Image->new(
			type         => im::BW,
			size         => [8,8],
			rop2         => rop::CopyPut,
			fillPattern  => $handle,
			preserveType => 1,
		);
		$p->bar(0,0,$p->size);
		$handle = $p;
	}

	# all calculations are in 32-bit RGBA or 8-bit GA
	my $source = $handle->to_rgba;
	$source->transform( matrix => $matrix )
		if $matrix;

	$matrix //= Prima::matrix->new;

	# target's size is the same as transformed's image enclosure
	my $target = ref($handle)->new(
		size        => [ $source-> size ],
		type        => (( $handle->type & im::GrayScale ) ? im::Byte : im::RGB),
		maskType    => 8,
		autoMasking => am::None,
	);

	if ( defined $opt{color}) {
		$target->backColor($opt{color});
		$target->clear;
	}
	if ( exists $opt{alpha} && $handle->isa('Prima::Icon')) {
		my $fill = chr($opt{alpha} & 0xff);
		$target->mask( $fill x $target->maskSize );
	}

	#
	# instead of calculating plotting positions directly, calculate them as if
	# we're plotting rectangular source ( before the transformation ) on a target
	# that is inversely transformed. I.e not this:
	#
	#  .......
	#  .     .
	#  . __  .
	#  ./_/...
	#
	# but this:
	#   .
	#  .  .
	# .  _ .
	#  .|_| .
	#   .  .
	#     .
	#

	my $enclosure = sub {
		my @D = @_;
		my ( $dx1, $dy1, $dx2, $dy2 ) = @D[0,1,0,1];
		for ( my $i = 0; $i < @D; $i += 2 ) {
			my ( $x, $y ) = @D[$i,$i+1];
			$dx1 = $x if $dx1 > $x;
			$dy1 = $y if $dy1 > $y;
			$dx2 = $x if $dx2 < $x;
			$dy2 = $y if $dy2 < $y;
		}
		return ( $dx1, $dy1, $dx2, $dy2 );
	};

	my @s = $handle->size;
	$s[$_] += $margin[$_] * 2 for 0,1;
	my @r = $matrix->transform(
		0,         0,
		$s[0] - 1, 0,
		$s[0] - 1, $s[1] - 1,
		0,         $s[1] - 1
	);

	my @aperture = $enclosure->(@r);
	my @d = ($aperture[2] - $aperture[0], $aperture[3] - $aperture[1]);
	my @D = $matrix->inverse_transform(
		0,         0,
		$d[0] - 1, 0,
		$d[0] - 1, $d[1] - 1,
		0        , $d[1] - 1
	);

	my ( $dx1, $dy1, $dx2, $dy2 ) = $enclosure->(@D);

	$dx1 /= $s[0];
	$dx2 /= $s[0];
	$dy1 /= $s[1];
	$dy2 /= $s[1];
	$dx1 = int($dx1) - 1;
	$dy1 = int($dy1) - 1;
	$dx2 = int($dx2) + 1;
	$dy2 = int($dy2) + 1;

	my $plotter = Prima::matrix->new( map { int }
		$r[2] - $r[0],
		$r[3] - $r[1],
		$r[0] - $r[6],
		$r[1] - $r[7],
		$aperture[0] + $margin[0],
		$aperture[1] + $margin[1],
	);

	# execute
	($dy2, $dy1) = (-$dy1, -$dy2);
	for ( my $y = $dy1; $y <= $dy2; $y++) {
		for ( my $x = $dx1; $x <= $dx2; $x++) {
			$target->put_image($plotter->transform($x,$y), $source);
		}
	}

	# finalize
	if ( $handle-> preserveType ) {
		$target->type( $handle->type);
		$target->maskType( $handle->maskType ) if $handle->isa('Prima::Icon');
	}

	return $target;
}

1;

=head1 NAME

Prima::Drawable::Basic

=head1 DESCRIPTION

Basic drawing routines for Prima::Drawable

=cut
