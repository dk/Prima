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
		style       => 'circle',
		active      => 1,
		buffered    => 1,
		value       => 50,
		showPercent => 1,
		color       => cl::White, 
		hiliteColor => cl::Black
	}
}

sub init
{
	my $self = shift;
	
	# Create the timer for the motion of the spinner
	$self->{timer} = $self->insert( Timer => 
		name        => 'Timer',
		timeout     => 200,
		delegations => ['Tick'],
	);
	
	# Init the Spinner
	my %profile = $self-> SUPER::init(@_);
	$self->{start_angle} = 0;
	$self-> $_($profile{$_}) for qw( value showPercent style active color hiliteColor);
	
	return %profile;
}

sub Timer_Tick
{
	my $self = shift;
	my $startang = $self->{start_angle}; #|| 0;
	if ($self->{style} eq 'drops') {
		$self->{start_angle} = $startang + 1 if $startang < 7;
		$self->{start_angle} = 0 if $startang == 7;
	}
	else {
		$self->{start_angle} = $startang - 1 if $startang > 0;
		$self->{start_angle} = 360 if $startang == 0;
	}

	$self->update_value;
	$self->repaint;
}

sub _pairwise(&$)
{
	my ($sub, $p) = @_;
	my @p;
    	for ( my $i = 0; $i < @$p; $i += 2 ) {
    		push @p, $sub->($p->[$i], $p->[$i+1]);
    	}
	return \@p;
}

sub _scale
{
	my ( $p, $xmul, $ymul) = @_;
	_pairwise { $xmul * $_[0], $ymul * $_[1] } $p
}

sub _v_flip($) { _scale(shift, 1, -1)   }
sub _h_flip($) { _scale(shift, -1, 1)   }
sub _rotate($) { _pairwise { pop, pop } shift }

# Events
sub on_paint
{
	my ($self,$canvas) = @_;

	$canvas->clear;
		
	my $x            = $self->width/2; 
	my $y            = $self->height/2;
	my $min          = ( $x < $y ) ? $x : $y;
	my $scale_factor = $min / 12;
	my $color1       = $self->color;
	my $color2       = $self->hiliteColor;

	if ($self->{style} eq "circle") {
		$canvas->color($color1);
		$canvas->lineWidth(3*$scale_factor);
		$canvas->ellipse($x, $y, 17.5*$scale_factor,17.5*$scale_factor);
		
		$canvas->lineWidth(1.5*$scale_factor);
		$canvas->color($color2);
		$canvas->lineEnd(le::Square);
		$canvas->arc($x, $y, 17.5*$scale_factor,17.5*$scale_factor,$self->{angle1}, $self->{angle2});

		if ( $self->{showPercent} ) {
			my $ref_text = '800%';
			my $ext = 10 * $scale_factor;
			my $attempts = 3;
			$canvas-> font-> height( 5 * $scale_factor );
			while ( $attempts-- ) {
				my $tw = $canvas-> get_text_width( $ref_text );
				last if $tw <= $ext;
				$canvas-> font-> height( $canvas-> font-> height - $scale_factor );
			}
			$ref_text = $self-> value . '%';
			my $tw = $canvas-> get_text_width( $ref_text );
			$canvas->text_out( $ref_text, $x - $tw/2, $y - $canvas->font->height / 2 );
		}
	}
	else {
                my @petal1 = (
			0, 2,
			-2, 8,
			0, 10,
			2, 8,
			0, 2
                );
                my @petal2 = (
			1, 1,
			3, 7,
			6, 7,
			6, 4,
			1, 1
                );
	
		$canvas->translate($x, $y);
		my $fill_spline = sub { $canvas->fill_spline(_scale(shift, $scale_factor, $scale_factor)) };

		# top
		if ($self->{start_angle} == 6  || $self->{start_angle} == 7 || $self->{start_angle} == 0) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }  
		$fill_spline->( \@petal1 );
	
		# top right
		if ($self->{start_angle} == 7 || $self->{start_angle} == 0 || $self->{start_angle} == 1) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->(\@petal2);
		
		# right
		if ($self->{start_angle} == 0 || $self->{start_angle} == 1 || $self->{start_angle} == 2) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
                $fill_spline->(_rotate \@petal1);
		
		# right bottom
		if ($self->{start_angle} == 1 || $self->{start_angle} == 2 || $self->{start_angle} == 3) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->( _v_flip \@petal2 );
	
		# bottom
		if ($self->{start_angle} == 2 || $self->{start_angle} == 3 || $self->{start_angle} == 4) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->( _v_flip \@petal1);
	
		# bottom left
		if ($self->{start_angle} == 3 || $self->{start_angle} == 4 || $self->{start_angle} == 5) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->( _h_flip _v_flip \@petal2);
		
		# left
		if ($self->{start_angle} == 4 || $self->{start_angle} == 5 || $self->{start_angle} == 6) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->( _h_flip _rotate \@petal1);
		
		# top left
		if ($self->{start_angle} == 5 || $self->{start_angle} == 6 || $self->{start_angle} == 7) {
			$canvas->color($color2);
		}
		else { $canvas->color($color1); }
		$fill_spline->( _h_flip \@petal2);
	}
}

