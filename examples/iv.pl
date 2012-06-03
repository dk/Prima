#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  $Id$
#
=pod 
=item NAME

A image viewer program

=item FEATURES

Demonstrates usage of Prima image subsystem, in particular:

- Standard open dialog. Note it's behavior with the multi-frame images.
- Standard save dialog. Note the graphic filters usage.
- Image conversion routines.
- Standard Prima::ImageViewer class.

Test the correct implementation of the internal image paint routines,
in particular on the paletted displays and the representation of 1-bit
images/icons with non-BW palette.

Note the mouse wheel interaction.

=cut

use strict;
use Prima qw(ImageViewer StdDlg MsgBox);
use Prima::Application name => "IV";

my $ico = Prima::Icon-> create;
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
	onSize      => \&iv_size,
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
					['N' => '~No halftoning' => sub {setconv(@_)}],
					['O' => '~Ordered' => sub {setconv(@_)}],
					['E' => '~Error diffusion' => sub {setconv(@_)}],
					['*P' => 'O~ptimized' => sub {setconv(@_)}],
				]],
				['~Zoom' => [
					['~Normal ( 100%)' => 'Ctrl+Z' => '^Z' => sub{$_[0]-> IV-> zoom(1.0)}],
					['~Best fit' => 'Ctrl+Shift+Z' => km::Shift|km::Ctrl|ord('z') => \&zbestfit],
					[],
					['abfit' => '~Auto best fit' => sub{
						zbestfit($_[0]) if $_[0]-> IV-> {autoBestFit} = $_[0]-> menu-> abfit-> toggle;
					}],
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
	$imgdlg  = Prima::ImageOpenDialog-> create();
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
	} else {
		Prima::MsgBox::message("Cannot reload ". $self-> {fileName}. ":$@");
	}
	$self-> unwatch_load_progress(0);
}

sub newwindow
{
	my ( $self, $filename, $i) = @_;
	my $w = Prima::Window-> create(
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
	} else {
		Prima::MsgBox::message("Cannot load $f:$@");
	}
	
	$self-> unwatch_load_progress(0);
}


sub fsave
{
	my $iv = $_[0]-> IV;
	Prima::MsgBox::message('Cannot save '.$iv-> {fileName}. ":$@")
		unless $iv-> image-> save( $iv-> {fileName});
}

sub fsaveas
{
	my $iv = $_[0]-> IV;
	my $dlg  = Prima::ImageSaveDialog-> create( image => $iv-> image);
	$iv-> {fileName} = $dlg-> fileName if $dlg-> save( $iv-> image);
	$dlg-> destroy;
}

sub setconv
{
	my ( $self, $menuID) = @_;
	return if $self-> {omenuID} eq $menuID;
	$self-> menu-> uncheck( $self-> {omenuID});
	$self-> menu-> check( $menuID);
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
	Prima::MsgBox::message_box(
		'',
		"File: ".$_[0]-> IV-> {fileName}."\n".
		"Width: ".$i-> width."\nHeight: ".$i-> height."\nBPP:".($i-> type&im::BPP)."\n".
		"Zoom: ".$_[0]-> IV-> zoom,
		0
	);
}

sub zbestfit
{
	my $iv = $_[0]-> IV;
	my @szA = $iv-> image-> size;
	my @szB = $iv-> get_active_area(2);
	my $x = $szB[0]/$szA[0];
	my $y = $szB[1]/$szA[1];
	$iv-> zoom( $x < $y ? $x : $y);
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
	$z = int( $z / 120);
	my $xv = $self-> bring(($mod & km::Shift) ? 'VScroll' : 'HScroll');
	return unless $xv;
	$z *= ($mod & km::Ctrl) ? $xv-> pageStep : $xv-> step;
	if ( $mod & km::Shift) {
		$self-> deltaX( $self-> deltaX - $z);
	} else {
		$self-> deltaY( $self-> deltaY - $z);
	}
}


sub iv_size
{
	zbestfit($_[0]-> owner) if $_[0]-> {autoBestFit};
}

sub iv_destroy
{
	$winCount--;
	$::application-> close unless $winCount;
}

my $w = Prima::Window-> create(
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
	Prima::MsgBox::message("Cannot load $_:$@"), next unless $i;
	newwindow( $w, $_, $i);
}

run Prima;


