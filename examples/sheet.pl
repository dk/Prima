=pod

=head1 NAME

examples/sheet.pl - the DetailedList widget example

=head1 FEATURES

Demonstrates the usage of the L<Prima::DetailedList> class.
Note the header-driven actions: column sort ( vertical, with click ) and
rearrangement ( horizontal, with drag ).

=cut

use strict;
use warnings;
use Prima qw(DetailedList Application);

package Sheet;

my $w = Prima::MainWindow-> new( packPropagate => 0);

my @items;
my $fname;

my $os = Prima::Application-> get_system_info;
if ( $os-> {apc} == apc::Unix) {
	$fname = '/etc/services';
} elsif ( $os-> {apc} == apc::Win32) {
	$fname =
	( $os-> {system} eq 'Windows NT') ?
		"$ENV{SYSTEMROOT}\\system32\\drivers\\etc\\services" : undef;
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
	aligns  => [undef, ta::Right],
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

Prima->run;

