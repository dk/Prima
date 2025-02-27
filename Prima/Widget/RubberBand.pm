package Prima::Widget::RubberBand;

use strict;
use warnings;
use Prima;

sub new
{
	my ( $class, %profile ) = @_;
	my $ref = {
		canvas   => $::application,
		rect     => [-1,-1,-1,-1],
		clipRect => [-1,-1,-1,-1],
		breadth  => 1,
	};
	my $self = bless $ref, shift;
	$self-> set(%profile);
	$self-> show;
	return $self;
}

sub _normalize_rect
{
	my $rect = shift;
	($$rect[2],$$rect[0]) = ($$rect[0],$$rect[2]) if $$rect[2] < $$rect[0];
	($$rect[3],$$rect[1]) = ($$rect[1],$$rect[3]) if $$rect[3] < $$rect[1];
}

sub _rect_changed
{
	my ( $a, $b ) = @_;
	my @r1 = @$a;
	my @r2 = @$b;
	_normalize_rect(\@r1);
	_normalize_rect(\@r2);
	return
		$r2[0] != $r1[0] ||
		$r2[1] != $r1[1] ||
		$r2[2] != $r1[2] ||
		$r2[3] != $r1[3];
}

sub set
{
	my ( $self, %profile ) = @_;
	my $visible = $self-> {visible};

	my ($rect_changed, $other_changed);
	if ( exists $profile{rect} ) {
		$rect_changed = 1 if _rect_changed($profile{rect}, $self->{rect});
	}
	if ( exists $profile{clipRect} ) {
		$other_changed = 1 if _rect_changed($profile{clipRect}, $self->{clipRect});
	}
	for my $accessor (grep { exists $profile{$_} } qw(canvas breadth)) {
		my $old = $self->$accessor();
		next if $old eq $profile{$accessor};
		$other_changed = 1;
		last;
	}
	$other_changed = 1 if $rect_changed and !$visible;
	$rect_changed  = 0 if $other_changed;
	return unless $rect_changed or $other_changed;

	if ( $other_changed ) {
		$self-> hide if $visible;
		$self-> $_( @{$profile{$_}} ) for grep { exists $profile{$_} } qw(rect clipRect);
		$self-> $_( $profile{$_}    ) for grep { exists $profile{$_} } qw(canvas breadth);
		$self-> show if $visible;
	} elsif ( $rect_changed ) {
		$self->{visible} = 0;
		$self->rect( @{ $profile{rect} });
		$self->_visible(1, 1);
	}
}

sub DESTROY { shift-> hide }

sub show
{
	my $self = shift;
	$self-> _visible(1) unless $self-> _visible;
}

sub hide
{
	my $self = shift;
	$self-> _visible(0) if $self-> _visible;
}

sub canvas
{
	return $_[0]-> {canvas} unless $#_;

	my ( $self, $canvas ) = @_;
	$self-> {canvas} = $canvas // $::application;
}

# geometry handlers

sub left   { $_[0]-> {rect}-> [0] }
sub bottom { $_[0]-> {rect}-> [1] }
sub right  { $_[0]-> {rect}-> [2] }
sub top    { $_[0]-> {rect}-> [3] }
sub width  { $_[0]-> {rect}-> [2] - $_[0]-> {rect}-> [0] + 1 }
sub height { $_[0]-> {rect}-> [3] - $_[0]-> {rect}-> [1] + 1 }
sub origin { $_[0]-> {rect}-> [0], $_[0]-> {rect}-> [1] }
sub size   { $_[0]-> {rect}-> [2] - $_[0]-> {rect}-> [0] + 1 , $_[0]-> {rect}-> [3] - $_[0]-> {rect}-> [1] +  1 }

sub rect
{
	return @{$_[0]-> {rect}} unless $#_;

	my ( $self, @rect ) = @_;
	Carp::confess("@rect") unless 4 == grep { defined } @rect;

	_normalize_rect(\@rect);
	@{$self-> {rect}} = @rect;
}

sub clipRect
{
	return @{$_[0]-> {clipRect}} unless $#_;

	my ( $self, @rect ) = @_;
	Carp::confess("@rect") unless 4 == grep { defined } @rect;

	_normalize_rect(\@rect);
	@{$self-> {clipRect}} = @rect;
}

sub has_clip_rect
{
	my $self = shift;
	return 4 != grep { $_ == -1 } @{ $self->{clipRect} };
}

