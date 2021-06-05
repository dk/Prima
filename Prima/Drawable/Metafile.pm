package Prima::Drawable::Metafile;

use strict;
use warnings;
use vars qw(@ISA);
use Prima;
@ISA = qw(Prima::Drawable);

sub profile_default
{
	my %def = %{ shift->SUPER::profile_default };
	return {
		%def,
		size => [1,1],
	};
}

sub init
{
	my ($self, %profile) = @_;
	%profile = $self->SUPER::init(%profile);
	$self->{size} = $profile{size};
	$self->{code} = [];
	return %profile;
}

my @props = qw(
	color backColor fillMode fillPattern font lineEnd
	lineJoin linePattern lineWidth rop rop2 miterLimit
	textOpaque textOutBaseline 
);

for my $prop_name (@props) {
	no strict 'refs';
	*{$prop_name} = sub {
		my $self = shift;
		if ( @_ && $self->get_paint_state == ps::Enabled ) {
			push @{ $self->{code} }, [ 'set', $prop_name, @_ ];
		}
		my $prop = 'SUPER::' . $prop_name;
		return $self->$prop(@_);
	};
}

for my $prop_name (qw(
	alpha arc bar bars chord ellipse fill_chord fill_ellipse
	fillpoly fill_sector flood_fill line lines polyline
	put_image_indirect rectangle sector text_out text_shape_out
)) {
	no strict 'refs';
	*{$prop_name} = sub {
		my $self = shift;
		if ( @_ && $self->get_paint_state == ps::Enabled ) {
			push @{ $self->{code} }, [ 'put', $prop_name, @_ ];
		}
	};
}

for my $prop_name (qw(
	translate clipRect
)) {
	no strict 'refs';
	*{$prop_name} = sub {
		my $self = shift;
		if ( @_ && $self->get_paint_state == ps::Enabled ) {
			push @{ $self->{code} }, [ $prop_name, @_ ];
		}
		my $prop = 'SUPER::' . $prop_name;
		return $self->$prop(@_);
	};
}

sub clear
{
	my $self = shift;
	return unless $self->get_paint_state == ps::Enabled;
	@{ $self->{code} } = () unless @_;
	push @{ $self->{code} }, [ 'put', 'clear', @_ ];
}

sub fillPatternOffset
{
	my $self = shift;
	if (@_ && $self->get_paint_state == ps::Enabled) {
		push @{ $self->{code} }, [ 'fillPatternOffset', @_[0,1] ];
	}
	return $self->SUPER::fillPatternOffset(@_);
}

sub region
{
	my $self = shift;
	if (@_ && $self->get_paint_state == ps::Enabled) {
		push @{ $self->{code} }, [ 'region', $_[0]->dup ];
	}
	return $self->SUPER::region(@_);
}

sub size
{
	return @{ $_[0]->{size} } unless $#_;
	my ( $self, $x, $y ) = @_;
	$x = 1 if $x < 1;
	$y = 1 if $y < 1;
	$self->{size} = [$x, $y];
}

sub width  { $_[0]->{size}->[0] }
sub height { $_[0]->{size}->[1] }

sub execute
{
	my ( $self, $canvas, $x, $y ) = @_;

	if ( UNIVERSAL::isa($canvas, 'Prima::Drawable::Metafile')) {
		push @{ $canvas->{code} }, [ 'translate', $x, $y ], @{ $self->{code} };
		return;
	}

	my %save = map { $_, $canvas->$_() } @props;
	my @clip = $canvas-> clipRect;
	my $rgn  = $canvas-> region;
	my $actual_rgn = $rgn;
	my @tx   = $canvas-> translate;
	my @fpo  = $canvas-> fillPatternOffset;
	$canvas->translate($x + $tx[0], $y + $tx[1]);

	for my $cmd ( @{ $self->{code} } ) {
		my ($cmd, @cmd) = @$cmd;
		if ( $cmd eq 'set' ) {
			my ($method, @args) = @cmd;
			$canvas->$method(@args);
		} elsif ( $cmd eq 'put' ) {
			my ($method, @args) = @cmd;
			$canvas->$method(@args);
		} elsif ( $cmd eq 'translate' ) {
			$cmd[$_] += $tx[$_] for 0,1;
			$canvas->translate(@cmd);
		} elsif ( $cmd eq 'clipRect' ) {
			if ( 4 == grep { $_ == -1 } @cmd ) {
				$actual_rgn = $rgn;
				$canvas->clipRect(@clip);
			} else {
				$rgn //= Prima::Region-> new( rect => \@clip);
				my $xrgn = Prima::Region-> new( rect => \@cmd );
				$xrgn->combine($rgn, rgnop::Intersect);
				$canvas->region($xrgn);
			}
		} elsif ( $cmd eq 'region' ) {
			$rgn //= Prima::Region-> new( rect => \@clip);
			my $xrgn = $cmd[0]->dup;
			$xrgn->combine($rgn, rgnop::Intersect);
			$canvas->region($xrgn);
		} elsif ( $cmd eq 'fillPatternOffset' ) {
			$cmd[$_] += $fpo[$_] for 0,1;
			$canvas-> fillPatternOffset(@cmd);
		}
	}

	$canvas->$_($save{$_}) for @props;
	$canvas->fillPatternOffset(@fpo);
	$canvas->translate(@tx);
	if ( $rgn ) {
		$canvas->region($rgn);
	} else {
		$canvas->clipRect(@clip);
	}
}

1;

=pod

=head1 NAME

Prima::Drawable::Metafile - graphic primitive recorder

=head1 DESCRIPTION

Metafiles can record graphic primitives and replay them later on another canvas.

=head1 SYNOPSIS

  my $metafile = Prima::Drawable::Metafile->new( size => [30, 30] );
  $metafile->begin_paint;
  $metafile->rectangle(10,10,20,20);
  $metafile->end_paint;

  $metafile->execute( $another_drawable, 100, 100 );

=head1 API

=over

=item clear

When called without parameters, clears the content before proceeding.
Otherwise same as C<Drawable.clear>.

=item execute CANVAS,X,Y

Draws the content on a CANVAS with X,Y offset

=item size X,Y

Sets metafile extensions; however the content is not clipped by it.

=back

=head1

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Buttons>, F<examples/buttons.pl>

=cut

