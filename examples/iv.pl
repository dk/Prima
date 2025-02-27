=pod

=head1 NAME

examples/iv.pl - an image viewer program

=head1 FEATURES

Demonstrates usage of the image subsystem, in particular:

=over 4

=item *

Standard image open dialog. Note its behavior with the multi-frame images, namely
the possibility to select and load a single frame.

=item *

Standard image save dialog. Note the usage of the graphic filters and the ability
to save images in base64-encoded format for hardcoded storage.

=item *

Image conversion routines

=item *

Standard L<Prima::ImageViewer> class.

=back

Tests the correct implementation of the internal image paint routines,
in particular on the paletted displays and the representation of 1-bit
images/icons with non-BW palette.

Note the mouse wheel scrolling.

=cut

use strict;
use warnings;
use Prima qw(ImageViewer Dialog::ImageDialog MsgBox sys::GUIException);
use Prima::Application name => "IV";

my $ico = Prima::Icon-> new;
$ico = 0 unless $ico-> load( 'hand.gif');


my $winCount  = 1;

my %iv_prf = (
	origin => [ 0, 0],
	growMode => gm::Client,
	quality => 1,
	name    => 'IV',
	valignment  => ta::Middle,
	alignment   => ta::Center,
	onMouseDown => \&iv_mousedown,
	onMouseUp   => \&iv_mouseup,
	onMouseMove => \&iv_mousemove,
	onMouseWheel => \&iv_mousewheel,
);

sub status
{
	my $iv = $_[0]-> IV;
	my $img = $iv-> image;
	my $str;
	if ( $img) {
		$str = $iv-> {fileName};
		$str =~ s/([^\\\/]*)$/$1/;
		$str = sprintf("%s (%dx%dx%d bpp)", $1,
			$img-> width, $img-> height, $img-> type & im::BPP);
	} else {
		$str = '.Untitled';
	}
	$_[0]-> text( $str);
	$::application-> name( $str);
}


sub menuadd
{
	unless ( $_[0]-> IV-> {menuadded}) {
		$_[0]-> {omenuID} = 'P';
		$_[0]-> {conversion} = ict::Optimized;
		$_[0]-> menu-> insert(
		[
			[ 'Reopen' => 'Ctrl+R' => '^R'       => \&freopen],
			[ '~New window...' => 'Ctrl+N' => '^N'       => \&fnewopen],
			[],
			[ '~Save'  => 'F2'     => 'F2'       => \&fsave],
			[ 'Save As...'                       => \&fsaveas],
		],
		'file', 1
		);
		$_[0]-> menu-> insert(
			[
			['~Edit' => [
				['~Copy' => 'Ctrl+Ins' => km::Ctrl|kb::Insert , sub {
					$::application-> Clipboard-> image($_[0]-> IV-> image)
				}],
				['~Paste' => 'Shift+Ins' => km::Shift|kb::Insert , sub {
					my $i = $::application-> Clipboard-> image;
					$_[0]-> IV-> image( $i) if $i;
					status($_[0]);
				}],
			]],
			['~Image' => [
				[ '~Convert to'=> [
					['~Monochrome' => sub {icvt($_[0],im::Mono)}],
					['~16 colors' => sub {icvt($_[0],im::bpp4)}],
					['~256 colors' => sub {icvt($_[0],im::bpp8)}],
					['~Grayscale' => sub {icvt($_[0],im::bpp8|im::GrayScale)}],
					['~RGB' => sub {icvt($_[0],im::RGB)}],
					['~Long' => sub {icvt($_[0],im::Long)}],
					[],
					['(N' => '~No halftoning' => sub {setconv(@_)}],
					['O' => '~Ordered' => sub {setconv(@_)}],
					['E' => '~Error diffusion' => sub {setconv(@_)}],
					[')*P' => 'O~ptimized' => sub {setconv(@_)}],
				]],
				['~Zoom' => [
					['~Normal ( 100%)' => 'Ctrl+Z' => '^Z' => sub{$_[0]-> IV-> zoom(1.0)}],
					['~Best fit' => 'Ctrl+Shift+Z' => km::Shift|km::Ctrl|ord('z') => sub { $_[0]->IV->apply_auto_zoom } ],
					[],
					['@abfit' => '~Auto best fit' => sub{ $_[0]->IV->autoZoom($_[2]) }],
					[],
					['25%' => sub{$_[0]-> IV-> zoom(0.25)}],
					['50%' => sub{$_[0]-> IV-> zoom(0.5)}],
					['75%' => sub{$_[0]-> IV-> zoom(0.75)}],
					['150%' => sub{$_[0]-> IV-> zoom(1.5)}],
					['200%' => sub{$_[0]-> IV-> zoom(2)}],
					['300%' => sub{$_[0]-> IV-> zoom(3)}],
					['400%' => sub{$_[0]-> IV-> zoom(4)}],
					['600%' => sub{$_[0]-> IV-> zoom(6)}],
					['1600%' => sub{$_[0]-> IV-> zoom(16)}],
					[],
					['~Increase' => '+' => '+' => sub{$_[0]-> IV-> zoom( $_[0]-> IV-> zoom * 1.1)}],
					['~Decrease' => '-' => '-' => sub{$_[0]-> IV-> zoom( $_[0]-> IV-> zoom / 1.1)}],
				]],
				['~Scaling' => [
					map {my $k = $_; [ $k, $k, sub { $_[0]->IV->scaling( $ist::{$k}() )} ]}
					qw(Box AND OR Triangle Quadratic Sinc Hermite Cubic Gaussian)
				]],
				['~Info' => 'Alt+F1' => '@F1' => \&iinfo],
			]],
			],
			'', 1,
		);
		$_[0]-> IV-> {menuadded}++;
	}
}

