package Prima::Widget::MouseScroller;

use strict;
use warnings;
use Prima;

my $scrollTimer;

sub scroll_timer_active
{
	return 0 unless defined $scrollTimer;
	return $scrollTimer-> {active};
}

sub scroll_timer_semaphore
{
	return 0 unless defined $scrollTimer;
	$#_ ?
		$scrollTimer-> {semaphore} = $_[1] :
		return $scrollTimer-> {semaphore};
}

sub scroll_timer_stop
{
	return unless defined $scrollTimer;
	$scrollTimer-> stop;
	$scrollTimer-> {active} = 0;
	$scrollTimer-> timeout( $scrollTimer-> {firstRate});
	$scrollTimer-> {newRate} = $scrollTimer-> {nextRate};
}

sub scroll_timer_start
{
	my $self = $_[0];
	$self-> scroll_timer_stop;
	unless ( defined $scrollTimer) {
		my @rates = $::application-> get_scroll_rate;
		$scrollTimer = Prima::Timer-> create(
			owner      => $::application,
			timeout    => $rates[0],
			name       => q(ScrollTimer),
			onTick     => sub { $_[0]-> {delegator}-> ScrollTimer_Tick( @_)},
			onDestroy  => sub { undef $scrollTimer },
		);
		@{$scrollTimer}{qw(firstRate nextRate newRate)} = (@rates,$rates[1]);
	}
	$scrollTimer-> {delegator} = $self;
	$scrollTimer-> {semaphore} = 1;
	$scrollTimer-> {active} = 1;
	$scrollTimer-> start;
}

sub ScrollTimer_Tick
{
	my ( $self, $timer) = @_;
	if ( exists $scrollTimer-> {newRate})
	{
		$timer-> timeout( $scrollTimer-> {newRate});
		delete $scrollTimer-> {newRate};
	}
	$scrollTimer-> {semaphore} = 1;
	$self-> notify(q(MouseMove), 0, $self-> pointerPos);
	$self-> scroll_timer_stop unless defined $self-> {mouseTransaction};
}

1;

=pod

=head1 NAME

Prima::Widget::MouseScroller - auto repeating mouse events

=head1 DESCRIPTION

Implements routines for emulation of auto repeating mouse events.
A code inside C<MouseMove> callback can be implemented by
the following scheme:

	if ( mouse_pointer_inside_the_scrollable_area) {
		$self-> scroll_timer_stop;
	} else {
		$self-> scroll_timer_start unless $self-> scroll_timer_active;
		return unless $self-> scroll_timer_semaphore;
		$self-> scroll_timer_semaphore( 0);
	}

The class uses a semaphore C<{mouseTransaction}>, which should
be set to non-zero if a widget is in mouse capture state, and set
to zero or C<undef> otherwise.

The class starts an internal timer, which sets a semaphore and
calls C<MouseMove> notification when triggered. The timer is
assigned the timeouts, returned by C<Prima::Application::get_scroll_rate>
( see L<Prima::Application/get_scroll_rate> ).

=head1 Methods

=over

=item scroll_timer_active

Returns a boolean value indicating if the internal timer is started.

=item scroll_timer_semaphore [ VALUE ]

A semaphore, set to 1 when the internal timer was triggered. It is advisable
to check the semaphore state to discern a timer-generated event from
the real mouse movement. If VALUE is specified, it is assigned to the semaphore.

=item scroll_timer_start

Starts the internal timer.

=item scroll_timer_stop

Stops the internal timer.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::ScrollBar>.

=cut
