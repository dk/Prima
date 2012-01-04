package Prima::RubberBand;

use strict;
use warnings;

sub new
{
	my ( $class, %profile ) = @_;
	my $ref = {
	        mode    => 'auto',  # auto, full, xor
		canvas  => $::application,
		rect    => [-1,-1,-1,-1],
		breadth => 1,
	};
	my $self = bless $ref, shift;
	$self-> set(%profile);
	$self-> show;
	return $self;
}

sub set
{
	my ( $self, %profile ) = @_;
	my $visible = $self-> {visible};
	$self-> hide if $visible;
	$self-> $_( @{$profile{$_}} ) for grep { exists $profile{$_} } qw(rect);
	$self-> $_( $profile{$_}    ) for grep { exists $profile{$_} } qw(mode canvas breadth);
	$self-> show if $visible;
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

sub _gfx_mode
{
	my $self = shift;
	if ( $self-> {mode} eq 'auto') {
		return (
			$^O =~ /win32/i && 
			$::application-> get_system_value( sv::CompositeDisplay ) &&
			$self-> {canvas} && 
			$self-> {canvas}-> isa('Prima::Widget')
		) ? 1 : 0;
	} else {
		return ( $self-> {mode} eq 'full' ) ? 1 : 0;
	}
}

sub canvas
{
	return $_[0]-> {canvas} unless $#_;

	my ( $self, $canvas ) = @_;
	$self-> {canvas} = $canvas // $::application;
}

sub mode
{
	return $_[0]-> {mode} unless $#_;

	my ( $self, $mode ) = @_;
	Carp::confess "mode(auto,full,xor)" unless $mode =~ /^(auto|full|xor)$/;
	$self-> {mode} = $mode;
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

	($rect[2],$rect[0]) = ($rect[0],$rect[2]) if $rect[2] < $rect[0];
	($rect[3],$rect[1]) = ($rect[1],$rect[3]) if $rect[3] < $rect[1];
	@{$self-> {rect}} = @rect;
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

	my ( $self, $visible ) = @_;
	$visible = ( $visible ? 1 : 0 );
	my $curr_visible = ( $self-> {visible} ? 1 : 0);
	return if $visible == $curr_visible;
	return unless $self-> {canvas};

	$self-> {visible} = $visible;

	my $canvas = $self-> {canvas};
		
	# just a regular xor
	unless ( $self-> _gfx_mode ) {
		$canvas-> rect_focus( $self-> rect, $self-> breadth );
		return;
	}

	# save all bits under the rect
	if ( $visible ) {
		my @desktop = $::application-> size;
		@desktop = ( 0, 0, $desktop[0] - 1, $desktop[1] - 1);

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

		@requests = grep { _intersect( $_, \@desktop ) } @requests;
		$self-> {_cache} = [];

		return unless @requests;
		
		for ( @requests ) {
			my ( $x1, $y1, $x2, $y2) = @$_;
			push @{ $self-> {_cache} }, [
				$x1 - $delta[0], 
				$y1 - $delta[1], 
				$::application-> get_image( $x1, $y1, $x2 - $x1 + 1, $y2 - $y1 + 1 )
			];
		}

		my ( $cl, $cl2, $rop, $fp) = ( $canvas-> color, $canvas-> backColor, $canvas-> rop, $canvas-> fillPattern);
		$canvas-> set(
			fillPattern => fp::SimpleDots,
			color       => cl::Set,
			backColor   => cl::Clear,
			rop         => rop::XorPut,
		);

		for ( @requests ) {
			my ( $x1, $y1, $x2, $y2) = @$_;
			$canvas-> bar(
				$x1 + $delta[0],
				$y1 + $delta[1],
				$x2 + $delta[0],
				$y2 + $delta[1]
			);
		}

		$canvas-> set(
			fillPattern => $fp,
			backColor   => $cl2,
			color       => $cl,
			rop         => $rop,
		);
	} else {
		# restore bits
		# $canvas-> rectangle( $_->[0], $_-> [1], $_->[0] - 1 + $_->[2]-> width, $_[1] - 1 + $_-> [2]-> height) for @{ $self-> {_cache} };
		$canvas-> put_image( @$_) for @{ $self-> {_cache} };
		$self-> {_cache} = [];
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
			$self-> {__rubberband} = Prima::RubberBand-> new(%profile);
		}
	}

	return $self-> {__rubberband};
}

1;

__DATA__

=pod

=head1 NAME

Prima::RubberBand - draw rubberbands 

=head1 DESCRIPTION

The motivation for this module was that I was tired to see corrupted screens on
Windows 7 when dragging rubberbands in Prima code. Even though MS somewhere
warned of not doing any specific hacks to circumvent the bug, I decided to give
it a go anyway.

This module thus is a C<Prima::Widget/rect_focus> with a safeguard. The only
thing it can do is to draw a static rubberband - but also remember the last
coordinates drawn, so cleaning comes for free.

The idea is that a rubberband object is meant to be a short-lived one: as soon
as it get instantiatet it draws itself on the screen. When it is destroyed, the
rubberband is erased too.

=head1 SYNOPSIS

	use strict;
	use Prima qw(Application RubberBand);
	
	sub xordraw
	{
		my ($self, @new_rect) = @_;
		my $o = $::application;
		$o-> begin_paint;
		$o-> rubberband( @new_rect ?
			( rect => \@new_rect ) :
			( destroy => 1 )
		);
		$o-> end_paint;
	}
	
	Prima::MainWindow-> create(
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
	
	run Prima;

=head1 API

=head2 Properties

=over

=item breadth INTEGER = 1

Defines rubberband breadth, in pixels.

=item canvas = $::application

Sets the painting surface, and also the widget (it must be a widget) used for drawing.

=item mode STRING = 'auto'

The module implements two techniques, standard classic 'xor' (using .rect_focus method) 
and a conservative method that explicitly saves and restores desktop pixels ('full').
The 'auto' mode checks the system and selects the appropriate mode.

Allowed modes: auto, xor, full

=item rect X1, Y1, X2, Y2

Defines the band geometry, in inclusive-inclusive coordinates. The band is drawn so that its body
is always inside these coordinates, no matter what breadth is.

=back

=head2 Methods

=over

=item hide

Hides the band, if drawn

=item set %profile

Applies all properties

=item left, right, top, bottom, width, height, origin, size

Same shortcuts as in C<Prima::Widget>, but read-only.

=back

=head1 Prima::Widget interface

The module adds a single method to C<Prima::Widget> namespace, C<rubberband>
(see example of use in the synopsis).

=over

=item rubberband(%profile)

Instantiates a C<Prima::RubberBand> with C<%profile>, also sets C<canvas> to C<$self>
unless C<canvas> is set explicitly.

=item rubberband()

Returns the existing C<Prima::RubberBand> object

=item rubberband(destroy => 1)

Destroys the existing C<Prima::RubberBand> object

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

This quote seems to explain the effect why screen sometimes gets badly
corrupted when using a normal xor rubberband. UCE ( Update Compatibility
Evaluator ?? ) seems to be hacky enough to recognize some situations, but not
all.  It seems that depending on which widget received mouse button just before
initialting rubberband drawing matters somehow. Anyway, the module tries to
see if we're under Windows 7 aero, and if so, turns the 'full' mode on.

=cut