my $imgdlg;
sub create_image_dialog
{
	return $imgdlg if $imgdlg;
	$imgdlg  = Prima::Dialog::ImageOpenDialog-> new();
}

sub fdopen
{
	my $self = $_[0]-> IV;

	my $dlg = create_image_dialog( $self);
	my $i   = $dlg-> load( progressViewer => $self);

	if ( $i) {
		menuadd( $_[0]);
		$self-> image( $i);
		$self-> {fileName} = $dlg-> fileName;
		status( $_[0]);
	}
}

sub freopen
{
	my $self = $_[0]-> IV;
	my $i = Prima::Image-> new;
	$self-> watch_load_progress( $i);
	if ( $i-> load( $self-> {fileName}, loadExtras => 1)) {
		$self-> image( $i);
		status( $_[0]);
		message( $i->{extras}->{truncated} ) if defined $i->{extras}->{truncated};
	} else {
		message("Cannot reload ". $self-> {fileName}. ":$@");
	}
	$self-> unwatch_load_progress(0);
}

sub newwindow
{
	my ( $self, $filename, $i) = @_;
	my $w = Prima::Window-> new(
		onDestroy => \&iv_destroy,
		menuItems => $self-> menuItems,
		onMouseWheel => sub { iv_mousewheel( shift-> IV, @_)},
		size         => [ $i-> width + 50, $i-> height + 50],
	);
	$winCount++;
	$w-> insert( ImageViewer =>
		size   => [ $w-> size],
		%iv_prf,
	);
	$w-> IV-> image( $i);
	$w-> IV-> {fileName} = $filename;
	$w-> {omenuID} = $self-> {omenuID};
	$w->IV->{menuadded} = 1;
	$w->{conversion} = ict::Optimized;
	$w-> select;
	status($w);
}

sub fnewopen
{
	my $self = $_[0]-> IV;
	my $dlg  = create_image_dialog( $self);
	my $i    = $dlg-> load;

	newwindow( $_[0], $dlg-> fileName, $i) if $i;
}


sub fload
{
	my $self = $_[0]-> IV;
	my $f = $_[1];
	my $i = Prima::Image-> new;
	$self-> watch_load_progress( $i);

	if ( $i-> load( $f, loadExtras => 1)) {
		menuadd( $_[0]);
		my @sizes = ( $i-> size, map { $_ * 0.9 } $::application-> size);
		$self-> owner-> size( map {
			( $sizes[$_] > $sizes[$_ + 2]) ?  $sizes[$_ + 2] : $sizes[$_]
		} 0,1);
		$self-> image( $i);
		$self-> {fileName} = $f;
		status( $_[0]);
		message( $i->{extras}->{truncated} ) if defined $i->{extras}->{truncated};
	} else {
		message("Cannot load $f:$@");
	}

	$self-> unwatch_load_progress(0);
}


sub fsave
{
	my $iv = $_[0]-> IV;
	message('Cannot save '.$iv-> {fileName}. ":$@")
		unless $iv-> image-> save( $iv-> {fileName});
}

sub fsaveas
{
	my $iv = $_[0]-> IV;
	my $dlg  = Prima::Dialog::ImageSaveDialog-> new( image => $iv-> image);
	$iv-> {fileName} = $dlg-> fileName if $dlg-> save( $iv-> image);
	$dlg-> destroy;
}

