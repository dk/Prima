=pod

=head1 NAME

examples/blend.pl - Prima alpha blending

=head1 FEATURES

Demonstrates use of Prima image alpha blending with C<rop::blend($ALPHA)>.

=cut


use strict;
use warnings;
use Prima qw(Application Sliders ImageViewer);

my ($rop_name, $rop_val);
my $w = Prima::MainWindow->new(
	size => [ 400, 300 ],
	designScale => [7, 16],
	text => 'Blending example',
	menuItems => [
		[ '~Binary' => [ map { [$_, $_, \&bin] } qw(
			CopyPut XorPut AndPut OrPut NotPut Invert Blackness
			NotDestAnd NotDestOr Whiteness NotSrcAnd NotSrcOr NotXor
			NotAnd NotOr NoOper
		)]],
		[ 'Porter-~Duff' => [ map { [$_, $_, \&pd] } qw(
			SrcOver Xor DstOver SrcCopy DstCopy Clear
			SrcIn DstIn SrcOut DstOut SrcAtop DstAtop
		)]],
		[ '~Photoshop' => [ map { [$_, $_, \&pd] } qw(
			Add Multiply Screen Overlay Darken
			Lighten ColorDodge ColorBurn HardLight
			SoftLight Difference Exclusion
		)]],
	]
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

sub repaint
{
	my $s = $w->Slider;
	my $rop = $s->visible ? rop::alpha( $rop_val, $s->value, $s->value) : $rop_val;
	$canvas->put_image(0,0,$a);
	$canvas->put_image(0,0,$b,$rop);
	$w-> ImageViewer1->repaint;
}

sub select_rop
{
	my ($newrop, $with_slider) = @_;
	if ( defined $rop_name ) {
		$w->menu->uncheck($rop_name);
	}
	$rop_name = $newrop;
	$rop_val = &{${rop::}{$rop_name}}();
	$w->Slider->enabled( $with_slider );
	$w->Slider->readOnly( !$with_slider );
	$w->menu->check($rop_name);
	repaint();
}

sub bin { select_rop($_[1], 0) }
sub pd  { select_rop($_[1], 1) } 


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
	name => 'Slider',
	pack => {side => 'top', fill => 'x', pad => 10},
	onChange => \&repaint,
);

select_rop('DstAtop', 1);

run Prima;
