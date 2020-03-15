package Prima::Drawable::CurvedText;

use strict;
use warnings;

*Prima::Drawable::curved_text_out  = \&curved_text_out;

sub init_pointer
{
	my ( $p, $beginning) = @_;
	my $P = {
		x      => $p-> [0],     # current coordinates of the aperture point
		y      => $p-> [1],     #
		i      => 0,            # index of the current segment in polygon
		n      => scalar(@$p)-2,# number of points
 		end    => 0,            # end of polygon?
		p      => $p,           # polygon

		a      => undef,        # angle of the current segment
		l      => undef,        # length of the current segment
		lleft  => undef,        # length of line between (x,y) and the end of the curr segment
		dx     => undef,        # tangent of the current segment
		dy     => undef,        #
	};
	$P->{i} = $P->{n} - 2 unless $beginning;
	calc_segment( $P);
	$P->{lleft} = 0 unless $beginning;
	return $P;
}

sub calc_segment
{
	my $p    = $_[0];
	my ($P,$I) = ($p->{p}, $p->{i});
	my $x = $p-> {dx} = $$P[$I + 2] - $$P[$I];
	my $y = $p-> {dy} = $$P[$I + 3] - $$P[$I + 1];
	$p-> {l} = $p->{lleft} = int( 0.5 + sqrt( $y * $y + $x * $x));
	$p-> {a} = atan2($y, $x) * 180 / 3.14159265358;
#print "next segment $$P[$I],$$P[$I+1]-$$P[$I+2],$$P[$I+3] len $p->{l} angle $p->{a}\n";
}

# skip to next segment
sub next_segment
{
	my $p = $_[0];
	my $P = $p->{p};
	while ( not $p-> {end}) {
		$p-> {i} += 2;
		$p-> {end}++, return 0 if $p-> {i} >= $p->{n};
		calc_segment( $p);
		$p-> {x} = $$P[ $p->{i}     ];
		$p-> {y} = $$P[ $p->{i} + 1 ];
		last if $p-> {l} > 0;
	}
	return 1;
}

# track back to previous segment
sub prev_segment
{
	my $p = $_[0];
	my $P = $p->{p};
	$p-> {end} = 0;
	$p-> {lleft} = 0;
	while ( 1) {
		return 0 if $p-> {i} == 0;
		$p-> {i} -= 2;
		calc_segment( $p);
		$p-> {x} = $$P[ $p->{i}     ];
		$p-> {y} = $$P[ $p->{i} + 1 ];
		last if $p-> {l} > 0;
	}
	return 1;
}

# move the pointer to a given offset, return the average angle of the passed path
sub move_pointer
{
	my ( $p, $o) = @_;
#print "shift $o pixels i=$p->{i} llen=$p->{lleft} l=$p->{l}\n";
	my $O = $o;
	my ($ox,$oy) = ($p->{x}, $p->{y});
	my $i = $p-> {i};

	if ( $o < 0) {
		$o = -$o;
		$o = int($o + 0.5);
		while ( $o + $p->{lleft} > $p-> {l} or $p-> {l} == 0) {
			$o -= $p->{l} - $p-> {lleft};
			goto EXIT unless prev_segment($p);
#print "prev segment $p->{i} o=$o lleft=$p->{lleft} l=$p->{l}\n";
		}
		goto EXIT if $o <= 0;
		$o = -$o;
	} else {
		$o = int($o + 0.5);
		while ( $o > $p->{lleft} or $p-> {l} == 0) {
			$o -= $p-> {lleft};
			goto EXIT unless next_segment($p);
#print "next segment $p->{i} o=$o\n";
		}
		goto EXIT if $o <= 0;
	}


	my $l = $p-> {l} - $p-> {lleft} + $o;
	$p-> {lleft} = $p-> {l} - $l;

	if ( $p-> {lleft} > 0) {
		my $P = $p->{p};
		$p-> {x} = $$P[ $p-> {i}     ] + $p-> {dx} * $l / $p-> {l};
		$p-> {y} = $$P[ $p-> {i} + 1 ] + $p-> {dy} * $l / $p-> {l};
		$_ = ( $_ < 0) ? int( $_ - 0.5) : int( $_ + 0.5) for ($p-> {x}, $p->{y});
#print "offset pointer to $p->{x},$p->{y}, lleft $p->{lleft}\n";
	} else {
		next_segment($p);
	}

EXIT:
	return $p-> {a} if $i == $p-> {i}; # same segment, don't recalculate the angle
	$ox = $p->{x} - $ox;
	$oy = $p->{y} - $oy;
	return $p->{a} if $ox == 0 and $oy == 0; # last point of the segment
	return atan2($oy, $ox) * 180 / 3.14159265358;
}

