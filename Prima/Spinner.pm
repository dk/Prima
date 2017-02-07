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
		style	    => 'circle',
		active	    => 1,
		buffered    => 1,
		value	    => 50,
		showPercent => 1,
	}
}

sub init
{
	my $self = shift;
	
	# Create the timer for the motion of the spinner
	$self->{timer} = $self->insert( Timer => 
		name	    => 'Timer',
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
	elsif ( $self->{style} eq 'circle' || $self->{style} eq 'yinyang') {
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

sub _v_flip($) { _scale(shift, 1, -1)	}
sub _h_flip($) { _scale(shift, -1, 1)	}
sub _rotate($) { _pairwise { pop, pop } shift }

# Events
sub on_paint
{
	my ($self,$canvas) = @_;

	$canvas->clear;
		
	my $x		 = $self->width/2; 
	my $y		 = $self->height/2;
	my $min		 = ( $x < $y ) ? $x : $y;
	my $scale_factor = $min / 12;

	if ($self->{style} eq "circle") {
		my $color1  = $self->color;
		my $color2  = $self->hiliteColor;
		$canvas->color($color2);
		$canvas->lineWidth(3*$scale_factor);
		$canvas->ellipse($x, $y, 17.5*$scale_factor,17.5*$scale_factor);
		
		$canvas->lineWidth(1.5*$scale_factor);
		$canvas->color($color1);
		$canvas->lineEnd(le::Square);
		$canvas->arc($x, $y, 17.5*$scale_factor,17.5*$scale_factor,$self->{angle1}, $self->{angle2});

		if ( $self->{showPercent} ) {
			my $ref_text = '100%';
			my $ext = 10 * $scale_factor;
			my $attempts = 2;
			$canvas-> font-> height( 4 * $scale_factor );
			while ( $attempts-- ) {
				my $tw = $canvas-> get_text_width( $ref_text );
				last if $tw <= $ext;
				$canvas-> font-> height( $canvas-> font-> height - $scale_factor );
			}
			if ( $attempts >= 0 ) {
				$ref_text = int($self-> value + .5) . '%';
				my $tw = $canvas-> get_text_width( $ref_text );
				$canvas->text_out( $ref_text, $x - $tw/2, $y - $canvas->font->height / 2 );
			}
		}
	} elsif ( $self->{style} eq 'drops') {
		my @petal1 = (
			0, 1.5,
			-2, 7.5,
			0, 9.5,
			2, 7.5,
			0, 1.5
		);
		my @petal2 = (
			1.5, 1,
			3.5, 7,
			6.5, 7,
			6.5, 4,
			1.5, 1
		);

		$canvas->translate($x, $y);

		my $gradient = $canvas->gradient_realize3d( 8+1, { palette => [ $self->color, $self->backColor ] });
		my @colors;
		for ( my $i = 0; $i < @$gradient; $i+=2 ) {
			push @colors, $gradient->[$i] for 1 .. $gradient->[$i+1];
		}

		my $fill_spline = sub { 
			my ( $n, $p ) = @_;
			$canvas->color( $colors[( $self->{start_angle} + $n) % 8] );
			$canvas->fill_spline(_scale($p, $scale_factor, $scale_factor));
		};

		$fill_spline->(7, \@petal2);
		$fill_spline->(6, _rotate \@petal1);
		$fill_spline->(5, _v_flip \@petal2 );
		$fill_spline->(4, _v_flip \@petal1);
		$fill_spline->(3, _h_flip _v_flip \@petal2);
		$fill_spline->(2, _h_flip _rotate \@petal1);
		$fill_spline->(1, _h_flip \@petal2);
		$fill_spline->(0, \@petal1 );
	} elsif ( $self->{style} eq 'yinyang') {
		my $sin = sin( -$self->{start_angle} / (2 * 3.14159265358));
		my $cos = cos( -$self->{start_angle} / (2 * 3.14159265358));
		my $render = sub {
			_pairwise { 
				$_[0] * $cos - $_[1] * $sin, 
				$_[0] * $sin + $_[1] * $cos 
			}
			[ map { $_ * $scale_factor * 11 } @_ ]
		};

		my $f	 = 0.415;
		my $d1	 = 0.8;
		my $dx	 = 0.05;
		my $d2	 = $dx * 4;
		my $apx1 = sub {  $_[0], 0.00, $_[0], $_[1] * $f };
		my $apx2 = sub {  $_[0], $_[1] * $f,  $_[0], 0.0 };
		my $quad = sub { -$_[0] * $f, $_[0], $_[0] * $f, $_[0] };

        	$canvas->fillWinding(1);
		$canvas->translate($x, $y);
		$canvas-> new_path-> spline( $render->(
			$apx1->($d1, $d1),
			@{ _h_flip( [ $quad->( $d1 + 1 * $dx) ] ) },
			@{ _rotate( [ $quad->(-$d1 - 2 * $dx) ] ) },
			@{ _v_flip( [ $quad->( $d1 + 3 * $dx) ] ) },
			$apx2->($d1 + 4 * $dx, -1 * $d1 - 4 * $dx)
		))->spline( $render->(
			$apx1->($d1 + 4 * $dx, $d2),
			$d1 + $d2 * $f, $d2,
			$d1 - $d2 * $f, $d2,
			$apx2->($d1 - 4 * $dx, $d2),
		))->spline( $render-> (
			$apx1->($d1 - 4 * $dx, -$d1 + 4 * $dx),
			$quad->(-$d1 + 3 * $dx),
			@{ _rotate _v_flip ( [ $quad->($d1 - 2 * $dx) ] ) },
			$quad->($d1 - 1 * $dx),
			$apx2->($d1, $d1)
		))->fill;
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
	} elsif ( $style eq 'circle' || $style eq 'yinyang') {
		$self->{timer}->timeout(8);
	} else {
		Carp::croak("bad style: $style");
	}
	$self->{style}	     = $style;
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
