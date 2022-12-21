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
	$self-> {rotate}      = 0;
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

sub is_custom_line
{
	my ($self, $for_closed_shapes) = shift;
	return $self->{lineEnd_flags} ? (
		$for_closed_shapes ? ( $self->{lineEnd_flags} & 2 ) : 1
	) : 0;
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

sub lineTail  { shift->line_properties( lineTail  => @_ ); }
sub lineHead  { shift->line_properties( lineHead  => @_ ); }
sub arrowTail { shift->line_properties( arrowTail => @_ ); }
sub arrowHead { shift->line_properties( arrewHead => @_ ); }

sub update_custom_line
{
	my ( $self, $le ) = @_;
	$le //= $self->SUPER::lineEnd;
	my $lp = (length($self->linePattern) > 1) ? 1 : 0;
	$self->{lineEnd_flags} = ref($le) ? 1 : $lp;
	if ( $self->{lineEnd_flags} ) {
		$self->{lineEnd_flags} |= 2 if $lp || defined($le->[2]) || defined($le->[3]);
	} else {
		$self-> {changed}-> {lineEnd} = 1;
	}
}

sub line_properties
{
	my ( $self, $lp, @cmd ) = @_;
	$lp = "SUPER::" . $lp;
	return $self-> $lp unless @cmd;
	$self->$lp(@cmd);
	return unless $self-> {can_draw};
	$self-> update_custom_line;
}

sub lineEnd
{
	return $_[0]-> SUPER::lineEnd unless $#_;
	my ( $self, $le ) = @_;
	$self-> SUPER::lineEnd($le);
	return unless $self-> {can_draw};
	$self-> update_custom_line( $le );
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
	$_[0]-> update_custom_line;
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

sub clipRect
{
	return @{$_[0]-> {clipRect}} unless $#_;
	$_[0]-> {clipRect} = [@_[1..4]];
	$_[0]-> {region} = undef;
	$_[0]-> change_transform;
}

sub matrix
{
	return $_[0]->SUPER::matrix unless $#_;
	my ( $self, @m ) = @_;
	$self->SUPER::matrix(@m);
	$self->change_transform;
}

sub change_transform  { Carp::croak "abstract call" }
sub apply_canvas_font { Carp::croak "abstract call" }
sub begin_doc         { Carp::croak "abstract call" }
sub end_doc           { Carp::croak "abstract call" }

sub region
{
	return undef;
}

sub scale
{
	my ( $self, @scale ) = @_;
	my $m = $self-> matrix;
	$m->scale(@scale);
	$self->matrix($m);
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
	my ( $self, $angle ) = @_;
	return if $angle == 0.0;
	my $m = $self-> matrix;
	$m->rotate($angle);
	$self->matrix($m);
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

sub graphic_context_push
{
	my $self = shift;
	return 0 unless $self->SUPER::graphic_context_push;
	my $stack = $self->{gc_stack} //= [];
	push @$stack, {
		(map { $_,  $self->$_()  } qw(reversed grayscale)),
		(map { $_, [$self->$_()] } qw(resolution clipRect))
	};
	return 1;
}

sub graphic_context_pop
{
	my $self = shift;
	return unless $self->SUPER::graphic_context_pop;
	my $stack = $self->{gc_stack} //= [];
	my $item = pop @$stack or return 0;
	@{$self->{$_}} = @{$item->{$_}} for qw(resolution clipRect);
	$self->{$_} = $item->{$_} for qw(reversed grayscale);
	$self->change_transform;
	return 1;
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
sub bar_alpha            { 0 }
sub can_draw_alpha       { 0 }

sub fonts
{
	my ( $self, $family, $encoding) = @_;
	$family   = undef if defined $family   && !length $family;
	$encoding = undef if defined $encoding && !length $encoding;

	my $enc = 'iso10646-1'; # unicode only
	if ( !defined $family ) {
		my @fonts;
		my $fm = $self-> font_mapper;
		my $num = $fm->count;
		if ( $num > 0 ) {
			for my $fid ( 1 .. $num ) {
				my $f = $fm->get($fid) or next;
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
	my $explicit_width = delete $font-> {width};

	my $div        = 72.27 / $self-> {resolution}-> [1];
	my $by_height  = defined($font->{height});
	$font = Prima::Drawable-> font_match( $font, $self-> {font});

	$self-> glyph_canvas_set_font(%$font);
	my $f1000 = { %{ $self->glyph_canvas->get_font }};
	$self-> apply_canvas_font( $f1000 );

	# convert Prima size definition to PS size definition
	#
	# PS doesn't account for internal leading, and thus there are two possibilities:
	# 1) enforce Prima model, but that results in $font->size(100) printed
	# will not exactly be 100 points by mm.
	#
	# 2) hack font structure on the fly, so that caller setting $font->size(100) 
	# will get $font->height padded slightly by internal leading.
	#
	# Here #2 is implemented
	my $ratio;
	my $ps_fix = $f1000->{size} / ( $f1000->{height} - $f1000->{internalLeading} );
	if ( $by_height ) {
		$self->{font_scale} = $font->{height} / $f1000->{height};
		$ratio              = $self->{font_scale} * $div / $ps_fix;
	} else {
		$ratio              = $font->{size} / $f1000->{size};
		$self->{font_scale} = $ratio / $div * $ps_fix;
	}
	%$font = ( %$f1000, direction => $font->{direction} );
	# When querying glyph extensions, remember to scale to the
	# difference between PS and Prima models, ie without and with the internal leading
	$font->{$_}   = int( $f1000->{$_} * $self->{font_scale} + .5)
		for qw(ascent height internalLeading externalLeading width);
	$font->{size} = int( $f1000->{size} * $ratio + .5);
	$font->{descent}    = $font->{height} - $font->{ascent};

	# we emulate wider fonts by PS scaling, but this factor is needed
	# when reporting horizontal glyph and text extension
	if ( $explicit_width ) {
		$self-> {font_x_scale}  = $explicit_width / $font->{width};
		$font-> {width}         = $explicit_width;
	} else {
		$self-> {font_x_scale}  = 1;
	}
	$self->{font} = $font;

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
	return [ map { $_ * $scale } @{ $canvas->get_font_abc($first, $last, $flags // 0) } ];
}

sub get_font_def
{
	my ( $self, $first, $last, $flags) = @_;
	$first = 0     if !defined ($first) || $first < 0;
	$last = $first if !defined ($last) || $last < $first;
	my $canvas = $self-> glyph_canvas;
	my $scale  = $self->{font_scale};
	return [ map { $_ * $scale } @{ $canvas->get_font_def($first, $last, $flags // 0) } ];
}

sub get_font_ranges    { shift->glyph_canvas->get_font_ranges    }
sub get_font_languages { shift->glyph_canvas->get_font_languages }

sub get_text_width
{
	my ( $self, $text, $flags, $from, $len) = @_;
	$flags //= 0;
	$from  //= 0;
	return 0 if $from < 0;
	my $glyphs;
	if ( ref($text) eq 'Prima::Drawable::Glyphs') {
		$glyphs = $text->glyphs;
		$len    = @$glyphs - $from if !defined($len) || $from + $len > @$glyphs;
	} elsif (ref($text)) {
		$len //= -1;
		return $text->get_text_width($self, $flags, $from, $len);
	} else {
		$len = length($text) - $from if !defined($len) || $from + $len > length($text);
	}
	return 0 if $len <= 0;

	my $w;
	if ( $glyphs && $text->advances ) {
		if ( $flags & to::AddOverhangs) {
			$w = $self->glyph_canvas-> get_text_width( $text, $flags & ~to::AddOverhangs, $from, $len);
			my ($g1,$g2) = @$glyphs[$from, $from + $len - 1];
			my $abc = $self->get_font_abc($g1,$g1,to::Glyphs) or goto NO_ABC;
			$w += ($abc->[0] < 0) ? -$abc->[0] : 0;
			if ( $g1 != $g2 ) {
				$abc = $self->get_font_abc($g2,$g2,to::Glyphs) or goto NO_ABC;
				$w += ($abc->[2] < 0) ? -$abc->[2] : 0;
			}
		NO_ABC:
		} else {
			$w = $self->glyph_canvas-> get_text_width( $text, $flags, $from, $len);
		}
	} else {
		$w = $self->glyph_canvas-> get_text_width( $text, $flags, $from, $len);
		$w *= $self->{font_scale};
	}
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

sub text_wrap
{
	my ( $self, $text, $width, @rest ) = @_;
	my $res;
	my $gc = $self->glyph_canvas;
	my $x  = $self->{font_scale};
	if ( $rest[-1] && ((ref($rest[-1]) // '') eq 'Prima::Drawable::Glyphs') && $rest[-1]->advances ) {
		my $s = $rest[-1];
		my @save  = ($s->advances, $s->positions);
		my @clone = map { Prima::array::clone($_) } @save;
		for my $v ( @clone ) {
			$_ /= $x for @$v;
		}
		$s->[ Prima::Drawable::Glyphs::ADVANCES()  ] = $clone[0];
		$s->[ Prima::Drawable::Glyphs::POSITIONS() ] = $clone[1];
		$res = $gc->text_wrap($text, $width / $x, @rest);
		$s->[ Prima::Drawable::Glyphs::ADVANCES()  ] = $save[0];
		$s->[ Prima::Drawable::Glyphs::POSITIONS() ] = $save[1];
	} else {
		$res = $gc->text_wrap($text, $width / $x, @rest);
	}
	return $res;
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

# primitive emulation

sub primitive
{
	my ( $self, $cmd, @param ) = @_;

	my $dst  = $self->new_path;

	my $src = Prima::Drawable::Path->new( undef, subpixel => 1 );
	$src->$cmd(@param);

	my @pp = map { @$_ } @{$src->points};
	for my $p ( @pp ) {
		next unless @$p;
		my $cmds = $self->render_polyline( $p, path => 1, integer => 0);
		for ( my $i = 0; $i < @$cmds;) {
			my $cmd   = $cmds->[$i++];
			my $param = $cmds->[$i++];
			if ( $cmd eq 'line') {
				$dst->line( $param );
			} elsif ( $cmd eq 'arc') {
				$dst->$cmd( @$param );
			} elsif ( $cmd eq 'conic') {
				$dst->spline( $param, degree => 2 );
			} elsif ( $cmd eq 'cubic') {
				$dst->spline( $param, degree => 3 );
			} elsif ( $cmd eq 'open') {
				$dst->close;
				$dst->open;
			} else {
				warn "** panic: unknown render_polyline command '$cmd'";
				last;
			}
		}
		$dst->close;
		$dst->open;
	}

	if ( $self->graphic_context_push ) {
		if ( $self-> rop2 == rop::CopyPut ) {
			my $color = $self->color;
			$self->color($self->backColor);
			$self->lineEnd(le::Flat);
			$self->linePattern(lp::Solid);
			$self->$cmd(@param);
			$self->color($color);
		}
		$self->fillPattern(fp::Solid);
		$dst->fill;
		$self->graphic_context_pop;
	}
}

sub prepare_image
{
	my ( $self, $image, $xFrom, $yFrom, $xLen, $yLen) = @_;

	my @is = $image-> size;
	$_ //= 0 for $xFrom, $yFrom;
	$xLen //= $is[0];
	$yLen //= $is[1];

	my $touch;
	$touch = 1, $image = $image-> image if $image-> isa('Prima::DeviceBitmap');

	unless ( $xFrom == 0 && $yFrom == 0 && $xLen == $image-> width && $yLen == $image-> height) {
		$image = $image-> extract( $xFrom, $yFrom, $xLen, $yLen);
		$touch = 1;
	}

	my $ib = $image-> get_bpp;
	if ( $ib != $self-> get_bpp) {
		$image = $image-> dup unless $touch;
		if ( $self-> {grayscale} || $image-> type & im::GrayScale) {
			$image-> type( im::Byte);
		} else {
			$image-> type( im::RGB);
		}
		$touch = 1;
	} elsif ( $self-> {grayscale} || $image-> type & im::GrayScale) {
		$image = $image-> dup unless $touch;
		$image-> type( im::Byte);
		$touch = 1;
	}

	$ib = $image-> get_bpp;
	if ($ib != 8 && $ib != 24) {
		$image = $image-> dup unless $touch;
		$image-> type( im::RGB);
		$touch = 1;
	}

	if ( $image-> type == im::RGB ) {
		# invert BGR -> RGB
		$image = $image-> dup unless $touch;
		$image-> set(data => $image->data, type => im::fmtBGR | im::RGB);
		$touch = 1;
	}

	if ( $image-> isa('Prima::Icon')) {
		if ( $image-> maskType != 1 && $image-> maskType != 8) {
			$image = $image-> dup unless $touch;
			$image-> set(maskType => 1);
			$touch = 1;
		}
	}

	return $image;
}

package
	Prima::PS::Drawable::Path;
use base qw(Prima::Drawable::Path);

sub reset
{
	$_[0]->SUPER::reset();
	delete $_[0]->{entries};
	delete $_[0]->{last_matrix};
	delete $_[0]->{last_point};
}

sub entries
{
	my $self = shift;
	unless ( $self->{entries} ) {
		local $self->{stack} = [];
		local $self->{curr}  = { matrix => [ $self-> identity ] };
		my $c = $self->{commands};
		$self-> {entries} = [];
		$self-> emit( $self->dict->{newpath});
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
	my $cubics = Prima::Drawable::Path->arc2cubics(
		0, 0, 2, 2,
		$from, $to);

	if ( $rel ) {
		my ($lx,$ly) = $self->last_point;
		my $pts = $cubics->[0];
		my $m = $self->{curr}->{matrix};
		my @s = $self->matrix_apply( $pts->[0], $pts->[1]);
		$m->[4] += $lx - $s[0];
		$m->[5] += $ly - $s[1];
	}
	my @p = map { $self-> matrix_apply( $_ ) } @$cubics;
	$_ = [$self-> canvas-> pixel2point(@$_)] for @p;
	$self-> set_current_point( @{$p[0]}[0,1] );
	my $cmd = $self->dict->{curveto};
	$self-> emit( @{$_}[2..7], $cmd) for @p;
}

sub stroke
{
	my $self = shift;
	my $c = $self->canvas;
	return $self-> canvas-> stroke( join("\n", @{ $self->entries }, $self->dict->{stroke} ))
		if $c-> is_custom_line;

	my $path = Prima::Drawable::Path-> new( undef,
		subpixel => 1,
		commands => $self->commands,
	);
	for ( map { @$_ } @{ $path->points }) {
		next if 4 > @$_;
		$c->primitive( polyline => $_ );
	}
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