# changes current coordinates to a point within given segment
sub set_pointer
{
	my ( $p, $x, $y) = @_;
	$p-> {x} = $x;
	$p-> {y} = $y;
	my $dx = $p-> {p}-> [$p-> {i}    ] - $x;
	my $dy = $p-> {p}-> [$p-> {i} + 1] - $y;
	my $l  = int( 0.5 + sqrt( $dy * $dy + $dx * $dx));
	$p-> {lleft} = $p-> {l} - $l;
#print "move pointer to $x,$y, set llen $p->{lleft}\n";
	next_segment($p) if $p->{lleft} <= 0;
}

sub update_box
{
	my ( $self, $x, $y, $angle, $t, $box) = @_;
	$self-> font-> direction( $angle) if defined $angle;
	@$box = @{$self-> get_text_box( $$t)};
	for ( my $j = 0; $j < 10; $j += 2) {
		$box-> [$j]   += $x;
		$box-> [$j+1] += $y;
	}
}

# rotate back @$a to angle=0, calculate transformation matrix,
# and store all in the cache
sub precalc_box
{
	my ( $angle, $box, $cache) = @_;
	my ($dx, $dy) = @$box[2,3];
	$cache-> [0] = $dx;   # offset x
	$cache-> [1] = $dy;   # offset y
	my ($x, $y) = @$box[0,1];
	# height
	$cache-> [2] = sqrt(($dx - $x) * ($dx - $x) + ($dy - $y) * ($dy - $y));
	($x, $y) = @$box[6,7];
	# width
	$cache-> [3] = sqrt(($dx - $x) * ($dx - $x) + ($dy - $y) * ($dy - $y));
#print "precalc box @$box to $cache->[3],$cache->[2] shift $dx,$dy rotated to $angle\n";
	$angle *= -3.14159265358/180;
	$cache-> [4] = sin($angle);
	$cache-> [5] = cos($angle);
}

# check whether rotated rectangle $b is inside box $a.
# $a is calculated from a rotated rectangle box by precalc_box()
sub boxes_overlap
{
	my ( $a, $b) = @_;

#print "overlap? @$b\n";
	my (@b,$i);
	my ($dx,$dy,$h,$w,$sin,$cos) = @$a;

	# rotate and shift $b
	for ( $i = 0; $i < 8; $i+=2) {
		my ( $x, $y) = @$b[$i,$i+1];
		$x -= $dx;
		$y -= $dy;
		my $X = $x * $cos - $y * $sin;
		my $Y = $x * $sin + $y * $cos;
		# check immediately if the point is inside the box
		if (
			( $X >= 0 and $X < $w) and
			( $Y >= 0 and $Y < $h)
		) {
#print "point $X,$Y is inside 0,0-$w,$h\n";
			return 1;
		}
		@b[$i,$i+1] = ($X,$Y);
	}

	# check whether any segment that forms @b intesects with $a

	# reshuffle order of get_text_box() points so [0] is lower left, [1] is upper left, [2] is upper right
	# also, point 8,9->0,1 for easier looping
	@b[0..3,8,9] = @b[2,3,0..3];
	for ( $i = 0; $i < 8; $i +=2 ) {
		my ($x1, $y1, $x2, $y2) = @b[$i .. $i + 3];
		my ( $dx, $dy) = ( $x2 - $x1, $y2 - $y1);

		# check intersections with vertical axes
		if ( $dx != 0) {
			my $tangent = $dy / $dx;
			for (0, $w) {
				next if
					( $_ > $x1 and $_ > $x2) or
					( $_ < $x1 and $_ < $x2);
				my $p = $y2 - $tangent * ( $x2 - $_ );
				next unless $p >= 0 and $p < $h;
#print "segment $x1,$y1-$x2,$y2 crosses vertical line x=$_ at y=0<=$p=>$h\n";
				return 1;
			}
		}
		# check intersections with horizontal axes
		if ( $dy != 0) {
			my $tangent = $dx / $dy;
			for (0, $h) {
				next if
					( $_ > $y1 and $_ > $y2) or
					( $_ < $y1 and $_ < $y2);
				my $p = $x2 - $tangent * ( $y2 - $_ );
				next unless $p >= 0 and $p < $w;
#print "segment $x1,$y1-$x2,$y2 crosses horizontal line y=$_ at x=0<=$p=>$w\n";
				return 1;
			}
		}
	}

	return 0;
}

