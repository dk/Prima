package Prima::Widget::Fader;
use strict;
use warnings;
use Prima;
use Carp qw(carp);

sub notification_types {{
	FadeIn      => nt::Action,
	FadeOut     => nt::Action,
	FadeRepaint => nt::Action,
}}

sub fader_action_property
{
	my ( $self, $f, %opt ) = @_;

	carp("fader_applicator: 'property' required"), return unless defined $opt{property};
	$f->{property} = $opt{property};
	return sub {
		my ( $self, $f, $value ) = @_;
		my $p = $f->{property};
		$self->$p( $value );
	};
}

sub fader_action_callback
{
	my ( $self, $f, %opt ) = @_;

	carp("fader_applicator: 'callback' required"), return unless defined $opt{callback};
	$f->{callback} = $opt{callback};
	return sub {
		my ( $self, $f, $value ) = @_;
		$f->{callback}->($self, $f, $value);
	};
}

sub fader_type_identity { sub { $_[2] } }

sub fader_type_colorblend
{
	my ( $self, $f, %opt ) = @_;

	for my $k ( qw(from to)) {
		carp("colorblend: '$k' color required"), return unless defined $opt{$k};
		$f->{$k} = $self->map_color($opt{$k});
	}

	return sub {
		my ( $self, $f, $value ) = @_;
		return cl::blend( $f->{from}, $f->{to}, $value);
	};
}

sub fader_function_linear
{
	my ( $self, $f, %opt ) = @_;
	return sub {
		my ( $self, $f ) = @_;
		return $f->{current} / $f->{max};
	};
}


sub fader_timer        { $_[0]->{__fader_timer} ? $_[0]->{__fader_timer}->{timer} : undef }
sub fader_timer_active { $_[0]->fader_timer ? $_[0]->fader_timer-> get_active : 0 }

sub fader_var
{
	return unless my $f = shift->{__fader_timer};
	$f = $f->{vars};
	return $#_ ? $f->{$_[0]} = $_[1] : $f->{$_[0]};
}

sub fader_var_delete
{
	return unless my $f = shift->{__fader_timer};
	delete $f->{vars}->{$_[0]};
}

sub fader_timer_stop
{
	my $self = shift;
	my $f;
	if ( $f = $self->{__fader_timer}) {
		$f->{timer}->destroy if $f->{timer};
		$f->{onEnd}->( $self, 0 ) if $f->{onEnd};
	}
	delete $self->{__fader_timer};
}

