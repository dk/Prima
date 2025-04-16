package Prima::Image::Animate;

use strict;
use warnings;
use Carp;
use Prima;
use Prima::Image::Loader;

sub new
{
	my $class = shift;
	my $self = bless {
		loadAll    => 0,
		model      => 'gif',
		@_,
		current    => -1,
	}, $class;

	$self->{loadAll} = 1 if $self->{images};
	$self->{images} //= [] if $self->{loadAll};

	$self-> reset;
	return $self;
}

sub detect_animation
{
	my (undef, $extras, $min_frames) = @_;

	$min_frames //= 2;
	return undef unless                    # more than 1 frame?
		$extras &&
		defined($extras->{codecID}) &&
		$extras->{frames} &&
		$extras->{frames} >= $min_frames;
	my $c = Prima::Image->codecs($extras-> {codecID}) or return 0;
	return undef unless $c;

	if ( $c->{name} eq 'GIFLIB') {
		return 'GIF';
	} elsif ($c->{name} =~ /^(WebP|PNG)$/) {
		return $c->{name};
	} elsif ($c->{name} eq 'JPEG-XL') {
		return 'JXL';
	} else {
		return undef;
	}
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

	if ( $opt{loadAll}) {
		my $i = Prima::Icon-> new(%events); # dummy object

		my @i = grep { defined } $i-> load(
			$where,
			loadExtras => 1,
			loadAll    => 1,
			iconUnmask => 1,
			blending   => 1,
			%args,
		);

		return (undef, $@) unless @i && $i[-1];

		my $model = $class->detect_animation($i[0]->{extras}, 1);
		return (undef, "not a recognized image or animation") unless $model;

		$model = 'Prima::Image::Animate::' . $model;
		return $model-> new( images => \@i);
	} else {
		my ($l,$error) = Prima::Image::Loader->new(
			$where,
			icons      => 1,
			iconUnmask => 1,
			blending   => 1,
			%args,
		);
		return (undef, $error) unless $l;

		my $model = $class->detect_animation($l->{extras}, 1);
		return (undef, "not a recognized image or animation") unless $model;

		$model = 'Prima::Image::Animate::' . $model;
		return $model-> new( loader => $l );
	}
}

sub loader { shift->{loader} }

