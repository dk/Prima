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
use Prima::ComboBox;
use Prima::Edit;
use Prima::ImageViewer;
use Prima::Label;
use Prima::Sliders;
use Prima::Image::TransparencyControl;

package Prima::Image::gif;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
	my $def = $_[ 0]-> SUPER::profile_default;
	my %prf = (
		width    => 480,
		text     => 'Gif89a filter',
		height   => 300,
		centered => 1,
		designScale => [ 7, 16],
	);
	@$def{keys %prf} = values %prf;
	return $def;
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $a = $self-> insert( qq(Prima::CheckBox) => 
		origin => [ 4, 261],
		name => 'Transparent',
		size => [ 133, 36],
		text => '~Transparent',
		delegations => ['Check'],
	);
	my $b = $self-> insert( qq(Prima::CheckBox) => 
		origin => [ 144, 261],
		name => 'Interlaced',
		size => [ 100, 36],
		text => '~Interlaced',
	);
	$self-> insert( qq(Prima::Image::TransparencyControl) => 
		origin => [ 4, 100],
		size => [ 364, 158],
		text => '',
		name => 'TC',
	);
	my $se = $self-> insert( qq(Prima::Edit) => 
		origin => [ 10, 10],
		name => 'Comment',
		size => [ 350, 71],
		vScroll => 1,
		text => '',
	);
	$self-> insert( qq(Prima::Label) => 
		origin => [ 10, 85],
		size => [ 100, 20],
		text => '~Comment',
		focusLink => $se,
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 380, 259],
		name => 'OK',
		size => [ 96, 36],
		text => '~OK',
		default => 1,
		modalResult => mb::OK,
		delegations => ['Click'],
	);
	$self-> insert( qq(Prima::Button) => 
		origin => [ 380, 213],
		size => [ 96, 36],
		text => 'Cancel',
		modalResult => mb::Cancel,
	);
	return %profile;
}


sub transparent
{
	my $self = $_[0];
	$self-> Transparent-> checked( $_[1]);
	$self-> TC-> enabled( $_[1]);
}

sub Transparent_Check
{
	my ( $self, $tr) = @_;
	$self-> transparent( $tr-> checked);
}

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	return unless $image;
	$self-> Interlaced-> checked( exists( $image-> {extras}-> {interlaced}) ? 
		$image-> {extras}-> {interlaced} : $codec-> {saveInput}-> {interlaced});
	$self-> transparent( $image-> {extras}-> {transparentColorIndex} ? 1 : 0);
	$self-> TC-> image( $image);
	$self-> TC-> index( exists( $image-> {extras}-> {transparentColorIndex}) ? 
		$image-> {extras}-> {transparentColorIndex} : 0);
	$self-> Comment-> text( exists( $image-> {extras}-> {comment}) ? 
		$image-> {extras}-> {comment} : ($codec-> {saveInput}-> {comment}));
}

sub OK_Click
{
	my $self = $_[0];
	if ( $self-> Transparent-> checked) {
		$self-> {image}-> {extras}-> {transparentColorIndex} = $self-> TC-> index;
	} else {
		delete $self-> {image}-> {extras}-> {transparentColorIndex};
	}
	$self-> {image}-> {extras}-> {interlaced} = $self-> Interlaced-> checked;
	my $x = $self-> Comment-> text;
	if ( length $x) {
		$self-> {image}-> {extras}-> {comment} = $x;
	} else {
		delete $self-> {image}-> {extras}-> {comment};
	}
	delete $self-> {image};
	$self-> TC-> image( undef);
}

sub save_dialog { Prima::Image::gif-> create }

1;
