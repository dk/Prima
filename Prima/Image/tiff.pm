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
use Prima::VB::VBLoader;

package Prima::Image::tiff;

sub on_change
{
	my ( $self, $codec, $image) = @_;
	$self-> {image} = $image;
	$self-> {image}-> {extras} ||= {};
	my $e = $image-> {extras};
	for ( @{$self-> TextSelector-> items}) {
		if ( exists $e->{$_}) {
			$self-> {TextFields}->{$_} = $e->{$_};
		} else {
			$self-> {TextFields}->{$_} = $codec-> {saveInput}-> {$_};
		}
	}
	for ( qw( Compression ResolutionUnit XResolution YResolution)) {
		$self-> bring($_)-> text( exists $e-> {$_} ?  $e-> {$_} : $codec-> {saveInput}->{$_});
	}
	$self-> TextField-> text( $self-> {TextFields}-> {$self-> TextSelector-> text});
}

sub OK_Click
{
	my $self = $_[0]-> owner;
	$self-> {image}-> {extras} ||= {};
	my $e = $self-> {image}-> {extras};
	$self-> {TextFields}->{$self-> TextSelector-> text} = $self-> TextField-> text;
	for ( @{$self-> TextSelector-> items}) {
		if ( exists($self-> {TextFields}->{$_}) && length($self-> {TextFields}->{$_})) {
			$e-> {$_} = $self-> {TextFields}->{$_};
		} else {
			delete $e->{$_};
		}
	}
	$e-> {Compression} = $self-> Compression-> text;
	$e-> {ResolutionUnit} = $self-> ResolutionUnit-> text;
	if ( $e-> {ResolutionUnit} eq 'none') {
		delete $e-> {XResolution};
		delete $e-> {YResolution};
		delete $e-> {ResolutionUnit};
	} else {
		$e-> {XResolution} = $self-> XResolution-> value;
		$e-> {YResolution} = $self-> YResolution-> value;
	}
	delete $self-> {image};
}

sub Cancel_Click
{
	my $self = $_[0]-> owner;
	delete $self-> {image};
}

sub save_dialog
{
	my $codec = $_[1];
	return Prima::VBLoad( 'Prima::Image::tiff.fm', 
		Form1 => { 
			visible => 0,
			onChange  => \&on_change,
		},
		Compression => { items => [ qw(NONE), 
			map { m/^Compression-(.*)$/ ? $1 : () } @{$codec-> {featuresSupported}} ]},
		OK      => { onClick => \&OK_Click },
		Cancel  => { onClick => \&Cancel_Click },
	);
}

1;
