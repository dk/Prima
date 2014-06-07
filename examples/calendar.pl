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

examples/calendar.pl - Standard calendar widget

=head1 FEATURES

Demonstrates usage of L<Prima::Calendar>.
Note the special check of C<useLocale> success.

=cut

use strict;
use warnings;
use Prima;
use Prima::Application name => 'Calendar';
use Prima::Calendar;

my $cal;
my $w = new Prima::MainWindow(
	text => "Calendar example",
	size => [ 200, 200],
	menuItems => [[ "~Options" => [
		[ 'locale', 'Use ~locale', 'Ctrl+L', '^L', sub {
			my ( $self, $mid) = @_;
			my $newstate;
			$cal-> useLocale( $newstate = $self-> menu-> toggle( $mid));
			$cal-> notify(q(Change));
			return unless $newstate && !$cal-> useLocale;
			$self-> menu-> uncheck( $mid);
			Prima::message("Selecting 'locale' failed");
		}],
		[ 'Re~set to current date', 'Ctrl+R', '^R', sub {
			$cal-> date_from_time( localtime( time));
		}],
		[ 'monday', '~Monday is the first day of week', sub {
			my ( $self, $mid) = @_;
			$cal-> firstDayOfWeek( $self-> menu-> toggle( $mid) ? 1 : 0);
		}],
	]]],
);

$cal = $w-> insert( Calendar =>
	useLocale => 1,
	onChange  => sub {
		$w-> text( "Calendar - ".$cal-> date_as_string);
	},
	pack => { expand => 1, fill => 'both'},
);

$w-> menu-> locale-> check if $cal-> useLocale;

run Prima;

