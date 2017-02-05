package Prima::Spinner;
use Prima;
use strict;
use warnings;
our @ISA = qw(Prima::Widget);

sub profile_default
{
	my $self = shift;
	return {
		%{$self-> SUPER::profile_default},
		style => 'circle',
		scaleFactor => 3,
		active => 1,
		buffered => 1,
		color => cl::White, 
		hiliteColor => cl::Black
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self = shift;
	
	# Create the timer for the motion of the spinner
	$self->{timer} = $self->insert(
	'Timer',
	name => 'Timer',
	timeout => 200,
	delegations => ['Tick'],
	);
	
	# Init the Spinner
	my %profile = $self-> SUPER::init(@_);

	for (qw( style scaleFactor active color hiliteColor))
	{$self-> $_($profile{$_}); }
	
	$self->{start_angle} = 0;
	$self->{end_angle} = 150;
	
	return %profile;
}

sub Timer_Tick {
	my $self = shift;
	my $startang = $self->{start_angle}; #|| 0;
		my $endang = $self->{end_angle}; #|| 360;
		if ($self->{style} eq 'drops') {
			$self->{start_angle} = $startang + 1 if ($startang < 7);
			$self->{start_angle} = 0 if ($startang == 7);
		}
		else {
			$self->{start_angle} = $startang - 1 if ($startang > 0);
			$self->{start_angle} = 360 if ($startang == 0);
		}
		$self->{end_angle} = $endang - 1 if ($endang > 0);
		$self->{end_angle} = 360 if ($endang == 0);
		
		$self->repaint();
}

# Events
sub on_paint {
	my ($self,$canvas) = @_;

	$canvas->clear;
		
	my $x = $self->width/2; my $y = $self->height/2;
	my $scale_factor = $self->scaleFactor;
	my $color1 = $self->color;
	my $color2 = $self->hiliteColor;	
		if ($self->{style} eq "circle") {
			$canvas->color($color1);
		 	$canvas->fill_ellipse($x, $y, 20*$scale_factor,20*$scale_factor);
			$canvas->color($self->backColor);
			$canvas->fill_ellipse($x, $y, 14*$scale_factor,14*$scale_factor);
			$canvas->lineWidth(1.5*$scale_factor);
			$canvas->color($color2);
			$canvas->arc($x, $y, 17.5*$scale_factor,17.5*$scale_factor,$self->{start_angle}, $self->{end_angle});
		}
		else {
			# top
			if ($self->{start_angle} == 6  || $self->{start_angle} == 7 || $self->{start_angle} == 0) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }  
			$canvas->fill_spline([ $x, $y+2*$scale_factor, $x-2*$scale_factor, $y+10*$scale_factor, $x, $y+12*$scale_factor, $x+2*$scale_factor, $y+10*$scale_factor, $x, $y+2*$scale_factor]);
		
			# top right
			if ($self->{start_angle} == 7 || $self->{start_angle} == 0 || $self->{start_angle} == 1) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x+1*$scale_factor, $y+1*$scale_factor,	$x+3*$scale_factor, $y+7*$scale_factor, $x+6*$scale_factor, $y+7*$scale_factor,	$x+6*$scale_factor, $y+4*$scale_factor, $x+1*$scale_factor, $y+1*$scale_factor]);
			
			# right
			if ($self->{start_angle} == 0 || $self->{start_angle} == 1 || $self->{start_angle} == 2) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x+2*$scale_factor, $y, $x+8*$scale_factor, $y+2*$scale_factor, $x+10*$scale_factor, $y, $x+8*$scale_factor, $y-2*$scale_factor, $x+2*$scale_factor, $y]);
			
			# right bottom
			if ($self->{start_angle} == 1 || $self->{start_angle} == 2 || $self->{start_angle} == 3) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x+1*$scale_factor, $y-1*$scale_factor, $x+6*$scale_factor, $y-4*$scale_factor, $x+6*$scale_factor, $y-7*$scale_factor, $x+3*$scale_factor, $y-7*$scale_factor, $x+1*$scale_factor, $y-1*$scale_factor]);
		
			# bottom
			if ($self->{start_angle} == 2 || $self->{start_angle} == 3 || $self->{start_angle} == 4) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x, $y-2*$scale_factor, $x+2*$scale_factor, $y-8*$scale_factor, $x, $y-10*$scale_factor, $x-2*$scale_factor, $y-8*$scale_factor, $x, $y-2*$scale_factor]);
		
			# bottom left
			if ($self->{start_angle} == 3 || $self->{start_angle} == 4 || $self->{start_angle} == 5) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x-1*$scale_factor, $y-1*$scale_factor, $x-6*$scale_factor, $y-4*$scale_factor, $x-6*$scale_factor, $y-7*$scale_factor, $x-3*$scale_factor, $y-7*$scale_factor, $x-1*$scale_factor, $y-1*$scale_factor]);
			
			# left
			if ($self->{start_angle} == 4 || $self->{start_angle} == 5 || $self->{start_angle} == 6) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x-2*$scale_factor, $y, $x-8*$scale_factor, $y+2*$scale_factor, $x-10*$scale_factor, $y, $x-8*$scale_factor, $y-2*$scale_factor, $x-2*$scale_factor, $y]);
			
			# top left
			if ($self->{start_angle} == 5 || $self->{start_angle} == 6 || $self->{start_angle} == 7) {
				$canvas->color($color2);
			}
			else { $canvas->color($color1); }
			$canvas->fill_spline([ $x-1*$scale_factor, $y+1*$scale_factor,	$x-3*$scale_factor, $y+7*$scale_factor, $x-6*$scale_factor, $y+7*$scale_factor,	$x-6*$scale_factor, $y+4*$scale_factor, $x-1*$scale_factor, $y+1*$scale_factor]);
		
		}

}

