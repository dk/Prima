package Prima::Drawable::Path;

use strict;
use warnings;

sub new
{
	my ( $class, $canvas, %opt ) = @_;
	return bless {
		canvas          => $canvas,
		points          => Prima::array->new_int,
		arcPrecision    => 100,
		splinePrecision => ref($canvas) ? $canvas->splinePrecision : 24,
		%opt
	}, $class;
}

sub make_global
{
	my $self = shift;
	my $p = $self->{points};
	push @$p, 0, 0 unless @$p;
	my @ret;
	for ( my $i = 0; $i < @_ ; $i += 2 ) {
		push @ret, $_[$i]	+ $p->[0];
		push @ret, $_[$i+1] + $p->[1];
	}
	return @ret;
}

sub line
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	push @{ $self->{points} }, @$p;
	return $self;
}

sub rline
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	push @{ $self->{points} }, $self-> make_global( @$p );
	return $self;
}

sub spline
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	Prima::array::append( $self->{points}, $self-> {canvas}-> render_spline( $p, $self->{splinePrecision} ) );
	return $self;
}

sub rspline
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	$p = $self-> {canvas}-> render_spline( $p, $self->{splinePrecision} );
	push @{ $self->{points} }, $self-> make_global( @$p );
	return $self;
}

sub E
{
	my ( $cx, $cy, $rx, $ry, $storage ) = @_;

	my $sint = $storage->{sint};
	my $cost = $storage->{cost};
	my $sine = $storage->{sine};
	my $cose = $storage->{cose};

	$cx + $rx * $cost * $cose - $ry * $sint * $sine,
	$cy + $rx * $sint * $cose + $ry * $cost * $sine
}

sub dE
{
	my ( $rx, $ry, $storage ) = @_;

	my $sint = $storage->{sint};
	my $cost = $storage->{cost};
	my $sine = $storage->{sine};
	my $cose = $storage->{cose};

	- $rx * $cost * $sine - $ry * $sint * $cose,
	- $rx * $sint * $sine + $ry * $cost * $cose
}

sub arc2splines
{
	my ( $self, $cx, $cy, $dx, $dy, $a1, $a2, $tilt ) = @_;
	my $rx = $dx / 2;
	my $ry = $dy / 2;
	return if $dx < 1 || $dy < 1 || $a1 == $a2;

	my $reverse = 0;
	if ( $a1 > $a2 ) {
		($a1, $a2) = ($a2, $a1);
		$reverse = 1;
	}

	my @out;
	my $max_r = ( $rx > $ry ) ? $rx : $ry;
	$a1 += 360, $a2 += 360 while $a2 < 0 || $a1 < 0;
	my $a0 = $a1;
	my $break_c = $self->{arcPrecision};
	my $break_a = $break_c / $max_r * 180 / 3.14159;

	push @out, $a0;
	while ($a0 < $a2) {
		my $q  = int($a0 / 90 + 1) * 90;
		my $an = $a0 + $break_a;
		$an = $a2 if $an > $a2;
		$a0 = ( $an < $q ) ? $an : $q;
		push @out, $a0;
	}
	@out = reverse @out if $reverse;

	my @splines;
	$tilt /= 180 / 3.14159;
	@out = map { $_ / 180 * 3.14159 } @out;

	my %storage;
	$storage{cost} = cos($tilt);
	$storage{sint} = sin($tilt);

	$storage{cosa} = cos($out[0]);
	$storage{sina} = sin($out[0]);
	$storage{eta}  = atan2( $storage{sina} / $ry, $storage{cosa} / $rx );
	$storage{cose} = cos($storage{eta});
	$storage{sine} = sin($storage{eta});

	$storage{apex} = [ E($cx, $cy, $rx, $ry, \%storage) ];

	for my $a ( @out[1..$#out] ) {
		my $cosa = cos($a);
		my $sina = sin($a);
		my $eta  = atan2( $sina / $ry, $cosa / $rx );
		my $deta = ($eta - $storage{eta}) / 2;
		my $tan = sin($deta)/cos($deta);
		my @Q  = map { $_ * $tan } dE($rx, $ry, \%storage);

		$storage{cosa} = $cosa;
		$storage{sina} = $sina;
		$storage{eta}  = $eta;
		$storage{cose} = cos($storage{eta});
		$storage{sine} = sin($storage{eta});
		my @P2 = E($cx, $cy, $rx, $ry, \%storage);
		$Q[$_] += $storage{apex}->[$_] for 0, 1;

		push @splines, [ @{$storage{apex} }, @Q, @P2 ];
		$storage{apex} = \@P2;
	}

	return @splines;
}

sub arc
{
	my ( $self, $cx, $cy, $dx, $dy, $a1, $a2 ) = @_;
	my $tilt = 0;
	$self->spline($_) for $self-> arc2splines( $cx, $cy, $dx, $dy, $a1, $a2, $tilt);
	return $self;
}

sub points { shift->{points} }
sub stroke { $_[0]->{canvas}->polyline( $_[0]->{points} ) }
sub fill   { $_[0]->{canvas}->fillpoly( $_[0]->{points} ) }

sub extents
{
	my $self = shift;
	my $p = $self->{points};
	return unless @$p;
	my ( $x1, $y1, $x2, $y2 ) = @{$p}[0,1,0,1];
	for ( my $i = 1; $i < $#$p; $i+=2) {
		my ($x, $y) = @{$p}[$i,$i+1];
		$x1 = $x if $x < $x1;
		$y1 = $y if $y < $y1;
		$x2 = $x if $x > $x2;
		$y2 = $y if $y > $y2;
	}
	return $x1, $y1, $x2, $y2;
}

sub clip
{
	my $self = shift;
	my ( $x1, $y1, $x2, $y2 ) = $self-> extents;

	my $p = Prima::DeviceBitmap->new( width => $x2, height => $y2, type => dbt::Bitmap );
	$p->clear;
	$p->fillpoly( $self->{points} );
	return $p->image;
}

1;

__END__

=head1 NAME

Prima::Drawable::Path - stroke and fill complex paths

=head1 DESCRIPTION

The module augments the C<Prima::Drawable> drawing and plotting functionality by
implementing paths that allow arbitrary combination of polylines, splines, and arc,
to be used for drawing or clipping shapes.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Object>, L<Prima::Widget>, L<Prima::Window>

=cut
