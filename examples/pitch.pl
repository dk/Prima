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
use Prima;
use Prima::Utils;
=pod 
=item NAME

A sound example

=item FEATURES

Tests the implementation of apc_beep_tone() function.

=cut


my %octave = (
	'C'  => 523,
	'B'  => 493,
	'A#' => 466,
	'A'  => 440,
	'G#' => 415,
	'G'  => 391,
	'F#' => 369,
	'F'  => 349,
	'E'  => 329,
	'D#' => 311,
	'D'  => 293,
	'C#' => 277,
);

$_ = 'E2D#EF2E2  G#2A4';
#$_ = 'AEAEAG#2 2EG#EG#A2 2EAEAG#2 2EG#EG#A2 AB2 BC2 C2BAG#A2 AB2 BC2 C2BAG#A3';

for ( m/\G([A-G\s][\#\d]*)/g) {
	my $d = (s/(\d+)$//) ? $1 : 1;
	$octave{$_} ? 
		Prima::Utils::sound( $octave{$_}, 100 * $d) : 
		select(undef,undef,undef,0.1 * $d);
}

