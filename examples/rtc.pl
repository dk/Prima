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

=pod

=head1 NAME

examples/rtc.pl - Prima scrollbar widget

=head1 FEATURES

A Prima toolkit demonstration example.
Tests the L<Prima::Scrollbar> widget and dynamic
change of its parameters.

=cut

use strict;
use warnings;

use Prima qw( Buttons ScrollBar);
use Prima::Application name => 'rtc';

package UserInit;

my $w = Prima::MainWindow-> create(
text=> "Test of RTC",
origin => [ 200, 200],
size   => [ 250, 300],
);

$w-> insert( "Button",
pack => { side => 'bottom', pady => 20 },
text => "Change scrollbar direction",
onClick=> sub {
	my $i = $_[0]-> owner-> scrollbar;
	$i-> vertical( ! $i-> vertical);
}
);

$w-> insert( "ScrollBar",
name    => "scrollbar",
pack => { pady => 60, padx => 60, fill => 'both', expand => 1 },
size => [ 150, 150],
onCreate => sub {
	Prima::Timer-> create(
		timeout=> 1000,
		timeout=> 200,
		owner  => $_[0],
		onTick => sub{
			# $_[0]-> owner-> vertical( !$_[0]-> owner-> vertical);
			my $t = $_[0]-> owner;
			my $v = $t-> partial;
			$t-> partial( $v+1);
			$t-> partial(1) if $t-> partial == $v;
			#$_[0]-> timeout( $_[0]-> timeout == 1000 ? 200 : 1000);
		},
	)-> start;
},
);

run Prima;
