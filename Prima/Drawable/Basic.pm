package Prima::Drawable::Basic; # for metacpan
package Prima::Drawable;

use strict;
use warnings;

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

sub rect_focus
{
	my ( $canvas, $x, $y, $x1, $y1, $width) = @_;
	( $x, $x1) = ( $x1, $x) if $x > $x1;
	( $y, $y1) = ( $y1, $y) if $y > $y1;

	$width = 1 if !defined $width || $width < 1;
	my ( $cl, $cl2) = ( $canvas-> color, $canvas-> backColor);
	my $fp = $canvas-> fillPattern;
	$canvas-> set(
		fillPattern => fp::SimpleDots,
		color       => cl::White,
		backColor   => cl::Black,
	);

	if ( $width * 2 >= $x1 - $x or $width * 2 >= $y1 - $y) {
		$canvas-> bar( $x, $y, $x1, $y1);
	} else {
		$width -= 1;
		$canvas-> bar( $x, $y, $x1, $y + $width);
		$canvas-> bar( $x, $y1 - $width, $x1, $y1);
		$canvas-> bar( $x, $y + $width + 1, $x + $width, $y1 - $width - 1);
		$canvas-> bar( $x1 - $width, $y + $width + 1, $x1, $y1 - $width - 1);
	}

	$canvas-> set(
		fillPattern => $fp,
		backColor   => $cl2,
		color       => $cl,
	);
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

	my $twFlags = tw::ReturnLines |
		(( $flags & dt::DrawMnemonic  ) ? ( tw::CalcMnemonic | tw::CollapseTilde) : 0) |
		(( $flags & dt::DrawSingleChar) ? 0 : tw::BreakSingle ) |
		(( $flags & dt::NewLineBreak  ) ? tw::NewLineBreak : 0) |
		(( $flags & dt::SpaceBreak    ) ? tw::SpaceBreak   : 0) |
		(( $flags & dt::WordBreak     ) ? tw::WordBreak    : 0) |
		(( $flags & dt::ExpandTabs    ) ? ( tw::ExpandTabs | tw::CalcTabs) : 0)
	;

	my @lines = @{$canvas-> text_wrap_shape( $string,
		( $flags & dt::NoWordWrap) ? undef : $w,
		options => $twFlags, tabs => $tabIndent
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

	for ( @lines) {
		last unless $linesToDraw--;
		my $xx;
		if ( $align == dt::Left) {
			$xx = $x;
		} elsif ( $align == dt::Center) {
			$xx = $x + int(( $w - $canvas-> get_text_width( $_)) / 2);
		} else {
			$xx = $x2 - $canvas-> get_text_width( $_);
		}
		$y -= $fh;
		$canvas-> text_out( $_, $xx, $y);
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
		$canvas-> line(
			$xx + $tildes-> {tildeStart}, $starty - $fh * $tl,
			$xx + $tildes-> {tildeEnd}  , $starty - $fh * $tl
		);
	}

	$canvas-> clipRect( @clipSave) if $flags & dt::UseClip;

	return $retVal;
}

sub prelight_color
{
	my ( $self, $color, $coeff ) = @_;
	$coeff //= 1.05;
	return 0 if $coeff <= 0;
	$color = $self->map_color($color) if $color & cl::SysFlag;
	if (( $color == 0xffffff && $coeff > 1) || ($color == 0 && $coeff < 1)) {
		$coeff = 1/$coeff;
	}
	$coeff = ($coeff - 1) * 256;
	my @channels = map { $_ & 0xff } cl::to_rgb($color);
	for (@channels) {
		my $amp = ( 256 - $_ ) / 8;
		$amp -= $amp if $coeff < 0;
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
		@{ $self-> text_wrap( $text, 2_000_000_000, tw::NewLineBreak ) } :
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


sub new_glyph_obj
{
	shift;
	require Prima::Drawable::Glyphs;
	return Prima::Drawable::Glyphs->new(@_);
}

sub stroke_primitive
{
	my ( $self, $request ) = (shift, shift);
	return 1 if $self->rop == rop::NoOper;
	return 1 if $self->linePattern eq lp::Null && $self->rop2 == rop::NoOper;

	my $path = $self->new_path;
	my @offset  = $self->translate;
	$path->translate(@offset);
	$path->$request(@_);
	my $ok = 1;
	if ( $self->lineWidth == 0 ) {
		# paths produce floating point coordinates and line end arcs,
		# here we need internal pixel-wise plotting
		for my $pp ( map { @$_ } @{ $path->points } ) {
			last unless $ok &= $self->polyline($pp);
		}
		return $ok;
	}

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
	my @box = $region1->box;
	$box[$_+2] += $box[$_] for 0,1;
	my $fp = $self->fillPattern;
	$self->fillPattern(fp::Solid);
	$self->translate(0,0);
	if ( $self-> rop2 == rop::CopyPut && $self->linePattern ne lp::Solid && $self->linePattern ne lp::Null ) {
		my $color = $self->color;
		$self->color($self->backColor);
		my $path3 = $path->widen( linePattern => lp::Solid );
		my $region3 = $path3->region;
		$region3->combine( $region1, rgnop::Diff);
		$region3->combine($region2, rgnop::Intersect) if $region2;
		$self->region($region3);
		$ok = $self->bar(@box);
		$self->color($color);
	}

	$region1->combine($region2, rgnop::Intersect) if $region2;
	$self->region($region1);
	$ok &&= $self->$method(@box);
	$self->region($region2);
	$self->fillPattern($fp);
	$self->translate(@offset);
	return $ok;
}

sub fill_primitive
{
	my ( $self, $request ) = (shift, shift);
	my $path = $self->new_path;
	$path->$request(@_);
	my @offset  = $self->translate;
	my $region1 = $path->region( $self-> fillMode);
	$region1->offset(@offset);
	my $region2 = $self->region;
	$region1->combine($region2, rgnop::Intersect) if $region2;
	my @box = $region1->box;
	$box[$_+2] += $box[$_] for 0,1;
	$self->region($region1);
	$self->translate(0,0);
	my $ok = $self->bar(@box);
	$self->translate(@offset);
	$self->region($region2);
	return $ok;
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

	$width = 1_000_000 unless defined $width;

	my $opt = delete($opt{options}) // tw::Default;

	my $wrapped = $self-> text_wrap( $text, $width, $opt, delete($opt{tabs}) // 8);
	return $wrapped if $opt & tw::ReturnChunks;

	my $tilde;
	$tilde = pop @$wrapped if $opt & (tw::CalcMnemonic | tw::CollapseTilde);

	my @shaped;
	for my $chunk ( @$wrapped ) {
		my $shaped = $self-> text_shape( $chunk, %opt );
		unless (defined $shaped) {
			push @$wrapped, $tilde if $tilde;
			return $wrapped;
		}
		$shaped = $chunk unless $shaped;
		push @shaped, $shaped;
	}

	if ( $tilde && defined($tilde->{tildeLine}) && ref(my $glyphs = $shaped[$tilde->{tildeLine}])) {
		my $pos = $tilde->{tildePos};
		my $index = -1;
		my $found = -1;
		my $indexes;
		my $ligature;

AGAIN:
		$indexes = $glyphs-> indexes;
		for my $c ( @$indexes ) {
			$index++;
			$c &= ~to::RTL;
			$found = $index, last if $c == $pos; # same glyph
			$found = $index if $c < $pos && $c > $found; # same cluster?
		}

		# check for ligature: "f~l" situation where "fl" is a single glyph
		# (also, ligatures won't work without the advances array)
		if ( $indexes->[$found+1] > $indexes->[$found] + 1 && !$ligature && $glyphs->advances ) {
			my $text = $wrapped->[$tilde->{tildeLine}];
			substr( $text, $pos++, 0, "\x{200c}");
			substr( $text, $pos + 1, 0, "\x{200c}");
			$glyphs = $self-> text_shape( $text, %opt );
			$ligature++;
			$index = $found = -1;
			goto AGAIN;
		}

		my ($A,$B,$C) = $glyphs-> abc($self, $found);
		my ($A0) = $glyphs-> abc($self, 0);
		my $x = $glyphs->get_sub_width( $self, 0, $found ) - (($A0 < 0) ? $A0 : 0);
		$tilde->{tildeStart}  = $x;
		$tilde->{tildeStart} += $A if $A < 0;
		$tilde->{tildeEnd}    = $x + $B;
		$tilde->{tildeEnd}   -= $C if $C < 0;
	}

	push @shaped, $tilde if $tilde;

	return \@shaped;
}

1;

=head1 NAME

Prima::Drawable::Basic

=head1 NAME

Basic drawing routines for Prima::Drawable

=cut