sub add
{
	my ( $self, $image) = @_;
	croak "cannot add an image to a progressive animation loader" unless $self->{loadAll};
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

sub total
{
	my ( $self, $index ) = @_;
	return $self->{loadAll} ?
		scalar @{$self->{images}} :
		$self->{loader}->frames;
}

sub get_extras { shift->{image}->{extras} // {} }

sub close
{
	my $self = shift;
	return unless $self->{loader};
	$self->{loader}->close;
	$self->{suspended} = 1;
}

sub suspended { $_[0]->{suspended} }

sub reload
{
	my $self = shift;
	return 1 unless $self->{loader};
	delete $self->{suspended};
	my ($ok) = $self->{loader}->reload;
	return 0 unless $ok;
	$self->reset;
	return 1;
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

	my $ix;
	if ( $self->{loadAll}) {
		my $i = $self-> {images};
		return unless @$i;
		$ix = $i-> [0];
	} else {
		$self->{loader}->current(0);
		$ix = $self->load_next_image;
	}
	return unless $ix;

	$self-> {image} = $ix;
	my $e = $self-> {info}  = $self->get_extras;
	$self-> {$_} = $e-> {$_} for qw(screenWidth screenHeight);
	$self-> {changedRect} = {};
	$self-> fixup_rect( $e, $ix);
}

sub warning { shift->{warning} }

sub load_next_image
{
	my $self = shift;
	return unless $self->{loader};
	$self->{warning} = undef;
	my ( $i, $err ) = $self->{loader}->next;

	unless ($i) {
		$self->{warning} = $err;
		$self->{loader}->reload;
		( $i, $err ) = $self->{loader}->next;
		$self->{current} = 0;
		$self->{warning} = $err unless $i;
	}
	return $i;
}

sub advance_frame
{
	my $self = shift;

	return 0 if $self->{suspended};

	my ( $oimg, $oinfo) = delete @{$self}{qw(image info)};
	my $curr = $self->{current};
	if ( ++$self-> {current} >= $self-> total) {
		# go back to first frame, or stop
		if ( defined $self-> {loopCount}) {
		    return 0 if --$self-> {loopCount} <= 0;
		}
		$self-> {current} = 0;
		$curr = -2;
	}

	if ( $self->{loadAll}) {
		$self->{image} = $self-> {images}-> [$self-> {current}];
	} elsif ( $curr >= 0 ) {
		$self->{image} = $self-> load_next_image;
	} elsif ( $curr == -2 ) {
		$self-> {loader}->current(0);
		$self->{image} = $self-> load_next_image;
	} else {
		$self->{image} = $oimg;
	}
	unless ($self->{image}) {
		$self->{image} = $oimg;
		$self->{info} = $oinfo;
	}

	my $info = $self->{info} = $self-> get_extras;
	$self-> fixup_rect( $info, $self-> {image});

	# load global extension data
	if ( $self-> {current} == 0) {
		unless ( defined $info-> {loopCount}) {
			$self-> {loopCount} = 1;
		} elsif ( $info-> {loopCount} == 0) {
			# loop forever
			$self-> {loopCount} = undef;
		} elsif ( !defined $self->{loopCount}) {
			$self-> {loopCount} = $info-> {loopCount};
		}
	}
	return 1;
}

sub next  { die }
sub icon  { die }
sub image { die }
sub draw  { die }

sub draw_background
{
	my ( $self, $canvas, $x, $y) = @_;
	return 0 unless $self-> {canvas};
	my $a = $self->bgAlpha // 0xff;
	return 0 if $a == 0 || !defined $self->bgColor;
	my $c = $canvas->color;
	$canvas->color($self->bgColor);
	$canvas->rop(rop::alpha(rop::SrcOver, $a)) if $a != 0xff;
	$canvas->bar($x, $y, $x + $self->{screenWidth} - 1, $y + $self->{screenHeight} - 1);
	$canvas->rop(rop::CopyPut);
	return 1;
}

sub is_stopped
{
	my $self = shift;
	return $self-> {current} >= $self-> total;
}

sub width   { $_[0]-> {canvas} ? $_[0]-> {canvas}-> width  : 0 }
sub height  { $_[0]-> {canvas} ? $_[0]-> {canvas}-> height : 0 }
sub size    { $_[0]-> {canvas} ? $_[0]-> {canvas}-> size   : (0,0) }
sub bgColor { $_[0]-> {bgColor} }
sub bgAlpha { $_[0]-> {bgAlpha} }
sub current { $_[0]-> {current} }

sub length
{
	my $self = shift;
	my $length = 0;
	if ( $self->{loadAll}) {
		$length += $_-> {delayTime} || 0 for
			map { $_-> {extras} || {} }
			@{$_[0]-> {images}};
	} else {
		return $self->{cached_length} if exists $self->{cached_length};

		my ($l2) = Prima::Image::Loader->new(
			$self->{loader}->source,
			noImageData => 1,
			loadExtras  => 1,
		);
		$self->{cached_length} = undef;
		return undef unless $l2;

		while ( !$l2->eof ) {
			my ($i) = $l2->next;
			last unless $i;
			$length += $i->{extras}->{delayTime} // 0;
		}
		$self->{cached_length} = $length / 1000 if defined $length;
	}

	return defined($length) ? $length / 1000 : undef;
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
	my $self = shift;
	my $e  = $self->SUPER::get_extras;
	$e-> {screenHeight} ||= $self->{image}-> height;
	$e-> {screenWidth}  ||= $self->{image}-> width;
	$e-> {$_} ||= 0 for qw(disposalMethod useScreenPalette delayTime left top);
	return $e;
}

sub next
{
	my $self = shift;
	my %ret;
	return 0 if $self->{suspended};

	# dispose from the previous frame and calculate the changed rect
	my $info = $self->{info};
	my @sz = ( $self-> {screenWidth}, $self-> {screenHeight});

	# dispose from the previous frame and calculate the changed rect
	if ( $info-> {disposalMethod} == DISPOSE_CLEAR) {
		$self-> {canvas}-> backColor( 0);
		$self-> {canvas}-> clear;
		$self-> {mask}-> backColor(0xffffff);
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
		$self->{saveCanvas} = $self->{canvas};
		$self->{canvas}     = $self->{canvas}->dup;
		$self->{saveMask}   = $self->{mask};
		$self->{mask}       = $self->{mask}->dup;
		$self->{saveRect}   = $self-> {changedRect};
	}

	$self-> {changedRect} = $self->union_rect( $self-> {changedRect}, $info-> {rect});
	%ret = %{ $self->union_rect( \%ret, $info-> {rect}) };

	# draw the current frame
	if ( defined $info-> {transparentColorIndex}) {
		my ( $xor, $and) = $self-> {image}-> split;
		# combine masks
		$self-> {mask}-> put_image(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$and,
			rop::AndPut,
		);
	} else {
		my @is = $self->{image}->size;
		$self-> {mask}-> color(0);
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

	my $e = $self-> {info};
	return unless $e;

	$self-> {$_} = $e-> {$_} for qw(screenWidth screenHeight);

	# create canvas and mask
	$self-> {canvas}  = Prima::Image-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => im::RGB,
		backColor  => 0,
	);
	$self-> {canvas}-> clear; # canvas is all-0 initially

	$self-> {mask}    = Prima::Image-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => im::BW,
		backColor  => 0xFFFFFF,
		color      => 0x000000,
	);
	$self-> {mask}-> clear; # mask is all-1 initially

	if ( defined $e-> {screenBackGroundColor}) {
		my $cm =
			$e-> {useScreenPalette} ?
				$e-> {screenPalette} :
				$self-> {image}-> palette;
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
	$i-> clear;
	$i-> put_image( 0, 0,$self-> {mask},   rop::AndPut);
	$i-> put_image( 0, 0,$self-> {canvas}, rop::XorPut);

	return $i;
}

sub draw
{
	my ( $self, $canvas, $x, $y) = @_;
	return unless $self-> {canvas};
	$canvas-> put_image( $x, $y, $self-> {mask},   rop::AndPut);
	$canvas-> put_image( $x, $y, $self-> {canvas}, rop::XorPut);
}


package Prima::Image::Animate::WebPNG;
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

sub load_next_image
{
	my $i = shift->SUPER::load_next_image;
	$i->maskType(im::bpp8) if $i && $i->isa('Prima::Icon');
	return $i;
}

sub get_extras
{
	my $self = shift;
	my $e  = $self->SUPER::get_extras;
	$e-> {screenHeight} ||= $self->{image}-> height;
	$e-> {screenWidth}  ||= $self->{image}-> width;
	$e-> {$_} ||= 0 for qw(disposalMethod blendMethod delayTime left top);
	return $e;
}

sub next
{
	my $self = shift;
	my $info = $self->{info} or return;
	my %ret;
	return 0 if $self->{suspended};

	if ( $info-> {disposalMethod} eq 'restore') {
		# cut to the previous frame, that we expect to be saved for us
		if ( $self-> {saveCanvas} ) {
			$self-> {canvas} = $self-> {saveCanvas};
		}
		delete $self-> {saveCanvas};
		%ret = %{ $info-> {rect} };
	} elsif ( $info-> {disposalMethod} eq 'background') {
		# dispose from the previous frame and calculate the changed rect
		$self-> {canvas}-> color(0);
		$self-> {canvas}-> bar(
			$info-> {rect}-> {left},
			$info-> {rect}-> {bottom},
			$self->{image}->width  + $info-> {rect}-> {left} - 1,
			$self->{image}->height + $info-> {rect}-> {bottom} - 1
		);
		%ret = %{ $info-> {rect} };
	}

	return unless $self->advance_frame;
	$info = $self->{info};
	@{$self}{qw(saveCanvas canvas)} = ($self->{canvas}, $self->{canvas}->dup)
		if $info-> {disposalMethod} eq 'restore';

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

	my $e = $self-> {info};
	return unless $e;

	$self-> {canvas}  = Prima::Icon-> new(
		width      => $e-> {screenWidth},
		height     => $e-> {screenHeight},
		type       => im::RGB,
		maskType   => 8,
		backColor  => 0,
	);
	$self-> {canvas}-> clear; # canvas is black and transparent

	if ( defined $e-> {background}) {
		$self-> {bgColor} = cl::from_rgb(cl::to_bgr($e->{background} & 0xffffff));
		$self-> {bgAlpha} = ($e->{background} >> 24) & 0xff;
	}
}

sub icon  { shift->{canvas}->dup }
sub image { shift->{canvas}->image }

sub draw
{
	my ( $self, $canvas, $x, $y) = @_;
	$canvas-> put_image( $x, $y, $self-> {canvas}, rop::Blend) if $self->{canvas};
}

package Prima::Image::Animate::WebP;
use base 'Prima::Image::Animate::WebPNG';

package Prima::Image::Animate::PNG;
use base 'Prima::Image::Animate::WebPNG';

sub new
{
	my $class = shift;
	my $self = $class->SUPER::new(@_);
	my $i = $self->{images} // [];
	shift @$i if @$i > 1 && $i->[0]->{extras}->{default_frame};
	return $self;
}

sub load_next_image
{
	my ( $self ) = shift;
	my $i = $self->SUPER::load_next_image;
	return unless $i;

	if ( $self->{loader}->current == 1 && $i->{extras}->{default_frame} ) {
		$self->{skip_default_frame} = 1;
		return $self->SUPER::load_next_image;
	}

	return $i;
}

sub total
{
	my $self = shift;
	my $total = $self->SUPER::total;
	$total-- if $self->{skip_default_frame};
	return $total;
}

package Prima::Image::Animate::JXL;
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

sub load_next_image
{
	my $i = shift->SUPER::load_next_image;
	if ($i && $i->isa('Prima::Icon')) {
		$i->maskType(im::bpp8);
		$i->premultiply_alpha;
	}
	return $i;
}

sub get_extras
{
	my $self = shift;
	my $e  = $self->SUPER::get_extras;
	$e-> {screenHeight} ||= $self->{image}-> height;
	$e-> {screenWidth}  ||= $self->{image}-> width;
	$e-> {$_} ||= 0 for qw(
		tps_numerator tps_denominator top left
	);
	$e->{loopCount} = $e->{num_loops};

	return $e;
}

sub next
{
	my $self = shift;
	my $info = $self->{info} or return;
	return 0 if $self->{suspended};
	return unless $self->advance_frame;

	return {
		left   => 0,
		bottom => 0,
		right  => $self->{image}->width  - 1,
		height => $self->{image}->height - 1,
		delay  =>
			($info->{duration} ? $info->{duration} / 1000 : undef ) //
			($info->{tps_numerator} ? ($info-> {tps_denominator} / $info->{tps_numerator}) : undef) //
			0.05,
	};
}

sub icon  { shift->{image}->dup }
sub image { shift->{image}->image }

sub draw
{
	my ( $self, $canvas, $x, $y) = @_;
	my $i = $self->{image} or return;
	$canvas-> put_image( $x, $y, $i, $i->isa('Prima::Icon') ? rop::Blend : rop::CopyPut);
}

1;

__END__

=pod

=head1 NAME

Prima::Image::Animate - animate gif,webp,png files

=head1 DESCRIPTION

The module provides high-level access to GIF, APNG, and WebP animation sequences.

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
expected to be an array of images, best if loaded from files with
the C<loadExtras> and C<iconUnmask> parameters set ( see L<Prima::image-load> for
details).

=head2 detect_animation $HASH

Checks the C<{extras} hash> obtained from an image loaded with the
C<loadExtras> flag set to detect whether the image is an animation or not, and
if loading of all of its frame is supported by the module. Returns the file
format name on success, undef otherwise.

=head2 load $SOURCE, %OPTIONS

Loads a GIF, APNG, or WebP animation sequence from C<$SOURCE> which is either a
file or a stream. Options are the same as used by the C<Prima::Image::load> method.

Depending on the C<loadAll> option, either loads all frames at once (1), or uses
C<Prima::Image::Loader> to load only a single frame at a time (0, default).
Depending on the loading mode, some properties may not be available.

=head2 add $IMAGE

Appends an image frame to the container.

Only available if the C<loadAll> option is on.

=head2 bgColor

Returns the background color specified by the sequence as the preferred
color to use when there is no specific background to superimpose the
animation on.

=head2 close

Releases eventual image file handle for loader-based animations. Sets the
C<{suspended}> flag so that all image operations are suspended. A later call
to C<reload> restores the status quo execpt the current frame prior to the C<close>
call.

Has no effect on animations loaded with the C<loadAll> option.

=head2 current

Returns the index of the current frame

=head2 draw $CANVAS, $X, $Y

Draws the current composite frame on C<$CANVAS> at the given coordinates

=head2 draw_background $CANVAS, $X, $Y

Fills the background on C<$CANVAS> at the given coordinates if the file
provides the color to fill.  Returns a boolean value whether the canvas was
drawn on or not.

=head2 height

Returns the height of the composite frame

=head2 icon

Returns a new icon object created from the current composite frame

=head2 image

Returns a new image object created from the current composite frame The
transparent pixels on the image are replaced with the preferred background
color

=head2 is_stopped

Returns true if the animation sequence was stopped, false otherwise.
If the sequence was stopped, the only way to restart it is to
call C<reset>.

=head2 length

Returns the total animation length (without repeats) in seconds.

=head2 loopCount [ INTEGER ]

Sets and returns the number of loops left, undef for indefinite.

=head2 next

Advances one animation frame. The step triggers changes to the internally kept
buffer image that creates the effect of transparency if needed. The method
returns a hash, where the following fields are initialized:

=over

=item left, bottom, right, top

Coordinates of the changed area since the last frame was updated

=item delay

Time in seconds how long the frame is expected to be displayed

=back

=head2 reload

Reloads the animation after a C<close> call.
Returns the success flag.

=head2 reset

Resets the animation sequence. This call is necessary either when the image
sequence was altered, or when the sequence display restart is needed.

=head2 size

Returns the width and height of the composite frame

=head2 suspended

Returns true if a call to the C<close> method was made.

=head2 total

Return the number of frames

=head2 warning

If an error occured during frame loading, it will be stored in the C<warning>
property. The animation will stop at the last successfully loaded frame

Only available if the C<loadAll> option is off.

=head2 width

Returns the width of the composite frame

=head1 SEE ALSO

L<Prima::image-load>, L<Prima::Image::Loader>.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
