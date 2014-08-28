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

examples/notebk.pl - Prima notebook widget

=head1 FEATURES

Demonstrates the basic Prima toolkit usage and
L<Prima::TabbedNotebook> standard class.

=cut

use strict;
use warnings;
use Prima qw(Buttons Notebooks ScrollWidget Application MsgBox);

package Bla;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);

	my $n = $self-> insert( TabbedScrollNotebook =>
		pack => { fill => 'both', expand => 1, padx => 20, pady => 20 },
#     		pageCount => 11,
		tabs => [0..5,5,5..10],
		name => 'book',
	);

	$n-> insert_to_page( 0 => 'Button');
	my $j = $n-> insert_to_page( 1 => 'CheckBox' => left => 200);
	$n-> insert_to_page( 2,
		[ Button => origin => [ 0, 0], ],
		[ Button => origin => [ 10, 40], ],
		[ Button => origin => [ 10, 70], ],
		[ Button => origin => [ 10,100], ],
		[ Button => origin => [ 110, 10], ],
		[ Button => origin => [ 110, 40], ],
		[ Button => origin => [ 110, 70], ],
		[ Button => origin => [ 110,100], ],
	);
	$n-> insert_transparent('Button',
		name   => 'TopMostButton',
		text   => 'Toggle Orientation',
		origin => [0,30],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n-> orientation($n-> orientation ? 0 : 1) },
	);
	
	$n-> insert_transparent('Button',
		name   => 'StyleButton',
		text   => 'Toggle Style',
		origin => [0,5],
		size   => [200,20],
		growMode => gm::XCenter,
		onClick => sub { $n-> style($n-> style ? 0 : 1) },
	);
	$n-> use_current_size;
	return %profile;
}


package Generic;

my $w = Bla-> create(
	size => [ 600, 300],
	y_centered  => 1,
	menuItems => [[ '~Action' => [
		[ '~New tab', 'Ctrl+N', '^N', sub {
			my $book   = shift->book;
			my $tabid  = scalar(@{$book->TabSet->tabs}) + 1;
			my $pageno = $book->insert_page("tab$tabid");
			$book->insert_to_page($pageno, Button => 
				origin  => [ 20, 20 ], 
				text    => "$tabid",
			),
		}],
		[ 'New ~page', 'Ctrl+M', '^M', sub {
			my $book = shift->book;
			my $tabid  = $book->page2tab($book->pageIndex) + 1;
			my $pageid = $book->pageIndex + 1;
			my $pageno = $book->insert_page("tab$tabid", $pageid - 1);
			$book->insert_to_page($pageno, Button => 
				origin  => [ 20, 20 ], 
				text    => "$tabid/$pageid",
			),
		}],
		[ '~Delete tab', 'Ctrl+W', '^W', sub {
			my $book = shift->book;
			$book->delete_page($book->pageIndex, 1);
		}],			
	]]],
);

run Prima;
