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
		my $c = int(SZ / 2 + .5) - 1;
		my $pix = $w->IV->{image}->pixel( $c, $c );
		$w->text(sprintf("%06x", $pix));
		$w->IV->repaint;
	});
}

$w = Prima::MainWindow->new(
	color    => cl::Red,
	text     => 'Antialias',
	sizeMin  => [100,100],
	onPaint  => sub {
		my ( $self, $canvas) = @_;
		$canvas-> clear;
		$canvas-> fillPattern(fp::CloseDot);
		$canvas-> new_path(antialias => 1)-> ellipse(100,100,100)->fill;
		$canvas-> fillPattern(fp::Solid);
		$canvas->lineWidth(5);
		$canvas-> graphic_context(
			backColor   => cl::Yellow,
			rop2        => rop::CopyPut,
			linePattern => lp::Dash,
			sub { $canvas-> new_aa_surface(alpha => 128)-> polyline([ 0, 0, $canvas->size ]) },
		);

		if ( $canvas->can_draw_alpha ) {
			$canvas->color(cl::Green);
			$canvas->antialias(1);
			$canvas->alpha(192);
			$canvas->fill_ellipse(100, 150, 100, 100);
			$canvas->font->set(size => 32);
			$canvas->text_out("TEXT", 50, 50);
			$canvas->font->set(size => 32, direction => 45);
			$canvas->text_shape_out("GLYPHS", 50, 50);

			$canvas->lineWidth(5);
			$canvas->line( 0, 100, $canvas->width - 100, $canvas->height);
			$canvas->antialias(0);
			$canvas->alpha(255);
		}

		$canvas-> color(cl::Black);
		$canvas->lineWidth(1);
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
	onKeyDown => sub {
		my ( $self, $code, $key, $mod ) = @_;
		my @d = (0,0);
		if    ( $key == kb::Left  ) { $d[0] = -1 }
		elsif ( $key == kb::Right ) { $d[0] =  1 }
		elsif ( $key == kb::Up    ) { $d[1] =  1 }
		elsif ( $key == kb::Down  ) { $d[1] = -1 }
		return if $d[0] == 0 and $d[1] == 0;
		$pos[$_] += $d[$_] for 0,1;
		$pos[0] = 0 if $pos[0] < 0;
		$pos[1] = 0 if $pos[1] < 0;
		my @sz = map { $_ - SZ } $self-> size;
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
			my $c = int(SZ * MUL / 2 + .5) + 1;
			$canvas->rectangle( $c, $c, $c + MUL + 1, $c + MUL + 1);
		} else {
			$canvas-> clear( 1, 1, (SZ * MUL) x 2);
			$canvas-> line( 0, 0, $self-> size );
			$canvas-> line( 0, $self-> size, 0 );
		}
	},
);

redraw;

run Prima;
