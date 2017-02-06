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
		if ( ref $backColor ) {
			$self-> gradient_bar($x + $width, $y + $width, $x1 - $width, $y1 - $width, $backColor);
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
	my ( $cl, $cl2, $rop) = ( $canvas-> color, $canvas-> backColor, $canvas-> rop);
	my $fp = $canvas-> fillPattern;
	$canvas-> set(
		fillPattern => fp::SimpleDots,
		color       => cl::Set,
		backColor   => cl::Clear,
		rop         => rop::XorPut,
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
		rop         => $rop,
	);
}

sub draw_text
{
	my ( $canvas, $string, $x, $y, $x2, $y2, $flags, $tabIndent) = @_;

	$flags     = dt::Default unless defined $flags;
	$tabIndent = 1 if !defined( $tabIndent) || $tabIndent < 0;

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

	my @lines = @{$canvas-> text_wrap( $string, 
		( $flags & dt::NoWordWrap) ? 1000000 : $w, 
		$twFlags, $tabIndent
	)};

	my $tildes;
	$tildes = pop @lines if $flags & dt::DrawMnemonic;

	return 0 unless scalar @lines;

	if (($flags & dt::BidiText) && $Prima::Bidi::available) {
		$_ = Prima::Bidi::visual($_) for @lines;
	}

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
	my @channels = map { $_ & 0xff } ($color >> 16), ($color >> 8), $color;
	for (@channels) {
		my $amp = ( 256 - $_ ) / 8;
		$amp -= $amp if $coeff < 0;
		$_ += $coeff + $amp;
		$_ = 255 if $_ > 255;
		$_ = 0   if $_ < 0;
	}
	return ( $channels[0] << 16 ) | ( $channels[1] << 8 ) | $channels[2];
}

sub gradient_polyline_to_points
{
	my ($self, $p) = @_;
	my @map;
	for ( my $i = 0; $i < @$p - 2; $i+=2) {
		my ($x1,$y1,$x2,$y2) = @$p[$i..$i+3];
		$x1 = 0 if $x1 < 0;
		my $dx = $x2 - $x1;
		if ( $dx > 0 ) {
			my $dy = ($y2 - $y1) / $dx;
			my $y = $y1;
			for ( my $x = $x1; $x <= $x2; $x++) {
				$map[$x] = $y;
				$y += $dy;
			}
		} else {
			$map[$x1] = $y1;
		}
	}
	return \@map;
}

sub gradient_realize3d
{
	my ( $self, $breadth, $request) = @_;

	my ($offsets, $points);

	unless ( $points = $request->{points}) {
		my @spline = (0,0);
		if ( my $s = $request->{spline} ) {
			push @spline, map { $_ * $breadth } @$s;
		}
		if ( my $s = $request->{poly} ) {
			push @spline, map { $_ * $breadth } @$s;
		}
		push @spline, $breadth, $breadth;
		my $polyline = ( @spline > 4 && $request->{spline} ) ? $self-> render_spline( \@spline ) : \@spline;
		$points = $self-> gradient_polyline_to_points($polyline);
	}

	unless ($offsets = $request->{offsets}) {
		my @o;
		my $n = scalar(@{$request->{palette}}) - 1;
		my $d = 0;
		for ( my $i = 0; $i < $n; $i++) {
			$d += 1/$n;
			push @o, $d;
		}
		push @o, 1;
		$offsets = \@o;
	}
	
	$request->{points}  = $points;
	$request->{offsets} = $offsets;

	return $self-> gradient_calculate(
		$request->{palette},
		[ map { $_ * $breadth } @$offsets ],
		sub { $points->[shift] }
	);
}

sub gradient_calculate_single
{
	my ( $self, $breadth, $start_color, $end_color, $function, $offset ) = @_;

	return if $breadth <= 0;

	$offset //= 0;
	$start_color = $self->map_color($start_color) if $start_color & cl::SysFlag;
	$end_color   = $self->map_color($end_color)   if $end_color   & cl::SysFlag;
	my @start = map { $_ & 0xff } ($start_color >> 16), ($start_color >> 8), $start_color;
	my @end   = map { $_ & 0xff } ($end_color   >> 16), ($end_color   >> 8), $end_color;
	my @color = @start;
	return $start_color, 1 if $breadth == 1;
	
	my @delta = map { ( $end[$_] - $start[$_] ) / ($breadth - 1) } 0..2;

	my $last_color = $start_color;
	my $color      = $start_color;
	my $width      = 0;
	my @ret;
	for ( my $i = 0; $i < $breadth; $i++) {
		my @c;
		my $j = $function ? $function->( $offset + $i ) - $offset : $i;
		for ( 0..2 ) {
			$color[$_] = $start[$_] + $j * $delta[$_] for 0..2;
			$c[$_] = int($color[$_] + .5);
			$c[$_] = 0xff if $c[$_] > 0xff;
			$c[$_] = 0    if $c[$_] < 0;
		}
		$color = ( $c[0] << 16 ) | ( $c[1] << 8 ) | $c[2];
		if ( $last_color != $color ) {
			push @ret, $last_color, $width;
			$last_color = $color;
			$width = 0;
		}

		$width++;
	}

	return @ret, $color, $width;
}

