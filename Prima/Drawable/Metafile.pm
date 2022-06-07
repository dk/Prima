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
		type => dbt::Pixmap,
	};
}

sub init
{
	my ($self, %profile) = @_;
	%profile = $self->SUPER::init(%profile);
	$self->{code} = [];
	$self->{explicit_cliprect} = 0;
	$self->{clipRect} = [ 0, 0, 1, 1  ];
	$self->{type} = $profile{type};
	$self-> size( @{ $profile{size} });
	return %profile;
}

my @props = qw(
	alpha antialias
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
	arc bar bar_alpha bars chord ellipse fill_chord fill_ellipse
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
	translate
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

sub size
{
	return @{ $_[0]->{size} } unless $#_;
	my ( $self, $x, $y ) = @_;
	$x = 1 if $x < 1;
	$y = 1 if $y < 1;
	$self->{size} = [ $x, $y ];
	$self->{clipRect} = [ 0, 0, $x - 1, $y - 1 ] unless $self->{explicit_cliprect};
}

sub clipRect
{
	return @{ $_[0]-> {clipRect} } unless $#_;
	my ( $self, $x1, $y1, $x2, $y2 ) = @_;
	$self->{explicit_cliprect} = 1;
	$x1 = 0 if $x1 < 0;
	$y1 = 0 if $y1 < 0;
	$x2 = $self->{size}->[0] - 1 if $x2 > $self->{size}->[0] - 1;
	$y2 = $self->{size}->[1] - 1 if $y2 > $self->{size}->[0] - 1;
	$self-> {clipRect} = [ $x1, $y1, $x2, $y2 ];
	push @{ $self->{code} }, [ 'clipRect', $x1, $y1, $x2, $y2 ];
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

sub graphic_context_push
{
	my $self = shift;
	my $ok = $self->SUPER::graphic_context_push;
	push @{ $self->{code}, [ 'graphic_context_push' ] }; 1 } if $ok;
	return $ok;
}

sub graphic_context_pop
{
	my $self = shift;
	my $ok = $self->SUPER::graphic_context_pop;
	push @{ $self->{code}, [ 'graphic_context_pop' ] }; 1 } if $ok;
	return $ok;
}

sub width  { $_[0]->{size}->[0] }
sub height { $_[0]->{size}->[1] }

sub type            { $_[0]->{type} }
sub can_draw_alpha  { shift->type != dbt::Bitmap  }
sub has_alpha_layer { shift->type == dbt::Layered }
sub get_bpp         { shift->type == dbt::Bitmap ? 1 : 24 }

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

	return unless $self->graphic_context_push;
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
			$canvas->translate($cmd[0] + $tx[0] + $x, $cmd[1] + $tx[1] + $y);
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

	$self->graphic_context_pop;
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

