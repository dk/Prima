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

use strict;
use Prima Application => {name => 'Frames sample'};
use Prima::FrameSet;
use Prima qw(Buttons);

my $w = Prima::MainWindow-> create(
	text   => "Frames example",
	size => [ map { $_ - 128} $::application-> size],
);

my $frame = $w-> insert(
	FrameSet =>
	size => [$w-> size],
	origin => [0, 0],
	frameSizes => [qw(211 20% 123 10% * 45% *)],
	opaqueResize => 0,
	frameProfiles => [ 0,0, { minFrameWidth => 123, maxFrameWidth => 123 }],
);

for (my $i = 0; $i < 6; $i++) {
	next if $i == 4;
	$frame-> insert_to_frame(
		$i,
		Button =>
		origin => [10, 10],
		text => '~Ok',
		onClick => sub {
			$_[0]-> text($_[0]-> owner-> name);
		},
	);
}

my $subframe = $frame-> insert_to_frame(
	4,
	FrameSet =>
	arrangement => fra::Vertical,
	size => [$frame-> frames-> [4]-> size],
	origin => [0, 0],
	opaqueResize => 0,
	frameSizes => [qw(33% 33% *)],
);

run Prima;

