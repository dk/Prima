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
=item NAME

Prima outline widget

=item FEATURES

Demonstrates the basic Prima toolkit usage
and Prima::Outlines class. 

=cut

use Prima qw(Application Outlines);

my $w = Prima::MainWindow-> create( size => [ 200, 200]);
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
	],
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