sub setconv
{
	my ( $self, $menuID) = @_;
	return if $self-> {omenuID} eq $menuID;
	$self-> {omenuID}    = $menuID;
	$self-> {conversion} = (
	( $menuID eq 'N') ? ict::None : (
	( $menuID eq 'O') ? ict::Ordered : (
	( $menuID eq 'E') ? ict::ErrorDiffusion : ict::Optimized
	))
	);
}

sub icvt
{
	my $im = $_[0]-> IV-> image;
	$im->resample( $im->rangeLo, $im->rangeHi, 0, 255)
		if $im->type & im::GrayScale && $im->get_bpp != 8;
	$im-> set(
		conversion => $_[0]-> {conversion},
		type       => $_[1],
	);
	status( $_[0]);
	$_[0]-> IV-> palette( $im-> palette);
	$_[0]-> IV-> repaint;
}


sub iinfo
{
	my $i = $_[0]-> IV-> image;
	message_box(
		'',
		"File: ".$_[0]-> IV-> {fileName}."\n".
		"Width: ".$i-> width."\nHeight: ".$i-> height."\nBPP:".($i-> type&im::BPP)."\n".
		"Zoom: ".$_[0]-> IV-> zoom,
		0
	);
}

sub iv_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {drag} || $btn != mb::Right;
	$self-> {drag}=1;
	$self-> {x} = $x;
	$self-> {y} = $y;
	$self-> {wasdx} = $self-> deltaX;
	$self-> {wasdy} = $self-> deltaY;
	$self-> capture(1);
	$self-> pointer( $ico) if $ico;
}

sub iv_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {drag} && $btn == mb::Right;
	$self-> {drag}=0;
	$self-> capture(0);
	$self-> pointer( cr::Default) if $ico;
}

sub iv_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	return unless $self-> {drag};
	my ($dx,$dy) = ($x - $self-> {x}, $y - $self-> {y});
	$self-> deltas( $self-> {wasdx} - $dx, $self-> {wasdy} + $dy);
}

sub iv_mousewheel
{
	my ( $self, $mod, $x, $y, $z) = @_;
	$z = (abs($z) > 120) ? int($z/120) : (($z > 0) ? 1 : -1);
	my $xv = $self-> bring(($mod & km::Shift) ? 'VScroll' : 'HScroll');
	return unless $xv;
	$z *= ($mod & km::Ctrl) ? $xv-> pageStep : $xv-> step;
	if ( $mod & km::Shift) {
		$self-> deltaX( $self-> deltaX - $z);
	} else {
		$self-> deltaY( $self-> deltaY - $z);
	}
}


sub iv_destroy
{
	$winCount--;
	$::application-> close unless $winCount;
}

my $w = Prima::Window-> new(
	size => [ 300, 300],
	onDestroy => \&iv_destroy,
	onMouseWheel => sub { iv_mousewheel( shift-> IV, @_)},
	menuItems => [
	[ file => '~File' => [
		[ '~Open' =>  'F3'     => kb::F3     , \&fdopen],
		[],
		[ 'E~xit' => 'Alt+X' => '@X' => sub {$::application-> close}],
	]],
	],
	icon => Prima::Icon->load_stream(<<STREAM),
R0lGODdhMAAwAJEAAAAAAGhoaL+/v////ywAAAAAMAAwAIEAAABoaGi/v7////8CwZyPqcvtD6Oc
tIGLs968ezB84kiW5tlV6rKt7tG+biyrdE3duKRHl9xzBH2aScpWJP5Cy8pQgUFEnUnI1HA1Vh/X
rDYjCRyhIMaTvJV6YemHeI1tM8G7eO9MvQjmZTYd/QWwZ9fH16QGhyaY8HeHlzHIOKUD9qcYqWhY
2AgHyeXhV8gnqacUYxkaivnZgooI4pnzwRqbh0e4uDJmVTpzy1jzWxd6OExbbGwhl6yIzNws+gyN
Ql1tXa15jS3N3e3tUAAAOw==
STREAM
);

$w-> insert( ImageViewer =>
	size   => [ $w-> size],
	%iv_prf,
);
status($w);

if ( @ARGV && $ARGV[0] =~ /^-z(\d+(\.\d*)?)$/) {
	$w-> IV-> zoom($1);
	shift @ARGV;
}
fload( $w, $ARGV[0]), shift if @ARGV;
for ( @ARGV) {
	my $i = Prima::Image-> load($_);
	message("Cannot load $_:$@"), next unless $i;
	newwindow( $w, $_, $i);
}

Prima->run;


