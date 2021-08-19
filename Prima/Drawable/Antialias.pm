package Prima::Drawable::Antialias;

use strict;
use warnings;

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

	$rc[0] = $cr[0] if $rc[0] < $cr[0];
	$rc[1] = $cr[1] if $rc[1] < $cr[1];
	$rc[2] = $cr[2] if $rc[2] > $cr[2];
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
	my ( $self, $x, $y, $alpha ) = @_;

	return 0 unless $self->{can_aa};

	$alpha-> size( map { $_ / $self->{factor} } $alpha-> size );

	my $canvas = $self->{canvas};
	if ( $canvas->isa('Prima::Image') && ! $canvas->get_paint_state ) {
		return $canvas-> put_image( $x, $y, $alpha, rop::SrcOver | rop::ConstantColor | rop::Premultiply );
	} else {
		my $bits = Prima::Image->new(
			size  => [ $alpha-> size ],
			type  => im::RGB,
			color => cl::premultiply( $canvas-> color, $self->{alpha} ),
		);
		$bits-> bar(0, 0, $alpha->size);
		my $icon = Prima::Icon-> create_combined( $bits, $alpha );

		return $canvas-> put_image( $x, $y, $icon, rop::SrcOver );
	}
}

sub polyline
{
	my ( $self, $poly ) = @_;
	goto FALLBACK unless $self->{can_aa};
	my ($x, $y, $w, $h) = $self->calc_poly_extents($poly);
	return 0 unless defined $x;

	my $bitmap = $self->alloc_surface($w, $h) or goto FALLBACK;
	my $canvas = $self->{canvas};

	if ($canvas-> lineWidth > 1 ) {
		$bitmap->region(
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
				region(fm::Winding)
		);
		$bitmap->bar( 0, 0, $bitmap->size);
	} else {
		$bitmap->translate(map { -1 * $self->{factor} * $_ } $x, $y);
		$poly = Prima::Drawable->render_polyline( $poly, matrix => [$self->{factor},0,0,$self->{factor},0,0], integer => 1);
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
	$mode &= ~fm::Overlay; # very slow otherwise due to manual region patch
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
	$bitmap->set( map { $_ => $canvas->$_() } qw(fillPattern) );
	my @fp = $canvas->fillPatternOffset;
	$fp[0] -= $x;
	$fp[1] -= $y;
	$bitmap->fillPatternOffset(@fp);
	$bitmap->bar( 0, 0, $bitmap->size);
	return $self->apply_surface($x, $y, $bitmap);

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

The module augments the C<Prima::Drawable> drawing functionality by
adding C<new_aa_surface> function, and contains two plotting functions,
C<polyline> and C<fillpoly>, identical to the ones in C<Prima::Drawable>.

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

Paints an antialiased polygon shape. The following properties from C<$CANVAS> are respected:
color, fillPattern, fillPatternOffset. Does not plot opaque
patterned lines.

=item polyline $POLY

Plots an antialiased polyline. The following properties from C<$CANVAS> are respected:
color, linePattern, lineWidth, lineEnd, lineJoin, miterLimit. Does not plot opaque
patterned lines.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>, F<examples/antialias.pl> 


=cut
