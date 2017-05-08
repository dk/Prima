=pod

=head1 NAME

examples/blend.pl - Prima alpha blending

=head1 FEATURES

Demonstrates use of Prima image alpha blending with C<rop::blend($ALPHA)>.

=cut


use strict;
use warnings;
use Prima qw(Application Sliders ImageViewer);

my $w = Prima::MainWindow->new(
	size => [ 300, 200 ],
	designScale => [7, 16],
	text => 'Blending example',
);

my $a = Prima::Image->new(
	size => [200,200],
	type => im::RGB,
);
$a->begin_paint;
$a->new_gradient( palette => [cl::LightRed, cl::Yellow ])-> ellipse($a->width/2,$a->height/2, $a->width, $a->height);
$a->end_paint;

my $b = $a->dup;
$b->begin_paint;
$b->new_gradient( palette => [cl::LightGreen, cl::Blue ])->bar(0, 0, $b->width,$b->height);
$b->end_paint;

my $canvas = $a->dup;

$w-> insert( ImageViewer => 
	pack => {side => 'top', fill => 'both', pad => 10, expand => 1},
	image => $canvas,
	stretch => 1,
);

$w->insert( Slider => 
	min => 0,
	max => 255,
	scheme => ss::Axis,
	increment => 16,
	pack => {side => 'top', fill => 'x', pad => 10},
	onChange => sub {
		$canvas->put_image(0,0,$a);
		$canvas->put_image(0,0,$b,rop::blend( shift-> value ));
		$w-> ImageViewer1->repaint;
	},
);

run Prima;
