#  Created by:
#     Vadim Belman <voland@lflat.org>
use strict;
use warnings;
use Prima;
use Prima::Const;

package
    fra; # Frame arragement constants.

use constant Vertical => 0;
use constant Horizontal => 1;

package
    frr; # Frame resize method constants.

use constant Simple => 0;
use constant Proportional => 1; # XXX Yet to be implemented.

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
	my $me = shift;

	my %profile = $me-> SUPER::init(@_);

	foreach my $key (qw(minFrameWidth maxFrameWidth)) {
		$me-> $key($profile{$key});
	}

	return %profile;
}

# Properties
sub minFrameWidth
{
	my $me = shift;
	if (@_ > 0) {
		return if defined ($me-> {minFrameWidth}) && $me-> {minFrameWidth} == $_[0];
		if (defined $_[0]) {
				$me-> {minFrameWidth} = $_[0] < 0 ? 0 : $_[0];
		} else {
				undef $me-> {minFrameWidth};
		}
	} else {
		return $me-> {minFrameWidth};
	}
}

sub maxFrameWidth
{
	my $me = shift;
	if (@_ > 0) {
		return if defined($me-> {maxFrameWidth}) && ($me-> {maxFrameWidth} == $_[0]);
		if (defined $_[0]) {
				$me-> {maxFrameWidth} = $_[0] < 0 ? 0 : $_[0];
		} else {
				undef $me-> {maxFrameWidth};
		}
	} else {
		return $me-> {maxFrameWidth};
	}
}



package Prima::FrameSet::Slider;
use strict;
use warnings;
use vars qw(@ISA);
use Prima::Widget::RubberBand;

@ISA = qw(Prima::Widget);

# Initialization
sub profile_default
{
	return {
		%{$_[0]-> SUPER::profile_default},
		vertical	=> 0,
		thickness	=> 4,
		growMode	=> gm::GrowHiX | gm::GrowLoX,
		frame1          => undef,
		frame2          => undef,
		sliderIndex     => 0,
	};
}

sub profile_check_in
{
	my ($me, $p, $default) = @_;
	my $userPointer = exists $p-> {pointerType};
	$me-> SUPER::profile_check_in($p, $default);
	if (exists $p-> {vertical} ? $p-> {vertical} : $default-> {vertical}) {
		$p-> {growMode} = gm::GrowHiY | gm::GrowLoY unless exists $p-> {growMode};
	}
}

sub init
{
	my $me = shift;
	my %profile = $me-> SUPER::init(@_);

	foreach my $prop (qw(thickness vertical frame1 frame2 sliderIndex)) {
		$me-> $prop($profile{$prop});
	}

	$me-> adjust_sizes;

	return %profile;
}

# Event handlers
sub on_paint
{
	my ($me, $canvas) = @_;
	my @sz = $canvas-> size;
	unless ($me-> enabled) {
		$me-> backColor( $me-> disabledBackColor);
		$me-> clear( 0, 0, $me-> size);
	} elsif ( $me-> skin eq 'flat') {
		$me-> backColor( $me-> {prelight} ?
			$me->hiliteBackColor :
			cl::blend(
				$me->map_color($me->hiliteBackColor),
				$me->map_color($me->backColor),
				0.5
			)
		);
		$me-> clear;
	} else {
		$me-> rect3d(
			0,
			0,
			$sz[0]-1,
			$sz[1]-1,
			1,
			$me-> {draggingMode} ? (
				$me-> dark3DColor,
				$me-> light3DColor,
			) : (
				$me-> light3DColor,
				$me-> dark3DColor,
			),
			$me-> {prelight} ? $me->prelight_color($me-> backColor) : $me->backColor
		);
	}
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }

sub on_mousedown
{
	my $me = shift;
	my ($btn, $mod, $x, $y) = @_;

	$me-> clear_event;
	if ($btn == mb::Left) {
		$me-> start_dragging;
		$me-> repaint;
		$me-> update_view;
		@{ $me}{qw(spotX spotY)} = ($x, $y);
		$me-> {traceRect} = [$me-> client_to_screen(0, 0), $me-> client_to_screen($me-> size)];
		$me-> xorrect(@{$me-> {traceRect}}) unless $me-> owner-> opaqueResize;
		$me-> capture(1);
	}
}