sub curved_text_out
{
	my ( $self, $text, $polyline, %options) = @_;

	return unless 4 == grep { defined } @$polyline[0..3];

	my $retval     = 1;
	my $fa         = $self-> font-> direction;

	my $collisions = $options{collisions} || 0;
	my $bevel      = not (exists $options{bevel})      || $options{bevel};

	my $offset = $options{offset} || 0;
	my $p = init_pointer( $polyline, $offset >= 0);
	move_pointer( $p, $offset);

	my ( @chunks, $try_text_wrap, $angle, @box);
	my ( %start, @walkback, @translated_box, @all_boxes, $fitting_direction); # collision detection

	$try_text_wrap = 1;
	@box[8,9] = ( $p->{x}, $p->{y});
	push @all_boxes, \@translated_box if $collisions == 1;

	$text = Prima::Bidi::visual($text) if $Prima::Bidi::enabled;

	while ( not $p-> {end} and length ($text) ) {
		# Try to fit next glyphs in the string. We don't know whether more than 1 glyph
		# fits, but if yes, text_wrap() will speed up things a lot. Otherwise, fit each
		# character individually
		my ( $t);
		my ( $x, $y) = @$p{qw(x y)};
#print "* point $x $y\n";
		# obtain next position
		if ( $try_text_wrap) {
			my $chunk = $self-> text_wrap(
				$text, $p-> {lleft},
				tw::BreakSingle|tw::ReturnFirstLineLength
			);
			$t = substr( $text, 0, $chunk, '');
			unless ( $collisions) {
#print "'$t' text_wrap plot at $x,$y,$p->{a}\n" if $chunk;
				push @chunks, [ $t, $p->{a}, $x, $y]
					if $chunk;
				unless ( $bevel) {
					# simple case
					update_box( $self, $x, $y, $angle = $p->{a}, \$t, \@box);
					set_pointer( $p, @box[8,9]);
					next_segment($p);
					next;
				}
			}
			unless ( $chunk) {
				$try_text_wrap = 0;
				goto SINGLE_GLYPH;
			}
			update_box( $self, $x, $y, $angle = $p->{a}, \$t, \@box);
			set_pointer( $p, @box[8,9]);
#print "text_wrap '$t' move to $p->{x},$p->{y}\n";
		} else {
		SINGLE_GLYPH:
			$t = substr( $text, 0, 1, '');
			my ( $a, $b, $c) = @{ $self-> get_font_abc(
				ord($t), ord($t),
				utf8::is_utf8($t)
			)};
			my $w  = $a + $b + $c;
			$angle = move_pointer( $p, $w);
			update_box( $self, $x, $y, $angle, \$t, \@box);
#print "'$t' single, move to $p->{x},$p->{y}\n";
			unless ( $collisions) {
				push @chunks, [ $t, $angle, $x, $y ];
				$try_text_wrap = 1;
#print "'$t' bevel plot at $x,$y,".(defined($angle)?$angle:"undef")."\n";
			}
		}
		next unless $collisions;

		# first glyphs don't need to be checked for collisions
		unless ( @translated_box) {
			%start = %$p;
			precalc_box( $angle, \@box, \@translated_box);
			push @all_boxes, [@translated_box] if $collisions > 1;
			push @chunks, [ $t, $angle, $x, $y ];
#print "plot '$t' at $x,$y,$angle init non-overlap\n";
			next;
		}

		# if glyphs overlap, move the pointer forwards,
		# otherwise backwards until the overlapping occurs
		my $start_direction = -1;
		for ( @all_boxes) {
			next unless boxes_overlap( $_, \@box);
			$start_direction = 1;
			last;
		}
		$fitting_direction =
			(
				not defined($fitting_direction) or
				$fitting_direction == $start_direction
			) ?
				$start_direction :
				undef
			;
		if ( defined $fitting_direction) {
			# retry
			@walkback = ($x, $y, $angle)
				if $fitting_direction < 0;
			move_pointer( \%start, $fitting_direction);
			%$p = %start;
#print "retry direction=$start_direction from $start{x} $start{y}\n";
			$text = $t . $text;
		} else {
			# done
			if ( $start_direction > 0) {
				# was moving backwards
				($x, $y, $angle) = @walkback;
			}
			$try_text_wrap = 1;
			%start = %$p;
#print "plot '$t' at $x,$y,$angle non-overlap\n";
			push @chunks, [ $t, $angle, $x, $y];
			precalc_box( $angle, \@box, \@translated_box);
			push @all_boxes, [@translated_box] if $collisions > 1;
			%start = %$p;
		}
	}
	if ( length $text and not $options{skiptail}) {
		$angle = $p-> {a} unless $bevel;
#print "'$text' at @box[8,9] ".(defined($angle)?$angle:"undef")." tail\n";
		push @chunks, [ $text, $angle, @box[8,9]];
	}

	$options{callback}->( $self, $p, \@chunks)
		if $options{callback};

	unless ( $options{nodraw}) {
		for ( @chunks) {
			my ( $text, $angle, $x, $y) = @$_;
			$self-> font-> direction($angle) if defined $angle;
			last unless $retval = $self-> text_out( $text, $x, $y);
		}
	}
	$self-> font-> direction($fa);

	return wantarray ? @chunks : $retval;
}