# properties
sub style
{
	my ($self, $style) = @_;
	return $self-> {style} unless $#_;

	# When the style property is changed, reset the timer frequency
	# and the start_angle and for style circle the end_angle, too
	if ( $style eq 'drops') {
		$self->{timer}->timeout(200);
	} elsif ( $style eq 'circle') {
		$self->{timer}->timeout(8);
	} else {
		Carp::croak("bad style: $style");
	}
	$self->{style}       = $style;
	$self->{start_angle} = 0;
	$self-> update_value;
	$self-> repaint;
}

sub update_value     
{
	my $self = shift;
	$self->{angle1} = (360 + $self->{start_angle} - $self->{value} * 360 / 100) % 360;
	$self->{angle2} = $self->{start_angle};
	$self->{angle2} += 360 if $self->{value} >= 100.0;
}

sub value
{
	my ( $self, $value ) = @_;
	return $self->{value} unless $#_;

	$value = 0   if $value < 0;
	$value = 100 if $value > 100;
	$self->{value} = $value;
	$self-> update_value;
	$self-> repaint;
}

sub showPercent
{
	my ( $self, $sp ) = @_;
	return $self->{showPercent} unless $#_;
	$self->{showPercent} = $sp;
	$self-> repaint;
}

sub active
{
	return $_[0]->{timer}->get_active unless $#_;
	my ($self, $active) = @_;
	$active ? $self->{timer}->start : $self->{timer}->stop;
}

sub start  { shift->active(1) }
sub stop   { shift->active(0) }
sub toggle { $_[0]->active(!$_[0]->active) }

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Prima::Spinner - Show a spinner animation

=head1 SYNOPSIS

  use Prima qw(Application Buttons Spinner);

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
	text => 'Start/Stop',
	checkable => 1,
	checked => 1,
	origin => [0,0],
	onClick => sub { $spinner->toggle },
	growMode => gm::XCenter
  );

  run Prima;

=head1 DESCRIPTION

Prima::Spinner provides a simple spinning animation with two designs and the
opportunity to specify the colors of the spinning animation. This is useful to
show an indeterminate progress of a running process.

=head1 USAGE

You can determine the following properties:

=head2 Properties

=over

=item active [BOOLEAN]

If now parameter is passed, by this method you can get the active state of the
spinning widget. '1' means that the spinner is running, '0' that it is stopped.
With C<active(1)> you can start the spinner animation, with C<active(0)> you
can stop it.

=item color COLOR

Inherited from L<Prima::Widget>. C<color> manages the basic foreground color.
Concrete for the spinner widget this means the background color of the circle
or the color of the inactive drops.

=item hiliteColor COLOR

Inherited from L<Prima::Widget>. The color used to draw alternate foreground
areas with high contrast. For the spinner widget this means the color of the
arc in the circle style or the color of the active drops in the drops style.

=item showPercent BOOLEAN

If set, displays completion percent as text. Only for I<circle> style.

=item start

Same as C< active(1) >

=item stop

Same as C< active(0) >

=item style STRING

C<style> can be 'circle' or 'drops'. With C<'circle'> an arc moving around a
circle is shown. C<'drops'> shows drops are shown that switches consecutively
the color.

=item value INT

Integer between 0 and 100, showing completion perentage. Only for I<circle> style.

=item toggle

Same as C< active(!active) >

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