# properties
sub style {
	my ($self) = @_;
	return $self-> {style} unless $#_;
	$self->{style} = $_[1];
	
	# When the style property is changed, reset the timer frequency
	# and the start_angle and for style circle the end_angle, too
	$self->{timer}->timeout(200) if ($_[1] eq 'drops');
	$self->{timer}->timeout(8) if ($_[1] eq 'circle');
	$self->{start_angle} = 0;
	$self->{end_angle} = 150;
}

sub scaleFactor {
	my ($self) = @_;
	return $self-> {scaleFactor} unless $#_;
	$self->{scaleFactor} = $_[1];
}

sub active {
	if ($#_) {
		my $self = shift;
		my $active = shift;
		if ($active == 0) {
			$self->{timer}->stop;
			$self->{active} = 0;
		}
		else {
			$self->{timer}->start;
			$self->{active} = 1;
		}
	}
	else
	{
		my $self = shift;
		return $self->{active};
	}
}

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Prima::Spinner - Show a spinner animation

=head1 SYNOPSIS

  use Prima;
  use Prima::Application;
  use Prima::Buttons;

  my $mw = Prima::MainWindow->new(
		size => [200, 400],
		text => 'Button Example');

  my $spinner = $mw->insert('Spinner',
	style => 'drops',
	size => [200,400],
	growMode => gm::Center
  );

  my $button = $mw->insert(
	'Button',
	text => 'Start/Stopp',
	checkable => 1,
	checked => 1,
	origin => [0,0],
	onClick => \&do_clicked,
	growMode => gm::XCenter
  );

  run Prima;

  sub do_clicked {
	my ($self) = @_;
	if ($spinner->active) {
		$spinner->active(0);
	}
	else {
		$spinner->active(1);
	}
  };

=head1 DESCRIPTION

Prima::Spinner provides a simple spinning animation with two designs and the opportunity to specify the colors of the spinning animation. This is useful to show an indeterminate progress of a running process.

=head1 USAGE

You can determine the following properties:

=head2 Properties

=over

=item style CHAR

C<style> can be 'circle' or 'drops'. With C<'circle'> an arc moving around a circle is shown. C<'drops'> shows drops are shown that switches consecutively the color.

=item color COLOR

Inherited from L<Prima::Widget>. C<color> manages the basic foreground color. Concrete for the spinner widget this means the background color of the circle or the color of the inactive drops.

=item hiliteColor COLOR

Inherited from L<Prima::Widget>. The color used to draw alternate foreground areas with high contrast. For the spinner widget this means the color of the arc in the circle style or the color of the active drops in the drops style.

=item active [BOOLEAN]

If now parameter is passed, by this method you can get the active state of the spinning widget. '1' means that the spinner is running, '0' that it is stopped. With C<active(1)> you can start the spinner animation, with C<active(0)> you can stop it.

=back

=head1 SEE ALSO

L<Prima>. L<Prima::Widget>, F<examples/spinner.pl>

=head1 AUTHOR

Maximilian Lika

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2017 by Maximilian Lika

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.22.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
