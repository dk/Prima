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

use strict;
use warnings;

use Prima::Config;
use Prima::noX11;
use Prima;
use Test::More;

our @ISA = qw(Exporter);
our @EXPORT_OK = qw(create_window set_flag get_flag reset_flag wait noX11);
our @EXPORT    = qw(create_window set_flag get_flag reset_flag wait);

my $noX11 = 1 if defined Prima::XOpenDisplay();

sub import
{
    my ($self) = @_;
    my @args = grep { $_ eq 'noX11' } @_;
    my $test_runs_without_x11 = scalar @args;

    $self->export_to_level( 1, @args);

    plan skip_all => "skipping all because noX11"
        if( $test_runs_without_x11 && $noX11 );
}

our $dong;
our $tick;

sub create_window {
	unless ($noX11) {
		eval "use Prima::Application name => 'failtester';"; die $@ if $@;
		return Prima::Window-> create(
			onDestroy => sub { $::application-> close},
			size => [ 200,200],
		);
	}    
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

sub set_flag {
    shift if ref $_[0]; # ok to call as a method
    my $flag = shift;
    if( $flag ) {
        $tick = 1;
    } else {
        $dong = 1;
    }
}

sub reset_flag {
    $dong = 0;
}

sub get_flag {
    return $dong;
}

1;
