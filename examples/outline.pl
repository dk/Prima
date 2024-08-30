=pod

=head1 NAME

examples/outline.pl - outline widget

=head1 FEATURES

Demonstrates the L<Prima::Outlines> widget

=cut

use strict;
use warnings;
use Prima qw(Application Outlines);

my $w = Prima::MainWindow-> new(
	size => [ 200, 200],
	designScale => [7, 16],
);
my $o = $w-> insert(
	DirectoryOutline =>
	#StringOutline =>
	popupItems => [
		['Delete this' => sub{
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			$_[0]-> delete_item( $x);
		}],
		['Insert updir below' => sub{
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			my ( $p, $o) = $_[0]-> get_item_parent( $x);
			$_[0]-> insert_items( $p, $o + 1, [
				[ $^O =~ /win32/ ? 'C:' : '/', ''], [], 0
			]);
		}],
		['Insert updir inside' => sub{
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			$_[0]-> insert_items( $x, 0, [
				[ $^O =~ /win32/ ? 'C:' : '/', ''], [], 0
			]);
		}],
		['Expand this' => sub{
			my ( $x, $l) = $_[0]-> get_item( $_[0]-> focusedItem);
			$_[0]-> expand_all( $x);
		}],
		['Toogle multi select' => sub {
			$_[0]-> multiSelect( !$_[0]-> multiSelect);
		}],
		[],
		[Style => [
			[ '(plusminus' => 'Plus-minus' => sub { $_[0]->style('plusminus') } ],
			[ '*triangle)' => 'Triangle' => sub { $_[0]->style('triangle') } ],
		]],
	],
	style => 'triangle',
	multiSelect => 0,
	extendedSelect => 1,
	path => '.',
	buffered => 1,
	pack => { expand => 1, fill => 'both'},
	onSelectItem => sub {
		my ($self, $index) = @_;
		#print $self-> path."\n";
	},
	#items => [['wdcdec']],
	items => [
		['Single string'],
		['Reference', [
			['Str1'],
		]],
		['Single string'],
		['Another single string', undef],
		['Empty reference', []],
		['Reference', [
			['Str1 -------------------------------------------------'],
			['Str2'],
			['Str3', [
				['Subref1'],
				['Subref2'],
			]]],
		],
		['XXL'],
	]
);
my ( $i, $l) = $o-> get_item( 1);
#$o-> expand_all;
#$o-> path('e:\Prima');

run Prima;
