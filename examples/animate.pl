=head1 NAME

examples/animate.pl - Animate using L<Prima::Image::AnimateGIF>

=cut

use strict;
use warnings;
use Prima qw(Application Image::AnimateGIF);

my $clip_debug = 0;

unless ( @ARGV) {
	my $f = $0;
	$f =~ s/([\\\/]|^)([^\\\/]*)$/$1/;
	$f .= "../Prima/sys/win32/sysimage.gif";
	print "(no argument given, trying to find at least $f...";
	if ( -f $f) {
		print "ok)\n";
		print "Next time specify some GIF animation as an argument\n";
		@ARGV = ($f);
	} else {
		print "nopes)\n";
		die "Please specify some GIF animation as an argument\n";
	}
}

my $x = Prima::Image::AnimateGIF->load($ARGV[0]);
die "Can't load $ARGV[0]:$@\n" unless $x;

my ( $X, $Y) = ( 100, 100);
my $g = $::application-> get_image( $X, $Y, $x-> size);

$::application-> begin_paint;

my $break;

$SIG{INT} = sub { $break++ };
while ( my $info = $x-> next) {
	my $c = $g-> dup;
	$c-> begin_paint;
	$x-> draw( $c, 0, 0);

	$::application-> clipRect(
		$X + $info-> {left},
		$Y + $info-> {bottom},
		$X + $info-> {right},
		$Y + $info-> {top},
	) if $clip_debug;

	$::application-> color(cl::Red);
	$::application-> rop(rop::XorPut);
	my @r = (
		$X + $info-> {left},
		$Y + $info-> {bottom},
		$X + $info-> {right},
		$Y + $info-> {top},
	);
	for ( 1..10) {
		next unless $clip_debug;
		$::application-> bar(@r);
		select(undef,undef,undef,0.03);
	}
	$::application-> rop(rop::CopyPut);
	$::application-> put_image( $X, $Y, $c);
	
	$::application-> sync;
	select(undef, undef, undef, $info-> {delay});

	last if $break;
}

$::application-> clipRect(0,0,$::application-> size);
$::application-> put_image( $X, $Y, $g, rop::CopyPut);
