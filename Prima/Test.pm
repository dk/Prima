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

our @ISA     = qw(Exporter);
our @EXPORT  = qw(create_window set_flag get_flag reset_flag wait_flag);
our $noX11   = 1 if defined Prima::XOpenDisplay();
our $flag;

sub import
{
	my @args    = grep { $_ ne 'noX11' } @_;
	my $needX11 = @args == @_;
	__PACKAGE__->export_to_level(1,@args);
	plan skip_all => "no X11 connection" if $needX11 and $noX11;
}

sub create_window
{
	return if $noX11;
	eval "use Prima::Application name => 'failtester';"; die $@ if $@;
	return Prima::Window-> create(
		onDestroy => sub { $::application-> close},
		size => [ 200,200],
	);
}

sub __wait
{
	return 0 if $noX11;

	my $tick = 0;
	my $t = Prima::Timer-> create( timeout => 500, onTick => sub { $tick = 1 });
	$flag = 0;
	$t-> start;
	$::application-> yield while not $flag and not $tick;
	$t-> destroy;
	return $flag;
}

sub set_flag   { $flag = 1 }
sub get_flag   { $flag }
sub reset_flag { $flag = 0 }
sub wait_flag  { get_flag || &__wait }

1;
