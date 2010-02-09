#  Copyright (c) 2008 Dmitry Karasik
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
# $Id$
package Prima::Image::AnimateGIF;

use strict;
use warnings;
use Carp;
use Prima;

use constant DISPOSE_NOT_SPECIFIED    => 0; # Leave frame, let new frame draw on top
use constant DISPOSE_KEEP             => 1; # Leave frame, let new frame draw on top
use constant DISPOSE_CLEAR            => 2; # Clear the frame's area, revealing bg
use constant DISPOSE_RESTORE_PREVIOUS => 3; # Restore the previous (composited) frame

sub new
{
	my $class = shift;
	my $self = bless {
		images     => [],
		@_,
		current    => -1,
	}, $class;

	$self-> reset;

	return $self;
}

sub load
{
	my $class = shift;

	my ( $where, %opt) = @_;

	# have any custom notifications?
	my ( %events, %args);
	while ( my ( $k, $v) = each %opt) {
		my $hash = ($k =~ /^on[A-Z]/ ? \%events : \%args);
		$hash-> {$k} = $v;
	}
	
	my $i = Prima::Icon-> new(%events); # dummy object

	my @i = $i-> load(
		$where,
		loadExtras => 1, 
		loadAll    => 1, 
		iconUnmask => 1,
		%args,
	);

	return unless @i;

	return $class-> new( images => \@i);
}

sub add
{
	my ( $self, $image) = @_;
	push @{$self-> {images}}, $image;
}

sub get_extras
{
	my ( $self, $ix) = @_;
	$ix = $self-> {images}-> [$ix];
	return unless $ix;

	my $e = $ix-> {extras} || {};

	$e-> {screenHeight}     ||= $ix-> height;
	$e-> {screenWidth}      ||= $ix-> width;
	$e-> {$_} ||= 0 for qw(disposalMethod useScreenPalette delayTime left top);

	$e-> {iconic} = 
		$ix-> isa('Prima::Icon') 
		&& $ix-> autoMasking != am::None; 
		# gif doesn't support explicit masks, therefore
		# when image actually has a mask, autoMaskign is set to am::Index

	return $e;
}

sub fixup_rect
{
	my ( $self, $info, $image) = @_;
	return if defined $info-> {rect};
	$info-> {rect} = {
		bottom => $self-> {screenHeight} - $info-> {top} - $image-> height,
		top    => $self-> {screenHeight} - $info-> {top} - 1,
		right  => $info-> {left} + $image-> width - 1,
		left   => $info-> {left},
	};
}

sub union_rect
{
	my ( $r1, $r2) = @_;
	return { %$r2 } unless grep { $r1-> {$_} } qw(left bottom right top);
	return { %$r1 } unless grep { $r2-> {$_} } qw(left bottom right top);

	my %ret = %$r1;


	for ( qw(left bottom)) {
		$ret{$_} = $r2-> {$_}
			if $ret{$_} > $r2-> {$_};
	}
	for ( qw(right top)) {
		$ret{$_} = $r2-> {$_}
			if $ret{$_} < $r2-> {$_};
	}

	return \%ret;
}

sub reset
{
	my $self = shift;
	$self-> {current} = -1;

	delete @{$self}{qw(canvas bgColor saveCanvas 
		saveMask image info 
		screenWidth screenHeight
		loopCount changedRect
		)};

	my $i = $self-> {images};
	return unless @$i;

	my $ix = $i-> [0];
	return unless $ix;

	my $e = $self-> get_extras(0);
	return unless $e;

	$self-> {image} = $self-> {images}-> [0];
	$self-> {info}  = $e;
	$self-> {$_} = $e-> {$_} for qw(screenWidth screenHeight);
	$self-> {changedRect} = {};
	$self-> fixup_rect( $e, $ix);

	# create canvas and mask
	$self-> {canvas}  = Prima::DeviceBitmap-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		monochrome => 0,
		backColor  => 0,
	);
	$self-> {canvas}-> clear; # canvas is all-0 initially

	$self-> {mask}    = Prima::DeviceBitmap-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		monochrome => 1,
		backColor  => 0xFFFFFF,
		color      => 0x000000,
	);
	$self-> {mask}-> clear; # mask is all-1 initially

	if ( defined $e-> {screenBackGroundColor}) {
		my $cm =
			$e-> {useScreenPalette} ?
				$e-> {screenPalette} :
				$self-> {images}-> [0]-> palette;
		my $i = $e-> {screenBackGroundColor} * 3;
		$self-> {bgColor} = (
			($$cm[$i+2] || 0) | 
			(($$cm[$i+1] || 0) << 8) | 
			(($$cm[$i] || 0) << 16)
		);
	}
}

