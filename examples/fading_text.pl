=pod

=head1 NAME

examples/fader.pl - fading text demo

=head1 FEATURES

Demonstrates use of Prima image alpha blending

=cut
use strict;
use warnings;
use Prima qw(Application ImageViewer);

my $w;
my @text;

my $f = @ARGV ? $ARGV[0] : $0;
die "Cannot open $f: $!\n" unless open F, "<", $f;

my $maxlen = 80;
for (<F>) {
	chomp;
	s/\t/    /g;
	if ( $maxlen >= length ) {
		push @text, $_;
	} else {
		push @text, substr( $_, 0, $maxlen, '') while $maxlen < length;
		push @text, $_ if length;
	}
}

close F;

my ( $canvas, $fader, $texter);

my $yofs = 0;
my $total_fadeout = 0.7;
my $start_from_top_at_yofs;
my ($fh, $fe, $next_yofs, $max_width, $text_height);

sub render_text
{
	exit if 
		defined($text_height) and 
		$yofs > $texter->height * $total_fadeout / 2 + $text_height;
	return if $yofs < ($next_yofs // 0);

	unless ( defined $max_width ) {
		$texter-> begin_paint_info;
		$max_width = 0;
		$text_height = 0;
		$fh = $texter->font->height;
		$fe = $texter->font->externalLeading;
		$text_height = @text * ( $fh + $fe );
		for ( @text ) {
			my $w = $texter->get_text_width($_, 1);
			$max_width = $w if $max_width < $w;
		}
		$texter-> end_paint_info;
	}
	if ( $max_width < $texter->width ) {
		$texter->width($max_width);
	}
	$texter->begin_paint;

	$texter-> clear;
	my $th = $texter->height;

	my $at_y = $yofs + $th / 2;
	for my $text ( @text ) {
		$at_y -= $fh + $fe;
		next if $at_y > $th * ( 1 + $total_fadeout) / 2 - ( $fh + $fe );
		last if $at_y < - ( $fh + $fe );
		$texter->text_out( $text, 2, $at_y);
	}
	$texter-> end_paint;
	$next_yofs = $yofs + $th / 2;
}

sub repaint
{
	$canvas->clear;
	$fader->clear;
	render_text;

	my ( $dx, $dy ) = (0, 0);
	my ( $cw, $ch ) = $fader->size;
	my ( $tw, $th ) = $texter->size;
	my $xangle = 0.5 * $cw / $ch;
	for ( my $i = 0; $i < $th / 2 * $total_fadeout; $i++){
		$fader->put_image_indirect( $texter,
			$dx, $dy,
			0, $i + $next_yofs - $yofs,
			$cw - $dx * 2, 1,
			$cw, 1,
			rop::CopyPut);
		$dx += $xangle;
		$dy ++;
	}
	$canvas->put_image(0,0,$fader,rop::Premultiply);

	$yofs += $ch / 100; 
	$w->Image->repaint;
}

sub resize
{
	my @size = $w->Image->size;
	$canvas = Prima::Image->new(
		size => \@size,
		type => im::RGB,
		backColor => cl::Black,
	);
	$fader = Prima::Icon->new(
		size => \@size,
		type => im::RGB,
		autoMasking => am::None,
		maskType => 8,
		backColor => cl::Black,
	);
	
	my $fader_alpha = Prima::Image->new(
		size => \@size,
		type => im::Byte,
	);
	$fader_alpha->new_gradient(
		palette => [cl::White, cl::Black],
		poly    => [0,0,$total_fadeout/2,0,$total_fadeout,1]
	)->bar( 0,0,$fader_alpha-> size);
	$fader->mask( $fader_alpha->data );
	
	$texter = Prima::Image->new(
		size => [ $size[0], $size[1] * 2 ],
		type => im::Byte,
		type => im::RGB,
		backColor => cl::Black,
		color => cl::Yellow,
		scaling => ist::None,
	);

	undef $next_yofs;
	repaint;
}

$w = Prima::MainWindow->new(
	size => [ 600, 500 ],
	text => 'Fading demo',
);

$w-> insert( Widget =>
	name => 'Image',
	pack => {side => 'top', fill => 'both', pad => 10, expand => 1},
	onPaint => sub { shift->put_image(0,0,$canvas) },
	onSize  => \&resize,
	sizeMin => [100,100],
);

resize;
repaint;

$w->insert( Timer => 
	timeout => 50,
	onTick  => \&repaint,
)->start;
run Prima;
