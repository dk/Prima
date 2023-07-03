package Prima::Widget::Fade;
use strict;
use warnings;
use Prima;
use Carp qw(carp);

sub fade_action_property
{
	my ( $self, $f, %opt ) = @_;

	carp("fade_applicator: 'property' required"), return unless defined $opt{property};
	$f->{property} = $opt{property};
	return sub {
		my ( $self, $f, $value ) = @_;
		my $p = $f->{property};
		$self->$p( $value );
	};
}

sub fade_action_callback
{
	my ( $self, $f, %opt ) = @_;

	carp("fade_applicator: 'callback' required"), return unless defined $opt{callback};
	$f->{callback} = $opt{callback};
	return sub {
		my ( $self, $f, $value ) = @_;
		$f->{callback}->($self, $f, $value);
	};
}

sub fade_type_identity { sub { $_[2] } }

sub fade_type_colorblend
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

sub fade_function_linear
{
	my ( $self, $f, %opt ) = @_;
	return sub {
		my ( $self, $f ) = @_;
		return $f->{current} / $f->{max};
	};
}


sub fade_timer        { $_[0]->{__fade_timer} ? $_[0]->{__fade_timer}->{timer} : undef }
sub fade_timer_active { $_[0]->fade_timer ? $_[0]->fade_timer-> get_active : 0 }

sub fade_var
{
	return unless my $f = shift->{__fade_timer};
	$f = $f->{vars};
	return $#_ ? $f->{$_[0]} = $_[1] : $f->{$_[0]};
}

sub fade_var_delete
{
	return unless my $f = shift->{__fade_timer};
	delete $f->{vars}->{$_[0]};
}

sub fade_timer_stop
{
	my $self = shift;
	my $f;
	if ( $f = $self->{__fade_timer}) {
		$f->{timer}->destroy if $f->{timer};
		$f->{onEnd}->( $self, 0 ) if $f->{onEnd};
	}
	delete $self->{__fade_timer};
}

sub fade_timer_start
{
	my ($self, %opt) = @_;

	$self-> fade_timer_stop;

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
	$self->{__fade_timer} = \%obj;

	$opt{type}     //= 'identity';
	$opt{function} //= 'linear';
	for my $selector (qw(function type action)) {
		carp("fade_timer_start: $selector required"), next unless defined $opt{$selector};
		next if $obj{$selector} = $self->can("fade_${selector}_$opt{$selector}")->($self, \%obj, %opt);
		delete $self->{__fade_timer};
		carp("unknown $selector $opt{$selector}");
		return;
	}

	$self->{__fade_timer}->{timer} = Prima::Timer-> new(
		owner      => $self,
		name       => q(FadeTimer),
		timeout    => $obj{quant},
		onTick     => sub { $self-> FadeTimer_Tick( @_)},
		onDestroy  => sub { undef $self->{__fade_timer} },
	);

	$self-> fade_timer-> start;
}

sub FadeTimer_Tick
{
	my ( $self, $timer) = @_;
	my $f = $self->{__fade_timer};
	if ( !$f || $f->{stop} ) {
		$f->{onEnd}->( $self, 0 ) if $f && $f->{onEnd};
		$self-> fade_timer_stop;
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

sub fade_in_mouse_enter
{
	my $self = shift;
	$self-> fade_timer_start(
		action   => 'callback',
		callback => sub {
			my ( $self, $f, $value ) = @_;
			$self->fade_var( current => $value );
			$self->repaint;
		},
		onEnd    => sub {
			my ( $self, $f, $ends_okay ) = @_;
			$self->fade_var_delete( 'current' );
			if ($ends_okay) {
				$self->{hilite} = 1;
				$self->repaint;
			}
		},
	);
	$self->fade_var( current => 0.0 );
}

sub fade_out_mouse_leave
{
	my $self = shift;
	if ( $self-> {hilite} or defined $self->fade_var('current') ) {
		$self-> fade_timer_start(
			action   => 'callback',
			callback => sub {
				my ( $self, $f, $value ) = @_;
				$self->fade_var( current => (1.0 - $value) );
				$self->repaint;
			},
			onEnd    => sub {
				shift->fade_var_delete( 'current' );
			},
		);
		undef $self-> {hilite};
	}
}

sub fade_current_value { shift->fade_var('current') }

sub fade_current_color
{
	my ( $self, $color) = @_;
	$color = $self->map_color($color);
	if ( $self->{hilite}) {
		return $self-> prelight_color($color);
	} elsif ( defined ( my $f = $self->fade_var('current'))) {
		return cl::blend( $color, $self-> prelight_color($color), $f);
	} else {
		return $color;
	}
}

1;

=head1 NAME

Prima::Widget::Fade

=head1 DESCRIPTION

Fading- in/out functions

=head2 SYNOPSIS

	use base qw(Prima::Widget Prima::Widget::Fade);

	sub on_mouseenter { shift-> fade_in_mouse_enter }
	sub on_mouseleave { shift-> fade_out_mouse_leave }

	sub on_paint
	{
		my ( $self, $canvas ) = @_;
		my $color = $self->{hilite} ? $self->hiliteBackColor : $self->backColor;
		$canvas->backColor( $self-> fade_current_color($color) );
		$canvas->clear;
	}

=cut
