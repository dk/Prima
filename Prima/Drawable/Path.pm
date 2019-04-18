package Prima::Drawable::Path;

use strict;
use warnings;

our $PI = 3.14159265358979323846264338327950288419716939937510;
our $RAD = 180.0 / $PI;

# | a  b  0 |
# | c  d  0 |
# | tx ty 1 |
# x' = ax + cy + x
# y' = bx + dy + y
use constant A => 0;
use constant B => 1;
use constant C => 2;
use constant D => 3;
use constant X => 4;
use constant Y => 5;


sub new
{
	my ( $class, $canvas, %opt ) = @_;
	return bless {
		canvas          => $canvas,
		commands        => [],
		precision       => undef,
		%opt
	}, $class;
}

sub dup
{
	my $self = shift;
	return ref($self)->new( undef,
		%$self,
		canvas   => $self->{canvas},
		commands => [ @{ $self->{commands} } ],
	);
}

sub cmd
{
	my $self = shift;
	push @{ $self->{commands} }, @_;
	delete $self->{points};
	return $self;
}

sub rcmd
{
	my $self = shift;
	$self->cmd(
		save     => (),
		relative => (),
		@_,
		restore  => (),
	);
}

sub append           { push @{shift->{commands}}, @{shift->{commands}} }
sub commands         { shift->{commands} }
sub save             { shift->cmd('save') }
sub open             { shift->cmd('open') }
sub close            { shift->cmd('close') }
sub moveto           { shift->cmd('moveto', shift, shift, 0) }
sub rmoveto          { shift->cmd('moveto', shift, shift, 1) }
sub restore          { shift->cmd('restore') } # no checks for underflow here, to allow append paths
sub precision        { shift->cmd(set => precision => shift) }

sub matrix_multiply
{
	my ( $m1, $m2 ) = @_;
	return [
		$m1->[A] * $m2->[A] + $m1->[B] * $m2->[C],
		$m1->[A] * $m2->[B] + $m1->[B] * $m2->[D],
		$m1->[C] * $m2->[A] + $m1->[D] * $m2->[C],
		$m1->[C] * $m2->[B] + $m1->[D] * $m2->[D],
		$m1->[X] * $m2->[A] + $m1->[Y] * $m2->[C] + $m2->[X],
		$m1->[X] * $m2->[B] + $m1->[Y] * $m2->[D] + $m2->[Y]
	];
}

sub matrix
{
	my ( $self, @m ) = @_;
	@_ == 7 or Carp::croak('bad parameters to matrix');
	$self->cmd( matrix => @m );
}

sub identity  { 1, 0, 0, 1, 0, 0 }

sub translate
{
	my ( $self, $dx, $dy) = @_;
	$dy //= $dx;
	shift-> matrix(1,0,0,1,$dx,$dy);
}

sub scale
{
	my ( $self, $x, $y ) = @_;
	$y //= $x;
	$self-> matrix($x,0,0,$y,0,0);
}

sub shear
{
	my ( $self, $x, $y ) = @_;
	$y //= $x;
	$self-> matrix(1,$x,$y,1,0,0);
}


sub rotate
{
	my ( $self, $angle ) = @_;
	return $self if $angle == 0.0;
	$angle /= $RAD;
	my $cos = cos($angle);
	my $sin = sin($angle);
	$self->matrix($cos, $sin, -$sin, $cos, 0, 0);
}

sub line
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	@$p % 2 and Carp::croak('bad parameters to line');
	$self->cmd( line => $p );
}

sub rline
{
	my $self = shift;
	my $p = $#_ ? \@_ : $_[0];
	@$p % 2 and Carp::croak('bad parameters to rline');
	$self->rcmd( line => $p);
}

sub spline
{
	my ($self, $p, %opt) = @_;
	(@$p % 2 || @$p < 6) and Carp::croak('bad parameters to spline');
	$self-> cmd( spline => $p, \%opt );
}

sub rspline
{
	my ($self, $p, %opt) = @_;
	(@$p % 2 || @$p < 6) and Carp::croak('bad parameters to spline');
	$self-> rcmd( spline => $p, \%opt );
}

sub circular_arc
{
	my $self = shift;
	2 == @_ or Carp::croak('bad parameters to circular_arc');
	$self-> cmd( arc => @_, 0 );
}