sub fader_timer_start
{
	my ($self, %opt) = @_;

	$self-> fader_timer_stop;

	my %obj = (
		quant   => abs($opt{quant} // 50),
		max     => abs($opt{timeout} // 300),
		step    => 0,
		current => 0,
		onEnd   => $opt{onEnd},
		vars    => {},
	);
	return if $obj{max} == 0;

	$obj{steps} = int($obj{max} / $obj{quant}) +!!+ ($obj{max} % $obj{quant});
	$self->{__fader_timer} = \%obj;

	$opt{type}     //= 'identity';
	$opt{function} //= 'linear';
	for my $selector (qw(function type action)) {
		carp("fader_timer_start: $selector required"), next unless defined $opt{$selector};
		next if $obj{$selector} = $self->can("fader_${selector}_$opt{$selector}")->($self, \%obj, %opt);
		delete $self->{__fader_timer};
		carp("unknown $selector $opt{$selector}");
		return;
	}

	$self->{__fader_timer}->{timer} = Prima::Timer-> new(
		owner      => $self,
		name       => q(FadeTimer),
		timeout    => $obj{quant},
		onTick     => sub { $self-> FadeTimer_Tick( @_)},
		onDestroy  => sub { undef $self->{__fader_timer} },
	);

	$self-> fader_timer-> start;
}

sub FadeTimer_Tick
{
	my ( $self, $timer) = @_;
	my $f = $self->{__fader_timer};
	if ( !$f || $f->{stop} ) {
		$f->{onEnd}->( $self, 0 ) if $f && $f->{onEnd};
		$self-> fader_timer_stop;
		return;
	}

	$f->{step}++;
	$f->{current} += $f->{quant};
	if ($f->{current} >= $f->{max}) {
		$f->{current} = $f->{max};
		$f->{stop} = 1;
	}

	my $value = $f-> {function}->( $self, $f );
	$value = $f->{type}->($self, $f, $value );
	$f->{ends_okay} = 1;
	$f->{action}->( $self, $f, $value );
	if ($f->{stop} && ( my $onEnd = delete $f->{onEnd})) {
		$onEnd->( $self, $f, $f->{ends_okay} );
	}
}

sub fader_cancel_if_unbuffered
{
	my $self = shift;
	if ( $self->fader_var('test_buffering')) {
		$self->fader_var_delete('test_buffering');

		my $pid;
		$pid = $self->add_notification( Paint => sub {
			$self->remove_notification($pid);
			if ( $self->buffered && !$self->is_surface_buffered ) {
				# refuse fading to avoid horrible blinking
				my $f = $self->{__fader_timer};
				$f->{current} = $f->{max};
				$f->{stop} = 1;
				$f->{ends_okay} = 0;
				$self->fader_var( current => (($self->fader_var('action') // '' ) eq 'in') ? 1 : 0);
			}
			$self->notify('Paint', $_[1]);
		});
	}
}

sub on_faderepaint
{
	my $self = shift;
	$self->fader_cancel_if_unbuffered;
	$self->repaint;
}

sub on_fadein  {}
sub on_fadeout {}

sub fader_in_mouse_enter
{
	my ($self) = @_;
	my $action = $self->fader_var('action') // '';
	return if $action eq 'in';

	my $turn_buffering_off = !$self->buffered;
	$self->buffered(1) if $turn_buffering_off;
	my $turn_syncpaint_off = !$self->syncPaint;
	$self->syncPaint(1) if $turn_syncpaint_off;
	$self-> fader_timer_start(
		action   => 'callback',
		callback => sub {
			my ( $self, $f, $value ) = @_;
			$self->fader_var( current => $value );
			$self->notify(q(FadeRepaint));
		},
		onEnd    => sub {
			my ( $self, $f, $ends_okay ) = @_;
			$self->notify(q(FadeIn), $ends_okay);
			$self->buffered(0) if $turn_buffering_off;
			$self->syncPaint(0) if $turn_syncpaint_off;
		},
	);
	$self->fader_var( current => 0.0 );
	$self->fader_var( action  => 'in');
	$self->fader_var( test_buffering => 1 );
}

sub fader_out_mouse_leave
{
	my ($self) = @_;

	my $action = $self->fader_var('action') // '';
	return if $action eq 'out';

	my $turn_buffering_off = !$self->buffered;
	$self->buffered(1) if $turn_buffering_off;
	my $turn_syncpaint_off = !$self->syncPaint;
	$self->syncPaint(1) if $turn_syncpaint_off;
	$self-> fader_timer_start(
		action   => 'callback',
		callback => sub {
			my ( $self, $f, $value ) = @_;
			$self->fader_var( current => (1.0 - $value) );
			$self->notify(q(FadeRepaint));
		},
		onEnd    => sub {
			my ( $self, $f, $ends_okay ) = @_;
			$self->notify(q(FadeOut), $ends_okay);
			$self->buffered(0) if $turn_buffering_off;
			$self->syncPaint(0) if $turn_syncpaint_off;
		},
	);
	$self->fader_var( action  => 'out');
	$self->fader_var( test_buffering => 1 ) if $self->buffered;
}

sub fader_current_value  { shift->fader_var('current') }

sub fader_prelight_color
{
	my ( $self, $color, $mul ) = @_;
	my $f = ($self-> fader_current_value // 1.0) * ( $mul // 1.0);
	my $p = $self-> prelight_color($color);
	return cl::blend( $color, $p, $f);
}

sub fader_set_blended_color
{
	my ( $self, $cl1, $cl2 ) = @_;
	my $f = $self->fader_current_value // 0;
	$cl1 = $self->map_color($cl1);
	$cl2 = $self->map_color($cl2);
	$self->color( cl::blend( $cl1, $cl2, $f ));
}

1;

=pod

=head1 NAME

Prima::Widget::Fader - fading- in/out functions

=head1 DESCRIPTION

The role implements fading effects in widgets

=head1 SYNOPSIS

	use base qw(Prima::Widget Prima::Widget::Fader);
	{
	my %RNT = (
		%{Prima::Widget-> notification_types()},
		%{Prima::Widget::Fader-> notification_types()},
	);
	sub notification_types { return \%RNT; }
	}

	sub on_mouseenter { shift-> fader_in_mouse_enter }
	sub on_mouseleave { shift-> fader_out_mouse_leave }

	sub on_paint
	{
		my ( $self, $canvas ) = @_;
		$canvas->backColor( $self-> fader_prelight_color( $self-> hiliteBackColor ));
		$canvas->clear;
	}


=head1 API

The API is currently under design so the parts that are documented are those that expected
to be staying intact.

=head2 Methods

=over

=item fader_in_mouse_enter

Initiates a fade-in transition, calls repaint on each step.

=item fader_out_mouse_leave

Initiates a fade-out transition, calls repaint on each step.

=item fader_current_value

Returns the current fader value in the range from 0 to 1.
Returns C<undef> if there is no current fading transition in effect

=item fader_prelight_color $COLOR [, $MULTIPLIER ]

Given a base C<$COLOR>, increases (or decreases) its brightness according to
C<fader_current_value> and an eventual C<$MULTIPLIER> that is expected to be in
the range from 0 to 1.

=back

=head2 Events

=over

=item FadeIn $ENDS_OK

Called when C<fader_in_mouse_enter> finishes the fading, the C<$ENDS_OK> flag
is set to 0 if the process was overridden by another fader call, 1 otherwise.

=item FadeOut $ENDS_OK

Called when C<fader_out_mouse_leave> finishes the fading, the C<$ENDS_OK> flag
is set to 0 if the process was overridden by another fader call, 1 otherwise.

=item FadeRepaint

By default repaints the whole widget, but can be overloaded if only some
widget parts need to reflect the fader effect.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Widget>

=cut
