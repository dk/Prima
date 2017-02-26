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
		precision       => 24,
		%opt
	}, $class;
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
sub save             { shift->cmd('save', 0) }
sub path             { shift->cmd('save', 1) }
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
	my $self = shift;
	my $p = $#_ ? [@_] : $_[0];
	(@$p % 2 || @$p < 6) and Carp::croak('bad parameters to spline');
	$self-> cmd( spline => $p );
}

sub rspline
{
	my $self = shift;
	my $p = $#_ ? [@_] : $_[0];
	(@$p % 2 || @$p < 6) and Carp::croak('bad parameters to rspline');
	$self->rcmd( spline => $p );
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
	$self-> path->
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

sub points
{
	my $self = shift;
	unless ( $self->{points} ) {
		local $self->{stack} = [];
		local $self->{curr}  = {
			matrix_inner  => [ identity ],
			matrix_outer  => [ identity ],
			matrix_actual => [ identity ],
			( map { $_, $self->{$_} } qw(precision) )
		};
		$self->{points} = Prima::array->new_int;
		my @c = @{ $self->{commands} };
		while ( my $cmd = shift @c ) {
			$self-> can("_$cmd")-> ( $self, \@c );
		}
	}

	return $self->{points};
}

sub matrix_apply
{
	my $self   = shift;
	my ($ref, $points) = $#_ ? (0, [@_]) : (1, $_[0]);
	my $m  = $self->{curr}->{matrix_actual};
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

	my ( $m1, $m2, $m3 ) = @_;
	if ( shift @$cmd ) {
		$m1 = [ identity ];
		$m2 = [ @{ $self->{curr}->{matrix_actual} } ];
	} else {
		$m1 = [ @{ $self->{curr}->{matrix_inner}  } ];
		$m2 = [ @{ $self->{curr}->{matrix_outer}  } ];
	}
	$m3 = [ @{ $self->{curr}->{matrix_actual} } ];
	$self->{curr} = {
		%{ $self->{curr} },
		matrix_inner  => $m1,
		matrix_outer  => $m2,
		matrix_actual => $m3,
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
	$self->{curr}->{matrix_inner}  = matrix_multiply( $self->{curr}->{matrix_inner}, $new );
	$self->{curr}->{matrix_actual} = matrix_multiply( $self->{curr}->{matrix_inner}, $self->{curr}->{matrix_outer} );
}

sub _relative
{
	my $self = shift;
	my $p  = $self->{points};
	return unless @$p;
	my $m  = $self->{curr}->{matrix_actual};
	my ( $x0, $y0 ) = $self-> matrix_apply(0, 0);
	$m->[X] += $p->[-2] - $x0;
	$m->[Y] += $p->[-1] - $y0;
}

sub _line
{
	my ( $self, $cmd ) = @_;
	my $line = shift @$cmd;
	push @{ $self->{points} }, @{ $self-> matrix_apply( $line ) };
}

sub _spline
{
	my ( $self, $cmd ) = @_;
	my $spline = shift @$cmd;
	Prima::array::append( $self->{points},
		Prima::Drawable->render_spline(
			$self-> matrix_apply( $spline )
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
			precision => $self->{curr}->{precision},
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
		my $p  = $self->{points};
		if ( @$p ) {
			my $pts = $nurbset->[0]->[0];
			my $m = $self->{curr}->{matrix_actual};
			my @s = $self->matrix_apply( $pts->[0], $pts->[1]);
			$m->[X] += $p->[-2] - $s[0];
			$m->[Y] += $p->[-1] - $s[1];
		}
	}

	for my $set ( @$nurbset ) {
		my ( $points, @options ) = @$set;
		Prima::array::append( $self->{points},
			Prima::Drawable->render_spline( 
				$self-> matrix_apply( $points ),
				@options
			)
		);
	}
}

sub stroke { $_[0]->{canvas} ? $_[0]->{canvas}->polyline( $_[0]->points ) : 0 }
sub fill   { $_[0]->{canvas} ? $_[0]->{canvas}->fillpoly( $_[0]->points ) : 0 }

sub extents
{
	my $self = shift;
	my $p = $self->points;
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
