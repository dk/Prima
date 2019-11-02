package Prima::Image::Animate;

use strict;
use warnings;
use Carp;
use Prima;

sub new
{
	my $class = shift;
	my $self = bless {
		images     => [],
		model      => 'gif',
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
		blending   => 1,
		%args,
	);

	return unless @i;

	my $c = Prima::Image->codecs($i[0]-> {extras}-> {codecID}) or return 0;
	my $model;
	if ( $c->{name} eq 'GIFLIB') {
		$model = 'GIF';
	} elsif ($c->{name} eq 'WebP') {
		$model = 'WebP';
	} elsif ($c->{name} eq 'PNG') {
		$model = 'APNG';
	} else {
		return 0;
	}
	$model = 'Prima::Image::Animate::' . $model;

	return $model-> new( images => \@i);
}

sub add
{
	my ( $self, $image) = @_;
	push @{$self-> {images}}, $image;
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
	my ( $self, $r1, $r2) = @_;
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
		loopCount changedRect cache
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

}

sub advance_frame
{
	my $self = shift;

	delete @{$self}{qw(image info)};
	if ( ++$self-> {current} >= @{$self-> {images}}) {
		# go back to first frame, or stop
		if ( defined $self-> {loopCount}) {
		    return 0 if --$self-> {loopCount} <= 0;
		}
		$self-> {current} = 0;
	}
	$self-> {image} = $self-> {images}-> [$self-> {current}];
	my $info = $self-> {info} = $self-> get_extras( $self-> {current} );
	$self-> fixup_rect( $info, $self-> {image});

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
	return 1;
}

sub next  { die }
sub icon  { die }
sub image { die }
sub draw  { die }
sub get_extras { die }

sub draw_background
{
	my ( $self, $canvas, $x, $y) = @_;
	return 0 unless $self-> {canvas};
        my $a = $self->bgAlpha // 0xff;
        return 0 if $a == 0 || !defined $self->bgColor;
        if ( $a == 0xff ) {
                my $c = $canvas->color;
                $canvas->color($self->bgColor);
                $canvas->bar($x, $y, $x + $self->{screenWidth}, $y + $self->{screenHeight});
                $canvas->color($c);
        } else {
                my $px = $self->{cache}->{bgpixel} //= Prima::Icon->new(
                        size     => [1,1],
                        type     => im::RGB,
                        maskType => im::bpp8,
                        data     => join('', map { chr } cl::to_bgr($self->bgColor)),
                        mask     => chr($a),
                );
                $canvas->stretch_image( $x, $y, $self->{screenWidth}, $self->{screenHeight}, $px, rop::SrcOver);
        }
        return 1;
}

sub is_stopped
{
	my $self = shift;
	return $self-> {current} >= @{$self-> {images}};
}

sub width   { $_[0]-> {canvas} ? $_[0]-> {canvas}-> width  : 0 }
sub height  { $_[0]-> {canvas} ? $_[0]-> {canvas}-> height : 0 }
sub size    { $_[0]-> {canvas} ? $_[0]-> {canvas}-> size   : (0,0) }
sub bgColor { $_[0]-> {bgColor} }
sub bgAlpha { $_[0]-> {bgAlpha} }
sub current { $_[0]-> {current} }
sub total   { scalar @{$_[0]-> {images}} }

sub length
{
	my $length = 0;
	$length += $_-> {delayTime} || 0 for
		map { $_-> {extras} || {} }
		@{$_[0]-> {images}};
	return $length / 1000;
}

sub loopCount
{
	return $_[0]-> {loopCount} unless $#_;
	$_[0]-> {loopCount} = $_[1];
}

package Prima::Image::Animate::GIF;
use base 'Prima::Image::Animate';

use constant DISPOSE_NOT_SPECIFIED    => 0; # Leave frame, let new frame draw on top
use constant DISPOSE_KEEP             => 1; # Leave frame, let new frame draw on top
use constant DISPOSE_CLEAR            => 2; # Clear the frame's area, revealing bg
use constant DISPOSE_RESTORE_PREVIOUS => 3; # Restore the previous (composited) frame