sub next
{
	my $self = shift;

	my $info = $self-> {info};
	return unless $info;

	my @sz = ( $self-> {screenWidth}, $self-> {screenHeight});
	my %ret;

	# dispose from the previous frame and calculate the changed rect
	if ( $info-> {disposalMethod} == DISPOSE_CLEAR) {
		$self-> {canvas}-> backColor( 0);
		$self-> {canvas}-> clear;
		$self-> {mask}-> backColor(cl::Set);
		$self-> {mask}-> clear;
		
		%ret = %{ $self-> {changedRect} };
		$self-> {changedRect} = {};
	} elsif ( $info-> {disposalMethod} == DISPOSE_RESTORE_PREVIOUS) {
		# cut to the previous frame, that we expect to be saved for us
		if ( $self-> {saveCanvas} && $self-> {saveMask}) {
			$self-> {canvas} = $self-> {saveCanvas};
			$self-> {mask}   = $self-> {saveMask};
		}
		$self-> {changedRect} = $self-> {saveRect};
		delete $self-> {saveCanvas};
		delete $self-> {saveMask};
		delete $self-> {saveRect};
		%ret = %{ $info-> {rect} };
	}
	
	# advance frame
	delete @{$self}{qw(image info)};
	if ( ++$self-> {current} >= @{$self-> {images}}) {
		# go back to first frame, or stop
		if ( defined $self-> {loopCount}) {
		    return if --$self-> {loopCount} <= 0;
		}
		$self-> {current} = 0;
	}
	$self-> {image} = $self-> {images}-> [$self-> {current}];
	my $old_info = $info;
	$info = $self-> {info} = $self-> get_extras( $self-> {current} );
	$self-> fixup_rect( $info, $self-> {image}); 
	my @is = $self-> {image}-> size;

	# load global extension data
	if ( $self-> {current} == 0) {
		unless ( defined $info-> {loopCount}) {
			$self-> {loopCount} = 1;
		} elsif ( $info-> {loopCount} == 0) {
			# loop forever
			$self-> {loopCount} = undef;
		} else {
			$self-> {loopCount} = $info-> {loopCount};
		}
	}

	# remember the background, if needed, and again update the # rect
	if ( $info-> {disposalMethod} == DISPOSE_RESTORE_PREVIOUS) {
		my $c  = Prima::DeviceBitmap-> new(
			width      => $sz[0],
			height     => $sz[1],
			monochrome => 0,
		);
		$c-> put_image( 0, 0, $self-> {canvas});
		$self-> {saveCanvas} = $self-> {canvas};
		$self-> {canvas} = $c;

		$c = Prima::DeviceBitmap-> new(
			width      => $sz[0],
			height     => $sz[1],
			monochrome => 1,
		);
		$c-> put_image( 0, 0, $self-> {mask});
		$self-> {saveMask} = $self-> {mask};
		$self-> {mask} = $c;

		$self-> {saveRect} = $self-> {changedRect};
	}
	$self-> {changedRect} = union_rect( $self-> {changedRect}, $info-> {rect});
	%ret = %{ union_rect( \%ret, $info-> {rect}) };

	# draw the current frame
	if ( $info-> {iconic}) {
		my ( $xor, $and) = $self-> {image}-> split;
		# combine masks
		$self-> {mask}-> set(
			color     => cl::Clear,
			backColor => cl::Set,
		);
		$self-> {mask}-> put_image(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$and,
			rop::AndPut,
		);
	} else {
		$self-> {mask}-> color(cl::Clear);
		$self-> {mask}-> bar(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$info-> {rect}-> {left}   + $is[0],
			$info-> {rect}-> {bottom} + $is[1],
		);
	}

	# put non-transparent image pixels
	$self-> {canvas}-> put_image(
		$info-> {rect}-> {left},
		$info-> {rect}-> {bottom},
		$self-> {image},
	);

	#
	$ret{delay} = $info-> {delayTime} / 100;
	$ret{$_} ||= 0 for qw(left bottom right top);

	return \%ret;
}

sub is_stopped
{
	my $self = shift;
	return $self-> {current} >= @{$self-> {images}};
}

sub icon
{
	my $self = shift;

	my $i = Prima::Icon-> new;
	$i-> combine( $self-> {canvas}-> image, $self-> {mask}-> image);
	return $i;
}

sub image
{
	my $self = shift;

	my $i = Prima::Image-> new(
		width     => $self-> {canvas}-> width,
		height    => $self-> {canvas}-> height,
		type      => im::RGB,
		backColor => $self-> {bgColor} || 0,
	);
	$i-> begin_paint;
	$i-> clear;
	$i-> set(
		color     => cl::Clear,
		backColor => cl::Set,
	);
	$i-> put_image( 0, 0,$self-> {mask},   rop::AndPut);
	$i-> put_image( 0, 0,$self-> {canvas}, rop::XorPut);
	$i-> end_paint;

	return $i;
}

