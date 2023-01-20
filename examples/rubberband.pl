use strict;
use warnings;
use Prima qw(Application Widget::RubberBand);

sub xordraw
{
	my ($self, @new_rect) = @_;
	$::application-> rubberband(
		@new_rect ?
		( rect => \@new_rect ) :
		( destroy => 1 ),
	);
}

Prima::MainWindow-> create(
	text => 'Click and drag!',
	onMouseDown => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		$self-> {anchor} = [$self-> client_to_screen( $x, $y)];
		xordraw( $self, @{$self-> {anchor}}, $self-> client_to_screen( $x, $y));
		$self-> capture(1);
	},
	onMouseMove => sub {
		my ( $self, $mod, $x, $y) = @_;
		xordraw( $self, @{$self-> {anchor}}, $self-> client_to_screen( $x, $y)) if $self-> {anchor};
	},
	onMouseUp => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		xordraw if delete $self-> {anchor};
		$self-> capture(0);
	},
);

run Prima;
