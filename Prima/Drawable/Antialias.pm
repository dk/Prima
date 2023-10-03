package Prima::Drawable::Antialias;

use strict;
use warnings;
use Prima;

sub new
{
	my ($class, $canvas, %opt) = @_;

	my %self = (
		%opt,
		canvas => $canvas,
		can_aa => $canvas->can_draw_alpha,
		alpha  => $opt{alpha} // $canvas->alpha,
		factor => $opt{factor} // 2,
	);
	return bless \%self, $class;
}

sub can_aa { $#_ ? $_[0]->{can_aa} = $_[1] : $_[0]->{can_aa} }

sub calc_poly_extents
{
	my ( $self, $poly ) = @_;
	return if 4 > @$poly;
	my @cr = $self->{canvas}->clipRect;
	return if 4 == grep { $_ == 0 } @cr;

	my @rc = @{ Prima::Drawable-> render_polyline( $poly, box => 1, integer => 1 ) || return };
	my @tr = $self->{canvas}->translate;
	$rc[$_] += $tr[$_] for 0,1;
	$rc[2] += $rc[0] - 1;
	$rc[3] += $rc[1] - 1;

	return if $rc[0] > $cr[2];
	$rc[0] = $cr[0] if $rc[0] < $cr[0];
	return if $rc[1] > $cr[3];
	$rc[1] = $cr[1] if $rc[1] < $cr[1];
	return if $rc[2] < $cr[0];
	$rc[2] = $cr[2] if $rc[2] > $cr[2];
	return if $rc[3] < $cr[1];
	$rc[3] = $cr[3] if $rc[3] > $cr[3];

	$rc[$_+2] -= $rc[$_] - 1 for 0,1;
	$rc[$_] -= $tr[$_]       for 0,1;
	return @rc;
}

sub alloc_surface
{
	my ( $self, @sz ) = @_;
	my $surface = Prima::Image->new(
		type      => im::Byte,
		size      => [ map { $_ * $self->{factor} } @sz ],
		color     => cl::Black,
		scaling   => ist::Triangle,
	) or return;
	$surface->bar(0,0,$surface->size);
	$surface->color($self->{alpha} * 0x10101);
	return $surface;
}

sub apply_surface
{
	my ( $self, $x, $y, $alpha, $colors ) = @_;

	return 0 unless $self->{can_aa};

	$alpha-> size( map { $_ / $self->{factor} } $alpha-> size );

	my $canvas = $self->{canvas};
	if ( $colors ) {
		my $icon = Prima::Icon-> create_combined( $colors, $alpha );
		$icon->premultiply_alpha;
		return $canvas-> put_image( $x, $y, $icon, rop::Blend );
	} elsif ( $canvas->isa('Prima::Image') && ! $canvas->get_paint_state ) {
		return $canvas-> put_image( $x, $y, $alpha, rop::SrcOver | rop::ConstantColor );
	} else {
		return $canvas-> put_image( $x, $y, $alpha->to_colormask( $canvas-> color) , rop::Blend );
	}
}

sub polyline
{
	my ( $self, $poly ) = @_;
	goto FALLBACK unless $self->{can_aa};
	my ($x, $y, $w, $h) = $self->calc_poly_extents($poly);
	return 0 unless defined $x;

	my $canvas = $self->{canvas};
	my $lp = $canvas->linePattern;
	my $r2 = $canvas->rop2;
	my $lw = $canvas->lineWidth;
	my $solid_line;
	if ($lp eq lp::Null) {
		return 1 if $r2 == rop::NoOper;
		return 0 unless $canvas-> graphic_context_push;
		$canvas->color($canvas->backColor);
		$canvas->linePattern(lp::Solid);
		my $ok = $self->polyline($poly);
		$canvas->graphic_context_pop;
		return $ok;
	} elsif ( $lp ne lp::Solid && $r2 == rop::CopyPut ) {
		$solid_line = 1;
	}


	my $bitmap = $self->alloc_surface($w, $h) or goto FALLBACK;

	if ($lw > 1 ) {
		my $line =
			$bitmap->new_path->
				scale($self->{factor})->
				translate(-$x, -$y)->
				line($poly)->
				widen(
					lineWidth   => $self->{factor} * ($canvas-> lineWidth - 1),
					map { $_ => $canvas->$_() } qw(
						linePattern lineJoin lineEnd miterLimit
					)
				)->
				region(fm::Winding);

		if ( $solid_line ) {
			my $solid = $bitmap->new_path->
				scale($self->{factor})->
				translate(-$x, -$y)->
				line($poly)->
				widen(
					lineWidth   => $self->{factor} * ($canvas-> lineWidth - 1),
					map { $_ => $canvas->$_() } qw(
						lineJoin lineEnd miterLimit
					)
				)->
				region(fm::Winding);

			my $base = $self->alloc_surface( $w, $h ) or goto FALLBACK;
			$solid->combine($line, rgnop::Xor);
			$base->region($solid);
			my $c = $canvas->color;
			$canvas->color($canvas->backColor);
			$base->bar( 0, 0, $base->size);
			my $ok = $self->apply_surface($x, $y, $base);
			$canvas->color($c);
			return 0 unless $ok;
		}

		$bitmap->region($line);
		$bitmap->bar( 0, 0, $bitmap->size);
	} else {
		$bitmap->translate(map { -1 * $self->{factor} * $_ } $x, $y);
		$poly = Prima::Drawable->render_polyline( $poly,
			matrix  => [($self->{factor},0,0) x 2],
			integer => 1
		);

		if ( $solid_line ) {
			my $base = $self->alloc_surface( $w, $h ) or goto FALLBACK;
			my $lp = $canvas->linePattern;
			$lp =~ s/(.)(.)/$2$1/gsm;
			$base->linePattern($lp);
			$base->polyline($poly);
			my $c = $canvas->color;
			$canvas->color($canvas->backColor);
			$base->polyline($poly);
			my $ok = $self->apply_surface($x, $y, $base);
			$canvas->color($c);
			return 0 unless $ok;
		}
		$bitmap->linePattern( $canvas->linePattern );
		$bitmap->polyline($poly);
	}
	return $self->apply_surface($x, $y, $bitmap);

FALLBACK:
	return $self->{canvas}->polyline($poly);
}

sub fillpoly
{
	my ( $self, $poly, $mode ) = @_;
	goto FALLBACK unless $self->{can_aa};
	my ($x, $y, $w, $h) = $self->calc_poly_extents($poly);
	return 0 unless defined $x;

	my $canvas = $self->{canvas};
	$mode //= $self->{canvas}->fillMode;
	$poly = Prima::Drawable->render_polyline( $poly,
		matrix => [$self->{factor},0,0,$self->{factor},0,0],
		integer => 1
	);
	my $rgn = Prima::Region->new(
		fillMode => $mode,
		polygon  => $poly,
	);
	$rgn->offset( map { -1 * $self->{factor} * $_ } $x, $y );

	my $bitmap = $self->alloc_surface($w, $h) or goto FALLBACK;
	$bitmap->region( $rgn );

	my $colors;
	my $restore_color;
	my $fp = $canvas->fillPattern;
	if ( ref($fp) ne 'ARRAY') {
		goto STIPPLE if $fp->type == im::BW && !$fp->isa('Prima::Icon');

	IMAGE:
		my @fp = $canvas->fillPatternOffset;
		$fp[0] -= $x;
		$fp[1] -= $y;
		$colors = Prima::Image->new(
			type              => ($canvas->isa('Prima::Image') ? $canvas->type : im::RGB),
			size              => [ $w, $h ],
			fillPattern       => $fp,
			fillPatternOffset => \@fp,
			color             => $canvas-> color,
			backColor         => $canvas-> backColor,
		);

		$colors->bar( 0, 0, $colors->size );
	} elsif ( fp::is_empty($fp)) {
		return 1 if $canvas->rop2 == rop::NoOper;
		$restore_color = $canvas->color;
		$canvas->color($canvas->backColor);
	} elsif ( !fp::is_solid($fp)) {
	STIPPLE:
		$fp = Prima::Image->new(
			type     => im::BW,
			size     => [8,8],
			data     => join('000', map { chr } @$fp),
		) if ref($fp) eq 'ARRAY';

		if ($canvas->rop2 == rop::CopyPut) {
			if ($fp->type == im::BW) {
				$fp->type(im::bpp1);
				$fp->colormap( $canvas->backColor, $canvas->color );
			}
			goto IMAGE;
		}

		$fp = $fp->clone( size => [ map { $_ * $self->{factor} } $fp-> size ] );
		$bitmap->fillPattern($fp);

		my @fp = $canvas->fillPatternOffset;
		$fp[0] -= $x * $self->{factor};
		$fp[1] -= $y * $self->{factor};
		$bitmap->fillPatternOffset(@fp);
	}

	$bitmap->bar( 0, 0, $bitmap->size);
	my $ok = $self->apply_surface($x, $y, $bitmap, $colors);
	$canvas->color($restore_color) if defined $restore_color;
	return $ok;

FALLBACK:
	return $self->{canvas}->fillpoly($poly);
}

1;

=pod

=head1 NAME

Prima::Drawable::Antialias - plot antialiased shapes

=head1 DESCRIPTION

Prima offers drawing antialiased lines and shapes, which is rather slow
but provides better visual feedback.

The module augments the C<Prima::Drawable> drawing functionality by adding the
C<new_aa_surface> function, which features two plotting methods, C<polyline>
and C<fillpoly>, identical to the ones in C<Prima::Drawable>.

=head1 SYNOPSIS

	$canvas-> new_aa_surface-> polyline([0, 0, 100, 100]);

	$canvas-> new_path(antialias => 1)-> ellipse(100,100,100)->fill;

=for podview <img src="Prima/aa.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/aa.gif">

=head1 API

=head2 Methods

=over

=item new $CANVAS

Creates a new AA surface object. The object is cheap to keep and reuse.

=item fillpoly $POLY [ $FILLMODE ]

Paints an antialiased polygon shape. The following properties from the C<$CANVAS>
are respected: color, backColor, fillPattern, fillPatternOffset, rop2.

=item polyline $POLY

Plots an antialiased polyline. The following properties from the C<$CANVAS> are respected:
color, backColor, linePattern, lineWidth, lineEnd, lineJoin, miterLimit, rop2

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>, F<examples/antialias.pl> 


=cut
