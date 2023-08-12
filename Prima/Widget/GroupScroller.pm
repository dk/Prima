package Prima::Widget::GroupScroller;
use base qw(Prima::Widget::IntIndents);

use strict;
use warnings;
use Prima;
use Prima::ScrollBar;

sub CORE_METHODS { qw(profile_default profile_check_in init skin) }

sub profile_default
{
	my ( $orig, $self) = @_;
	my $def = $orig->($self);
	my $new = {
		autoHScroll       => 1,
		autoVScroll       => 1,
		borderWidth       => undef,
		hScroll           => 0,
		hScrollBarProfile => {},
		scrollBarClass    => 'Prima::ScrollBar',
		vScroll           => 0,
		vScrollBarProfile => {},
		%$def,
	};
	return $new;
}

sub profile_check_in
{
	my ( $orig, $self, $p, $default) = @_;
	$orig->($self, $p, $default);
	$p-> {autoHScroll} = 0 if exists $p-> {hScroll};
	$p-> {autoVScroll} = 0 if exists $p-> {vScroll};
	if ( ! defined $p->{borderWidth} && ! defined $default->{borderWidth}) {
		my $skin = $p->{skin} // $default->{skin} // ( $p->{owner} ? $p->{owner}->skin : '' );
		$p->{borderWidth} = ($skin eq 'flat') ? 1 : 2;
	}
}

sub init
{
	my ($orig, $self) = (shift, shift);
	$self->{$_} = 0 for qw(autoVScroll autoHScroll hScroll vScroll borderWidth GS_extra_border);
	my %profile = $orig->($self, @_);
	$self->{$_} = $profile{$_} for qw(scrollBarClass hScrollBarProfile vScrollBarProfile);
	$self-> $_( $profile{ $_}) for qw(borderWidth autoHScroll autoVScroll hScroll vScroll);
	return %profile;
}

sub skin
{
	my $orig = shift;
	return $orig->(@_) unless $#_;
	my $self = shift;
	$orig->($self, $_[0]);
	$self->{GS_extra_border} = ($orig->($self) eq 'flat') ? 1 : 0;
	$self->repaint;
}

sub setup_indents
{
	my ($self) = @_;
	$self-> {indents} = [ 0,0,0,0];
	my $bw = $self-> {borderWidth};
	my $ebw = $self->{GS_extra_border} - 1;
	$self-> {indents}-> [$_] += $bw for 0..3;
	$self-> {indents}-> [1] += $self-> {hScrollBar}-> height + $ebw if $self-> {hScroll};
	$self-> {indents}-> [2] += $self-> {vScrollBar}-> width  + $ebw if $self-> {vScroll};
}

sub set_border_width
{
	my ( $self, $bw) = @_;

	my @size = $self-> size;
	$bw = 0 if $bw < 0;
	$bw = 1 if $bw > $size[1] / 2;
	$bw = 1 if $bw > $size[0] / 2;
	return if $bw == $self-> {borderWidth};

	my $obw  = $self-> {borderWidth};
	$self-> {borderWidth} = $bw;

	my $ebw = $self->{GS_extra_border} - 1;
	$self-> {hScrollBar}-> set(
		left   => $bw + $ebw,
		bottom => $bw + $ebw,
		width  => $size[0] - 
			2 * $bw -
			( $self-> {vScroll} ?
				$self-> {vScrollBar}-> width + $ebw * 3:
				-$ebw * 2),
	) if $self-> {hScroll};

	$self-> {vScrollBar}-> set(
		right        => $size[0] - $bw - $ebw,
		top          => $size[1] - $bw - $ebw,
		bottom       => $bw - $ebw + ( $self-> {hScroll} ? $self-> {hScrollBar}-> height + 3 * $ebw : 0),
	) if $self-> {vScroll};

	$self-> insert_bone if defined $self-> {bone};

	$self-> setup_indents;
	$self-> reset_indents;
}

sub reset_indents {}

sub insert_bone
{
	my $self = $_[0];
	my $bw   = $self-> {borderWidth};
	$self-> {bone}-> destroy if defined $self-> {bone};

	my $ebw = $self->{GS_extra_border} - 1;
	$self-> {bone} = Prima::Widget-> new(
		owner       => $self,
		name        => q(Bone),
		pointerType => cr::Arrow,
		origin      => [
			$self-> width - $self-> {vScrollBar}-> width - $ebw * 2 - $bw,
			$bw + $ebw
		],
		size        => [
			$self-> {vScrollBar}-> width  + $ebw,
			$self-> {hScrollBar}-> height + $ebw
		],
		growMode    => gm::GrowLoX,
		widgetClass => wc::ScrollBar,
		designScale => undef,
		onPaint     => sub {
			my ( $self, $canvas, $owner, $w, $h) =
				($_[0], $_[1], $_[0]-> owner, $_[0]-> size);
			return $canvas->clear if $owner->skin eq 'flat';
			$canvas-> color( $self-> backColor);
			$canvas-> bar( 0, 1, $w - 2, $h - 1);
			$canvas-> color( $owner-> light3DColor);
			$canvas-> line( 0, 0, $w - 1, 0);
			$canvas-> line( $w - 1, 0, $w - 1, $h - 1);
		},
	);
}