1;

__END__

=pod

=head1 NAME

Prima::Drawable::CurvedText - fit text to path

=head1 DESCRIPTION

The module registers single function C<curved_text_out> in C<Prima::Drawable>
namespace. The function plots the line of text along the path, which given as a
set of points. Various options regulate behavior of the function when glyphs
collide with the path boundaries and each other.

=head1 SYNOPSIS

  use Prima qw(Application Drawable::CurvedText);
  $spline = [qw(100 100 150 150 200 100)];
  $::application-> begin_paint;
  $::application-> spline($spline);
  $::application-> curved_text_out( 'Hello, world!', $::application-> render_spline( $spline ));

=for podview <img src="Prima/curvedtext.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/curvedtext.gif">

=head2 curved_text_out $TEXT, $POLYLINE, %OPTIONS

C<$TEXT> is a line of text, no special treatment is given to tab and newline characters.
The text is plotted over C<$POLYLINE> path that should be an array of coordinate
numeric pairs, in the same format as C<Prima::Drawable::polyline> expects.

The text begins to plot by drawing the first glyphs at the first path point, unless
specified otherwise with the C<offset> option. The glyph is plotted with the angle
perpendicular to the path segment; therefore the path may contain floating point numbers if
futher plotting angle accuracy is desired.

When text cannot be fit along a single segment, it is plotted along the next segment in the
path. Depending on the C<bevel> boolean option, the next glyph is either simply drawn on
the next segment with the angle corresponding to the tangent of that segment (value 0), or
is drawn with the normal text concatenation offset, with the angle averaged between
tangents of the two segments it is plotted between (value 1). The default value of
C<bevel> option is 1.

The glyph positioning rules differ depending on C<collisions> integer option. If
0 (default), the next glyph position always corresponds with the glyph width as
projected to the path. That means, that glyphs will overlap when plotted inside
segments forming an acute angle. Also, when plotting along a reflex angle, the
glyphs will be visually more distant from each other that when plotted along
the straight line.

Simple collision detection can be turned on with setting C<collisions> to 1 so
that no two neighbour glyphs may overlap. Also, the glyphs will be moved
together to the minimal distance, when possible. With this option set the
function will behave slower. If detection of not only neighbouring glyphs is
required, C<collisions> value can be set to 2, in which case a glyph is
guaranteedly will never overlap any other glyph.  This option may be needed
when, for example, text is plotted inside an acute angle and upper parts of
glyphs plotted along one segment will overlap with lower parts of glyphs
plotted along the other one.  Setting C<collisions> to 2 will slow the function
even more.

The function internally creates an array of tuples where each contains text,
plotting angle, and horisontal and vertical coordinates for the text to be
plotted. In the array context the function returns this array. In the scalar
context the function returns the success flag that is the result of last call
to C<text_out>.

Options:

=over

=item bevel BOOLEAN=true

If set, glyphs between two adjoining segments will be plotted with bevelled angle.
Otherwise glyphs will strictly follow the angles of the segments in the path.

=item callback CODE($SELF, $POLYLINE, $CHUNKS)

If set, the callback is called with C<$CHUNKS> after the calculations were made
but before the text is plotted. C<$CHUNKS> is an array of tuples where each
consists of text, angle, x and y coordinates for each text. The callback is
free to modify the array.

=item collisions INTEGER=0

If 0, collision detection is disabled, glyphs plotted along the path. If 1,
no two neighbour glyphs may overlap, and no two neighbour glyph will be
situated further away from each other than it is necessary. If 2, same
functionality as with 1, and also two glyphs (in all text) will overlap.

=item nodraw BOOLEAN=false

If set, calculate glyph positions but do not draw them.

=item offset INTEGER=0

Sets offset from the beginning of the path where the first glyph is plotted.
If offset is negative, it is calculated from the end of the path.

=item skiptail BOOLEAN=false

If set, the remainder of the text that is left after the path is completely
traversed, is not shown. Otherwise (default), the tail text is shown with the
angle used to plot the last glyph (if bevelling was requested) or the angle
perpendicular to the last path segment (otherwise).

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Drawable>

=cut