sub breadth
{
	return $_[0]-> {breadth} unless $#_;

	my ( $self, $breadth ) = @_;
	$breadth = 1 if $breadth < 1;
	$breadth = 64 if $breadth > 64; # well, huh?
	$self-> {breadth} = $breadth;
}

# drawing handlers

sub _intersect
{
	my ( $rect, $outer ) = @_;

	return if
		$rect-> [0] > $outer-> [2] ||
		$rect-> [1] > $outer-> [3] ||
		$rect-> [2] < $outer-> [0] ||
		$rect-> [3] < $outer-> [1];

	my @res = @$rect;
	$res[0] = $outer-> [0] if $res[0] < $outer-> [0];
	$res[1] = $outer-> [1] if $res[1] < $outer-> [1];
	$res[2] = $outer-> [2] if $res[2] > $outer-> [2];
	$res[3] = $outer-> [3] if $res[3] > $outer-> [3];

	return \@res;
}

sub _visible
{
	return $_[0]-> {visible} unless $#_;

	my ( $self, $visible, $optimized_rect_change ) = @_;
	$visible = ( $visible ? 1 : 0 );
	my $curr_visible = ( $self-> {visible} ? 1 : 0);
	return if $visible == $curr_visible;
	return unless $self-> {canvas};

	$self-> {visible} = $visible;

	my $canvas = $self-> {canvas};
	if ( $visible ) {
		my @clip = $self-> has_clip_rect ?
			$canvas-> client_to_screen( $self-> clipRect ) :
			( 0, 0, $::application->width - 1, $::application-> height - 1 );

		my @outer = $self-> rect;
		my @delta = $canvas-> client_to_screen(0,0);
		$outer[$_] += $delta[0] for 0,2;
		$outer[$_] += $delta[1] for 1,3;

		my @inner   = @outer;
		my $breadth = $self-> {breadth};
		$inner[$_] += $breadth - 1 for 0,1;
		$inner[$_] -= $breadth - 1 for 2,3;

		# save bits:
		#  11111111
		#  22    33
		#  00000000

		my @requests;
		if ( $inner[0] >= $inner[2] || $inner[1] >= $inner[3] ) {
			push @requests, [ @outer ];
		} else {
			push @requests, [ $outer[0], $outer[1], $outer[2], $inner[1] ];
			push @requests, [ $outer[0], $inner[3], $outer[2], $outer[3] ];
			push @requests, [ $outer[0], $inner[1] + 1, $inner[0], $inner[3] - 1 ];
			push @requests, [ $inner[2], $inner[1] + 1, $outer[2], $inner[3] - 1 ];
		}
		@requests = map { _intersect( $_, \@clip ) } @requests;

		goto LEAVE unless @requests;

		_normalize_rect($_) for @requests;

		unless ( $self->{_widgets} ) {
			push @{ $self-> {_widgets} }, Prima::Widget->new(
				origin      => [ 0, 0 ],
				size        => [ 1, 1 ],
				owner       => $::application,
				color       => cl::White,
				backColor   => cl::Black,
				visible     => 0,
				onPaint     => sub {
					my ( $self, $canvas ) = @_;
					$canvas->rop2(rop::CopyPut);
					$canvas->fillPattern(
						($self->left % 2) ?
							[ (0xaa, 0x55) x 4 ] :
							[ (0x55, 0xaa) x 4 ]
					);
					$canvas->bar(0,0,$self->size);
				},
			) for 1..4;
		}

		my $i = 0;
		for ( @requests ) {
			my ( $x1, $y1, $x2, $y2) = @$_;
			my $w = $x2 - $x1 + 1;
			my $h = $y2 - $y1 + 1;
			$self->{_widgets}->[$i++]->set(
				origin  => [ $x1, $y1 ],
				size    => [ $w, $h ],
				visible => 1,
			);
		}

	LEAVE:
		return unless $self->{_widgets};
		if ( $optimized_rect_change ) {
			for ( my $i = 0; $i < 4; $i++) {
				$self->{_widgets}->[$i]->visible( defined $requests[$i] );
			}
		}
		for ( my $i = 0; $i < 4; $i++) {
			$self->{_widgets}->[$i]->bring_to_front;
		}
	} else {
		return unless $self->{_widgets};
		$_->hide for @{$self->{_widgets}};
	}
}

package Prima::Widget;

sub rubberband
{
	my ($self, %profile) = @_;

	if ($profile{destroy}) {
		$self-> {__rubberband}-> hide if $self-> {__rubberband};
		return delete $self-> {__rubberband}
	}

	if ( keys %profile) {
		if ( $self-> {__rubberband}) {
			$self-> {__rubberband}-> set(%profile);
		} else {
			$profile{canvas} //= $self;
			$self-> {__rubberband} = Prima::Widget::RubberBand-> new(%profile);
		}
	}

	return $self-> {__rubberband};
}

