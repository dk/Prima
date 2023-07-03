package Prima::Widget::Fader;
use strict;
use warnings;
use Prima;
use Carp qw(carp);

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
	$f->{action}->( $self, $f, $value );
	if ($f->{stop} && ( my $onEnd = delete $f->{onEnd})) {
		$onEnd->( $self, $f, 1 );
	}
}

sub fader_in_mouse_enter
{
	my $self = shift;
	$self-> fader_timer_start(
		action   => 'callback',
		callback => sub {
			my ( $self, $f, $value ) = @_;
			$self->fader_var( current => $value );
			$self->repaint;
		},
		onEnd    => sub {
			my ( $self, $f, $ends_okay ) = @_;
			$self->fader_var_delete( 'current' );
			if ($ends_okay) {
				$self->{hilite} = 1;
				$self->repaint;
			}
		},
	);
	$self->fader_var( current => 0.0 );
}

sub fader_out_mouse_leave
{
	my $self = shift;
	if ( $self-> {hilite} or defined $self->fader_var('current') ) {
		$self-> fader_timer_start(
			action   => 'callback',
			callback => sub {
				my ( $self, $f, $value ) = @_;
				$self->fader_var( current => (1.0 - $value) );
				$self->repaint;
			},
			onEnd    => sub {
				shift->fader_var_delete( 'current' );
			},
		);
		undef $self-> {hilite};
	}
}

sub fader_current_value { shift->fader_var('current') }

sub fader_current_color
{
	my ( $self, $color) = @_;
	$color = $self->map_color($color);
	if ( $self->{hilite}) {
		return $self-> prelight_color($color);
	} elsif ( defined ( my $f = $self->fader_var('current'))) {
		return cl::blend( $color, $self-> prelight_color($color), $f);
	} else {
		return $color;
	}
}

1;

=head1 NAME

Prima::Widget::Fader

=head1 DESCRIPTION

Fading- in/out functions

=head2 SYNOPSIS

	use base qw(Prima::Widget Prima::Widget::Fade);

	sub on_mouseenter { shift-> fader_in_mouse_enter }
	sub on_mouseleave { shift-> fader_out_mouse_leave }

	sub on_paint
	{
		my ( $self, $canvas ) = @_;
		my $color = $self->{hilite} ? $self->hiliteBackColor : $self->backColor;
		$canvas->backColor( $self-> fader_current_color($color) );
		$canvas->clear;
	}

=cut