sub gradient_calculate
{
	my ( $self, $palette, $offsets, $function ) = @_;
	my @ret;
	my $last_offset = 0;
	$offsets = [ $offsets ] unless ref $offsets;
	for ( my $i = 0; $i < @$offsets; $i++) {
		my $breadth = $offsets->[$i] - $last_offset;
		push @ret, $self-> gradient_calculate_single( $breadth, $palette->[$i], $palette->[$i+1], $function, $last_offset);
		$last_offset = $offsets->[$i];
	}
	return \@ret;
}

sub gradient_bar
{
	my ( $self, $x1, $y1, $x2, $y2, $request ) = @_;

	$_ = int($_) for $x1, $y1, $x2, $y2;

	($x1,$x2)=($x2,$x1) if $x1 > $x2;
	($y1,$y2)=($y2,$y1) if $y1 > $y2;

	my $gradient = $request->{gradient} //= $self-> gradient_realize3d( 
		$request->{vertical} ? 
			$x2 - $x1 + 1 :
			$y2 - $y1 + 1, 
		$request
	);

	my @bar        = ($x1,$y1,$x2,$y2);
	my ($ptr1,$ptr2) = $request->{vertical} ? (0,2) : (1,3);
	my $max          = $bar[$ptr2];
	for ( my $i = 0; $i < @$gradient; $i+=2) {
		$bar[$ptr2] = $bar[$ptr1] + $gradient->[$i+1] - 1;
		$self->color( $gradient->[$i]);
		$self->bar( @bar );
		$bar[$ptr1] = $bar[$ptr2] + 1;
		last if $bar[$ptr1] > $max;
	}
	if ( $bar[$ptr1] <= $max ) {
		$bar[$ptr2] = $max;
		$self->bar(@bar);
	}
}

sub gradient_ellipse
{
	my ( $canvas, $x, $y, $dx, $dy, $request ) = @_;
	return if $dx <= 0 || $dy <= 0;

	$_ = int($_) for $x, $y, $dx, $dy;
	my $diameter = ($dx > $dy) ? $dx : $dy;
	my $mx = $dx / $diameter;
	my $my = $dy / $diameter;
	my $gradient = $canvas-> gradient_realize3d( $diameter, $request);
	for ( my $i = 0; $i < @$gradient; $i+=2) {
		$canvas->color( $gradient->[$i]);
		$canvas->fill_ellipse( $x, $y, $mx * $diameter, $my * $diameter );
		$diameter -= $gradient->[$i+1];
	}
}

sub gradient_sector
{
	my ( $canvas, $x, $y, $dx, $dy, $start, $end, $request ) = @_; 
	my $angle = $end - $start;
	$angle += 360 while $angle < 0;
	$angle %= 360;
	my $min_angle = 1.0 / 64; # x11 limitation

	my $max = ($dx < $dy) ? $dy : $dx;
	my $df  = $max * 3.14 / 360;
	my $arclen = int($df * $angle + .5);
	my $gradient = $canvas-> gradient_realize3d( $arclen, $request );
	my $accum = 0;
	for ( my $i = 0; $i < @$gradient - 2; $i+=2) {
		$canvas->color( $gradient->[$i]);
		my $d = $gradient->[$i+1] / $df;
		if ( $accum + $d < $min_angle ) {
			$accum += $d;
			next;
		}
		$d += $accum;
		$accum = 0;
		$canvas->fill_sector( $x, $y, $dx, $dy, $start, $start + $d + $min_angle);
		$start += $d;
	}
	if ( @$gradient ) {
		$canvas->color( $gradient->[-2]);
		$canvas->fill_sector( $x, $y, $dx, $dy, $start, $end);
	}
}

sub text_split_lines
{
	my ($self, $text) = @_;
	return ref($text) ? 
		@{ $self-> text_wrap( $text, 2_000_000_000, tw::NewLineBreak ) } :
		split "\n", $text;
}

1;

=head1 NAME

Prima::Drawable::Basic

=head1 NAME

Basic drawing routines for Prima::Drawable

=cut
