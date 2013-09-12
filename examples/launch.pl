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

=head1 NAME

A Prima example launcher

=head1 FEATURES

Uses to execute several Prima examples in one task space.
Many examples inflict the view af a user-selected widget, but
lack existence of the appropriate one. The launcher helps
such experiments. See cv.pl or fontdlg.pl in particular.

=cut

use strict;
use warnings;

use Prima;
use Prima::Classes;
use Prima::StdDlg;

my $dlg;
my $closeAction = 0;
my $myApp;

package MyOpenDialog;
use vars qw( @ISA);
@ISA = qw( Prima::OpenDialog);

sub ok
{
	Launcher::launch( $_[0]-> Name-> text);
}

package MyApp;
use vars qw( @ISA);
@ISA = qw( Prima::Application);

sub close
{
	$_[0]-> SUPER::close if $closeAction;
}

sub destroy
{
	$_[0]-> SUPER::destroy if $closeAction;
}


package Generic;

sub start {
	$myApp = $::application = MyApp-> create( name => "Launcher");
	$dlg = MyOpenDialog-> create(
		name   => 'Launcher',
		filter => [
			['Scripts' => '*.pl'],
			['All files' => '*'],
		],
		onEndModal => sub {
			$closeAction++;
			$::application-> close;
			$closeAction--;
		},
	);
	my $cl = $dlg-> Cancel;

	$cl-> text('Close');
	$cl-> set(
		onClick => sub {
			$closeAction++;
			$dlg-> cancel;
			$closeAction--;
		},
	);
	$dlg-> execute_shared;
}

package Prima::Application;

sub create
{
	my $x = shift;
	return $myApp ? $myApp : $x-> SUPER::create( @_);
}

package Launcher;

sub launch
{
	my $pgm = $_[0];
	my $ok;
	my $app = $::application;
	{
		my $package = $pgm;
		$package =~ s{^.*[\\/]([^\\/]+)\..*$}{$1};
		print "require '$pgm' != 1 || run $package;";
		$ok = eval( "require '$pgm' != 1 || run $package; 1;");
	}
	$::application = $app;
	print $@ unless $ok;
}

Generic::start;

run Prima;

