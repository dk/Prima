package Prima::Drawable::Path;

use strict;
use warnings;
use Prima;

our $PI = 3.14159265358979323846264338327950288419716939937510;
our $PI_2 = $PI / 2;
our $PI_4 = $PI / 4;
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
	Carp::cluck("Option 'antialias' is deprecated, use 'subpixel' instead") if exists $opt{antialias};
	return bless {
		canvas          => $canvas,
		commands        => [],
		precision       => undef,
		subpixel        => $opt{subpixel} // (($canvas && ref($canvas)) ? $canvas->antialias : 0),
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

sub canvas { $#_ ? $_[0]->{canvas} = $_[1] : $_[0]->{canvas} }

sub cmd
{
	my ($self, $cmd, @param) = @_;
	push @{ $self->{commands} }, $cmd, scalar(@param), @param;
	delete $self->{points};
	return $self;
}

sub rcmd
{
	my $self = shift;
	push @{ $self->{commands} }, 
		save     => 0,
		relative => 0,
		shift, scalar(@_), @_,
		restore  => 0,
	;
	delete $self->{points};
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
sub subpixel         { $#_ ? $_[0]->{subpixel} = $_[1] : $_[0]->{subpixel} }
sub antialias        { Carp::cluck("Property 'antialias' is deprecated, use 'subpixel' instead") }

sub reset
{
	$_[0]->{commands} = [];
	delete $_[0]->{points};
	delete $_[0]->{stack};
	delete $_[0]->{curr};
}

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
	if ( (1 == @m) && ref($m[0]) ) {
		my $matrix = $m[0];
		if ( UNIVERSAL::isa($matrix, 'Prima::Matrix')) {
			$self->cmd( matrix => @{$matrix->get} );
		} else {
			$self->cmd( matrix => @$matrix );
		}
	} elsif ( @m == 6 ) {
		$self->cmd( matrix => @m );
	} else {
		Carp::croak('bad parameters to matrix');
	}
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
	$self-> matrix(1,$y,$x,1,0,0);
}

sub rotate
{
	my ( $self, $angle ) = @_;
	return $self if $angle == 0.0;
	$angle /= $RAD;
	my $cos = cos($angle);
	my $sin = sin($angle);
	($sin, $cos) = Prima::Utils::nearest_d($sin, $cos);;
	$self->matrix($cos, $sin, -$sin, $cos, 0, 0);
}

sub line
{
	my $self = shift;
	my $p = $#_ ? [@_] : $_[0];
	@$p % 2 and Carp::croak('bad parameters to line');
	$self->cmd( line => $p );
}

*polyline = \&line;

sub rline
{
	my $self = shift;
	my $p = $#_ ? [@_] : $_[0];
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

sub _outline2path
{
	my ( $self, $fill, $outline ) = @_;

	my @p;
	my @anchor;
	my $do_anchor;
	my $size = scalar(@$outline);
	for ( my $i = 0; $i < $size; ) {
		my $cmd = $outline->[$i++];
		my $pts = $outline->[$i++] * 2;
		my @pts = map { $outline->[$i++] / 64.0 } 0 .. $pts - 1;
		if ( $cmd == ggo::Move ) {
			$self->close unless $fill;
			$self->moveto(@anchor = @pts);
			$do_anchor = 0;
		} elsif ( $cmd == ggo::Line ) {
			$self->line([ @p, @pts ]);
			$do_anchor = 1;
		} elsif ( $cmd == ggo::Conic ) {
			$self->spline([ @p, @pts ]);
			$do_anchor = 1;
		} elsif ( $cmd == ggo::Cubic ) {
			$self->spline([ @p, @pts ], degree => 3 );
			$do_anchor = 1;
		} else {
			next;
		}
		@p = @pts[-2,-1];
	}
	$self->line(@anchor) if $do_anchor && @anchor;
	$self->close;
}

sub glyph
{
	my ($self, $ix, %opt) = @_;
	return unless $self->{canvas};
	my $outline = $self->{canvas}->render_glyph( $ix, %opt );
	return unless $outline;

	my $fill = delete $opt{fill};

	if ( !$opt{color} ) {
		$self->_outline2path($fill, $outline);
		return $self;
	}

	my @ret   = ( $self->{canvas}->new_path );
	my $size  = scalar(@$outline);
	my ($i,$start);
	for ( $i = $start = 0; $i < $size; ) {
		my $cmd = $outline->[$i++];
		my $pts = $outline->[$i++] * 2;
		if ( $cmd != ggo::SetColor ) {
			$i += $pts;
			next;
		}
		if ( $start < $i - 2 ) {
			my @sub_outline = @$outline[ $start .. $i - 3 ];
			$ret[-1]->_outline2path( $fill, \@sub_outline);
		}
		push @ret, $outline->[$i], $self->{canvas}->new_path
			unless @ret > 1 && $outline->[$i] == $ret[-2];
		$i += $pts;
		$start = $i;
	}
	if ( $start != $i ) {
		my @sub_outline = @$outline[ $start .. $#$outline ];
		$ret[-1]->_outline2path( $fill, \@sub_outline);
	}

	return @ret;
}

sub text
{
	my ($self, $text, %opt) = @_;
	return if $opt{color};
	return unless my $c = $self->{canvas};
	my $state = $c->get_paint_state;
	unless ($state) {
		return unless $c->begin_paint_info;
	}

	$self->translate( 0, $c->font->descent )
		unless $opt{baseline} // $c->textOutBaseline;
	my $cache   = $opt{cache} || {};
	my $unicode = utf8::is_utf8($text);
	for my $char ( split //, $text ) {
		my $ix = ord($char);
		$self->glyph($ix, %opt, unicode => $unicode);
		my $r = $cache->{$char} //= do {
			my $p = $c->get_font_abc($ix,$ix,$unicode);
			$p->[0] + $p->[1] + $p->[2]
		};
		$self->translate($r,0);
	}

	$c->end_paint_info unless $state;
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
	@_ > 5 or Carp::croak('bad parameters to arc');
	my ( $cx, $cy, $dx, $dy, $from, $to, $tilt) = @_;
	return $self if $from == $to;
	unless ($self->{subpixel}) {
		$dx--;
		$dy--;
		return $self if $dx <= 0 || $dy <= 0;
	}
	if ( $tilt // 0.0 ) {
		return $self-> save->
			scale( $dx / 2, $dy / 2)->
			rotate( $tilt)->
			translate( $cx, $cy )->
			circular_arc( $from, $to )->
			restore;
	} else {
		return $self-> save->
			matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
			circular_arc( $from, $to )->
			restore;
	}
}

sub rarc
{
	my $self = shift;
	@_ > 3 or Carp::croak('bad parameters to arcto');
	my ( $dx, $dy, $from, $to, $tilt) = @_;
	return $self if $from == $to;
	unless ($self->{subpixel}) {
		$dx--;
		$dy--;
		return $self if $dx <= 0 || $dy <= 0;
	}
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
	unless ($self->{subpixel}) {
		$dx--;
		$dy--;
		return $self if $dx <= 0 || $dy <= 0;
	}
	$self-> save->
		matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
		rotate( $tilt // 0.0)->
		circular_arc( 0.0, 360.0 )->
		restore;
}

sub chord
{
	my $self = shift;
	@_ == 6 or Carp::croak('bad parameters to chord');
	my ( $cx, $cy, $dx, $dy, $start, $end) = @_;
	unless ($self->{subpixel}) {
		$dx--;
		$dy--;
		return $self if $dx <= 0 || $dy <= 0;
	}
	$self-> save->
		matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
		circular_arc( $start, $end )->
		close->
		restore;
}

sub lines
{
	my $self = shift;
	my $p = $#_ ? [@_] : $_[0];
	@$p % 4 and Carp::croak('bad parameters to lines');
	for ( my $i = 0; $i < @$p; $i += 4 ) {
		$self->open;
		$self->cmd( line   => [ @$p[ $i .. $i + 3 ] ] );
	}
	$self->open;
}

sub rectangle
{
	my $self = shift;
	@_ == 4 or Carp::croak('bad parameters to rectangle');
	my ( $x1, $y1, $x2, $y2) = @_;
	$self-> line([$x1, $y1, $x2, $y1, $x2, $y2, $x1, $y2])-> close;
}

sub sector
{
	my $self = shift;
	@_ == 6 or Carp::croak('bad parameters to sector');
	my ( $cx, $cy, $dx, $dy, $start, $end) = @_;
	unless ($self->{subpixel}) {
		$dx--;
		$dy--;
		return $self if $dx <= 0 || $dy <= 0;
	}
	$self-> save->
		matrix( $dx / 2, 0, 0, $dy / 2, $cx, $cy )->
		line(0,0)->
		circular_arc( $start, $end )->
		close->
		restore;
}

sub round_rect
{
	my ( $self, $x, $y, $x1, $y1, $maxd) = @_;
	return $self->rectangle($x, $y, $x1, $y1) if $maxd <= 0;

	( $x1, $x) = ( $x, $x1) if $x > $x1;
	( $y1, $y) = ( $y, $y1) if $y > $y1;
	my ( $dx, $dy) = ( $x1 - $x, $y1 - $y);
	$dx = $maxd if $dx > $maxd;
	$dy = $maxd if $dy > $maxd;
	my $d = ( $dx < $dy ) ? $dx : $dy;
	my $r = $d/2;
#  plots roundrect:
# A'        B'
#  /------\
#  |A    B|
#  |      |  arcs cannot have diameter larger than $maxd
#  |C    D|
#  \------/
# C'        D'
	my @r = (
		# coordinates of C and B, so A=r[0,3],B=r[2,3],C=r[0,1],D=[2,1]
		$x + $r, $y + $r,
		$x1 - $r, $y1 - $r,
		# coordinates of C' and B'
		$x, $y,
		$x1, $y1,
	);
	$self-> line( @r[2,7,0,7]) if $r[0] < $r[2];
	$self-> arc( @r[0,3], $d, $d, 90, 180);
	$self-> line( @r[4,3,4,1]) if $r[1] < $r[3];
	$self-> arc( @r[0,1], $d, $d, 180, 270);
	$self-> line( @r[0,5,2,5]) if $r[0] < $r[2];
	$self-> arc( @r[2,1], $d, $d, 270, 360);
	$self-> line( @r[6,1,6,3]) if $r[1] < $r[3];
	$self-> arc( @r[2,3], $d, $d, 0, 90);
	return $self;
}

sub new_array { shift->{subpixel} ? Prima::array->new_double : Prima::array->new_int }

sub new_contour
{
	my $self = shift;
	my $p = $self->{points};
	while (@$p && @{$p->[-1]} < 4) {
		$self->{hint_map}->[ $#$p ] = undef
			if exists $self->{hint_map};
		pop @$p;
	}
	push @$p, $self->new_array;
}

sub points
{
	my ($self, %opt) = @_;

	my ($with_hints, $hints);
	if ( $opt{hints}) {
		delete $self->{points};
		$with_hints = 1;
	}

	unless ( $self->{points} ) {
		local $self->{stack} = [];
		local $self->{curr}  = {
			matrix => [ identity ],
			( map { $_, $self->{$_} } qw(precision) )
		};
		my $p = $self->{points} = [ $self->new_array ];

		my ( $lj_signal, $lj_override ) = (0,0);
		my $c = $self->{commands};
		$hints = $self->{hint_map} = [] if $with_hints;

		for ( my $i = 0; $i < @$c; ) {

			my ($cmd,$len) = @$c[$i,$i+1];

			my ($np, $got_hint);
			if ( $with_hints ) {
				if ( $cmd eq 'spline' || $cmd eq 'arc') {
					$lj_signal = 1;
				} elsif ( $cmd eq 'line') {
					$lj_signal = 0;
				}
				if ( $lj_signal != $lj_override ) {
					push @{ $hints->[$#$p] }, scalar(@{$p->[-1]}) / 2;
					$lj_override = $lj_signal;
					$got_hint = 1;
				}
				$np = @$p;
			}

			$self-> can("_$cmd")-> ( $self, @$c[$i+2..$i+$len+1] );
			$i += $len + 2;

			$lj_signal = $lj_override = $got_hint = 0 if $with_hints &&
				( $np != @$p || ($got_hint && !defined( $hints->[$#$p] )));
		}
		$self-> new_contour;
		pop @$p;

		$self->{last_matrix} = $self->{curr}->{matrix};
	}

	if ( $opt{hints}) {
		delete $self->{hint_map};
		return $self->{points}, $hints;
	}

	return wantarray ? @{$self->{points}} : $self->{points};
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
	my $ret = Prima::Drawable->render_polyline( $points, matrix => $self->{curr}->{matrix} );
	return $ref ? $ret : @$ret;
}

sub get_approximate_matrix_scaling
{
	my @m = map { abs $_ } @{ shift->{curr}->{matrix} };
	my $r = $m[A];
	$r = $m[B] if $r < $m[B];
	$r = $m[C] if $r < $m[C];
	$r = $m[D] if $r < $m[D];
	return $r;
}

sub _save
{
	my $self = shift;

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
	$self->{curr} = pop @{ $self->{stack} } or die "stack underflow";
}

sub _set
{
	my ($self, $prop, $val) = @_;
	$self->{curr}->{$prop} = $val;
}

sub _matrix
{
	my $self = shift;
	$self->{curr}->{matrix}  = matrix_multiply( \@_, $self->{curr}->{matrix} );
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

sub  _moveto
{
	my ( $self, $mx, $my, $rel) = @_;
	($mx, $my) = $self->matrix_apply($mx, $my);
	my ($lx, $ly) = $rel ? $self->last_point : (0,0);
	push @{$self->{points}->[-1]},
		$self->{subpixel} ?
			($lx + $mx, $ly + $my) :
			Prima::Utils::nearest_i($lx + $mx, $ly + $my);
}

sub _open { shift-> new_contour }

sub _close
{
	my $self = shift;
	my $p = $self->{points};
	return unless @$p;
	my $l = $p->[-1];
	push @$l, $$l[0], $$l[1] if @$l && ($$l[0] != $$l[-2] || $$l[1] != $$l[-1]);
	$self-> new_contour;
}

sub _line
{
	my ( $self, $line ) = @_;
	my $ppp = Prima::Drawable->render_polyline( $line,
		matrix  => $self->{curr}->{matrix},
		integer => !$self->{subpixel},
	);
	Prima::array::deduplicate($ppp,2,4);
	Prima::array::append( $self->{points}->[-1], $ppp);
}

sub _spline
{
	my ( $self, $points, $options ) = @_;
	my $ppp = Prima::Drawable->render_spline(
		$self-> matrix_apply( $points ),
		%$options,
		integer => !$self->{subpixel},
	);
	Prima::array::deduplicate($ppp,2,4);
	Prima::array::append( $self->{points}->[-1], $ppp);
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
		if ( $a2 - $a1 > 90 ) {
			push @out, $a1 += 90;
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
			Prima::Utils::nearest_d(\@points),
			closed    => 0,
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
	my ( $self, $from, $to, $rel ) = @_;

	my $scaling = $self->get_approximate_matrix_scaling;
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
	$xopt{precision} = $self->{curr}->{precision} // 24;
	$xopt{precision} = $scaling if $xopt{precision} > $scaling;
	$xopt{integer}   = !$self->{subpixel};
	$xopt{precision} *= 2 if
		$self->{subpixel} &&
		$self->canvas && 
		$self->canvas->get_paint_state == ps::Disabled; # emulated AA for images
	for my $set ( @$nurbset ) {
		my ( $points, @options ) = @$set;
		my $poly = $self-> matrix_apply( $points );
		my $m = $self->{curr}->{matrix};
		if ( $xopt{integer} && $xopt{precision} < 3) {
			my $n = $self->new_array;
			push @$n, Prima::Utils::nearest_i(@$poly[0,1,-2,-1]);
			$poly = $n;
		} elsif ($xopt{precision} >= 2) {
			$poly = Prima::Drawable->render_spline( $poly, @options, %xopt);
		}
		Prima::array::deduplicate($poly,2,4);
		Prima::array::append( $self->{points}->[-1], $poly);
	}
}

sub stroke
{
	return 0 unless $_[0]->{canvas};
	for ( $_[0]->points ) {
		next if 4 > @$_;
		return 0 unless $_[0]->{canvas}->polyline($_);
	}
	return 1;
}

sub fill
{
	my ( $self, $fillMode ) = @_;
	return 0 unless my $c = $self->{canvas};
	my $ok = 1;
	my $save;
	if ( defined $fillMode ) {
		$save = $c->fillMode;
		$c->fillMode($fillMode);
	}
	for ( $self->points ) {
		next if 4 > @$_;
		last unless $ok &= $c->fillpoly($_);
	}
	$c->fillMode($save) if defined $save;
	return $ok;
}

sub fill_gradient
{
	my ( $self, %opt ) = @_;
	return unless my $c = $self->{canvas};

	my ($method, @xopt);
	my $filler = delete $opt{type} // 'horizontal';
	if ( $filler eq 'horizontal') {
		$method = 'bar';
		push @xopt, 0;
	} elsif ( $filler eq 'vertical') {
		$method = 'bar';
		push @xopt, 1;
	} elsif ( $filler eq 'circular') {
		$method = 'ellipse';
	} elsif ( $filler eq 'radial') {
		$method = 'sector';
	} else {
		Carp::carp("unknown filler type: $filler");
		return;
	}

	my $fm       = delete $opt{fill} // (fm::Winding | fm::Overlay);
	my $gradient = $c->new_gradient(%opt);

	$c->graphic_context( sub {
		my $rgn = Prima::Region->new( polygons => scalar($self->points), fillMode => $fm, matrix => $c->get_matrix);
		$c->add_region( $rgn );
		$c->matrix->identity;

		$c->antialias(0);
		if ( $method eq 'bar') {
			$gradient->bar( $rgn->rect, @xopt );
		} elsif ( $method eq 'ellipse') {
			my @box = $rgn->box;
			my $d = int(sqrt( $box[2]*$box[2] + $box[3]*$box[3] ) + .5);
			$gradient->ellipse(
				$box[0] + $box[2] / 2,
				$box[1] + $box[3] / 2,
				$d, $d
			);
		} elsif ( $method eq 'sector') {
			my @box = $rgn->box;
			my $d = int(sqrt( $box[2]*$box[2] + $box[3]*$box[3] ) + .5);
			$gradient->sector(
				$box[0] + $box[2] / 2,
				$box[1] + $box[3] / 2,
				$d, $d, 0, 360
			);
		}
	} );
}

sub fill_stroke
{
	my ( $self, $fillMode ) = @_;
	return 0 unless my $c = $self->{canvas};
	my $color = $c->color;
	$c->color( $c-> backColor);
	my $ok = $self->fill($fillMode);
	$c->color( $color );
	$ok &= $self->stroke;
	return $ok;
}

sub flatten
{
	my ($self, $opt_prescale) = @_;
	local $self->{stack} = [];
	local $self->{curr}  = {
		matrix => [ identity ],
		( map { $_, $self->{$_} } qw(precision ) )
	};
	my $c = $self->{commands};
	my @dst;
	for ( my $i = 0; $i < @$c; ) {
		my ($cmd,$len) = @$c[$i,$i+1];
		my @param = @$c[$i+2..$i+$len+1];
		$i += $len + 2;

		if ( $cmd =~ /^(matrix|set|save|restore)$/) {
			# to get the right precision and prescaling
			$self-> can("_$cmd")-> ( $self, @param );
			push @dst, $cmd, $len, @param;
		} elsif ( $cmd eq 'arc') {
			my ( $from, $to, $rel ) = @param;
			my $prescale;
			unless ( defined $opt_prescale ) {
				my @m = map { abs } @{ $self-> {curr}->{matrix} };
				# pre-shoot scaling ractor for rasterization
				$prescale = $m[A];
				$prescale = $m[B] if $prescale < $m[B];
				$prescale = $m[C] if $prescale < $m[C];
				$prescale = $m[D] if $prescale < $m[D];
				$prescale = 1 if $prescale == 0.0;
			} else {
				$prescale = $opt_prescale;
			}

			my %xopt;
			$xopt{precision} = $self->{curr}->{precision} if defined $self->{curr}->{precision};
			$xopt{integer}   = !$self->{subpixel};
			$xopt{precision} *= 2 if $self->{subpixel} && $self->canvas->get_paint_state == ps::Disabled; # emulated AA for images
			my $polyline;
			my $nurbset = $self->arc2nurbs( $from, $to);
			for my $set ( @$nurbset ) {
				my ( $points, @options ) = @$set;
				my $p = Prima::Drawable->render_spline( 
					[map { $_ * $prescale } @$points],
					@options, %xopt
				);
				if ( $polyline ) {
					Prima::array::append( $polyline, $p );
				} else {
					$polyline = $p;
				}
			}
			if ( scalar @$polyline ) {
				push @dst, save     => 0;
				push @dst, relative => 0 if $rel;
				push @dst, matrix   => 6, 1.0/$prescale, 0, 0, 1.0/$prescale, 0, 0;
				push @dst, line     => 1, $polyline;
				push @dst, restore  => 0;
			}
		} else {
			push @dst, $cmd, $len, @param;
		}
	}
	return ref($self)->new( undef,
		%$self,
		canvas   => $self->{canvas},
		commands => \@dst
	);
}

# L.Maisonobe 2003
# http://www.spaceroots.org/documents/ellipse/elliptical-arc.pdf
sub arc2cubics
{
	my ( undef, $x, $y, $dx, $dy, $start, $end) = @_;

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
		push @cubics, Prima::Utils::nearest_d(\@points);
	}
	@cubics = reverse @cubics if $reverse;
	return \@cubics;
}

sub to_line_end
{
	my ($self, $opt_prescale) = @_;
	local $self->{stack} = [];
	local $self->{curr}  = {
		matrix => [ identity ],
		( map { $_, $self->{$_} } qw(precision ) )
	};
	my $c = $self->{commands};
	my @dst;
	my @last_point = (0,0);
	for ( my $i = 0; $i < @$c; ) {
		my ($cmd,$len) = @$c[$i,$i+1];
		my @param = @$c[$i+2..$i+$len+1];
		$i += $len + 2;

		if ( $cmd eq 'arc') {
			my ( $from, $to, $rel ) = @param;
			my $cubics = $self->arc2cubics( 0, 0, 2, 2, $from, $to);
			if ( $rel ) {
				my ($lx,$ly) = @last_point;
				my $pts = $cubics->[0];
				my $m = $self->{curr}->{matrix};
				my @s = $self->matrix_apply( $pts->[0], $pts->[1]);
				$m->[4] += $lx - $s[0];
				$m->[5] += $ly - $s[1];
			}
			for my $c (@$cubics) {
				my $poly = $self-> matrix_apply( $c );
				push @dst, 'cubic' => $poly;
				@last_point = @$poly[-2,-1];
			}
		} elsif ( $cmd eq 'relative') {
			my $m  = $self->{curr}->{matrix};
			my ( $x0, $y0 ) = $self-> matrix_apply(0, 0);
			$m->[X] += $last_point[0] - $x0;
			$m->[Y] += $last_point[1] - $y0;
		} elsif ( $cmd eq 'line') {
			push @dst, line => $self-> matrix_apply($param[0]);
			@last_point = @{$dst[-1]}[-2,-1];
		} elsif ( $cmd eq 'spline') {
			my ( $points, $options) = @param;
			push @dst,
				((($options->{degree} // 2) == 2) ? 'conic' : 'cubic'),
				$self-> matrix_apply($points)
				;
			@last_point = @{$dst[-1]}[-2,-1];
		} elsif ( $cmd =~ /^(open|close|moveto)$/) {
			# no warnings, just ignore
			last;
		} else {
			$self-> can("_$cmd")-> ( $self, @param );
		}
	}

	return \@dst;
}

sub contours
{
	my $self = shift;
	my @ret;
	for my $pp ( $self->points ) {
		my @contour;
		next if @$pp < 2;
		my $closed = $pp->[0] == $pp->[-2] && $pp->[1] == $pp->[-1];
		for ( my $i = 0; $i < @$pp - 2; $i += 2 ) {
			my @a = @{$pp}[$i,$i+1];
			my @b = @{$pp}[$i+2,$i+3];
		
			my ( $delta_y, $delta_x, $dir);
			next if $a[0] == $b[0] && $a[1] == $b[1] && @$pp > 4;

			$delta_y = $b[1] - $a[1];
			$delta_x = $b[0] - $a[0];
			$dir = 1 if abs($delta_y) > abs($delta_x);
	
			my ( $curr_maj, $curr_min, $to_maj, $delta_maj, $delta_min ) = $dir ? 
				($a[1], $a[0], $b[1], $delta_y, $delta_x) :
				($a[0], $a[1], $b[0], $delta_x, $delta_y);
			my $inc_maj = ($delta_maj != 0) ?
				(abs($delta_maj)==$delta_maj ? 1 : -1) : 0;
			my $inc_min = ($delta_min != 0) ?
				(abs($delta_min)==$delta_min ? 1 : -1) : 0;
			$delta_maj = abs($delta_maj);
			$delta_min = abs($delta_min);
			my $d      = ($delta_min * 2) - $delta_maj;
			my $d_inc1 = ($delta_min * 2);
			my $d_inc2 = (($delta_min - $delta_maj) * 2);
	
			while(1) {
				my @p = $dir ? ($curr_min, $curr_maj) : ($curr_maj, $curr_min);
				push @contour, @p;
				last if $curr_maj == $to_maj;
				$curr_maj += $inc_maj;
				if ($d < 0) {
					$d += $d_inc1;
				} else {
					$d += $d_inc2;
					$curr_min += $inc_min;
				}
			}
			pop @contour, pop @contour if $closed || $i > 0;
		}
		push @ret, \@contour if @contour;
	}
	return @ret;
}

sub poly2patterns
{
	my ($pp, $lp, $lw, $int) = @_;
	$lw = 1 if $lw < 1;
	my @steps = map { 1 + $lw * (ord($_) - 1 ) } split '', $lp;
#	print "$lw: steps: @steps\n";
	my @dst;
	my @sqrt;
	for my $p ( @$pp ) {
		if ( @$p <= 2 ) {
			push @dst, $p;
			next;
		}
		my $closed = $p->[0] == $p->[-2] && $p->[1] == $p->[-1];
		my ($segment, @strokes);
		my ($i,$strokecolor,$step,$new_point,$new_stroke,$advance,$joiner) = 
			(0,0,0,1,1,0,0,0);
		my ( @a, @b, $black, $dx, $dy, $pixlen, @r, @a1, @b1, $plotted, $draw, $strokelen);
		while ( 1) {
			if ( $advance == 0 && $new_stroke ) {
				$strokecolor = !$strokecolor;
				$strokelen = $steps[$step++];
#				print "new stroke #$step: $strokelen " . ($strokecolor ? "black" : "white") . " pixels\n";
				$step = 0 if $step == @steps;
				push @strokes, $segment = [] if $strokecolor;
				$joiner = 0;
			}
			if ($new_point ) {
				@a  = @$p[$i,$i+1];
				last if @$p <= ($i += 2);
				@b  = @$p[$i,$i+1];
				$dx = $b[0] - $a[0];
				$dy = $b[1] - $a[1];
				my $dl = $dx * $dx + $dy * $dy;
				$pixlen = (($dl < 1024 && $int ) ?
					$sqrt[$dl + .5] //= sqrt(int($dl + .5)) :
					sqrt($dl)
				);
#				print "new point $i: (@a) + $pixlen -> @b\n";
				@r = ($pixlen > 0) ? 
					($dx / $pixlen, $dy / $pixlen):
					(1,1);
				$pixlen = int( $pixlen + .5 ) if $int;
				if (($i == $#$p - 1 && !$closed) || ($pixlen == 0)) {
					$pixlen++;
				} else {
					$b[0] -= $r[0];
					$b[1] -= $r[1];
				}
				@a1 = @a;
				@b1 = @b;
				$plotted = 0;
				splice( @$segment, -2, 2) if $joiner && $advance == 0;
				$joiner = 0;
			}
			($draw, $black) = ( $advance > 0 ) ? ($advance, 0) : ($strokelen, $strokecolor);
#			print "draw:$advance/$strokelen pixlen:$pixlen plotted:$plotted black:$black\n";
			my $next_seg_advance = $black ? $lw - 1 : 1;
			if ( $draw < $pixlen ) {
				$plotted += $draw;
				@b1 = ($draw == 1) ? @a1 : (
					($plotted - 1) * $r[0] + $a[0],
					($plotted - 1) * $r[1] + $a[1],
				);
#				print "pix($black): @a1 -> @b1\n";
				push @$segment, @a1, @b1 if $black;
				$pixlen -= $draw;
				$advance += ($advance > 0) ? -$draw : $next_seg_advance;
				@a1 = ( $b1[0] + $r[0], $b1[1] + $r[1]);
#				print "new adv to @a1? =$advance\n";
				($new_point, $new_stroke) = (0,1);
			} elsif ( $draw == $pixlen ) {
				push @$segment, @a1, @b if $black;
				$new_stroke = $new_point = 1;
				$advance += ($advance > 0) ? -$draw : $next_seg_advance;
#				print "=: pix($black): @a1 -> @b\n";
				$joiner = $black;
			} elsif ( $black && $draw == 1 && $pixlen <= 0 )  {
				$new_point = $new_stroke = 1;
				$advance = $next_seg_advance;
#				print "skip tail\n";
			} else {
#				print ">: pix($black): @a1 -> @b\n";
				push @$segment, @a1, @b if $black;
				($new_point, $new_stroke) = (1,0);
				if ($advance > 0) {
					$advance -= $pixlen;
				} else {
					$strokelen -= $pixlen;
					$joiner = $black;
				}
			}
		}
#		print "done with @$p\n";

		pop @strokes if @strokes && !@{$strokes[-1]};
		my $first;
		push @dst, $first = shift @strokes;
		push @dst, @strokes;
		if ( @strokes && $closed && $steps[0] > 1 && $strokelen > 1 ) {
			my $last = pop @dst;
			unshift @$first, @$last[2 .. $#$last];
		}
	}
	my @r;
	for my $r ( @dst ) {
		my $n = Prima::array->new_double;
		push @$n, @$r;
		Prima::array::deduplicate($n,2,2);
		push @r, $n;
	}
	return \@r;
}

# Adapted from wine/dlls/gdi32/path.c:WidenPath()
# (c) Martin Boehme, Huw D M Davies, Dmitry Timoshkov, Alexandre Julliard
#
# I keep this for easy debugging and reference, however this implementation
# doesn't include the line join hinting that is present in render_polyline()
sub widen_old
{
	my ( $self, %opt ) = @_;

	my $dst = ref($self)->new( undef,
		%$self,
		canvas   => $self->{canvas},
		commands => [],
	);

	my ($lw, $lj, $le, $lp) = map {
		my $opt = exists($opt{$_}) ? $opt{$_} : (
			$self->{canvas} ? $self->{canvas}->$_() : 0
		);
		$opt = 0 if $_ ne 'linePattern' and $opt < 0;
		$opt;
	} qw(lineWidth lineJoin lineEnd linePattern);

	my $pp;
	{
		local $self->{subpixel} = 1;
		$pp = $self->points;
	}
	return $dst if $lp eq lp::Null;
	$pp = poly2patterns($pp, $lp, $lw, !$self->{subpixel}) if $lp ne lp::Solid;

	my $no_line_ends = ($lw < 2.5) && !$self->{subpixel};

	if ( $no_line_ends && $lw < 1.5 ) {
		for my $p ( @$pp ) {
			$dst->line($p);
			$dst->line([map { @{$p}[-2*$_,-2*$_+1] } 1..@$p/2 ])
				if $lp eq lp::Solid; # so fill() won't autoclose the shape, if any
			$dst->open;
		}
		return $dst;
	}

	my $ml = exists($opt{miterLimit}) ? $opt{miterLimit} : ($self->{canvas} ? $self->{canvas}->miterLimit : 10);
	$ml = 20        if $ml > 20;
	$lw = 16834     if $lw > 16834;
	$lj = lj::Miter if $lj > lj::Miter;
	$le = le::Round if $le > le::Round;
	my $sqrt2;

	my @dst;
	my $lw2 = $lw / 2;

	# classic widening algorithm doesn't do that (I believe) because there's no fm::Overlay - but Prima has it
	$lw2 -= 0.5 unless $self->{subpixel};

	for ( @$pp ) {
		next unless @$_;

		my (@u,@d);
		my @p = Prima::array::list($_); # expand once as access is expensive
		my $closed = $p[0] == $p[-2] && $p[1] == $p[-1];
		my $last = @p - ($closed ? 4 : 2);

		if ( $last == 0 ) {
			my ($x,$y) = @p;
			if ( $le == le::Square || $no_line_ends ) {
				$dst->line( 
					$x - $lw2, $y - $lw2,
					$x - $lw2, $y + $lw2,
					$x + $lw2, $y + $lw2,
					$x + $lw2, $y - $lw2,
				);
			} elsif ( $le == le::Round ) {
				$dst->ellipse( $x, $y, $lw);
			}
			$dst->open;
			next;
		}

		my ($firstout, $firstin, $firstsign);
		for ( my $i = 0; $i <= $last; $i += 2 ) {
			$opt{callback}->(
				$i, \@p, {
					lineJoin  => sub { $lj = shift },
					lineEnd   => sub { $le = shift },
					lineWidth => sub { $lw2 = ($lw = shift) / 2 },
				}
			) if $opt{callback};
			if ( !$closed && ($i == 0 || $i == $last )) {
				my ( $xo, $yo, $xa, $ya) = @p[ $i ? (map { $i + $_ } 0,1,-2,-1) : (0..3)];
        	        	my $theta = atan2( $ya - $yo, $xa - $xo );
				if ( $le == le::Flat || $no_line_ends) {
					my ($sin, $cos) = (sin($theta + $PI_2), cos($theta + $PI_2));
					push @u, [ line => [
						$xo + $lw2 * $cos,
						$yo + $lw2 * $sin,
						$xo - $lw2 * $cos,
						$yo - $lw2 * $sin
					] ];
				} elsif ( $le == le::Square ) {
					$sqrt2 //= sqrt(2.0) * $lw2;
					push @u, [ line => [
						$xo - $sqrt2 * cos($theta - $PI_4),
						$yo - $sqrt2 * sin($theta - $PI_4),
						$xo - $sqrt2 * cos($theta + $PI_4),
						$yo - $sqrt2 * sin($theta + $PI_4)
					] ];
				} else {
					push @u, [ arc => 
						$xo, $yo, $lw, $lw, 
						$RAD * ($theta + $PI_2),
						$RAD * ($theta + 3 * $PI_2),
					];
				}
			} else {
				my ($prev, $next);
				if ( $i > 0 && $i < $last) {
					($prev, $next) = ($i - 2, $i + 2);
				} elsif ( $i == 0) {
					($prev, $next) = ($last, $i + 2);
				} else {
					($prev, $next) = ($i - 2, 0);
				}
				my ($xo,$yo,$xa,$ya,$xb,$yb) = @p[$i,$i+1,$prev,$prev+1,$next,$next+1];
				my $dya = $yo - $ya;
				my $dxa = $xo - $xa;
				my $dyb = $yb - $yo;
				my $dxb = $xb - $xo;
				my $theta = atan2( $dya, $dxa );
        	        	my $alpha = atan2( $dyb, $dxb ) - $theta;
				$alpha += $PI * (($alpha > 0) ? -1 : 1);
				# next if $alpha == 0.0; # XXX
				my $sign = ( $alpha > 0) ? -1 : 1;
				my ( $in, $out) = ($alpha > 0) ? (\@u,\@d) : (\@d,\@u);
				my ( $dx1, $dy1, $dx2, $dy2) = map { $sign * $lw2 * $_ } (
					cos($theta + $PI_2),
					sin($theta + $PI_2),
					cos($theta + $alpha + $PI_2),
					sin($theta + $alpha + $PI_2)
				);
				my $_lj = $lj;
				my $dmin = 3;
				$_lj = lj::Miter if $_lj != lj::Miter &&
					abs($dya) < $dmin && abs($dxa) < $dmin && abs($dyb) < $dmin && abs($dxb) < $dmin;
				$_lj = lj::Bevel if
					$_lj == lj::Miter && ($alpha == 0 || $ml < abs( 1 / sin($alpha/2)));
				if ($i == 0) {
					@$firstin = ( $xo + $dx1, $yo + $dy1);
					$firstsign = $sign;
				}
				next if $dxa == 0 && $dya == 0;
				next if $dxb == 0 && $dyb == 0;
				push @$in, [ line => [ $xo + $dx1, $yo + $dy1 ]];
				push @$in, [ line => [ $xo - $dx2, $yo - $dy2 ]];
				if ( $_lj == lj::Miter) {
        	                	my $miterWidth = abs($lw2 / cos($PI_2 - abs($alpha) / 2));
					push @$out, [ line => [
						$xo + $miterWidth * cos($theta + $alpha / 2),
						$yo + $miterWidth * sin($theta + $alpha / 2)
					]];
					@$firstout = @{ $out->[-1][1] }
						if $i == 0;
				} elsif ( $_lj == lj::Bevel || $no_line_ends ) {
					@$firstout = ( $xo - $dx1, $yo - $dy1 )
						if $i == 0;
					push @$out, [ line => [ $xo - $dx1, $yo - $dy1 ]];
					push @$out, [ line => [ $xo + $dx2, $yo + $dy2 ]];
				} else {
					@$firstout = ( $xo - $dx1, $yo - $dy1 )
						if $i == 0;
					push @$out, [ arc =>
						$xo, $yo,
						$lw, $lw,
						($alpha > 0) ? (
							$RAD * ($theta + $alpha - $PI_2),
							$RAD * ($theta + $PI_2),
						) : (
							$RAD * ($theta - $PI_2),
							$RAD * ($theta + $alpha + $PI_2),
						)
					];
				}
				if ( $i == $last ) {
					( $firstin, $firstout ) = ( $firstout, $firstin )
						if $sign != $firstsign;
					push @$in, [ line => $firstin ];
					push @$out, [ line => $firstout ];
				}
			}
		}
		push @u, reverse @d;
		@d = (); 
		for ( @u ) {
			my ( $cmd, @param ) = @$_;
			if ( $cmd eq 'line' && @d && $d[-1][0] eq 'line' ) {
				push @{ $d[-1][1] }, @{$param[0]};
			} else {
				push @d, $_;
			}
		}
		for ( @d ) {
			my ( $cmd, @param ) = @$_;
			$dst->$cmd(@param);
		}
		$dst->open;
	}
	return $dst;
}

sub widen_new
{
	my ( $self, %opt ) = @_;

	my $dst = ref($self)->new( undef,
		%$self,
		canvas   => $self->{canvas},
		commands => [],
	);

	my ($pp, $hints);
	{
		local $self->{subpixel} = 1;
		($pp, $hints) = $self-> points( hints => 1);
	}

	for (my $j = 0; $j < @$pp; $j++ ) {
		my $p = $pp->[$j];
		my $h = $hints->[$j];

		my $cmds = $self->{canvas}->render_polyline( $p, %opt,
			path    => 1,
			integer => ($self->{subpixel} ? 0 : 1),
			line_join_hints => $h,
		);

		for ( my $i = 0; $i < @$cmds;) {
			my $cmd   = $cmds->[$i++];
			my $param = $cmds->[$i++];
			if ( $cmd eq 'line') {
				$dst->line( $param );
			} elsif ( $cmd eq 'arc') {
				$dst->$cmd( @$param );
			} elsif ( $cmd eq 'conic') {
				$dst->spline( $param, degree => 2, closed => 0 );
			} elsif ( $cmd eq 'cubic') {
				$dst->spline( $param, degree => 3, closed => 0 );
			} elsif ( $cmd eq 'open') {
				$dst->open;
			} else {
				warn "** panic: unknown render_polyline command '$cmd'";
				last;
			}
		}
	}

	return $dst;
}

*widen = \&widen_new;
#*widen = \&widen_old;

sub extents
{
	my $self = shift;
	my @pp = $self->points;
	return unless @pp;
	my ( $x1, $y1, $x2, $y2 ) = @{$pp[0]}[0,1,0,1];
	for my $p ( @pp ) {
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
	$p->fillpoly($_) for $self->points;
	return $p->image;
}

sub region
{
	my ($self, $mode) = @_;
	$mode //= fm::Winding | fm::Overlay;
	return Prima::Region->new( polygons => scalar($self->points), fillMode => $mode);
}

sub _debug_commands
{
	my $self = shift;
	my $c = $self->{commands};
	for ( my $i = 0; $i < @$c; ) {
		my ($cmd,$len) = @$c[$i,$i+1];
		my @p = @$c[$i+2..$i+$len+1];
		@p = map { 'ARRAY' eq ref($_) ? "[@$_]" : $_ } @p;
		print STDERR ".$cmd(@p)\n";
		$i += $len + 2;
	}
}

1;

__END__

=pod

=head1 NAME

Prima::Drawable::Path - stroke and fill complex paths

=head1 DESCRIPTION

The module augments the C<Prima::Drawable>'s drawing and plotting functionality by
implementing paths that allow arbitrary combinations of polylines, splines, and arcs,
to be used for drawing or clipping shapes.

=head1 SYNOPSIS

	# draws an elliptic spiral
	my ( $d1, $dx ) = ( 0.8, 0.05 );
	$canvas-> new_path->
		rotate(45)->
		translate(200, 100)->
		scale(200, 100)->
		arc( 0, 0, $d1 + $dx * 0, $d1 + $dx * 1, 0, 90)->
		arc( 0, 0, $d1 + $dx * 2, $d1 + $dx * 1, 90, 180)->
		arc( 0, 0, $d1 + $dx * 2, $d1 + $dx * 3, 180, 270)->
		arc( 0, 0, $d1 + $dx * 4, $d1 + $dx * 3, 270, 360)->
	stroke;

=for podview <img src="Prima/path.gif">

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

Adds an elliptic arc to the path. The arc is centered around the (CENTER_X,CENTER_Y) point.

Important: if the intention is an immediate rendering, especially with a 1-pixel
line width, consider decreasing diameters by 1. This is because all arc
calculations are made with the floating point precision, where the diameter is also
given not in pixels but in geometrical coordinates, to allow for matrix
transformations.  Before rendering is performed, arcs are transformed into
spline vertices and then the transformation matrix is applied, and by that time the
notion of an arc diameter is lost to be successfully converted into pixel size
minus one.

Read more about this in L<Prima::Drawable/Antialiasing and alpha> .

=item close, open

Closes the current shape and opens a new one.  close() is the same as open() but
makes sure the shape's first point is equal to its last point.

=item circular_arc ANGLE_START, ANGLE_END

Adds a circular arc to the path. Note that adding transformations will effectively
make it into an elliptic arc, which is used internally by C<arc> and C<rarc>.

=item chord CENTER_X, CENTER_Y, DIAMETER_X, DIAMETER_Y, ANGLE_START, ANGLE_END.

Adds a chord to the path. Is there only for compatibility with C<Prima::Drawable>.

=item ellipse CENTER_X, CENTER_Y, DIAMETER_X, DIAMETER_Y = DIAMETER_X, TILT = 0

Adds a full ellipse to the path.

=item glyph INDEX, %OPTIONS

Adds a glyph outline to the path. C<%OPTIONS> are passed as is to
L<Prima::Drawable/renger_glyph>, except the C<fill> option.

Note that filled glyphs require C<fillMode> without the C<fm::Overlay> bit set,
and also the C<fill> option set to generate proper shapes with holes.

If the C<color> is option added, reports an array of scalars where each is
either a new path object, or an integer scalar thet sets the 24-bit RGB color
for the subsequent path(s) to be painted with. If the integer scalar is -1 that
means to use the current color.

=item line, rline @POINTS

Adds a polyline to the path

=item lines [X1, Y1, X2, Y2]..

Adds a set of multiple, unconnected lines to the path. Is there only for
compatibility with C<Prima::Drawable>.

=item moveto, rmoveto X, Y

Stops plotting the current shape and moves the plotting position to X, Y.

=item rarc DIAMETER_X, DIAMETER_Y, ANGLE_START, ANGLE_END, TILT = 0

Adds an elliptic arc to the path so that the first point of the arc starts on
the last point of the previous primitive, or (0,0) if there's none.

=item rectangle X1, Y1, X2, Y2

Adds a rectangle to the path. Is there only for compatibility with
C<Prima::Drawable>.

=item round_rect X1, Y1, X2, Y2, MAX_DIAMETER

Adds a round rectangle to the path.

=item sector CENTER_X, CENTER_Y, DIAMETER_X, DIAMETER_Y, ANGLE_START, ANGLE_END

Adds a sector to the path. Is there only for compatibility with C<Prima::Drawable>.

=item spline, rspline $POINTS, %OPTIONS.

Adds a B-spline to the path. See L<Prima::Drawable/spline> for C<%OPTIONS> descriptions.

=item text TEXT, %OPTIONS

Adds C<TEXT> to the path. C<%OPTIONS> are the same as in L<Prima::Drawable/render_glyph>, 
except that C<unicode> is deduced automatically based on whether the C<TEXT> has utf8 bit
on or off. An extra option C<cache> with a hash can be used to speed up the function
with subsequent calls. The C<baseline> option is the same as L<Prima::Drawable/textOutBaseline>.

Note that filled glyphs require C<fillMode> without the C<fm::Overlay> bit set,
and also the C<fill> option set to generate proper shapes with holes.

=back

=head2 Transformations

Transformation calls change the current path properties (matrix etc) so that
all subsequent calls would use them until a call to C<restore> is made. The
C<save> and C<restore> methods implement the stacking mechanism so that local
transformations can be made.

=head2 Properties

=over

=item canvas DRAWABLE

Provides access to the attached drawable object

=item matrix A, B, C, D, Tx, Ty

Applies a transformation matrix to the path. The matrix, as used by the module,
is formed as follows:

  A  B  0
  C  D  0
  Tx Ty 1

When applied to 2D coordinates, the transformed coordinates are calculated as

  X' = AX + CY + Tx
  Y' = BX + DY + Ty

=item precision INTEGER

Selects current precision for splines and arcs. See L<Prima::Drawable/spline>, C<precision> entry.

=item restore

Pops the stack entry and replaces the current matrix and graphic properties with it.

=item rotate ANGLE

Adds rotation to the current matrix

=item save

Saves the current matrix and graphic properties on the stack.

=item shear X, Y = X

Adds shearing to the current matrix

=item scale X, Y = X

Adds scaling to the current matrix

=item subpixel BOOLEAN

Turns on and off slow but more precise floating-point calculation mode

Default: depends on the canvas antialiasing mode

=item translate X, Y = X

Adds an offset to the current matrix

=back

=head2 Operations

These methods perform path rendering and create an array of points that can be
used for drawing

=over

=item clip %options

Returns a 1-bit image with the clipping mask created from the path. C<%options> can be used to
pass the C<fillMode> property that affects the result of the filled shape.

=item contours

Same as L<points> but further reduces lines into a set of 8-connected points,
suitable to be traced pixel-by-pixel.

=item extents

Returns two points that box the path.

=item last_matrix

Returns the current transform matrix (CTM) after running all commands

=item fill fillMode=undef

Paints a filled shape over the path. If C<fillMode> is set, it is used instead of the one
selected on the canvas.

=item fill_gradient %OPTIONS

Applies the path as a region, then calls a gradient filler on it.

Recognizes the following options:

=over

=item type = horizontal

One of: vertical, horizontal, circular, radial

=item fill = fm::Overlay | fm::Winding

The fillMode, if any

=back

Other options are passed as is to the C<new_gradient> method; see L<Prima::Drawable::Gradient> for their description

=item fill_stroke fillMode=undef

Paints a filled shape over the path with the background color. If C<fillMode>
is set, it is used instead of the one selected on the canvas. Thereafter, draws
a polyline over the path.

=item flatten PRESCALE

Returns new objects where arcs are flattened into lines. The lines are
rasterized with a scaling factor that is as close as possible to the device
pixels, to be suitable for a call to the polyline() method. If the PRESCALE
factor is set, it is used instead to premultiply coordinates of arc anchor
points used to render the lines.

=item points

Runs all accumulated commands, returns rendered set of points suitable
for the C<Prima::Drawable::polyline> and C<Prima::Drawable::fillpoly> methods.

=item region MODE=fm::Winding|fm::Overlay

Creates a region object from the path. If MODE is set, applies fill mode (see
L<Prima::Drawable/fillMode> for more)

=item stroke

Draws a polyline over the path

=item widen %OPTIONS

Expands the path into a new path object containing outlines of the original
path as if drawn with selected line properties. The values of C<lineWidth>,
C<lineEnd>, C<lineJoin>, and C<linePattern> are read from C<%OPTIONS>, or from
the attached canvas when available. Supports the C<miterLimit> option with
values from 0 to 20.

Note: if the intention is to immediately render non-antialiased lines, decrease
lineWidth by 1 (they are 1 pixel wider because paths are built around the
assumption that pixel size is 0, which makes them scalable).

=back

=head2 Methods for custom primitives

=over

=item append PATH

Copies all commands from another PATH object. The PATH object doesn't need to
have balanced stacking brackets C<save> and C<restore>, and can be viewed
as a macro.

=item identity

Returns the identity matrix

=item matrix_apply @POINTS

Applies the CTM to POINTS, returns the transformed points.  If @POINTS is a
list, returns the transformed points as a list; if it is an array reference,
returns an array reference.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Drawable>

=cut