sub draw
{
	my ( $self, $canvas, $x, $y) = @_;

	return unless $self-> {canvas};

	my %save = map { $_ => $canvas-> $_() } qw(color backColor);
	$canvas-> set(
		color     => cl::Clear,
		backColor => cl::Set,
	);
	$canvas-> put_image( $x, $y, $self-> {mask},   rop::AndPut);
	$canvas-> put_image( $x, $y, $self-> {canvas}, rop::XorPut);
	$canvas-> set( %save);
}


sub masks   { ( $_[0]-> {canvas}, $_[0]-> {mask} ) }
sub width   { $_[0]-> {canvas} ? $_[0]-> {canvas}-> width  : 0 }
sub height  { $_[0]-> {canvas} ? $_[0]-> {canvas}-> height : 0 }
sub size    { $_[0]-> {canvas} ? $_[0]-> {canvas}-> size   : (0,0) }
sub bgColor { $_[0]-> {bgColor} }
sub current { $_[0]-> {current} }
sub total   { scalar @{$_[0]-> {images}} }

sub length
{
	my $length = 0;
	$length += $_-> {delayTime} || 0 for 
		map { $_-> {extras} || {} } 
		@{$_[0]-> {images}};
	return $length / 100;
}

sub loopCount
{
	return $_[0]-> {loopCount} unless $#_;
	$_[0]-> {loopCount} = $_[1];
}

1;

__END__

=pod

=head1 NAME

Prima::Image::AnimateGIF - animate gif files

=head1 DESCRIPTION

The module provides high-level access to GIF animation sequences.

=head1 SYNOPSIS

	use Prima qw(Application Image::AnimateGIF);
	my $x = Prima::Image::AnimateGIF->load($ARGV[0]);
	die $@ unless $x;
	my ( $X, $Y) = ( 0, 100);
	my $background = $::application-> get_image( $X, $Y, $x-> size);
	$::application-> begin_paint;

	while ( my $info = $x-> next) {
		my $frame = $background-> dup;
		$frame-> begin_paint;
		$x-> draw( $frame, 0, 0);
		$::application-> put_image( $X, $Y, $frame);

		$::application-> sync;
		select(undef, undef, undef, $info-> {delay});
	}

        $::application-> put_image( $X, $Y, $g);

=head2 new $CLASS, %OPTIONS

Creates an empty animation container. If C<$OPTIONS{images}> is given, it is
expected to be an array of images, best if loaded from gif files with
C<loadExtras> and C<iconUnmask> parameters set ( see L<Prima::image-load> for
details).

=head2 load $SOURCE, %OPTIONS

Loads GIF animation sequence from file or stream C<$SOURCE>. Options
are the same as understood by C<Prima::Image::load>, and are passed
down to it. 

=head2 add $IMAGE

Appends an image frame to the container.

=head2 bgColor

Return the background color specified by the GIF sequence as the preferred
background color to use when there is no specific background to superimpose the
animation to.

=head2 current

Return index of the current frame

=head2 draw $CANVAS, $X, $Y

Draws the current composite frame on C<$CANVAS> at the given coordinates.

=head2 height

Returns height of the composite frame.

=head2 icon

Creates and returns an icon object off the current composite frame.

=head2 image

Creates and returns an image object off the current composite frame.  The
transparent pixels on the image are replaced with the preferred background
color.

=head2 is_stopped

Returns true if the animation sequence was stopped, false otherwise.
If the sequence was stopped, the only way to restart it is to
call C<reset>.

=head2 length

Returns total animation length (without repeats) in seconds.

=head2 loopCount [ INTEGER ]

Sets and returns number of loops left, undef for indefinite.

=head2 masks

Return the AND and XOR masks, that can be used to display the current 
composite frame.

=head2 next

Advances one animation frame. The step triggers changes to the internally kept
AND and XOR masks that create effect of transparency, if needed.  The method
return a hash, where the following field are initialized:

=over

=item left, bottom, right, top

Coordinates of the changed area since the last frame was updated.

=item delay

Time ins seconds how long the frame is expected to be displayed.

=back

=head2 reset

Resets the animation sequence. This call is necessary either when image sequence was altered,
or when sequence display restart is needed.

=head2 size

Returns width and height of the composite frame.

=head2 total

Return number fo frames

=head2 width

Returns width of the composite frame.

=head1 SEE ALSO

L<Prima::image-load>,
L<http://www.the-labs.com/GIFMerge/ >

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
