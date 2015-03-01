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
package Prima::Test;
# use vars qw( @ISA $noX11 $w $dong);
use strict;
# startup
use Prima::Config;
use Prima::noX11;
use Prima;

our @ISA;
our $w;
our $dong;
# testing if is running over a dumb terminal
our $noX11 = 1 if defined Prima::XOpenDisplay();

my $tick;

init();

sub init
{
	unless ( $noX11) {
		eval "use Prima::Application name => 'failtester';"; die $@ if $@;
		$w = Prima::Window-> create(
			onDestroy => sub { $::application-> close},
			size => [ 200,200],
		);
	}
}

sub end_wait {
	$tick = 1;
}

sub wait {
	return 0 if $noX11;
	$tick = 0;
	my $t = Prima::Timer-> create( timeout => 500, 
		onTick => sub { $tick = 1 });
	$dong = 0;
	$t-> start;
	while ( 1) {
		last if $dong || $tick;
		$::application-> yield;
	}
	$t-> destroy;
	return $dong;
}

sub set_dong {
	$dong = 1;
}

1;
