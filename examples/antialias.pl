=pod

=head1 NAME

examples/antialias.pl - antialiased shapes

=cut

use strict;
use warnings;
use Prima qw(Application Utils);
use Prima::Application name => 'Generic';

my $w;
my @pos = (0,0);
use constant SZ  => 16;
use constant MUL => 10;


sub redraw
{
	Prima::Utils::alarm( 500, sub {
		$w->IV->{image} = $::application->get_image( $w->client_to_screen(map { $_ + 1 } @pos), (SZ - 2) x 2);
		$w->IV->repaint;
	});
}

$w = Prima::MainWindow->new(
	color    => cl::LightRed,
	text     => 'Antialias',
	sizeMin  => [100,100],
	onPaint  => sub {
		my ( $self, $canvas) = @_;
		my $color = $self-> color;
		$canvas-> color( $self-> backColor);
		$canvas-> bar( 0, 0, $canvas-> size);
		$canvas-> color( $color);
		$canvas-> new_aa_surface-> polyline([ 0, 0, $canvas->size ]);
		$canvas-> new_path(antialias => 1)-> ellipse(100,100,100)->fill;

		$canvas-> color(cl::Black);
		$canvas-> rectangle( @pos, map { $_ + SZ } @pos);
	},
	onSize => sub {
		my ( $self, $ox, $oy, $x, $y ) = @_;
		$pos[0] = $x - SZ if $pos[0] > $x - SZ;
		$pos[1] = $y - SZ if $pos[1] > $y - SZ;
		$pos[0] = 0 if $pos[0] < 0;
		$pos[1] = 0 if $pos[1] < 0;
	},
	onMouseDown => sub {
		my ( $self, $btn, $mod, $x, $y) = @_;
		my @sz = map { $_ - SZ } $self-> size;
		@pos = map { $_ - SZ / 2 } $x, $y;
		$pos[0] = 0 if $pos[0] < 0;
		$pos[1] = 0 if $pos[1] < 0;
		$pos[0] = $sz[0] if $pos[0] > $sz[0];
		$pos[1] = $sz[1] if $pos[1] > $sz[1];
		$self->repaint;
		$self->update_view;
		redraw;
	},
);


$w->insert( Widget => 
	name      => 'IV',
	size      => [ ( 2 + SZ * MUL ) x 2 ],
	origin    => [ $w-> width - SZ * MUL - 2, $w->height - SZ * MUL - 2],
	growMode  => (gm::GrowLoX|gm::GrowLoY),
	onPaint   => sub {
		my ( $self, $canvas ) = @_;
		$canvas->rectangle(0, 0, map { $_ - 1 } $self->size);
		if ( $self->{image} ) {
			$canvas->stretch_image( 1, 1, (SZ * MUL) x 2, $self->{image});
		} else {
			$canvas-> clear( 1, 1, (SZ * MUL) x 2);
			$canvas-> line( 0, 0, $self-> size );
			$canvas-> line( 0, $self-> size, 0 );
		}
	},
);

redraw;

run Prima;
