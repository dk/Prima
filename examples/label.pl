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

Prima label widget

=item FEATURES

Demonstrates the basic usage of a Prima toolkit
and Prima::Label class capabilites, in particular
text wrapping.

=cut

use Prima;
use Prima::Const;
use Prima::Buttons;
use Prima::Label;
use Prima::Application;

my $w = Prima::MainWindow-> create(
size => [ 430, 200],
text => "Static texts",
);

my $b1 = $w-> insert( Button => left => 20 => bottom => 0);

$w-> insert( Label =>
# font => { height => 24},
	origin => [ 20, 50],
	text => "#define inherited CComponent-> \n#define my  ((( P~AbstractMenu) self)-> self)-> \n #define var (( PAbstractMenu) self)-> ",
	focusLink => $b1,
	wordWrap => 1,
	height => 80,
	width => 112,
	alignment => ta::Center,
	growMode => gm::Client,
	showPartial => 0,
);

my $b2 = $w-> insert( Button =>
left => 320,
bottom => 0,
growMode => gm::GrowLoX,
);

$w-> insert( Label =>
	origin    => [ 320, 50],
	text   => 'Disab~led',
	focusLink => $b2,
	autoHeight => 1,
	enabled   => 0,
	growMode  => gm::GrowLoX,
);


run Prima;