sub arc
{
	my $self = shift;
	@_ > 5 or Carp::croak('bad parameters to arcto');
	my ( $cx, $cy, $dx, $dy, $from, $to, $tilt) = @_;
	return $self if $from == $to;
	$self-> save->
		matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
		rotate( $tilt // 0.0)->
		circular_arc( $from, $to )->
		restore;
}

sub rarc
{
	my $self = shift;
	@_ > 3 or Carp::croak('bad parameters to arcto');
	my ( $dx, $dy, $from, $to, $tilt) = @_;
	return $self if $from == $to;
	$self->save;
	$self->scale( $dx / 2, $dy / 2);
	$self->rotate( $tilt // 0.0);
	$self->cmd( arc => $from, $to, 1 );
	$self->restore;
}

sub ellipse
{
	my $self = shift;
	@_ > 2 or Carp::croak('bad parameters to ellipse');
	my ( $cx, $cy, $dx, $dy, $tilt) = @_;
	$dy //= $dx;
	$self-> save->
		matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
		rotate( $tilt // 0.0)->
		circular_arc( 0.0, 360.0 )->
		restore;
}

sub points
{
	my $self = shift;
	unless ( $self->{points} ) {
		local $self->{stack} = [];
		local $self->{curr}  = {
			matrix => [ identity ],
			( map { $_, $self->{$_} } qw(precision ) )
		};
		$self->{points} = [ Prima::array->new_int ];
		my @c = @{ $self->{commands} };
		while ( my $cmd = shift @c ) {
			$self-> can("_$cmd")-> ( $self, \@c );
		}
		@{$self->{points}} = grep { @$_ > 2 } @{$self->{points}};
		$self->{last_matrix} = $self->{curr}->{matrix};
	}

	return $self->{points};
}

sub last_matrix
{
	my $self = shift;
	$self->points;
	return $self->{last_matrix};
}

sub last_point
{
	for ( reverse @{ shift->{points} }) {
		return $$_[-2], $$_[-1] if @$_;
	}
	return 0,0;
}

sub matrix_apply
{
	my $self   = shift;
	my ($ref, $points) = $#_ ? (0, [@_]) : (1, $_[0]);
	my $m  = $self->{curr}->{matrix};
	my @ret;
	for ( my $i = 0; $i < @$points; $i += 2 ) {
		my ( $x, $y ) = @{$points}[$i,$i+1];
		push @ret,
			$$m[A] * $x + $$m[C] * $y + $$m[X],
			$$m[B] * $x + $$m[D] * $y + $$m[Y]
			;
	}
	return $ref ? \@ret : @ret;
}

sub _save
{
	my ( $self, $cmd )  = @_;

	push @{ $self->{stack} }, $self->{curr};
	my $m = [ @{ $self->{curr}->{matrix} } ];
	$self->{curr} = {
		%{ $self->{curr} },
		matrix => $m,
	};
}

sub _restore
{
	my $self = shift;
	$self->{curr} = pop @{ $self->{stack} } or die "stack undeflow";
}

sub _set
{
	my ($self, $cmd) = @_;
	my $prop = shift @$cmd;
	my $val  = shift @$cmd;
	$self->{curr}->{$prop} = $val;
}

sub _matrix
{
	my ( $self, $cmd ) = @_;
	my $new = [ splice( @$cmd, 0, 6 ) ];
	$self->{curr}->{matrix}  = matrix_multiply( $new, $self->{curr}->{matrix} );
}

sub _relative
{
	my $self = shift;
	my ($lx,$ly) = $self->last_point;
	my $m  = $self->{curr}->{matrix};
	my ( $x0, $y0 ) = $self-> matrix_apply(0, 0);
	$m->[X] += $lx - $x0;
	$m->[Y] += $ly - $y0;
}

sub _moveto
{
	my ( $self, $cmd) = @_;
	my ( $mx, $my, $rel) = splice(@$cmd, 0, 3);
	($mx, $my) = $self->matrix_apply($mx, $my);
	my ($lx, $ly) = $rel ? $self->last_point : (0,0);
	push @{$self->{points}}, Prima::array->new_int;
	push @{$self->{points}->[-1]}, $lx + $mx, $ly + $my;
}

sub _open
{
	my ( $self, $cmd) = @_;
	push @{$self->{points}}, Prima::array->new_int;
}

sub _close
{
	my $self = shift;
	my $p = $self->{points};
	push @{$p->[-1]}, $p->[0][0], $p->[0][1] if @{$p->[-1]};
	push @{$self->{points}}, Prima::array->new_int;
}

sub _line
{
	my ( $self, $cmd ) = @_;
	my $line = shift @$cmd;
	push @{ $self->{points}->[-1] }, @{ $self-> matrix_apply( $line ) };
}

sub _spline
{
	my ( $self, $cmd ) = @_;
	my $points  = shift @$cmd;
	my $options = shift @$cmd;
	Prima::array::append( $self->{points}->[-1],
		Prima::Drawable->render_spline(
			$self-> matrix_apply( $points ),
			%$options
		)
	)
}

# Reference:
#
# One method for representing an arc of ellipse by a NURBS curve
# E. Petkov, L.Cekov
# Jan 2005

sub arc2nurbs
{
	my ( $self, $a1, $a2 ) = @_;
	my ($reverse, @out);
	($a1, $a2, $reverse) = ( $a2, $a1, 1 ) if $a1 > $a2;

	push @out, $a1;
	while (1) {
		if ( $a2 - $a1 > 180 ) {
			push @out, $a1 += 180;
		} else {
			push @out, $a2;
			last;
		}
	}
	@out = map { $_ / $RAD } @out;

	my @set;
	my @knots = (0,0,0,1,1,1);
	my ( $cosa1, $sina1 );

	for ( my $i = 0; $i < $#out; $i++) {
		( $a1, $a2 ) = @out[$i,$i+1];
		my $b        = $a2 - $a1;
		my $cosb2    = cos($b/2);
		my $d        = 1 / $cosb2;
		$cosa1     //= cos($a1);
		$sina1     //= sin($a1);
		my @points = (
			$cosa1, $sina1,
			cos($a1 + $b/2) * $d, sin($a1 + $b/2) * $d,
			cos($a2), sin($a2),
		);
		($cosa1, $sina1) = @points[4,5];
		my @weights = (1,$cosb2,1);
		@points[0,1,4,5] = @points[4,5,0,1] if $reverse;

		push @set, [
			\@points,
			degree    => 2,
			weights   => \@weights,
			knots     => \@knots,
		];
	}
	@set = reverse @set if $reverse;

	return \@set;
}

sub _arc
{
	my ( $self, $cmd ) = @_;
	my $from = shift @$cmd;
	my $to   = shift @$cmd;
	my $rel  = shift @$cmd;

	my $nurbset = $self->arc2nurbs( $from, $to);
	if ( $rel ) {
		my ($lx,$ly) = $self->last_point;
		my $pts = $nurbset->[0]->[0];
		my $m = $self->{curr}->{matrix};
		my @s = $self->matrix_apply( $pts->[0], $pts->[1]);
		$m->[X] += $lx - $s[0];
		$m->[Y] += $ly - $s[1];
	}

	my %xopt;
	$xopt{precision} = $self->{curr}->{precision} if defined $self->{curr}->{precision};

	for my $set ( @$nurbset ) {
		my ( $points, @options ) = @$set;
		Prima::array::append( $self->{points}->[-1],
			Prima::Drawable->render_spline(
				$self-> matrix_apply( $points ),
				@options,
				%xopt
			)
		);
	}
}

sub stroke {
	return 0 unless $_[0]->{canvas};
	for ( @{ $_[0]->points }) {
		return 0 unless $_[0]->{canvas}->polyline($_);
	}
	return 1;
}

sub fill {
	return 0 unless $_[0]->{canvas};
	for ( @{ $_[0]->points }) {
		return 0 unless $_[0]->{canvas}->fillpoly($_);
	}
	return 1;
}

sub extents
{
	my $self = shift;
	my $pp = $self->points;
	return unless @$pp;
	my ( $x1, $y1, $x2, $y2 ) = @{$pp->[0]}[0,1,0,1];
	for my $p ( @$pp ) {
		for ( my $i = 2; $i < $#$p; $i+=2) {
			my ($x, $y) = @{$p}[$i,$i+1];
			$x1 = $x if $x < $x1;
			$y1 = $y if $y < $y1;
			$x2 = $x if $x > $x2;
			$y2 = $y if $y > $y2;
		}
	}
	return $x1, $y1, $x2, $y2;
}

sub clip
{
	my ($self, %opt) = @_;
	my ( $x1, $y1, $x2, $y2 ) = $self-> extents;
	my ( $tx, $ty ) = (0,0);
	$x2 -= $x1, $tx -= $x1 if $x1 < 0;
	$y2 -= $y1, $ty -= $y1 if $y1 < 0;

	my $p = Prima::DeviceBitmap->new( width => $x2, height => $y2, type => dbt::Bitmap );
	$p->clear;
	$p->set(%opt) if scalar keys %opt;
	$p->translate($tx, $ty);
	$p->fillpoly($_) for @{ $self->{points} };
	return $p->image;
}

sub region
{
	my ($self, $winding) = @_;
	my $pp  = $self->points;
	my $reg = Prima::Region->new(
		polygon => $pp->[0],
		winding => $winding,
	);
	$reg->combine( 
		Prima::Region->new( polygon => $_, winding => $winding),
		rgnop::Union
	) for @$pp[1..$#_];
	return $reg;
}

1;

__END__

=head1 NAME

Prima::Drawable::Path - stroke and fill complex paths

=head1 DESCRIPTION

The module augments the C<Prima::Drawable> drawing and plotting functionality by
implementing paths that allow arbitrary combination of polylines, splines, and arcs,
to be used for drawing or clipping shapes.

=head1 SYNOPSIS

	# draws elliptic spiral
	my ( $d1, $dx ) = ( 0.8, 0.05 );
	$canvas-> new_path->
		scale(200, 100)->
		rotate(45)->
		arc( 0, 0, $d1 + $dx * 0, $d1 + $dx * 1, 0, 90)->
		arc( 0, 0, $d1 + $dx * 2, $d1 + $dx * 1, 90, 180)->
		arc( 0, 0, $d1 + $dx * 2, $d1 + $dx * 3, 180, 270)->
		arc( 0, 0, $d1 + $dx * 4, $d1 + $dx * 3, 270, 360)->
	stroke;

=for podview <img src="path.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/path.gif">

=head1 API

=head2 Primitives

All primitives come in two versions, with absolute and relative coordinates.
The absolute version draws a graphic primitive so that its starting point
(or a reference point) is at (0,0). The relative version, called with an 'r'
(f.ex. C<line> vs C<rline>) has its starting point as the ending point of
the previous primitive (or (0,0) if there's none).

=over

=item arc CENTER_X, CENTER_Y, DIAMETER_X, DIAMETER_Y, ANGLE_START, ANGLE_END, TILT = 0

Adds elliptic arc to path centered around (CENTER_X,CENTER_Y).

=item close, open

Closes the current shape and opens a new one
close() is same as open() but makes sure the shape's first point is equal to its last point.

=item circular_arc ANGLE_START, ANGLE_END

Adds circular arc to the path. Note that adding transformations will effectively
make it into elliptic arc, which is used internally by C<arc> and C<rarc>.

=item ellipse CENTER_X, CENTER_Y, DIAMETER_X, DIAMETER_Y = DIAMETER_X, TILT = 0

Adds full ellipse to the path.

=item line, rline @POINTS

Adds a polyline to path

=item moveto, rmoveto X, Y

Stops plotting the current shape and moves the plotting position to X, Y.

=item rarc DIAMETER_X, DIAMETER_Y, ANGLE_START, ANGLE_END, TILT = 0

Adds elliptic arc to path so that the first point of the arc starts on the last
point of the previous primitive, or (0,0) if there's none.

=item spline, rspline $POINTS, %OPTIONS.

Adds B-spline to path. See L<Prima::Drawable/spline> for C<%OPTIONS> descriptions.

=back

=head2 Transformations

Transformation calls change the current path properties (matrix etc)
so that all subsequent calls will use them until a call to C<restore>
is used. C<save> and C<restore> implement a stacking mechanism, so that
local transformations can be made.

The final transformations calculate coordinates the new and the existing matrices:

  P' = NewMatrix * P

=over

=item matrix A, B, C, D, Tx, Ty

Applies transformation matrix to the path. The matrix, as used by the module,
is formed as such:

  A  B  0
  C  D  0
  Tx Ty 1

and when applied to 2D coordinates, is calculated as

  X' = AX + CY + Tx
  Y' = BX + DY + Ty

=item precision INTEGER

Selects current precision for splines and arcs. See L<Prima::Drawable/spline>, C<precision> entry.

=item restore

Pops the stack entry and replaces the current matrix and graphic properties with it.

=item rotate ANGLE

Adds rotation to the current matrix

=item save

Duplicates the current matrix and graphic properties and pushes them to the stack.

=item shear X, Y = X

Adds shearing to the current matrix

=item scale X, Y = X

Adds scaling to the current matrix

=item translate X, Y = X

Adds offset to the current matrix

=back

=head2 Operations

These methods perform actual path rendering, that was delayed until that, and will
create an array of points that can be used for actual drawing.

=over

=item clip %options

Returns 1-bit image with clipping mask of the path. C<%options> can be used to
pass C<fillWinding> property that affects the result of the filled shape.

=item extents

Returns 2 points that box the path.

=item points

Runs all accumulated commands, and returns rendered set of points, suitable
for further calls to C<Prima::Drawable::polyline> and C<Prima::Drawable::fillpoly>.

=item last_matrix

Return CTM resulted after running all commands

=item fill

Paints a filled shape over the path

=item stroke

Draws a polyline over the path

=item region WINDING=0

Creates a region object from polygonal shape. If WINDING is set, applies fill winding
mode (see L<Drawable/fillWinding> for more).

=back

=head2 Methods for custom primitives

=over

=item append PATH

Copies all commands from another PATH object. The PATH object doesn't need to
have balanced stacking brackets C<save> and C<restore>, and can be viewed
as a macro.

=item identity

Returns identity matrix

=item matrix_apply @POINTS

Applies current matrix to POINTS, returns the transformed points.
If @POINTS is a list, returns list; if it is an array reference, returns
array reference.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>

=cut
