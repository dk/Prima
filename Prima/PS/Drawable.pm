package Prima::PS::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::Drawable);

use strict;
use warnings;
use Prima;

{
my %RNT = (
	%{Prima::Drawable-> notification_types()},
	Spool => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		grayscale        => 0,
		pageSize         => [ 598, 845],
		pageMargins      => [ 12, 12, 12, 12],
		resolution       => [ 300, 300],
		reversed         => 0,
		rotate           => 0,
		scale            => [ 1, 1],
		textOutBaseline  => 0,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	$self-> {clipRect}    = [0,0,0,0];
	$self-> {pageSize}    = [0,0];
	$self-> {pageMargins} = [0,0,0,0];
	$self-> {resolution}  = [72,72];
	$self-> {scale}       = [ 1, 1];
	$self-> {rotate}      = 1;
	my %profile = $self-> SUPER::init(@_);
	$self-> $_( $profile{$_}) for qw( grayscale rotate reversed );
	$self-> $_( @{$profile{$_}}) for qw( pageSize pageMargins resolution scale );
	$self->{fpType} = 'F';
	return %profile;
}

sub save_state
{
	my $self = $_[0];

	$self-> {save_state} = {};
	$self-> {save_state}-> {$_} = $self-> $_() for qw(
		color backColor fillPattern lineEnd linePattern lineWidth miterLimit
		rop rop2 textOpaque textOutBaseline font lineJoin fillMode
	);
	$self->{save_state}->{fpType} = $self->{fpType};
	$self-> {save_state}-> {$_} = [$self-> $_()] for qw(
		translate clipRect
	);
}

sub restore_state
{
	my $self = $_[0];
	for ( qw( color backColor fillPattern lineEnd linePattern lineWidth miterLimit
			rop rop2 textOpaque textOutBaseline font lineJoin fillMode)) {
		$self-> $_( $self-> {save_state}-> {$_});
	}
	$self->{fpType} = $self->{save_state}->{fpType};
	for ( qw( translate clipRect)) {
		$self-> $_( @{$self-> {save_state}-> {$_}});
	}
}

sub pixel2point
{
	my $self = shift;
	my $i;
	my @res;
	for ( $i = 0; $i < scalar @_; $i+=2) {
		my ( $x, $y) = @_[$i,$i+1];
		push @res, int( $x * 7227 / $self-> {resolution}-> [0] + 0.5) / 100;
		push @res, int( $y * 7227 / $self-> {resolution}-> [1] + 0.5) / 100 if defined $y;
	}
	return @res;
}

sub point2pixel
{
	my $self = shift;
	my $i;
	my @res;
	for ( $i = 0; $i < scalar @_; $i+=2) {
		my ( $x, $y) = @_[$i,$i+1];
		push @res, $x * $self-> {resolution}-> [0] / 72.27;
		push @res, $y * $self-> {resolution}-> [1] / 72.27 if defined $y;
	}
	return @res;
}

our $PI = 3.14159265358979323846264338327950288419716939937510;
our $RAD = 180.0 / $PI;

# L.Maisonobe 2003
# http://www.spaceroots.org/documents/ellipse/elliptical-arc.pdf
sub arc2cubics
{
	my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;

	my ($reverse, @out);
	($start, $end, $reverse) = ( $end, $start, 1 ) if $start > $end;

	push @out, $start;
	# see defects appearing after 45 degrees:
	# https://pomax.github.io/bezierinfo/#circles_cubic
	while (1) {
		if ( $end - $start > 45 ) {
			push @out, $start += 45;
			$start += 45;
		} else {
			push @out, $end;
			last;
		}
	}
	@out = map { $_ / $RAD } @out;

	my $rx = $dx / 2;
	my $ry = $dy / 2;

	my @cubics;
	for ( my $i = 0; $i < $#out; $i++) {
		my ( $a1, $a2 ) = @out[$i,$i+1];
		my $b           = $a2 - $a1;
		my ( $sin1, $cos1, $sin2, $cos2) = ( sin($a1), cos($a1), sin($a2), cos($a2) );
		my @d1  = ( -$rx * $sin1, -$ry * $cos1 );
		my @d2  = ( -$rx * $sin2, -$ry * $cos2 );
		my $tan = sin( $b / 2 ) / cos( $b / 2 );
		my $a   = sin( $b ) * (sqrt( 4 + 3 * $tan * $tan) - 1) / 3;
		my @p1  = ( $rx * $cos1, $ry * $sin1 );
		my @p2  = ( $rx * $cos2, $ry * $sin2 );
		my @points = (
			@p1,
			$p1[0] + $a * $d1[0],
			$p1[1] - $a * $d1[1],
			$p2[0] - $a * $d2[0],
			$p2[1] + $a * $d2[1],
			@p2
		);
		$points[$_] += $x for 0,2,4,6;
		$points[$_] += $y for 1,3,5,7;
		@points[0,1,2,3,4,5,6,7] = @points[6,7,4,5,2,3,0,1] if $reverse;
		push @cubics, \@points;
	}
	return \@cubics;
}

sub conic2curve
{
	my ($self, $x0, $y0, $x1, $y1, $x2, $y2) = @_;
	my (@cp1, @cp2);
	$cp1[0] = $x0 + 2 / 3 * ($x1 - $x0);
	$cp1[1] = $y0 + 2 / 3 * ($y1 - $y0);
	$cp2[0] = $x2 + 2 / 3 * ($x1 - $x2);
	$cp2[1] = $y2 + 2 / 3 * ($y1 - $y2);
	return @cp1, @cp2, $x2, $y2;
}

sub begin_paint_info
{
	my $self = $_[0];
	return 0 if $self-> get_paint_state;
	my $ok = $self-> SUPER::begin_paint_info;
	return 0 unless $ok;
	$self-> save_state;
}

sub end_paint_info
{
	my $self = $_[0];
	return if $self-> get_paint_state != ps::Information;
	$self-> SUPER::end_paint_info;
	$self-> restore_state;
}

sub spool
{
	shift-> notify( 'Spool', @_);
	return 1;
}

# properties

sub color
{
	return $_[0]-> SUPER::color unless $#_;
	$_[0]-> SUPER::color( $_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {fill} = 1;
}

sub fillPatternOffset
{
	return $_[0]-> SUPER::fillPatternOffset unless $#_;
	$_[0]-> SUPER::fillPatternOffset($_[1], $_[2]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {fillPatternOffset} = 1;
}

sub lineEnd
{
	return $_[0]-> SUPER::lineEnd unless $#_;
	$_[0]-> SUPER::lineEnd($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineEnd} = 1;
}

sub lineJoin
{
	return $_[0]-> SUPER::lineJoin unless $#_;
	$_[0]-> SUPER::lineJoin($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineJoin} = 1;
}

sub fillMode
{
	return $_[0]-> SUPER::fillMode unless $#_;
	$_[0]-> SUPER::fillMode($_[1]);
}

sub linePattern
{
	return $_[0]-> SUPER::linePattern unless $#_;
	$_[0]-> SUPER::linePattern($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {linePattern} = 1;
}

sub lineWidth
{
	return $_[0]-> SUPER::lineWidth unless $#_;
	$_[0]-> SUPER::lineWidth($_[1]);
	return unless $_[0]-> {can_draw};
	$_[0]-> {changed}-> {lineWidth} = 1;
}

sub miterLimit
{
	return $_[0]-> SUPER::miterLimit unless $#_;
	my ( $self, $ml ) = @_;
	$ml = 1.0 if $ml < 0;
	$self-> SUPER::miterLimit($ml);
	return unless $self-> {can_draw};
	$self-> {changed}-> {miterLimit} = 1;
}

sub rop
{
	return $_[0]-> SUPER::rop unless $#_;
	my ( $self, $rop) = @_;
	$rop = rop::CopyPut if
		$rop != rop::Blackness || $rop != rop::Whiteness || $rop != rop::NoOper;
	$self-> SUPER::rop( $rop);
}

sub rop2
{
	return $_[0]-> SUPER::rop2 unless $#_;
	my ( $self, $rop) = @_;
	$rop = rop::CopyPut if
		$rop != rop::Blackness && $rop != rop::Whiteness && $rop != rop::NoOper;
	$self-> SUPER::rop2( $rop);
}

sub translate
{
	return $_[0]-> SUPER::translate unless $#_;
	my $self = shift;
	$self-> SUPER::translate(@_);
	$self-> change_transform;
}

sub clipRect
{
	return @{$_[0]-> {clipRect}} unless $#_;
	$_[0]-> {clipRect} = [@_[1..4]];
	$_[0]-> {region} = undef;
	$_[0]-> change_transform;
}

sub region
{
	return undef;
}

sub scale
{
	return @{$_[0]-> {scale}} unless $#_;
	my $self = shift;
	$self-> {scale} = [@_[0,1]];
	$self-> change_transform;
}

sub reversed
{
	return $_[0]-> {reversed} unless $#_;
	my $self = $_[0];
	$self-> {reversed} = $_[1] unless $self-> get_paint_state;
	$self-> calc_page;
}

sub rotate
{
	return $_[0]-> {rotate} unless $#_;
	my $self = $_[0];
	$self-> {rotate} = $_[1];
	$self-> change_transform;
}

sub resolution
{
	return @{$_[0]-> {resolution}} unless $#_;
	return if $_[0]-> get_paint_state;
	my ( $x, $y) =  @_[1..2];
	return if $x <= 0 || $y <= 0;
	$_[0]-> {resolution} = [$x, $y];
	$_[0]-> calc_page;
}

sub grayscale
{
	return $_[0]-> {grayscale} unless $#_;
	$_[0]-> {grayscale} = $_[1] unless $_[0]-> get_paint_state;
}

sub calc_page
{
	my $self = $_[0];
	my @s =  @{$self-> {pageSize}};
	my @m =  @{$self-> {pageMargins}};
	if ( $self-> {reversed}) {
		@s = @s[1,0];
		@m = @m[1,0,3,2];
	}
	$self-> {size} = [
		int(( $s[0] - $m[0] - $m[2]) * $self-> {resolution}-> [0] / 72.27 + 0.5),
		int(( $s[1] - $m[1] - $m[3]) * $self-> {resolution}-> [1] / 72.27 + 0.5),
	];
}

sub pageSize
{
	return @{$_[0]-> {pageSize}} unless $#_;
	my ( $self, $px, $py) = @_;
	return if $self-> get_paint_state;
	$px = 1 if $px < 1;
	$py = 1 if $py < 1;
	$self-> {pageSize} = [$px, $py];
	$self-> calc_page;
}

sub pageMargins
{
	return @{$_[0]-> {pageMargins}} unless $#_;
	my ( $self, $px, $py, $px2, $py2) = @_;
	return if $self-> get_paint_state;
	$px = 0 if $px < 0;
	$py = 0 if $py < 0;
	$px2 = 0 if $px2 < 0;
	$py2 = 0 if $py2 < 0;
	$self-> {pageMargins} = [$px, $py, $px2, $py2];
	$self-> calc_page;
}

sub size
{
	return @{$_[0]-> {size}} unless $#_;
	$_[0]-> raise_ro("size");
}

sub flood_fill           { 0 }
sub get_bpp              { return $_[0]-> {grayscale} ? 8 : 24 }
sub get_nearest_color    { return $_[1] }
sub get_physical_palette { return $_[0]-> {grayscale} ? [map { $_, $_, $_ } 0..255] : 0 }
sub get_handle           { return 0 }
sub alpha                { 0 }
sub can_draw_alpha       { 0 }

sub fonts
{
	my ( $self, $family, $encoding) = @_;
	$family   = undef if defined $family   && !length $family;
	$encoding = undef if defined $encoding && !length $encoding;

	my $enc = 'iso10646-1'; # unicode only
	if ( !defined $family ) {
		my @fonts;
		my $num = $self->fontMapperPalette(-1);
		if ( $num > 0 ) {
			for my $fid ( 1 .. $num ) {
				my $f = $self->fontMapperPalette($fid) or next;
				$f->{encodings} = [$enc];
				$f->{encoding} = $enc;
				push @fonts, $f;
			}
		}
		return \@fonts;
	} else {
		return [] if defined($encoding) && $encoding ne '' && $encoding ne $enc;

		my @f = @{$::application->fonts($family) // []};
		return [] unless @f;
		$f[0]->{encoding} = $enc;
		return [$f[0]];
	}
}

sub glyph_canvas
{
	my $self = shift;
	return $self->{glyph_canvas} //= Prima::DeviceBitmap->create(
		width           => 1,
		height          => 1,
		textOutBaseline => 1,
	);
}

sub glyph_canvas_set_font
{
	my ($self, %font) = @_;

	my $g = $self-> glyph_canvas;
	$font{style} &= ~(fs::Underlined|fs::StruckOut);
	delete @font{qw(height width direction)};
	$font{size} = 1000;
	$g-> font(\%font);
}

sub get_font {+{%{$_[0]-> {font}}}}

sub set_font
{
	my ( $self, $font) = @_;

	my $canvas = $self-> glyph_canvas;
	my ($curr_font, $new_font) = ('', '');
	$curr_font = ($self->{font}->{size} // '-1'). '.' . ($self->{glyph_font} // '');

	$font = { %$font };
	my $wscale     = $font-> {width};
	delete $font-> {width};

	my $div        = 72.27 / $self-> {resolution}-> [1];
	my $by_height  = defined($font->{height});
	$font = Prima::Drawable-> font_match( $font, $self-> {font});
	delete $font->{$by_height ? 'size' : 'height'};
	$canvas->set_font( $font );
	$font = $self-> {font} = { %{ $canvas->get_font } };

	# convert Prima size definition to PS size definition
	#
	# PS doesn't account for internal leading, and thus there are two possibilities:
	# 1) enforce Prima model, but that results in $font->size(100) printed
	# will not exactly be 100 points by mm.
	#
	# 2) hack font structure on the fly, so that caller setting $font->size(100) 
	# will get $font->height slightly less (by internal leading) in pixels.
	#
	# Here #2 is implemented
	if ( $by_height ) {
		$font->{size} = int($font->{height} * $div + .5);
	} else {
		my $new_h        = $font->{size} / $div;
		my $ratio        = $font->{height} / $new_h;
		$font->{height}  = int( $new_h + .5);
		$font->{ascent}  = int( $font->{ascent} / $ratio + .5 );
		$font->{descent} = $font->{height} - $font->{ascent};
	}

	# we emulate wider fonts by PS scaling, but this factor is needed
	# when reporting horizontal glyph and text extension
	my $font_width_divisor  = $font->{width};
	$font-> {width} = $wscale if $wscale;
	$self-> {font_x_scale}  = $font->{width} / $font_width_divisor;

	$self-> glyph_canvas_set_font(%$font);
	my $f1000 = $self->glyph_canvas->font;
	$self-> apply_canvas_font( $f1000 );

	# When querying glyph extensions, remember to scale to the
	# difference between PS and Prima models. 
	my $y_scale = 1.0 + $f1000->internalLeading / $f1000->height;
	# Also, note that querying is on the canvas that has size=1000.
	$self->{font_scale} = $font->{height} / $f1000->height * $y_scale;

	$new_font = $font->{size} . '.' . $self->{glyph_font};
	$self-> {changed}->{font} = 1 if $curr_font ne $new_font;
}

sub get_font_abc
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my $canvas = $self-> glyph_canvas;
	my $scale  = $self->{font_scale} * $self->{font_x_scale};
	return [ map { $_ * $scale } @{ $canvas->get_font_abc($first, $last, $flags) } ];
}

sub get_font_def
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my $canvas = $self-> glyph_canvas;
	my $scale  = $self->{font_scale};
	return [ map { $_ * $scale } @{ $canvas->get_font_def($first, $last, $flags) } ];
}

sub get_font_ranges    { shift->glyph_canvas->get_font_ranges    }
sub get_font_languages { shift->glyph_canvas->get_font_languages }

sub get_text_width
{
	my ( $self, $text, $flags, $from, $len) = @_;
	$flags //= 0;
	$from  //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_width($self, $flags, $from, $len);
	} else {
		$len = length($text) if !defined($len) || $len < 0 || $len > length($text);
	}
	return 0 unless $len;

	my $w = $self->glyph_canvas-> get_text_width( $text, $flags, $from, $len);
	$w *= $self->{font_scale} unless $glyphs && $text->advances;
	return int( $w * $self-> {font_x_scale} + .5);
}

sub _rotate
{
	my ( $angle, $arr ) = @_;
	my $s = sin( $angle / 57.29577951);
	my $c = cos( $angle / 57.29577951);
	my $i;
	for ( $i = 0; $i < 10; $i+=2) {
		my ( $x, $y) = @$arr[$i,$i+1];
		$$arr[$i]   = $x * $c - $y * $s;
		$$arr[$i+1] = $x * $s + $y * $c;
	}
}

sub get_text_box
{
	my ( $self, $text, $from, $len) = @_;

	$from //= 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs if !defined($len) || $len < 0 || $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_box($self, $from, $len);
	} else {
		$len  = length($text) if !defined($len) || $len < 0 || $len > length($text);
	}
	return [ (0) x 10 ] unless $len;

	my $wmul = $self->{font_x_scale};
	my $dir  = $self->{font}->{direction};
	my @ret;

	@ret = @{ $self-> glyph_canvas-> get_text_box( $text, $from, $len) };
	my $div = $self->{font_scale};
	if ($glyphs && $text->advances) {
		$_ *= $div for @ret[1,3,5,7,9];
	} else {
		$_ *= $div for @ret;
	}

	if ( $wmul != 0.0 && $wmul != 1.0 ) {
		_rotate(-$dir, \@ret) if $dir != 0;
		$ret[$_] *= $wmul for 0,2,4,6,8;
		_rotate($dir, \@ret) if $dir != 0;
	}

	return \@ret;
}

sub text_shape
{
	my ( $self, $text, %opt ) = @_;

	my $canvas = $self-> glyph_canvas;
	my $shaped = $canvas->text_shape($text, %opt);
	return $shaped unless $shaped;
	$shaped->[Prima::Drawable::Glyphs::CUSTOM()] = $text;
	if ( $shaped-> advances ) {
		my $scale  = $self->{font_scale};
		$_ *= $scale for @{ $shaped->advances  };
		$_ *= $scale for @{ $shaped->positions };
	}
	return $shaped;
}

sub render_glyph {}

package
	Prima::PS::Drawable::Path;
use base qw(Prima::Drawable::Path);

sub entries
{
	my $self = shift;
	unless ( $self->{entries} ) {
		local $self->{stack} = [];
		local $self->{curr}  = { matrix => [ $self-> identity ] };
		my $c = $self->{commands};
		$self-> {entries} = [];
		for ( my $i = 0; $i < @$c; ) {
			my ($cmd,$len) = @$c[$i,$i+1];
			$self-> can("_$cmd")-> ( $self, @$c[$i+2..$i+$len+1] );
			$i += $len + 2;
		}
		$self->{last_matrix} = $self->{curr}->{matrix};
	}
	return $self-> {entries};
}

sub emit { push @{shift->{entries}}, join(' ', @_) }

sub last_point { @{$_[0]->{last_point} // [0,0]} }

sub _open
{
	my $self = shift;
	$self-> {move_is_line} = 0;
	$self->emit('')
}

sub _close     { $_[0]->emit( $_[0]-> dict-> {closepath} ) }

sub  _moveto
{
	my ( $self, $mx, $my, $rel) = @_;
	($mx, $my) = $self-> canvas-> pixel2point( $mx, $my );
	($mx, $my) = $self->matrix_apply($mx, $my);
	my ($lx, $ly) = $rel ? $self->last_point : (0,0);
	$lx += $mx;
	$ly += $my;
	@{$self-> {last_point}} = ($lx, $ly);
	$self-> emit($lx, $ly, $self->dict->{moveto} );
}

sub _line
{
	my ( $self, $line ) = @_;
	my @line = $self-> canvas-> pixel2point( @$line );
	@line = @{ $self-> matrix_apply( \@line ) };
	$self-> set_current_point( shift @line, shift @line );
	@{$self-> {last_point}} = @line[-2,-1];
	my $cmd = $self->dict->{lineto};
	for ( my $i = 0; $i < @line; $i += 2 ) {
		$self->emit(@line[$i,$i+1], $cmd);
	}
}

sub _spline
{
	my ( $self, $points, $options ) = @_;
	my @p = $self-> canvas-> pixel2point( @$points );
	@p = @{ $self-> matrix_apply( \@p ) };

	$options->{degree} //= 2;
	return if $options->{degree} > 3;
	my @p0 = @p[0,1];
	$self-> set_current_point( @p0 );
	my $cmd = $self->dict->{curveto};
	if ( $options->{degree} == 2 ) {
		for ( my $i = 2; $i < @p; $i += 4 ) {
			my @pp = $self->canvas->conic2curve( @p0, @p[$i .. $i + 3] );
			$self->emit(@pp, $cmd);
			@p0 = @pp[-2,-1];
		}
	} else {
		for ( my $i = 2; $i < @p; $i += 4 ) {
			my @pp = @p[$i .. $i + 5];
			$self->emit(@pp, $cmd);
		}
	}
}

sub _arc
{
	my ( $self, $from, $to, $rel ) = @_;
	my $cubics = $self->canvas->arc2cubics( 0, 0, 1, 1, $from, $to);

	if ( $rel ) {
		my ($lx,$ly) = $self->last_point;
		my $pts = $cubics->[0];
		my $m = $self->{curr}->{matrix};
		my @s = $self->matrix_apply( $pts->[0], $pts->[1]);
		$m->[4] += $lx - $s[0];
		$m->[5] += $ly - $s[1];
	}
	my @p = map { $self-> matrix_apply( $_ ) } @$cubics;
	$self-> set_current_point( @{$p[0]}[0,1] );
	my $cmd = $self->dict->{curveto};
	$self-> emit( @{$_}[2..7], $cmd) for @p;
}

sub stroke
{
	my $self = shift;
	$self-> canvas-> stroke( join("\n", @{ $self->entries }, $self->dict->{stroke} ));
}

sub fill
{
	my ( $self, $fillMode ) = @_;
	$fillMode //= $self->canvas->fillMode;
	$fillMode = ((($fillMode & fm::Winding) == fm::Alternate) ? 'alt' : 'wind');
	$self-> canvas-> fill( join("\n", @{ $self->entries }, $self-> dict->{"fill_$fillMode"} ));
}

package
	Prima::PS::Drawable::Region;

sub new
{
	my ($class, $entries) = @_;
	bless {
		path   => $entries,
		offset => [0,0],
	}, $class;
}

sub get_handle { "$_[0]" }
sub get_boxes    { [] }
sub point_inside { 0 }
sub rect_inside  { 0 }
sub box          { 0,0,0,0 }

sub offset
{
	my ( $self, $dx, $dy ) = @_;
	$self->{offset}->[0] += $dx;
	$self->{offset}->[1] += $dy;
}

sub apply_offset
{
	my $self = shift;
	my $path = $self->{path};
	my @offset = @{ $self->{offset} };
	return $path if 0 == grep { $_ != 0 } @offset;

	my $n = '';
	my $ix = 0;
	while ( 1 ) {
		$path =~ m/\G(\d+(?:\.\d+)?)/gcs and do {
			$n .= $1 + $offset[$ix];
			$ix = $ix ? 0 : 1;
			redo;
		};
		$path =~ m/\G(\s+)/gcs and do {
			$n .= $1;
			redo;
		};
		$path =~ m/\G(\D+)/gcs and do {
			$n .= $1;
			$ix = 0;
			redo;
		};
		$path =~ m/\G$/gcs and last;
	}
	$path = $n;
}

1;

__END__

=pod

=head1 NAME

Prima::PS::Drawable - Common routines for PS drawables

=cut