sub set_h_scroll
{
	my ( $self, $hs) = @_;
	return if ($hs ? 1 : 0) == $self-> {hScroll};
	my $bw = $self-> {borderWidth} || 0;
	my $ebw = $self->{GS_extra_border} - 1;
	if ( $hs) {
		$self-> {hScrollBar} = $self->{scrollBarClass}-> new(
			owner       => $self,
			name        => q(HScroll),
			vertical    => 0,
			origin      => [ $bw + $ebw, $bw + $ebw],
			growMode    => gm::GrowHiX,
			pointerType => cr::Arrow,
			width       => $self-> width -
				2 * $bw -
				( $self-> {vScroll} ?
					$self-> {vScrollBar}-> width + $ebw * 3:
					-$ebw * 2),
			delegations => ['Change'],
			designScale => undef,
			%{ $self->{hScrollBarProfile} || {} },
		);
		$self-> {hScroll} = 1;

		$self-> setup_indents;

		if ( $self-> {vScroll}) {
			my $h = $self-> {hScrollBar}-> height;
			$self-> {vScrollBar}-> set(
				bottom => $self-> {vScrollBar}-> bottom + $h + $ebw,
				top    => $self-> {vScrollBar}-> top,
			);
			$self-> insert_bone;
		}
	} else {
		$self-> {hScroll} = 0;
		$self-> setup_indents;
		$self-> {hScrollBar}-> destroy;

		if ( $self-> {vScroll})
		{
			$self-> {vScrollBar}-> set(
				bottom => $bw + $ebw,
				height => $self-> height - $bw * 2 - $ebw * 2,
			);
			$self-> {bone}-> destroy;
			delete $self-> {bone};
		}
	}
	$self-> reset_indents;
}

sub set_v_scroll
{
	my ( $self, $vs) = @_;
	return if ($vs ? 1 : 0) == $self-> {vScroll};

	my $bw = $self-> {borderWidth} || 0;
	my @size = $self-> size;
	my $ebw = $self->{GS_extra_border} - 1;

	if ( $vs) {
		my $width = exists( $self->{vScrollBarProfile}->{width} ) ?
			$self->{vScrollBarProfile}->{width} :
			$Prima::ScrollBar::stdMetrics[0];
		$self-> {vScrollBar} = $self->{scrollBarClass}-> new(
			owner        => $self,
			name         => q(VScroll),
			vertical     => 1,
			left         => $size[0] - $bw - $width - $ebw,
			top          => $size[1] - $bw - $ebw,
			bottom       => $bw - $ebw + ( $self-> {hScroll} ? $self-> {hScrollBar}-> height + 3 * $ebw : 0),
			growMode     => gm::GrowLoX | gm::GrowHiY,
			pointerType  => cr::Arrow,
			delegations  => ['Change'],
			designScale  => undef,
			%{ $self->{vScrollBarProfile} || {} },
		);
		$self-> {vScroll} = 1;

		$self-> setup_indents;

		if ( $self-> {hScroll}) {
			$self-> {hScrollBar}-> width(
				$size[0] - 2 * $bw - $ebw * 2 - $self->{vScrollBar}->width - $ebw, 
			);
			$self-> insert_bone;
		}
	} else {
		$self-> {vScroll} = 0;
		$self-> setup_indents;
		$self-> {vScrollBar}-> destroy;
		if ( $self-> {hScroll})
		{
			$self-> {hScrollBar}-> width( $size[0] - 2 * $bw - $ebw * 2);
			$self-> {bone}-> destroy;
			delete $self-> {bone};
		}
	}
	$self-> reset_indents;
}

sub autoHScroll
{
	return $_[0]-> {autoHScroll} unless $#_;
	my $v = ( $_[1] ? 1 : 0);
	return unless $v != $_[0]-> {autoHScroll};
	$_[0]-> {autoHScroll} = $v;
}

sub autoVScroll
{
	return $_[0]-> {autoVScroll} unless $#_;
	my $v = ( $_[1] ? 1 : 0);
	return unless $v != $_[0]-> {autoVScroll};
	$_[0]-> {autoVScroll} = $v;
}

