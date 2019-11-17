=pod

=head1 NAME

examples/blend.pl - Prima alpha blending

=head1 FEATURES

Demonstrates use of Prima image alpha blending with C<rop::blend($ALPHA)>.

=cut


use strict;
use warnings;
use Prima qw(Application Label Sliders ImageViewer);

my ($rop_name, $rop_val);
my $w = Prima::MainWindow->new(
	size => [ 600, 500 ],
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
$a->new_gradient( palette => [cl::LightGreen, cl::Blue ])->bar(0, 0, $a->size);
$a->end_paint;
my $ia = Prima::Icon->new(
	size => [200,200],
	type => im::RGB,
	maskType => im::bpp8,
	autoMasking => am::MaskColor,
	maskColor => cl::Black,
);
$ia->put_image(0,0,$a);
$ia->color(cl::Black);
$ia->bar(0,0,30,200);
$ia->bar(170,0,200,200);
my ( $xa, $ma ) = $ia->split;

my $b = $a->dup;
$b->begin_paint;
$b->bar(0,0,$b->size);
$b->new_gradient( palette => [cl::LightRed, cl::Yellow ])-> ellipse($b->width/2,$b->height/2, $b->size);
$b->end_paint;

my $ib = Prima::Icon->new(
	size => [200,200],
	type => im::RGB,
	maskType => im::bpp8,
	autoMasking => am::MaskColor,
	maskColor => cl::Black,
);
$ib->put_image( 0,0,$b);
my ( $xb, $mb ) = $ib->split;
undef $b;

my $base = $a->dup;
$base->set( backColor => cl::White, color => cl::Black, fillPattern => [(0xF0)x4, (0xF)x4], rop2 => rop::CopyPut);
$base->bar(0,0,$base->size);

my $canvas = $a->dup;
my $precanvas = Prima::Icon->new(
	size => [200,200],
	type => im::RGB,
	maskType => im::bpp8,
	autoMasking => am::None,
);
undef $a;
$ia->autoMasking(am::None);

my $mask = Prima::Image->new(
	size => [200,200],
	type => im::Byte,
);

sub repaint
{
	my $sa = $w->SliderA;
	my $sb = $w->SliderB;
	if ( $sa->enabled ) {
		$mask->put_image(0,0,$ma,rop::alpha(rop::SrcIn, undef, $sa->value));
		$ia->put_image(0,0,$xa);
		$ia->put_image(0,0,$mask,rop::AlphaCopy);
		$ia->premultiply_alpha;
		$precanvas->put_image(0,0,$ia,rop::SrcCopy);

		$mask->put_image(0,0,$mb,rop::alpha(rop::SrcIn, undef, $sb->value));
		$ia->put_image(0,0,$xb);
		$ia->put_image(0,0,$mask,rop::AlphaCopy);
		$ia->premultiply_alpha;
		$precanvas->put_image(0,0,$ia,$rop_val);

		$canvas->put_image(0,0,$base);
		$canvas->put_image(0,0,$precanvas);
	} else {
		$canvas->put_image(0,0,$xa);
		$canvas->put_image(0,0,$xb,$rop_val);
	}
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
	$w->SliderA->enabled( $with_slider );
	$w->SliderA->readOnly( !$with_slider );
	$w->SliderB->enabled( $with_slider );
	$w->SliderB->readOnly( !$with_slider );
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

$w->insert( Label => 
	text => '~Green transparency',
	focusLink => 'SliderA',
	pack => {side => 'top', fill => 'x', padx => 15},
);

$w->insert( Slider =>
	min => 0,
	max => 255,
	value => 200,
	scheme => ss::Axis,
	increment => 16,
	name => 'SliderA',
	pack => {side => 'top', fill => 'x'},
	onChange => \&repaint,
);

$w->insert( Label => 
	text => '~Red transparency',
	focusLink => 'SliderB',
	pack => {side => 'top', fill => 'x', padx => 15},
);

$w->insert( Slider =>
	min => 0,
	max => 255,
	scheme => ss::Axis,
	increment => 16,
	value => 200,
	name => 'SliderB',
	pack => {side => 'top', fill => 'x'},
	onChange => \&repaint,
);

select_rop('SrcOver', 1);

run Prima;