sub get_extras
{
	my ( $self, $ix) = @_;
	$ix = $self-> {images}-> [$ix];
	return unless $ix;

	my $e = $ix-> {extras} || {};

	$e-> {screenHeight}     ||= $ix-> height;
	$e-> {screenWidth}      ||= $ix-> width;
	$e-> {$_} ||= 0 for qw(disposalMethod useScreenPalette delayTime left top);

	# gif doesn't support explicit masks, therefore
	# when image actually has a mask, autoMaskign is set to am::Index
	$e-> {iconic} = $ix-> isa('Prima::Icon') && $ix-> autoMasking != am::None;

	return $e;
}

sub next
{
	my $self = shift;
	my %ret;

	# dispose from the previous frame and calculate the changed rect
	my $info = $self->{info};
	my @sz = ( $self-> {screenWidth}, $self-> {screenHeight});

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

	return unless $self->advance_frame;

	$info = $self->{info};
	if ( $info-> {disposalMethod} == DISPOSE_RESTORE_PREVIOUS) {
		my $c  = Prima::DeviceBitmap-> new(
			width      => $sz[0],
			height     => $sz[1],
			type       => dbt::Pixmap,
		);
		$c-> put_image( 0, 0, $self-> {canvas});
		$self-> {saveCanvas} = $self-> {canvas};
		$self-> {canvas} = $c;

		$c = Prima::DeviceBitmap-> new(
			width      => $sz[0],
			height     => $sz[1],
			type       => dbt::Bitmap,
		);
		$c-> put_image( 0, 0, $self-> {mask});
		$self-> {saveMask} = $self-> {mask};
		$self-> {mask} = $c;

		$self-> {saveRect} = $self-> {changedRect};
	}

	$self-> {changedRect} = $self->union_rect( $self-> {changedRect}, $info-> {rect});
	%ret = %{ $self->union_rect( \%ret, $info-> {rect}) };

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
		my @is = $self->{image}->size;
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

	$ret{$_} ||= 0 for qw(left bottom right top);
	$ret{delay} = $info-> {delayTime} / 100;

	return \%ret;
}

sub reset
{
	my $self = shift;
	$self-> SUPER::reset;

	my $e = $self-> get_extras(0);
	return unless $e;

	$self-> {$_} = $e-> {$_} for qw(screenWidth screenHeight);

	# create canvas and mask
	$self-> {canvas}  = Prima::DeviceBitmap-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => dbt::Pixmap,
		backColor  => 0,
	);
	$self-> {canvas}-> clear; # canvas is all-0 initially

	$self-> {mask}    = Prima::DeviceBitmap-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => dbt::Bitmap,
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
		$self-> {bgColor} = cl::from_rgb(map { $_ || 0 } @$cm[$i..$i+2]);
		$self-> {bgAlpha} = 0xff;
	}
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


package Prima::Image::Animate::WebP;
use base 'Prima::Image::Animate';

sub new
{
	my ( $class, %opt ) = @_;

	# rop::SrcCopy works only with 8-bit alpha
	for (@{ $opt{images} // [] }) {
		$_->maskType(im::bpp8) if $_->isa('Prima::Icon');
	}

	return $class->SUPER::new(%opt);
}

sub get_extras
{
	my ( $self, $ix) = @_;
	$ix = $self-> {images}-> [$ix];
	return unless $ix;

	my $e = $ix-> {extras} || {};

	$e-> {screenHeight}     ||= $ix-> height;
	$e-> {screenWidth}      ||= $ix-> width;
	$e-> {$_} ||= 0 for qw(disposalMethod blendMethod delayTime left top);

	return $e;
}

sub next
{
	my $self = shift;
	my $info = $self->{info};
	my %ret;

	# dispose from the previous frame and calculate the changed rect
	if ( $info-> {disposalMethod} eq 'background') {
		$self-> {canvas}-> color(cl::Clear);
		$self-> {canvas}-> bar(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$self->{image}->width  + $info-> {rect}-> {left},
			$self->{image}->height + $info-> {rect}-> {bottom}
		);
		%ret = %{ $info-> {rect} };
	}

	return unless $self->advance_frame;
	$info = $self->{info};

	%ret = %{ $self->union_rect( \%ret, $info-> {rect}) };

	# draw the current frame
	$self-> {canvas}-> put_image(
		$info-> {rect}-> {left},
		$info-> {rect}-> {bottom},
		$self-> {image},
		(( $info-> {blendMethod} eq 'blend') ? rop::SrcOver : rop::SrcCopy)
	);

	$ret{$_} ||= 0 for qw(left bottom right top);
	$ret{delay} = $info-> {delayTime} / 1000;

	return \%ret;
}

sub reset
{
	my $self = shift;
	$self-> SUPER::reset;

	my $e = $self-> get_extras(0);
	return unless $e;

	$self-> {canvas}  = Prima::DeviceBitmap-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => dbt::Layered,
		backColor  => 0,
	);
	$self-> {canvas}-> clear; # canvas is black and transparent

	if ( defined $e-> {background}) {
		$self-> {bgColor} = cl::from_rgb(cl::to_bgr($e->{background} & 0xffffff));
		$self-> {bgAlpha} = ($e->{background} >> 24) & 0xff;
	}
}