sub on_mouseup
{
	my $me = shift;
	my ($btn, $mod, $x, $y) = @_;

	$me-> clear_event;

	if ($btn == mb::Left) {
		$me-> stop_dragging;
		$me-> owner-> slider_moved( $me, $me-> get_delta($x, $y));
		$me-> repaint;
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
	my $me = shift;
	my ($mod, $x, $y) = @_;

	if ($me-> {draggingMode}) {
		$me-> clear_event;

		my $delta = $me-> get_delta($x, $y);
		my @sz = $me-> size;
		my ($fmin, $fmax);
		if ($delta > 0) {
			$fmin = $me-> frame2;
			$fmax = $me-> frame1;
		} else {
			$fmin = $me-> frame1;
			$fmax = $me-> frame2;
		}
		my ($fminWidth, $fmaxWidth);
		if ($me-> {vertical}) {
			$fminWidth = $fmin-> width;
			$fmaxWidth = $fmax-> width;
		} else {
			$fminWidth = $fmin-> height;
			$fmaxWidth = $fmax-> height;
		}
		$delta = ($fminWidth - $fmin-> {minFrameWidth}) * ($delta < 0 ? -1 : 1)
				if defined($fmin-> {minFrameWidth})
					&& (($fminWidth - abs($delta)) < $fmin-> {minFrameWidth});
		$delta = ($fmax-> {maxFrameWidth} - $fmaxWidth) * ($delta < 0 ? -1 : 1)
				if defined($fmax-> {maxFrameWidth})
					&& (($fmaxWidth + abs($delta)) > $fmax-> {maxFrameWidth});

		if ($me-> owner-> opaqueResize) {
			$me-> owner-> slider_moved($me, $delta);
		} else {
			my @oldrect = @{$me-> {traceRect}};
			if ($me-> {vertical}) {
				$me-> {traceRect} = [$me-> client_to_screen($delta, 0), $me-> client_to_screen($sz[0] + $delta, $sz[1])];
			} else {
				$me-> {traceRect} = [$me-> client_to_screen(0, $delta), $me-> client_to_screen($sz[0], $sz[1] + $delta)];
			}
			my $different = 0;
			$different ||= $oldrect[$_] != $me-> {traceRect}-> [$_] foreach 0..3;
			if ($different) {
# Don't redraw dragging rect if the old and the new rectangle are the same.
				$me-> xorrect(@{$me-> {traceRect}});
			}
		}
	}
}

sub on_keydown
{
	my $me = shift;
	my ($code, $key, $mod) = @_;

	if (($key == kb::Esc) && $me-> {draggingMode}) {
		$me-> stop_dragging;
		$me-> repaint;
	}
}

# Helpers
sub adjust_sizes
{
	my $me = shift;
	my ($w, $h);
	my $owner = $me-> owner;
	if ($me-> {vertical}) {
		$h = $owner-> height;
		$w = $me-> {thickness};
	} else {
		$h = $me-> {thickness};
		$w = $owner-> width;
	}
	$me-> size($w, $h);
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
	my $me = shift;
	my ($x, $y) = @_;
	return $me-> {vertical} ? $x - $me-> {spotX} : $y - $me-> {spotY};
}

sub start_dragging
{
	my $me = shift;
	$me-> {draggingMode} = 1;
	$me-> {oldFocus} = $::application-> get_focused_widget;
	$me-> focus;
}

sub stop_dragging
{
	my $me = shift;

	$me-> xorrect unless $me-> owner-> opaqueResize;
	$me-> {draggingMode} = 0;
	$me-> capture(0);
	$me-> {oldFocus}-> focus
		if defined $me-> {oldFocus};
}

# Properties
sub vertical
{
	my $me = shift;
	if (@_ > 0) {
		return if exists($me-> {vertical}) && $me-> {vertical} == $_[0];
		$me-> {vertical} = $_[0];
		$me-> pointerType($_[0] ? cr::SizeWE : cr::SizeNS);
		$me-> adjust_sizes;
		$me-> repaint;
	}
	else {
		return $me-> {vertical};
	}
}

sub thickness
{
	my $me = shift;
	if (@_ > 0) {
		return if exists($me-> {thickness}) && $me-> {thickness} == $_[0];
		$me-> {thickness} = $_[0];
		$me-> adjust_sizes;
		$me-> repaint;
	} else {
		return $me-> {thickness};
	}
}


sub frame1
{
	my $me = shift;
	if (@_ > 0) {
		return unless $_[0] && $_[0]-> isa('Prima::FrameSet::Frame');
		$me-> {frame1} = $_[0];
	} else {
		return $me-> {frame1};
	}
}

sub frame2
{
	my $me = shift;
	if (@_ > 0) {
		return unless $_[0] && $_[0]-> isa('Prima::FrameSet::Frame');
		$me-> {frame2} = $_[0];
	} else {
		return $me-> {frame2};
	}
}

sub sliderIndex
{
	my $me = shift;
	if (@_ > 0) {
		$me-> {sliderIndex} = $_[0];
	} else {
		return $me-> {sliderIndex};
	}
}



package Prima::FrameSet;
use strict;
use warnings;
use vars qw(@ISA);

@ISA = qw(Prima::Widget);

# Initialization.
sub profile_default
{
	return {
		%{ $_[0]-> SUPER::profile_default},
		arrangement => fra::Horizontal,		# Vertical or horizontal insertion of frames.
		frameCount => 2,			# Number of frames, no less than 2.
		flexible => 1,				# Frame can be resized by user at any time at any place.
		sliders => [qw(1)],			# Where resize sliders must be located. Array size depends on number of frames and must be (nframes+1) long.
		sliderWidth => 4,
		separatorWidth => 1,			# Separator is an immovable slider.
		frameSizes => [qw(50% *)],		# Sizes of frames in percents, pixel or '*' for automatic.
		growMode => gm::Client,
		origin => [0, 0],
		opaqueResize => 0,
		resizeMethod => frr::Simple,
		frameClass    => 'Prima::FrameSet::Frame',
		frameProfile  => {},
		frameProfiles => [],
		sliderClass    => 'Prima::FrameSet::Slider',
		sliderProfile  => {},
		sliderProfiles => [],
	};
}

sub profile_check_in
{
	my $me = shift;
	my ($profile, $default) = @_;
	$me-> SUPER::profile_check_in(@_);
	$profile-> {frameCount} = @{$profile-> {frameSizes} // $default->{frameSizes}} unless exists $profile-> {frameCount};
	$profile-> {frameCount} = 2 if $profile-> {frameCount} < 2;
	if (! exists($profile-> {sliders}) && ! exists($profile-> {flexible})) {
		$profile-> {sliders} = [(1) x ($profile-> {frameCount} - 1)];
	}
}

sub init
{
	my $me = shift;
	my %profile = $me-> SUPER::init(@_);

	$me-> {frameSizes} = $profile{frameSizes};

	foreach my $prop (qw(frameCount arrangement sliderWidth separatorWidth
				flexible opaqueResize resizeMethod)) {
		$me-> $prop($profile{$prop});
	}

	for (my $i = 0; $i < $profile{frameCount}; $i++) {
		my %xp = %{$profile{frameProfile}};
		%xp = ( %xp, %{$profile{frameProfiles}-> [$i]})
			if $profile{frameProfiles}-> [$i] &&
			ref($profile{frameProfiles}-> [$i]) eq 'HASH';
		my $frame = $me-> insert(
			$profile{frameClass} =>
			name => "Frame$i",
			packPropagate => 0,
			%xp,
		);
		push @{$me-> {frames}}, $frame;
	}

	my $sn;
	for ($sn = 0; $sn < ($profile{frameCount} - 1); $sn++) {
		my $moveable = $profile{sliders}-> [$sn] ? 1 : 0;
		$moveable = $profile{flexible};
		my %xp = %{$profile{sliderProfile}};
		%xp = ( %xp, %{$profile{sliderProfiles}-> [$sn]})
			if $profile{sliderProfiles}-> [$sn] &&
			ref($profile{sliderProfiles}-> [$sn]) eq 'HASH';
		my $slider = $me-> insert(
				$profile{sliderClass} =>
				name => "Slider#$sn",
				$moveable ?
					(
					thickness => $profile{sliderWidth},
					enabled => 1,
					) : (
					thickness => $profile{separatorWidth},
					enabled => 0,
					),
# Horizontal arrangement of frames means we need a vertically oriented slider.
				vertical => $me-> {arrangement} == fra::Horizontal,
				frame1 => $me-> {frames}-> [$sn],
				frame2 => $me-> {frames}-> [$sn + 1],
				sliderIndex => $sn,
				%xp,
		);
		push @{$me-> {sliders}}, $slider;
	}

	$me-> recalc_frames(initial => 1);
	$me-> reset;

	return %profile;
}

# Event handlers
sub on_size
{
	my $me = shift;

	if ($_[0] == 0 && $_[1] == 0) {
# We get it when initial resize is performed.
		$me-> recalc_frames(initial => 1);
	}
	else {
		$me-> recalc_frames(resize => 1, sizes => \@_);
	}
	$me-> reset;
}

sub on_paint
{
	my ($me, $canvas) = @_;

	$canvas->rop2(rop::CopyPut);
	$canvas-> fillPattern(fp::Interleave);
	$canvas-> bar(0, 0, $me-> size);
}

# Helpers
sub recalc_frames
{
	my $me = shift;
	my (%profile) = @_;

	return unless $me-> owner;

	my @sizes = @{$me-> {sizes} || []};
	if ($profile{initial}) {

		my @frameSizes = @{$me-> {frameSizes}};
		my $asterixCount = 0;
		my $percents = 0;
		my $pixels = 0;

		foreach my $fsz (@frameSizes) {
			if ($fsz eq '*') {
				$asterixCount++;
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

		my $totalSize;	# Total width or height depending on arrangement.
		if ($me-> {arrangement} == fra::Vertical) {
			$totalSize = $me-> height;
		} else {
			$totalSize = $me-> width;
		}

		my $size = $totalSize - $pixels; # Size left after discounting all pixel-based and sliders sizes.

		foreach my $slider (@{$me-> {sliders}}) {
			$size -= $slider-> thickness;
		}

		my $percentSize = ($size * $percents / 100.);

		my $autoSize = $asterixCount ? ($size - $percentSize) / $asterixCount : 0; # Size of an automaticly-sized frame.

		@sizes = (0) x $me-> {frameCount};
		my $frac = 0;
		my $origSize;
		for (my $i = 0; $i < $me-> {frameCount}; $i++) {
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

		$me-> {sizes} = [@sizes];
		$me-> {virtual_sizes} = [@sizes];

	} elsif ($profile{resize}) {

		my $idx = $me-> {arrangement} == fra::Horizontal ? 0 : 1;
		my $old_size = @{$profile{sizes}}[$idx];
		my $new_size = @{$profile{sizes}}[$idx + 2];
		return if ($old_size == $new_size);

		my $virtual_sizes = $me-> {virtual_sizes} || [];

		if ($me-> {resizeMethod} == frr::Simple) {
			my $i;
			for ($i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				$old_size -= $me-> {sliders}-> [$i]-> {thickness};
				$new_size -= $me-> {sliders}-> [$i]-> {thickness};
			}
			my (@nsizes, @vsizes);
			my $newTotal = 0;
			my $ratio = ($old_size && $new_size) ? ( $new_size / $old_size ) : 1;
			my ($f, $ns);
			for ($i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				$f = $me-> {frames}-> [$i];
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
			$f = $me-> {frames}-> [$i];
			$ns = $new_size - $newTotal;
			$vsizes[$i] = $ns;
			$ns = 1 if $ns < 1;
			$ns = $f-> {minFrameWidth}
				if defined($f-> {minFrameWidth}) && ($ns < $f-> {minFrameWidth});
			$ns = $f-> {maxFrameWidth}
				if defined($f-> {maxFrameWidth}) && ($ns > $f-> {maxFrameWidth});
			$nsizes[$i] = $ns;

			$me-> {sizes} = \@nsizes;
			$me-> {virtual_sizes} = \@vsizes;
		}

	}
	return (wantarray ? @sizes : \@sizes);
}

sub reset
{
	my $me = shift;

	return unless $me-> owner;

	my $origin = [0, 0];
	my $end = [$me-> {arrangement} == fra::Vertical ? ($me-> width, 1) : (1, $me-> height)];
	my $idx = $me-> {arrangement} == fra::Vertical ? 1 : 0; # What element of origin/size array we change.
	my @sliders = @{$me-> {sliders}};

	for (my $i = 0; $i < $me-> {frameCount}; $i++) {
		$end-> [$idx] = $origin-> [$idx] + $me-> {sizes}-> [$i];
		$me-> {frames}-> [$i]-> rect(@$origin, @$end);
		$origin-> [$idx] += $me-> {sizes}-> [$i];
		if ($i < @{$me-> {sliders}}) {
			$sliders[$i]-> origin(@$origin);
			$origin-> [$idx] += $sliders[$i]-> thickness;
		}
	}
}

sub slider_moved
{
	my $me = shift;
	my ($slider, $delta) = @_;
	return unless $delta;

	my $si = $slider-> sliderIndex;

	if ($me-> {resizeMethod} == frr::Simple) {
		my $frame1 = $slider-> frame1;
		my $frame2 = $slider-> frame2;

		my ($w1, $w2);
		if ($me-> {arrangement} == fra::Horizontal) {
			$w1 = $frame1-> width;
			$w2 = $frame2-> width;
		} else {
			$w1 = $frame1-> height;
			$w2 = $frame2-> height;
		}

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
		if ($me-> {arrangement} == fra::Horizontal) {
			$frame1-> width($nw1);
			@rect = $slider-> rect;
			$rect[0] += $delta;
			$rect[2] += $delta;
			$slider-> rect(@rect);
			@rect = $frame2-> rect;
			$rect[0] += $delta;
		} else {
			$frame1-> height($nw1);
			@rect = $slider-> rect;
			$rect[1] += $delta;
			$rect[3] += $delta;
			$slider-> rect(@rect);
			@rect = $frame2-> rect;
			$rect[1] += $delta;
		}
		$frame2-> rect(@rect);
		$frame1-> update_view;
		$frame2-> update_view;

		$me-> {virtual_sizes}-> [$si]     = $me-> {sizes}-> [$si]     = $nw1;
		$me-> {virtual_sizes}-> [$si + 1] = $me-> {sizes}-> [$si + 1] = $nw2;
	}
}

# Properties
sub arrangement
{
	my $me = shift;
	if (@_ > 0) {
		my $haveIt = exists($me-> {arrangement});
		return if $haveIt && ($me-> {arrangement} == $_[0]);
		$me-> {arrangement} = $_[0];
		if ($me-> {sliders}) {
			for (my $i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				$me-> {sliders}-> [$i]-> vertical($_[0] == fra::Horizontal);
			}
		}
		if ($haveIt) {
			$me-> recalc_frames;
			$me-> reset;
		}
	} else {
		return $me-> {arrangement};
	}
}

sub frameCount
{
# XXX frameCount property only sets number of frames. Needed adjustments
# are too complicated and various to be implemented right here.
	my $me = shift;
	if (@_ > 0) {
		my $haveIt = exists $me-> {frameCount};
		return if $haveIt && $me-> {frameCount} == $_[0];

		my $oldval = ($haveIt ? $me-> {frameCount} : undef);

		$me-> {frameCount} = $_[0];
	} else {
		return $me-> {frameCount};
	}
}

sub sliderWidth
{
	my $me = shift;
	if (@_ > 0) {
		my $haveIt = exists($me-> {sliderWidth});
		return if ($haveIt && ($me-> {sliderWidth} == $_[0])) || ($_[0] < 0);
		$me-> {sliderWidth} = $_[0];
		if ($haveIt) {
			for (my $i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				if ($me-> {sliders}-> [$i]-> enabled) {
					$me-> {sliders}-> [$i]-> thickness($_[0]);
				}
			}
			$me-> recalc_frames;
			$me-> reset;
		}
	} else {
		return $me-> {sliderWidth};
	}
}

sub separatorWidth
{
	my $me = shift;
	if (@_ > 0) {
		my $haveIt = exists($me-> {separatorWidth});
		return if ($haveIt && ($me-> {separatorWidth} == $_[0])) || ($_[0] < 0);
		$me-> {separatorWidth} = $_[0];
		if ($haveIt) {
			for (my $i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				if ($me-> {sliders}-> [$i]-> enabled) {
					$me-> {sliders}-> [$i]-> thickness($_[0]);
				}
			}
			$me-> recalc_frames;
			$me-> reset;
		}
	} else {
		return $me-> {separatorWidth};
	}
}

sub flexible
{
	my $me = shift;
	if (@_ > 0) {
		my $haveIt = exists($me-> {flexible});
		return if $haveIt && ! ($me-> {flexible} xor $_[0]);
		$me-> {flexible} = $_[0] ? 1 : 0;
		if ($haveIt) {
			for (my $i = 0; $i < ($me-> {frameCount} - 1); $i++) {
				$me-> {sliders}-> [$i]-> thickness($me-> {flexible} ? $me-> {sliderWidth} : $me-> {separatorWidth});
				$me-> {sliders}-> [$i]-> enabled( $me-> {flexible});
			}
			$me-> recalc_frames;
			$me-> reset;
	}
	} else {
		return $me-> {flexible};
	}
}

sub frameSizes
{
	return [@{$_[0]-> {frameSizes}}] unless $#_;
	my $me = shift;
	my @fs = ( $_[0] && ref($_[0]) eq 'ARRAY' && 1 == scalar @_) ? @{$_[0]} : @_;
	$me-> {frameSizes} = \@fs;
	$me-> recalc_frames( initial => 1);
	$me-> reset;
}

sub opaqueResize
{
	my $me = shift;
	if (@_ > 0) {
		return if exists($me-> {opaqueResize}) && $me-> {opaqueResize} == $_[0];
		$me-> {opaqueResize} = $_[0];
	} else {
		return $me-> {opaqueResize};
	}
}

sub resizeMethod
{
	my $me = shift;
	if (@_ > 0) {
		return if exists($me-> {resizeMethod}) && $me-> {resizeMethod} == $_[0];
		$me-> {resizeMethod} = $_[0];
	} else {
		return $me-> {resizeMethod};
	}
}

# User interface
sub firstFrame
{
	my $me = shift;
	return $me-> {frames}-> [0];
}

sub lastFrame
{
	my $me = shift;
	return $me-> {frames}-> [-1];
}

sub frames
{
	my $me = shift;
	return wantarray ? @{$me-> {frames}} : $me-> {frames};
}

sub frame
{
	my ( $self, $frameIndex) = @_;
	return $self-> {frames}-> [$frameIndex];
}

sub insert_to_frame
{
	my $me = shift;
	my $frameIdx = shift;

	$frameIdx = $me-> {frameCount} - 1 if $frameIdx > ($me-> {frameCount} - 1);

	$me-> lock;
	my @ctrls = $me-> {frames}-> [$frameIdx]-> insert(@_);
	$me-> unlock;

	return wantarray ? @ctrls : $ctrls[0];
}



1;

=pod

=head1 NAME

Prima::FrameSet - standard frameset widget

=head1 SYNOPSIS

	use Prima qw(Application Buttons FrameSet);

	my $w = Prima::MainWindow->create( size => [300, 150] );

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

Provides standard means of framesets manipulations. It includes sharing of
common workspace among several widget groups; redistribution of space, occupied
by frames; isolation of different frames from each other.

This module defines C<fra::> and C<frr::> packages for constants, used by L<arrangement>
and L<resizeMethod> properties, respectively.

Two additional auxiliary packages are defined within this module: L<Prima::FrameSet::Frame>
and L<Prima::FrameSet::Slider>.

=head1 AUTHOR

Vadim Belman, E<lt>voland@lflat.orgE<gt>

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>,
F<examples/frames.pl>.

=cut
