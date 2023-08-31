=head1 NAME

examples/star_and_ring.pl - transparency and matrix transforms

=head2 SEE ALSO

Idea from cairo-demo/png/star_and_ring.c by Red Hat

=cut

use strict;
use warnings;
use Prima qw(Application);

my $alpha = 0;
my $w = Prima::MainWindow->new(
	layered      => 1,
	buffered     => 1,
	text         => 'Star & ring',
	size         => [1600,1200],
	backColor    => 0,
	onMouseDown  => sub { shift->{grab} = 1 },
	onMouseUp    => sub { shift->{grab} = 0 },
	onPaint => sub {
		my ( $self, $canvas ) = @_;
		$self->clear;
		my @size = $self->size;
		my $lw   = (($size[0] < $size[1]) ? $size[0] : $size[1]) / 15;
		$lw = 2 if $lw < 2;
		if ( $self-> {grab} ) {
			my ( $x, $y ) = $self-> pointerPos;
			$x -= $size[0]/2;
			$y -= $size[1]/2;
			$alpha = atan2($x, $y) * -57;
		}
		$canvas-> set(
			antialias => 1,
			lineWidth => $lw,
			color     => cl::LightRed,
			alpha     => 192
		);
		$canvas-> matrix->
			rotate($alpha)->
			scale(map { $_ / 2 - $lw * 2 } @size)->
			translate(map { $_ / 2 } @size);
		$canvas-> ellipse( 0, 0, 2, 2);
		$canvas-> color( cl::LightBlue );
		$canvas-> fillpoly([
			0.15,0.15,
			0,1,
			-0.15,0.15,
			-1,0,
			-0.15,-0.15,
			0,-1,
			0.15,-0.15,
			1,0
		]);
	}
);

$w-> insert( Timer =>
	timeout => 5,
	onTick  => sub {
		$alpha += 1;
		$alpha = 0 if $alpha > 360;
		$w-> repaint;
	}
)-> start;

run Prima;
