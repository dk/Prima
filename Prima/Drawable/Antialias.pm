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

sub apply_surface
{
	my ( $self, $x, $y, $alpha, $colors ) = @_;

	return 0 unless $self->{can_aa};
	return 1 if $self->{alpha} == 0;
	$alpha->premultiply_alpha($self->{alpha}) if $self->{alpha} < 255;

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

	my $canvas = $self->{canvas};
	my $lp = $canvas->linePattern;
	my $r2 = $canvas->rop2;
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

	my $render = $canvas->new_path->line($poly);
	if ( $solid_line ) {
		my $c = $canvas->color;
		$canvas->color($canvas->backColor);
		my $lines = $render->widen(linePattern => lp::Solid, antialias => 1)->points;
		for my $line ( @$lines ) {
			my $v = $canvas->render_polyline( $line, aafill => 1) or goto FALLBACK;
			my ($x, $y, $bitmap) = @$v;
			goto FALLBACK unless defined $bitmap;
			$self->apply_surface($x, $y, $bitmap);
		}
		$canvas->color($c);
	}

	my $lines = $render->widen(antialias => 1)->points;
	for my $line ( @$lines ) {
		my $v = $canvas->render_polyline( $line, aafill => 1) or goto FALLBACK;
		my ($x, $y, $bitmap) = @$v;
		goto FALLBACK unless defined $bitmap;
		$self->apply_surface($x, $y, $bitmap);
	}
	return 1;


FALLBACK:
	return $self->{canvas}->polyline($poly);
}

sub fillpoly
{
	my ( $self, $poly, $mode ) = @_;
	goto FALLBACK unless $self->{can_aa};

	my $canvas = $self->{canvas};
	my $v = $canvas->render_polyline( $poly, aafill => 1) or goto FALLBACK;
	my ($x, $y, $bitmap) = @$v;
	return 0 unless defined $bitmap;

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
			size              => [ $bitmap->size ],
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
		if ($canvas->rop2 == rop::CopyPut) {
			$fp = Prima::Image->new(
				type     => im::BW,
				size     => [8,8],
				data     => join('000', map { chr } @$fp),
			) if ref($fp) eq 'ARRAY';

			if ($fp->type == im::BW) {
				$fp->type(im::bpp1);
				$fp->colormap( $canvas->backColor, $canvas->color );
			}
			goto IMAGE;
		}

		my @fp = $canvas->fillPatternOffset;
		$fp[0] -= $x;
		$fp[1] -= $y;
		$bitmap-> set(
			fillPattern       => $fp,
			fillPatternOffset => \@fp,
			color             => 0xffffff,
			backColor         => 0x000000,
			rop               => rop::AndPut,
			rop2              => rop::CopyPut,
		);
		$bitmap->bar( 0, 0, $bitmap->size);
	}

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