sub borderWidth     {($#_)?($_[0]-> set_border_width( $_[1])):return $_[0]-> {borderWidth}}
sub hScroll         {($#_)?$_[0]-> set_h_scroll       ($_[1]):return $_[0]-> {hScroll}}
sub vScroll         {($#_)?$_[0]-> set_v_scroll       ($_[1]):return $_[0]-> {vScroll}}

sub draw_border
{
	my ( $self, $canvas, $backColor, @size) = @_;

	@size = $self-> size unless @size;
	@size = Prima::rect->new(@size)->inclusive;
	if ( $self-> skin eq 'flat') {
		if ( defined $backColor ) {
			$canvas-> rect_fill(
				@size,
				$self->{borderWidth},
				$self-> dark3DColor,
				$backColor
			);
		} else {
			$canvas-> rect_solid(
				@size,
				$self->{borderWidth},
				$self-> dark3DColor,
			);
		}
	} else {
		$self-> rect_bevel(
			$canvas, @size,
			width => $self-> {borderWidth},
			panel => 1,
			fill  => $backColor,
		);
	}
}

1;

=pod

=head1 NAME

Prima::Widget::GroupScroller - optional automatic scroll bars

=head1 DESCRIPTION

The class is used for widgets that contain optional scroll bars, and provides means for
their maintenance. The class is the descendant of L<Prima::IntIndents>, and adjusts
the L<indents> property when scrollbars are shown or hidden, or L<borderWidth> is changed.

The class does not provide range selection for the scrollbars; the descentant classes
must implement that.

The descendant classes must follow the guidelines:

=over

=item *

A class must provide C<borderWidth>, C<hScroll>, and C<vScroll> property keys in profile_default() .
A class may provide C<autoHScroll> and C<autoVScroll> property keys in profile_default() .

=item *

A class' init() method must set C<{borderWidth}>, C<{hScroll}>, and C<{vScroll}>
variables to 0 before the initialization, call C<setup_indents> method,
and then assign the properties from the object profile.

If a class provides C<autoHScroll> and C<autoVScroll> properties, these must be set to
0 before the initialization.

=item *

If a class needs to overload one of C<borderWidth>, C<hScroll>, C<vScroll>,
C<autoHScroll>, and C<autoVScroll> properties,
it is mandatory to call the inherited properties.

=item *

A class must implement the scroll bar notification callbacks: C<HScroll_Change> and C<VScroll_Change>.

=item *

A class must not use the reserved variable names, which are:

	{borderWidth}  - internal borderWidth storage
	{hScroll}      - internal hScroll value storage
	{vScroll}      - internal vScroll value storage
	{hScrollBar}   - pointer to the horizontal scroll bar
	{vScrollBar}   - pointer to the vertical scroll bar
	{bone}         - rectangular widget between the scrollbars
	{autoHScroll}  - internal autoHScroll value storage
	{autoVScroll}  - internal autoVScroll value storage

The reserved method names:

	set_h_scroll
	set_v_scroll
	insert_bone
	setup_indents
	reset_indents
	borderWidth
	autoHScroll
	autoVScroll
	hScroll
	vScroll

The reserved widget names:

	HScroll
	VScroll
	Bone

=back

=head1 Properties

=over

=item autoHScroll BOOLEAN

Selects if the horizontal scrollbar is to be shown and hidden dynamically,
depending on the widget layout.

=item autoVScroll BOOLEAN

Selects if the vertical scrollbar is to be shown and hidden dynamically,
depending on the widget layout.

=item borderWidth INTEGER

Width of 3d-shade border around the widget.

Recommended default value: 2

=item hScroll BOOLEAN

Selects if the horizontal scrollbar is visible. If it is, C<{hScrollBar}>
points to it.

=item vScroll BOOLEAN

Selects if the vertical scrollbar is visible. If it is, C<{vScrollBar}>
points to it.

=item scrollBarClass STRING = Prima::ScrollBar

Create-only property that allows to change scrollbar class

=item hScrollBarProfile, vScrollBarProfile HASH

Create-only property that allows to change scrollbar parameters when it is being created

=back

=head1 Methods

=over

=item setup_indents

The method is never called directly; it should be called whenever widget
layout is changed so that indents are affected. The method is a request
to recalculate indents, depending on the widget layout.

The method is not reentrant; to receive this callback and update the widget
layout, that in turn can result in more C<setup_indents> calls, overload
C<reset_indents> .

=item reset_indents

Called after C<setup_indents> is called and internal widget layout is updated,
to give a chance to follow-up the layout changes.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Lists>, L<Prima::Edit>