sub icon  { shift->{canvas}->icon }
sub image { shift->{canvas}->image }

sub draw
{
	my ( $self, $canvas, $x, $y) = @_;
	$canvas-> put_image( $x, $y, $self-> {canvas}, rop::SrcOver) if $self->{canvas};
}

package Prima::Image::Animate::APNG;
use base 'Prima::Image::Animate::WebP';

sub next
{
	my $self = shift;
	my $info = $self->{info};
	my %ret;

	if ( $info-> {disposalMethod} eq 'restore') {
		# cut to the previous frame, that we expect to be saved for us
		if ( $self-> {saveCanvas} ) {
			$self-> {canvas} = $self-> {saveCanvas};
		}
		delete $self-> {saveCanvas};
		%ret = %{ $info-> {rect} };
	} elsif ( $info-> {disposalMethod} eq 'background') {
		# dispose from the previous frame and calculate the changed rect
		$self-> {canvas}-> color(cl::Clear);
		$self-> {canvas}-> bar(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$self->{image}->width  + $info-> {rect}-> {left},
			$self->{image}->height + $info-> {rect}-> {bottom}
		);
		%ret = %{ $info-> {rect} };
	}

	return unless $self->advance_frame;
	$info = $self->{info};
	if ( $info-> {disposalMethod} eq 'restore') {
		my @sz = ( $self-> {screenWidth}, $self-> {screenHeight});
		my $c  = Prima::DeviceBitmap-> new(
			width      => $sz[0],
			height     => $sz[1],
			type       => dbt::Layered,
		);
		$c-> put_image( 0, 0, $self-> {canvas});
		$self-> {saveCanvas} = $self-> {canvas};
		$self-> {canvas} = $c;
	}

	%ret = %{ $self->union_rect( \%ret, $info-> {rect}) };

	# draw the current frame
	$self-> {canvas}-> put_image(
		$info-> {rect}-> {left},
		$info-> {rect}-> {bottom},
		$self-> {image},
		(( $info-> {blendMethod} eq 'blend') ? rop::SrcOver : rop::SrcCopy)
	);

	$ret{$_} ||= 0 for qw(left bottom right top);
	$ret{delay} = $info-> {delayTime} / 1000;

	return \%ret;
}

1;

__END__

=pod

=head1 NAME

Prima::Image::Animate - animate gif and webp files

=head1 DESCRIPTION

The module provides high-level access to GIF and WebP animation sequences.

=head1 SYNOPSIS

	use Prima qw(Application Image::Animate);
	my $x = Prima::Image::Animate->load($ARGV[0]);
	die $@ unless $x;
	my ( $X, $Y) = ( 0, 100);
        my $want_background = 1; # 0 for eventual transparency
	my $background = $::application-> get_image( $X, $Y, $x-> size);
	$::application-> begin_paint;

	while ( my $info = $x-> next) {
		my $frame = $background-> dup;
		$frame-> begin_paint;
		$x-> draw_background( $frame, 0, 0) if $want_background;
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

Loads GIF or WebP animation sequence from file or stream C<$SOURCE>. Options
are the same as understood by C<Prima::Image::load>, and are passed
down to it.

=head2 add $IMAGE

Appends an image frame to the container.

=head2 bgColor

Return the background color specified by the sequence as the preferred
background color to use when there is no specific background to superimpose the
animation to.

=head2 current

Return index of the current frame

=head2 draw $CANVAS, $X, $Y

Draws the current composite frame on C<$CANVAS> at the given coordinates.

=head2 draw_background $CANVAS, $X, $Y

Fills the background on C<$CANVAS> at the given coordinates if file provides that.
Returns whether the canvas was tainted or not.

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
L<http://www.the-labs.com/GIFMerge/>

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
