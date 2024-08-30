=pod

=head1 NAME

examples/grip.pl - screen and widget grabbing example

=head1 FEATURES

Provided two standalone tests.

The "grip test" copies the graphic content of the widget in a monochrome
bitmap, testing the widget representation on a monochrome display.
This test is useful together with the L<launch> example, that allows execution
of several Prima examples in one task space.

The "grab test" copies the selected region of a screen.  Tests the correct
implementation of the apc_application_get_bitmap() function, especially on the
paletted displays.

=cut

use strict;
use warnings;
use Prima;
use Prima::ImageViewer;
use Prima::Application;
use Prima::Widget::RubberBand;
use Prima::Drawable::Subcanvas;

package Generic;

sub xordraw
{
	my ($self, $new_rect) = @_;
	$::application-> rubberband( $new_rect ?
		( rect => [$self-> {capx},$self-> {capy}, $self-> {dx},$self-> {dy}]) :
		( destroy => 1 )
	);
}

my $w = Prima::MainWindow-> new(
	size => [ 200, 200],
	menuItems => [
		[ "~Grip" => sub {
			my $self = $_[0];
			$self-> capture( 1);
			$self-> pointer( cr::Move);
			$self-> { cap} = 1;
		}],
		[ "G~rab" => sub {
			my $self = $_[0];
			$self-> capture( 1);
			$self-> { cap} = 2;
		}],
		[ "~Full screen" => sub { shift-> IV-> image( $::application->get_fullscreen_image) }],
	],
	onMouseDown => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		return unless defined $self-> {cap};
		return unless $self-> {cap} == 2;
		return unless $btn == mb::Left;
		$self-> {cap} = 3;
		($self-> {capx},$self-> {capy}) = $self-> client_to_screen( $x, $y);
		($self-> {dx},$self-> {dy}) = ($self-> {capx},$self-> {capy});
		xordraw( $self, 1);
	},
	onMouseMove => sub {
		my ( $self, $mod, $x, $y) = @_;
		return unless defined $self-> {cap};
		return unless $self-> {cap} == 3;
		my @od = ($self-> {dx},$self-> {dy});
		($self-> {dx},$self-> {dy}) = $self-> client_to_screen( $x, $y);
		xordraw( $self, 1);
	},
	onMouseUp => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		return unless defined $self-> { cap};
		my $cap = $self-> { cap};
		$self-> { cap} = undef;
		$self-> capture(0);
		if ( $cap == 1) {
			$self-> pointer( cr::Default);
			my $v = $::application-> get_widget_from_point( $self-> client_to_screen( $x, $y));
			return unless $v;
			$self-> IV-> image( $v->screenshot );
		} elsif ( $cap == 3) {
			($self-> {dx},$self-> {dy}) = $self-> client_to_screen( $x, $y);
			xordraw( $self);
			my ( $xl, $yl) = (abs($self-> {dx} - $self-> {capx}), abs($self-> {dy} - $self-> {capy}));
			$x = $self-> {capx} > $self-> {dx} ? $self-> {dx} : $self-> {capx};
			$y = $self-> {capy} > $self-> {dy} ? $self-> {dy} : $self-> {capy};
			my $i = $::application-> get_image( $x, $y, $xl, $yl);
			$self-> IV-> image( $i);
		}
	},
);

$w-> insert( ImageViewer =>
	pack    => { expand => 1, fill => 'both' },
	name    => 'IV',
	valignment  => ta::Center,
	alignment   => ta::Center,
	quality => 1,
);

print "don't run this by itself, run launch.pl then run grip.pl and some other example. Then 'grip' or 'grap' that other example\n";

run Prima;
