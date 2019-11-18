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
		$precanvas->put_image(0,0,$ia, rop::alpha( rop::SrcCopy | rop::Premultiply, $sa->value));
		$precanvas->put_image(0,0,$ib, rop::alpha( $rop_val     | rop::Premultiply, $sb->value));
		$canvas->put_image(0,0,$base);
		$canvas->put_image(0,0,$precanvas);
	} else {
		$canvas->put_image(0,0,$a);
		$canvas->put_image(0,0,$b,$rop_val);
	}

	my $fader = $w->Fader;
	$canvas->put_image(2, 2, $fader->{banner}, rop::alpha( rop::SrcOver | rop::Premultiply, $fader->{alpha} ))
		if $fader->{banner};
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
	$w->Fader->{text} = $rop_name;
	delete $w->Fader->{left};
	$w->Fader->{steps} = 30;
	$w->Fader->start;
	repaint();
}

sub set_slider
{
	$w->Fader->{text} = shift->value;
	$w->Fader->{steps} = 15;
	delete $w->Fader->{left};
	$w->Fader->start;
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
	onChange => \&set_slider,
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
	onChange => \&set_slider,
);

$w-> insert( Timer => 
	timeout => 50,
	name => 'Fader',
	onTick => sub {
		my $self = shift;
		if ( !defined $self->{left}) {
			$self->{left} = $self->{steps};
			$canvas-> begin_paint_info;
			$canvas->font->set( size => 18, style => fs::Bold );
			my $f = $canvas->font;
			my $b = Prima::Icon->new(
				type        => im::RGB,
				size        => [ $canvas->get_text_width( $self->{text}, 1 ) + 4, $f-> height + 4 ],
				color       => cl::Yellow,
				backColor   => cl::Black,
				lineWidth   => 5,
				maskType    => 8,
				autoMasking => 0,
			);
			$canvas-> end_paint_info;
			$b->begin_paint;
			$b->clear;
			$b->font->set( size => 18, style => fs::Bold );
			$b->color(0x010101); # black but not zero
			my $path = $b->new_path;
			$path->translate(2,2);
			$path->text( $self->{text});
			$path->stroke;
			$b->color(cl::Yellow);
			$b->text_out($self->{text}, 2, 2);
			$b->end_paint;

			my $bb = $b->clone( type => im::Byte, rop2 => rop::CopyPut );
			$bb->map(0x000000); # black and zero
			$b->mask( $bb->data );

			$self->{banner} = $b;
			$self->{alpha} = 255;
		} elsif ( --$self->{left} <= 0 ) {
			delete $self->{banner};
			delete $self->{left};
			$self->stop;
		} else {
			$self->{alpha} -= 256 / $self->{steps};
		}
		repaint;
	},
);
$w->Fader->{steps} = 30;

select_rop('SrcOver', 1);

run Prima;
