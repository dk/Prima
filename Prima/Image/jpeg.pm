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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  $Id$
#

use strict;
use Prima;
use Prima::Buttons;
use Prima::Label;
use Prima::Sliders;
use Prima::Edit;

package Prima::Image::jpeg;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);


sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		text   => 'JPEG filter',
		width  => 241,
		height => 192,
		designScale => [ 7, 16],
		centered => 1,
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $se = $self-> insert( qq(Prima::SpinEdit) => 
		origin => [ 5, 145],
		name => 'Quality',
		size => [ 74, 20],
		min => 1,
		max => 100,
	);
	$self-> insert( qq(Prima::Label) => 
		origin => [ 5, 169],
		size => [ 131, 20],
		text => '~Quality (1-100)',
		focusLink => $se,
	);
	$self-> insert( qq(Prima::CheckBox) => 
		origin => [ 5, 105],
		name => 'Progressive',
		size => [ 131, 36],
		text => '~Progressive',
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 141, 150],
		name => 'OK',
		size => [ 96, 36],
		text => '~OK',
		default => 1,
		modalResult => mb::OK,
		delegations => ['Click'],
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 141, 105],
		name => 'Cancel',
		size => [ 96, 36],
		text => 'Cancel',
		modalResult => mb::Cancel,
	);
	my $comm = $self-> insert( qq(Prima::Edit) => 
		origin => [ 5, 5],
		size   => [ 231, 75],
		name   => 'Comment',
		text   => '',
	);
	$self-> insert( qq(Prima::Label) => 
		origin => [ 5, 85],
		size => [ 131, 20],
		text => '~Comment',
		focusLink => $comm,
	);
	return %profile;
}

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	$self-> Quality-> value( exists( $image-> {extras}-> {quality}) ? 
		$image-> {extras}-> {quality} : $codec-> {saveInput}-> {quality});
	$self-> Progressive-> checked( exists( $image-> {extras}-> {progressive}) ? 
		$image-> {extras}-> {progressive} : $codec-> {saveInput}-> {progressive});
	$self-> Comment-> text( exists( $image-> {extras}-> {comment}) ?
		$image-> {extras}-> {comment} : '');
}

sub OK_Click
{
	my $self = $_[0];
	$self-> {image}-> {extras}-> {quality} = $self-> Quality-> value;
	$self-> {image}-> {extras}-> {progressive} = $self-> Progressive-> checked;
	$self-> {image}-> {extras}-> {comment} = $self-> Comment-> text;
	delete $self-> {image};
}

sub save_dialog
{
	return Prima::Image::jpeg-> create;
}

sub exif_get_orientation
{
# courtesy from gtk/gdk-pixbuf/io-jpeg.c
	my $i = shift;

	return unless exists $i->{extras}->{codecID};
	return unless Prima::Image->codecs->[ $i->{extras}->{codecID} ]->{fileShortType} eq 'JPEG';
	
	my $exif;
	for my $a ( @{ $i->{extras}->{appdata} || [] }) {
		next unless defined $a && $a =~ s/^Exif\0\0//;
		$exif = $a;
	}
	return unless $exif && length($exif) > 32;

        # Check for exif header and catch endianess */
	# Just skip data until exif header - it should be within 16 bytes from marker start.
	#  Normal structure relative to APP1 marker -
	#       0x0000: APP1 marker entry = 2 bytes
	#  	0x0002: APP1 length entry = 2 bytes
	#       0x0004: Exif Identifier entry = 6 bytes
	#       0x000A: Start of exif header (Byte order entry) - 4 bytes  
	#           	- This is what we look for, to determine endianess.
	#       0x000E: 0th IFD offset pointer - 4 bytes
	#
	#       exif_marker->data points to the first data after the APP1 marker
	#       and length entries, which is the exif identification string.
	#       The exif header should thus normally be found at i=6, below,
	#       and the pointer to IFD0 will be at 6+4 = 10.
	my $order;
	if ( $exif =~ s/.{0,12}\x49\x49\x2a\x00//) {
		$order = 'v'; # LE
	} elsif ( $exif =~ s/.{0,12}\x4d\x4d\x00\x2a// ) {
		$order = 'n'; # BE
	} else {
		return;
	}

        # Read out the offset pointer to IFD0 
	my $offset = unpack("\U$order", $exif) - 4;
	return if $offset < 0 || length($exif) < $offset;
	substr( $exif, 0, $offset, '');

	# Find out how many tags we have in IFD0. As per the exif spec, the first
	# two bytes of the IFD contain a count of the number of tags.
	my $tags = unpack($order, $exif);
	$exif =~ s/^..//;
	return if $tags * 12 > length $exif;

	# Check through IFD0 for tags of interest */
	while ($tags--){
		# The tags are listed in consecutive 12-byte blocks
		my ($tag, $type, $count, $val) = unpack("$order$order\U$order\L$order", $exif);
		$exif =~ s/^.{12}//;
		# Is this the orientation tag? 
		next unless $tag == 0x112;
		return if $type != 3 or $count != 1 or $val > 8;
		return $val;
	}

	return; # nothing found
}

#      1        2       3      4         5            6           7          8
#
#    888888  888888      88  88      8888888888  88                  88  8888888888
#    88          88      88  88      88  88      88  88          88  88      88  88
#    8888      8888    8888  8888    88          8888888888  8888888888          88
#    88          88      88  88
#    88          88  888888  888888
#

sub exif_transform_image
{
        my ( $image, $orientation ) = @_;

	return unless defined $orientation and $orientation > 1;

	if ( $orientation == 2 ) {
		$image->mirror(0);
	} elsif ( $orientation == 3 ) {
		$image->rotate(180);
	} elsif ( $orientation == 4 ) {
		$image->mirror(1);
	} elsif ( $orientation == 5 ) {
                $image->rotate(90);
		$image->mirror(0);
	} elsif ( $orientation == 6 ) {
		$image->rotate(90);
	} elsif ( $orientation == 7 ) {
                $image->rotate(90);
                $image->mirror(1);
	} elsif ( $orientation == 8 ) {
                $image->rotate(270);
	}
}