1;

=pod

=head1 NAME

Prima::Widget::RubberBand - dynamic rubberbands

=head1 DESCRIPTION

The motivation for this module was that I was tired of seeing corrupted screens on
Windows 7 when dragging rubberbands in Prima code. Even though MS somewhere
warned of not doing any specific hacks to circumvent the bug, I decided to give
it a go anyway.

This module thus is a C<Prima::Widget/rect_focus> with a safeguard. The only
thing it can do is to draw a static rubberband - but also remember the last
coordinates drawn, so cleaning and animation come for free.

The idea is that a rubberband object is meant to be a short-lived one: as soon
as it gets instantiated it draws itself on the screen. When it is destroyed, the
rubberband is erased too.

=head1 SYNOPSIS

	use strict;
	use Prima qw(Application Widget::RubberBand);

	sub xordraw
	{
		my ($self, @new_rect) = @_;
		$::application-> rubberband( @new_rect ?
			( rect => \@new_rect ) :
			( destroy => 1 )
		);
	}

	Prima::MainWindow-> new(
		onMouseDown => sub {
			my ( $self, $btn, $mod, $x, $y) = @_;
			$self-> {anchor} = [$self-> client_to_screen( $x, $y)];
			xordraw( $self, @{$self-> {anchor}}, $self-> client_to_screen( $x, $y));
			$self-> capture(1);
		},
		onMouseMove => sub {
			my ( $self, $mod, $x, $y) = @_;
			xordraw( $self, @{$self-> {anchor}}, $self-> client_to_screen( $x, $y)) if $self-> {anchor};
		},
		onMouseUp => sub {
			my ( $self, $btn, $mod, $x, $y) = @_;
			xordraw if delete $self-> {anchor};
			$self-> capture(0);
		},
	);

	Prima->run;

=head1 API

=over

=item new %properties

Creates a new RubberBand instance. See the description of its properties below.

=back

=head2 Properties

=over

=item breadth INTEGER = 1

Defines the rubberband breadth in pixels.

=item canvas = $::application

Sets the painting surface, and also the widget (it must be a widget) used for drawing.

=item clipRect X1, Y1, X2, Y2

Defines the clipping rectangle in inclusive-inclusive coordinates. If set to [-1,-1,-1,-1],
means no clipping is needed.

=item rect X1, Y1, X2, Y2

Defines the band geometry in inclusive-inclusive coordinates. The band is drawn so that its body
is always inside these coordinates, no matter what the breadth is.

=back

=head2 Methods

=over

=item hide

Hides the band

=item has_clip_rect

Checks whether clipRect contains an actual clipping rectangle or it is empty.

=item set %profile

Applies all properties

=item left, right, top, bottom, width, height, origin, size

The same shortcuts as in C<Prima::Widget>, but read-only.

=item show

Shows the band

=back

=head1 Prima::Widget interface

The module adds a single method to the C<Prima::Widget> namespace, C<rubberband>
(see example of use in the synopsis).

=over

=item rubberband(%profile)

Instantiates a C<Prima::RubberBand> object with C<%profile>, also sets C<canvas> to C<$self>
( unless C<canvas> is set explicitly ).

=item rubberband()

Returns the existing C<Prima::RubberBand> object

=item rubberband(destroy => 1)

Destroys the existing C<Prima::Widget::RubberBand> object

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Widget/rect_focus>, L<examples/grip.pl>

=head2 Windows 7 Aero mode

Quote from L<http://blogs.msdn.com/b/greg_schechter/archive/2006/05/02/588934.aspx> :

"One particularly dangerous practice is writing to the screen, either through
the use of GetDC(NULL) and writing to that, or attempting to do XOR rubber-band
lines, etc  ...  Since the UCE doesn't know about it, it may get cleared in the
next frame refresh, or it may persist for a very long time, depending on what
else needs to be updated on the screen.  (We really don't allow direct writing
to the primary anyhow, for that very reason... if you try to access the
DirectDraw primary, for instance, the DWM will turn off until the accessing
application exits)"

This quote seems to explain the effect of why the screen sometimes gets badly
corrupted when using a normal xor rubberband. UCE ( Update Compatibility
Evaluator ?? ) seems to be hacky enough to recognize some situations, but not
all.

=cut

