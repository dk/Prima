#  Created by:
#     Vadim Belman <voland@lflat.org>
use strict;
use warnings;
use Prima;
use Prima::Const;

package Prima::FrameSet::Frame;
use strict;
use warnings;
use base qw(Prima::Widget Prima::Widget::MouseScroller);

# Initialization
sub profile_default
{
	return {
		%{ $_[0]-> SUPER::profile_default},
		minFrameWidth => 5,
		maxFrameWidth => undef,
	};
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> $_($profile{$_}) for qw(minFrameWidth maxFrameWidth);
	return %profile;
}

# Properties
sub minFrameWidth
{
	return $_[0]-> {minFrameWidth} unless $#_;
	my ($self, $mfw) = @_;
	$self-> {minFrameWidth} = (($mfw // 0) < 0) ? 0 : $mfw;
}

sub maxFrameWidth
{
	return $_[0]-> {maxFrameWidth} unless $#_;
	my ($self, $mfw) = @_;
	$self-> {maxFrameWidth} = (($mfw // 0) < 0) ? 0 : $mfw;
}


package Prima::FrameSet::Slider;
use strict;
use warnings;
use base qw(Prima::Widget);
use Prima::Widget::RubberBand;

# Initialization
sub profile_default
{
	return {
		%{$_[0]-> SUPER::profile_default},
		vertical	=> 1,
		thickness	=> 4,
		growMode	=> gm::GrowHiX | gm::GrowLoX,
		frame1          => undef,
		frame2          => undef,
		sliderIndex     => 0,
		selectable      => 0,
	};
}

sub profile_check_in
{
	my ($self, $p, $default) = @_;
	$self-> SUPER::profile_check_in($p, $default);
	if (exists $p-> {vertical} ? $p-> {vertical} : $default-> {vertical}) {
		$p-> {growMode} = gm::GrowHiY | gm::GrowLoY unless exists $p-> {growMode};
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	$self->{no_adjust_sizes} = 1;
	$self-> $_($profile{$_}) for qw(thickness vertical frame1 frame2 sliderIndex);
	delete $self->{no_adjust_sizes};

	$self-> adjust_sizes;

	return %profile;
}

# Event handlers
sub on_paint
{
	my ($self, $canvas) = @_;
	my @sz = $canvas-> size;
	unless ($self-> enabled) {
		$self-> backColor( $self-> disabledBackColor);
		$self-> clear( 0, 0, $self-> size);
	} elsif ( $self-> skin eq 'flat') {
		$self-> backColor( $self-> {prelight} ?
			$self->hiliteBackColor :
			cl::blend(
				$self->map_color($self->hiliteBackColor),
				$self->map_color($self->backColor),
				0.5
			)
		);
		$self-> clear;
	} else {
		$self-> rect3d(
			0, 0,
			$sz[0]-1, $sz[1]-1,
			1,
			$self-> {draggingMode} ? (
				$self-> dark3DColor,
				$self-> light3DColor,
			) : (
				$self-> light3DColor,
				$self-> dark3DColor,
			),
			$self-> {prelight} ?
				$self->prelight_color($self-> backColor) :
				$self->backColor
		);
	}
}

sub on_enable  { $_[0]-> repaint }
sub on_disable { $_[0]-> repaint }

sub on_mousedown
{
	my $self = shift;
	my ($btn, $mod, $x, $y) = @_;

	$self-> clear_event;
	if ($btn == mb::Left) {
		$self-> start_dragging;
		$self-> repaint;
		$self-> update_view;
		@{ $self}{qw(spotX spotY)} = ($x, $y);
		$self-> {traceRect} = [$self-> client_to_screen(0, 0), $self-> client_to_screen($self-> size)];
		$self-> xorrect(@{$self-> {traceRect}}) unless $self-> owner-> opaqueResize;
		$self-> capture(1);
	}
}

sub on_mouseup
{
	my $self = shift;
	my ($btn, $mod, $x, $y) = @_;

	$self-> clear_event;

	if ($btn == mb::Left) {
		$self-> stop_dragging;
		$self-> owner-> slider_moved( $self, $self-> get_delta($x, $y));
		$self-> repaint;
	}
}

sub on_mouseenter
{
	my $self = shift;
	$self->{prelight} = 1;
	$self->repaint;
}

sub on_mouseleave
{
	my $self = shift;
	$self->{prelight} = 0;
	$self->repaint;
}

sub on_mousemove
{
	my $self = shift;
	my ($mod, $x, $y) = @_;

	if ($self-> {draggingMode}) {
		$self-> clear_event;

		my $delta = $self-> get_delta($x, $y);
		my @sz = $self-> size;
		my ($fmin, $fmax);
		if ($delta > 0) {
			$fmin = $self-> frame2;
			$fmax = $self-> frame1;
		} else {
			$fmin = $self-> frame1;
			$fmax = $self-> frame2;
		}
		my ($fminWidth, $fmaxWidth) = $self->{vertical} ? 
			( $fmin->width, $fmax->width ) :
			( $fmin->height, $fmax->height );
		$delta = ($fminWidth - $fmin-> {minFrameWidth}) * ($delta < 0 ? -1 : 1)
			if defined ($fmin-> {minFrameWidth})
				&& (($fminWidth - abs($delta)) < $fmin-> {minFrameWidth});
		$delta = ($fmax-> {maxFrameWidth} - $fmaxWidth) * ($delta < 0 ? -1 : 1)
			if defined($fmax-> {maxFrameWidth})
				&& (($fmaxWidth + abs($delta)) > $fmax-> {maxFrameWidth});

		if ($self-> owner-> opaqueResize) {
			$self-> owner-> slider_moved($self, $delta);
		} else {
			my @oldrect = @{$self-> {traceRect}};
			if ($self-> {vertical}) {
				$self-> {traceRect} = [$self-> client_to_screen($delta, 0), $self-> client_to_screen($sz[0] + $delta, $sz[1])];
			} else {
				$self-> {traceRect} = [$self-> client_to_screen(0, $delta), $self-> client_to_screen($sz[0], $sz[1] + $delta)];
			}
			my $difference = 0;
			$difference ||= $oldrect[$_] != $self-> {traceRect}-> [$_] foreach 0..3;
			$self-> xorrect(@{$self-> {traceRect}}) if $difference != 0;
		}
	}
}

sub on_keydown
{
	my $self = shift;
	my ($code, $key, $mod) = @_;

	if (($key == kb::Esc) && $self-> {draggingMode}) {
		$self-> stop_dragging;
		$self-> repaint;
	}
}

# Helpers
sub adjust_sizes
{
	my $self = shift;
	return if $self->{no_adjust_sizes};
	my $owner = $self-> owner;
	my ($w, $h) = $self-> {vertical} ?
		( $self->{thickness}, $owner->height ) :
		( $owner->width, $self->{thickness} );
	$self-> size($w, $h);
}

sub xorrect
{
	my ( $self, @r) = @_;
	my $p = $self->get_parent;
	$::application-> rubberband(
		clipRect => [ $p->client_to_screen( 0,0,$p-> size) ],
		@r ?
			( rect => \@r, breadth => $self->{thickness} ) :
			( destroy => 1 )
	);
}

sub get_delta
{
	my $self = shift;
	my ($x, $y) = @_;
	return $self-> {vertical} ? $x - $self-> {spotX} : $y - $self-> {spotY};
}

sub start_dragging
{
	my $self = shift;
	$self-> {draggingMode} = 1;
}

sub stop_dragging
{
	my $self = shift;

	$self-> xorrect unless $self-> owner-> opaqueResize;
	$self-> {draggingMode} = 0;
	$self-> capture(0);
}

# Properties
sub vertical
{
	return $_[0]->{vertical} unless $#_;
	my ($self, $v) = @_;
	return if exists($self-> {vertical}) && $self-> {vertical} == $v;
	$self-> {vertical} = $v;
	$self-> pointerType($v ? cr::SizeWE : cr::SizeNS);
	$self-> adjust_sizes;
	$self-> repaint;
}

sub thickness
{
	return $_[0]->{thickness} unless $#_;
	my ($self, $t) = @_;
	return if exists($self-> {thickness}) && $self-> {thickness} == $t;
	$self-> {thickness} = $t;
	$self-> adjust_sizes;
	$self-> repaint;
}

sub frame1
{
	return $_[0]->{frame1} unless $#_;
	my ($self, $f) = @_;
	return unless $f && $f-> isa('Prima::FrameSet::Frame');
	$self-> {frame1} = $f;
}

sub frame2
{
	return $_[0]->{frame2} unless $#_;
	my ($self, $f) = @_;
	return unless $f && $f-> isa('Prima::FrameSet::Frame');
	$self-> {frame2} = $f;
}

sub sliderIndex { $#_ ? $_[0]->{sliderIndex} = $_[1] : $_[0]->{sliderIndex} }

package Prima::FrameSet;
use strict;
use warnings;
use base qw(Prima::Widget);

# Initialization.
sub profile_default
{
	return {
		%{ $_[0]-> SUPER::profile_default},
		frameCount     => 2,
		flexible       => 1,
		frameSizes     => [qw(50% *)],
		growMode       => gm::Client,
		origin         => [0, 0],
		opaqueResize   => 1,
		sliderWidth    => int( 4 * (  $::application ? $::application->uiScaling : 1 ) + .5 ),
		vertical       => 0,
		frameClass     => 'Prima::FrameSet::Frame',
		frameProfile   => {},
		frameProfiles  => [],
		sliderClass    => 'Prima::FrameSet::Slider',
		sliderProfile  => {},
		sliderProfiles => [],
	};
}

sub profile_check_in
{
	my $self = shift;
	my ($profile, $default) = @_;
	$self-> SUPER::profile_check_in(@_);
	$profile-> {frameCount} = @{$profile-> {frameSizes} // $default->{frameSizes}} unless exists $profile-> {frameCount};
	$profile-> {frameCount} = 2 if $profile-> {frameCount} < 2;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	$self-> {$_} = $profile{$_} for qw(frameCount frameSizes);
	$self-> $_($profile{$_}) for qw(vertical sliderWidth flexible opaqueResize );

	for (my $i = 0; $i < $profile{frameCount}; $i++) {
		my %xp = %{$profile{frameProfile}};
		%xp = ( %xp, %{$profile{frameProfiles}-> [$i]})
			if $profile{frameProfiles}-> [$i] &&
			ref($profile{frameProfiles}-> [$i]) eq 'HASH';
		my $frame = $self-> insert(
			$profile{frameClass} =>
			name          => "Frame$i",
			packPropagate => 0,
			%xp,
		);
		push @{$self-> {frames}}, $frame;
	}

	my $sn;
	for ($sn = 0; $sn < ($profile{frameCount} - 1); $sn++) {
		my $moveable = $profile{flexible};
		my %xp = %{$profile{sliderProfile}};
		%xp = ( %xp, %{$profile{sliderProfiles}-> [$sn]})
			if $profile{sliderProfiles}-> [$sn] &&
			ref($profile{sliderProfiles}-> [$sn]) eq 'HASH';
		my $slider = $self-> insert( $profile{sliderClass},
			thickness   => $profile{sliderWidth},
			enabled     => $moveable,
			name        => "Slider#$sn",
			vertical    => !$self-> {vertical},
			frame1      => $self-> {frames}-> [$sn],
			frame2      => $self-> {frames}-> [$sn + 1],
			sliderIndex => $sn,
			%xp,
		);
		push @{$self-> {sliders}}, $slider;
	}

	$self-> recalc_frames(initial => 1);
	$self-> reset;

	return %profile;
}

# Event handlers
sub on_size
{
	my $self = shift;

	if ($_[0] == 0 && $_[1] == 0) {
# We get it when initial resize is performed.
		$self-> recalc_frames(initial => 1);
	}
	else {
		$self-> recalc_frames(resize => 1, sizes => \@_);
	}
	$self-> reset;
}

sub on_paint
{
	my ($self, $canvas) = @_;

	$canvas-> rop2(rop::CopyPut);
	$canvas-> fillPattern(fp::Interleave);
	$canvas-> bar(0, 0, $self-> size);
}

# Helpers
sub recalc_frames
{
	my $self = shift;
	my (%profile) = @_;

	return unless $self-> owner;

	my @sizes = @{$self-> {sizes} || []};
	if ($profile{initial}) {

		my @frameSizes = @{$self-> {frameSizes}};
		my $asteriskCount = 0;
		my $percents = 0;
		my $pixels = 0;

		foreach my $fsz (@frameSizes) {
			if ($fsz eq '*') {
				$asteriskCount++;
				next;
			}

			(my $nfsz = $fsz) =~ s/\%$//;

			$nfsz = 0 if $nfsz < 0;

			if ($fsz =~ /\%$/) {
				$percents += $nfsz;
			} else {
				$pixels += $nfsz;
			}
		}

		my $totalSize = $self->{vertical} ? $self-> height : $self->width;
		my $size = $totalSize - $pixels; # Size left after discounting all pixel-based and sliders sizes.

		foreach my $slider (@{$self-> {sliders}}) {
			$size -= $slider-> thickness;
		}

		my $percentSize = ($size * $percents / 100.);

		my $autoSize = $asteriskCount ? ($size - $percentSize) / $asteriskCount : 0; # Size of an automaticly-sized frame.

		@sizes = (0) x $self-> {frameCount};
		my $frac = 0;
		my $origSize;
		for (my $i = 0; $i < $self-> {frameCount}; $i++) {
			if (! defined($frameSizes[$i])
				|| ($frameSizes[$i] eq '*')) {
				$sizes[$i] = int($autoSize + .5);
				$frac += $autoSize - $sizes[$i];
			}
			elsif ($frameSizes[$i] =~ /\%$/) {
				(my $nfsz = $frameSizes[$i]) =~ s/\%$//;
				$sizes[$i] = int( ($origSize = ($size * $nfsz) / 100.) + .5);
				$frac += $origSize - $sizes[$i];
			}
			else {
				$sizes[$i] = int(($origSize = $frameSizes[$i]) + .5);
				$frac += $origSize - $sizes[$i];
			}

			if (abs($frac) >= 1) {
				$sizes[$i] += int($frac);
				$frac = $frac - int($frac);
			}
		}

		$self-> {sizes} = [@sizes];
		$self-> {virtual_sizes} = [@sizes];

	} elsif ($profile{resize}) {

		my $idx = $self-> {vertical} ? 1 : 0;
		my $old_size = @{$profile{sizes}}[$idx];
		my $new_size = @{$profile{sizes}}[$idx + 2];
		return if ($old_size == $new_size);

		my $virtual_sizes = $self-> {virtual_sizes} || [];

		my $i;
		for ($i = 0; $i < ($self-> {frameCount} - 1); $i++) {
			$old_size -= $self-> {sliders}-> [$i]-> {thickness};
			$new_size -= $self-> {sliders}-> [$i]-> {thickness};
		}
		my (@nsizes, @vsizes);
		my $newTotal = 0;
		my $ratio = ($old_size && $new_size) ? ( $new_size / $old_size ) : 1;
		my ($f, $ns);
		for ($i = 0; $i < ($self-> {frameCount} - 1); $i++) {
			$f = $self-> {frames}-> [$i];
			$ns = $virtual_sizes-> [$i] * $ratio;
			$vsizes[$i] = $ns;
			$ns = int( $ns + 0.5 );
			$ns = $f-> {minFrameWidth}
				if defined($f-> {minFrameWidth}) && ($ns < $f-> {minFrameWidth});
			$ns = $f-> {maxFrameWidth}
				if defined($f-> {maxFrameWidth}) && ($ns > $f-> {maxFrameWidth});
			$nsizes[$i] = $ns;
			$newTotal += $ns;
		}
# Calculate for the last frame.
		$f = $self-> {frames}-> [$i];
		$ns = $new_size - $newTotal;
		$vsizes[$i] = $ns;
		$ns = 1 if $ns < 1;
		$ns = $f-> {minFrameWidth}
			if defined($f-> {minFrameWidth}) && ($ns < $f-> {minFrameWidth});
		$ns = $f-> {maxFrameWidth}
			if defined($f-> {maxFrameWidth}) && ($ns > $f-> {maxFrameWidth});
		$nsizes[$i] = $ns;

		$self-> {sizes} = \@nsizes;
		$self-> {virtual_sizes} = \@vsizes;
	}
	return (wantarray ? @sizes : \@sizes);
}

sub reset
{
	my $self = shift;

	return unless $self-> owner;

	my $origin = [0, 0];
	my $end = [$self-> {vertical} ? ($self-> width, 1) : (1, $self-> height)];
	my $idx = $self-> {vertical} ? 1 : 0; # What element of origin/size array we change.
	my @sliders = @{$self-> {sliders}};

	for (my $i = 0; $i < $self-> {frameCount}; $i++) {
		$end-> [$idx] = $origin-> [$idx] + $self-> {sizes}-> [$i];
		$self-> {frames}-> [$i]-> rect(@$origin, @$end);
		$origin-> [$idx] += $self-> {sizes}-> [$i];
		if ($i < @{$self-> {sliders}}) {
			$sliders[$i]-> origin(@$origin);
			$origin-> [$idx] += $sliders[$i]-> thickness;
		}
	}
}

sub slider_moved
{
	my $self = shift;
	my ($slider, $delta) = @_;
	return unless $delta;

	my $si = $slider-> sliderIndex;

	my $frame1 = $slider-> frame1;
	my $frame2 = $slider-> frame2;

	my ($w1, $w2) = $self->{vertical} ?
		($frame1->height, $frame2->height) :
		($frame1->width, $frame2->width);

# Adjust delta in a way it doesn't clash with frame's min/max sizes.
	my $nw1 = $w1 + $delta;
	if (defined $frame1-> minFrameWidth && $nw1 < $frame1-> minFrameWidth) {
		$nw1 = $frame1-> minFrameWidth;
	} elsif (defined $frame1-> maxFrameWidth && $nw1 > $frame1-> maxFrameWidth) {
		$nw1 = $frame1-> maxFrameWidth;
	}
	$delta = $nw1 - $w1;
	my $nw2 = $w2 - $delta;
	if (defined $frame2-> minFrameWidth && $nw2 < $frame2-> minFrameWidth) {
		$nw2 = $frame2-> minFrameWidth;
	} elsif (defined $frame2-> maxFrameWidth && $nw2 > $frame2-> maxFrameWidth) {
		$nw2 = $frame2-> maxFrameWidth;
	}
	$delta = $w2 - $nw2;
	$nw1 = $w1 + $delta;

	my @rect;
	if ($self-> {vertical}) {
		$frame1-> height($nw1);
		@rect = $slider-> rect;
		$rect[1] += $delta;
		$rect[3] += $delta;
		$slider-> rect(@rect);
		@rect = $frame2-> rect;
		$rect[1] += $delta;
	} else {
		$frame1-> width($nw1);
		@rect = $slider-> rect;
		$rect[0] += $delta;
		$rect[2] += $delta;
		$slider-> rect(@rect);
		@rect = $frame2-> rect;
		$rect[0] += $delta;
	}
	$frame2-> rect(@rect);
	$frame1-> update_view;
	$frame2-> update_view;

	$self-> {virtual_sizes}-> [$si]     = $self-> {sizes}-> [$si]     = $nw1;
	$self-> {virtual_sizes}-> [$si + 1] = $self-> {sizes}-> [$si + 1] = $nw2;
}

# Properties
sub vertical
{
	return $_[0]->{vertical} unless $#_;
	my ( $self, $v) = @_;
	my $haveIt = defined $self->{vertical};
	return if $haveIt && $self-> {vertical} == $v;
	$self-> {vertical} = $v;
	$_->vertical($v) for @{ $self-> {sliders} // [] };
	if ($haveIt) {
		$self-> recalc_frames;
		$self-> reset;
	}
}

sub frameCount { $_[0]->{frameCount} }

sub sliderWidth
{
	return $_[0]->{sliderWidth} unless $#_;
	my ($self, $sw) = @_;
	my $haveIt = exists($self-> {sliderWidth});
	return if ($haveIt && ($self-> {sliderWidth} == $sw)) || ($sw < 0);
	$self-> {sliderWidth} = $sw;
	return unless $haveIt;

	$self->{sliders}->[$_]-> thickness($sw) for 0 .. $self-> {frameCount} - 1;
	$self-> recalc_frames;
	$self-> reset;
}

sub flexible
{
	return $_[0]->{flexible} unless $#_;
	my ($self, $f) = @_;
	my $haveIt = exists($self-> {flexible});
	return if $haveIt && ! ($self-> {flexible} xor $f);
	$self-> {flexible} = $f ? 1 : 0;
	return unless $haveIt;

	$self->{sliders}->[$_]-> enabled($f) for 0 .. $self-> {frameCount} - 1;
	$self-> recalc_frames;
	$self-> reset;
}

sub frameSizes
{
	return [@{$_[0]-> {frameSizes}}] unless $#_;
	my $self = shift;
	my @fs = ( $_[0] && ref($_[0]) eq 'ARRAY' && 1 == scalar @_) ? @{$_[0]} : @_;
	$self-> {frameSizes} = \@fs;
	$self-> recalc_frames( initial => 1);
	$self-> reset;
}

sub opaqueResize { $#_ ? $_[0]->{opaqueResize} = $_[1] : $_[0]->{opaqueResize} }

# User interface
sub firstFrame { $_[0]-> {frames}-> [0]  }
sub lastFrame  { $_[0]-> {frames}-> [-1] }
sub frames     { wantarray ? @{$_[0]-> {frames}} : $_[0]-> {frames} }
sub frame      { $_[0]-> {frames}-> [$_[1]] }

sub insert_to_frame
{
	my ($self, $frameIdx) = (shift, shift);

	$frameIdx = 0 if $frameIdx < 0;
	$frameIdx = $self-> {frameCount} - 1 if $frameIdx > ($self-> {frameCount} - 1);
	return if $frameIdx < 0;

	$self-> lock;
	my @ctrls = $self-> {frames}-> [$frameIdx]-> insert(@_);
	$self-> unlock;

	return wantarray ? @ctrls : $ctrls[0];
}

1;

=pod

=head1 NAME

Prima::FrameSet - frameset widget

=head1 SYNOPSIS

	use Prima qw(Application Buttons FrameSet);

	my $w = Prima::MainWindow->new( size => [300, 150] );

	my $frame = $w-> insert( 'FrameSet' =>
		pack          => { fill => 'both', expand => 1 },
	        frameSizes    => [qw(60% *)],
	        frameProfiles => [ 0,0, { minFrameWidth => 123 }],
	);

	$frame->insert_to_frame( 0, Button =>
		bottom        => 50,
	        text          => '~Ok',
	);

	run Prima;

=for podview <img src="frameset.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/frameset.gif">

=head1 DESCRIPTION

Provides the standard frameset widget. The frameset divides its surface among
groups of children and allows interactive change of the surface by dragging
the frame bars with the mouse.

=head1 API

=head2 Properties

=over

=item frameCount INTEGER

Number of frames, no less than 2.

This property can be set during creation only, thereafter it is readonly

Default: 2

=item flexible BOOLEAN

Frame can be resized by user

Default: 1

=item frameSizes @SIZES

Defines the widths of the frames, where each item in the array represents the width
of the related frame. The item can be one of:

=over

=item NUMBER%

F.ex. 50%, to occupy the part defined by the procent of the whole width

=item NUMBER

Explicit width in pixels

=item '*'

Asterisk calculated the width automatically

=back

Example:

   frameSizes => [qw(50% *)],

=item opaqueResize BOOLEAN

Whether dragging of a frame bar is interactive, or is an old-style with a marquee

Default: 1

=item sliderWidth PIXELS

Slider width in pixels.

When setting it explicitly, consider L<Prima::Application/uiScaling> , the visual aspect.

=item vertical BOOLEAN

Sets the direction in which frames are inserted

Default: 0

=back

=head2 Methods

=over

=item firstFrame

Returns the first frame

=item lastFrame

Returns the last frame

=item frames

Returns all the frames

=item frame N

Returns the frame N

sub insert_to_frame N, CLASS, PARAMETERS

CLASS and PARAMETERS are same as in C<insert>, but reference the frame N

=back

=head1 AUTHOR

Vadim Belman, E<lt>voland@lflat.orgE<gt>

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>,
F<examples/frames.pl>.

=cut
