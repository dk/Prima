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

Prima DetailedList widget example

=head1 FEATURES

Demonstrates the usage of L<Prima::DetailedList> class.
Note the header-driven actions: column sort ( vertical, with click ) and 
rearrangement ( horizontal, with drag ).

=cut

use strict;
use warnings;
use Prima;
use Prima::DetailedList;

package Sheet;


use Prima;
use Prima::Header;
use Prima::Application;

my $w = Prima::MainWindow-> create( packPropagate => 0);

my @items;

my $fname;

my $os = Prima::Application-> get_system_info;
if ( $os-> {apc} == apc::Unix) {
	$fname = '/etc/services';
} elsif ( $os-> {apc} == apc::Win32) {
	$fname =
	( $os-> {system} eq 'Windows NT') ?
		"$ENV{SYSTEMROOT}\\system32\\drivers\\etc\\services" : undef;
	$fname = 'z:/etc/services' if Prima::Utils::username eq 'dk';
};

if ( defined $fname) {
	open F, $fname;
	while (<F>) {
		next if /^#/;
		chomp;
		s/\t/ /g;
		next unless /^(\w+)\s+(\d+)(\\|\/)(\w+)\s+?\#\s*(.*)$/;
		push ( @items, [ $1, $2, $4, $5]);
	}
	close F;
} else {
	push ( @items,
	[qw(domain   53 tcp	   Domain_Name_Server    )],
	[qw(domain   53 udp	   Domain_Name_Server    )],
	[qw(xns-ch   54 tcp	   XNS_Clearinghouse     )],
	[qw(xns-ch   54 udp	   XNS_Clearinghouse     )],
	[qw(isi-gl   55 tcp	   ISI_Graphics_Language )],
	[qw(isi-gl   55 udp	   ISI_Graphics_Language )],
	[qw(xns-auth 56 tcp	   XNS_Authentication    )],
	);
}

my $l = $w-> insert( DetailedList =>
	pack    => { expand => 1, fill => 'both' },
	items   => \@items,
	headers => [ 'Service' , 'Port', 'Protocol', 'Description'],
	columns => 4,
	onSort => sub {
		my ( $self, $col, $dir) = @_;
		return if $col != 1;
		if ( $dir) {
			$self-> {items} = [
				sort { $$a[$col] <=> $$b[$col]}
				@{$self-> {items}}
			];
		} else {
			$self-> {items} = [
				sort { $$b[$col] <=> $$a[$col]}
				@{$self-> {items}}
			];
		}
		$self-> clear_event;
	},
);

#$::application-> yield;
#my $h = $l-> { header};
#$h-> { widths}-> [0] += 50;
#$h-> recalc_maxwidth;
#$l-> Header_SizeItem( $h, 0, $h-> { widths}-> [0] - 50, $h-> { widths}-> [0]);

run Prima;

