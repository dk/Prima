#
#  Copyright (c) 1997-2003 The Protein Laboratory, University of Copenhagen
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
#  Created by Dmitry Karasik <dmitry@karasik.eu.org>
#
#  $Id$

use Prima qw(Application Themes ScrollBar Buttons InputLine ExtLists Notebooks ScrollWidget);

=pod 
=item NAME

Theme selector

=item FEATURES

Demonstrates usage of Prima::Themes, stores selected theme
in rc file. Other programs can use the selection by running

perl -MPrima::Themes program

=cut

my ( $w, $c, $playground, $t);

sub test
{
	$t-> destroy if $t;
	my @themes;	
	if (  $c-> count) {
		for ( 0 .. $c-> count - 1) { 
			push @themes, $c-> get_item_text($_) if $c-> button($_);
		}
	}
	Prima::Themes::select( @themes);
	my $failed = join(',', grep { ! Prima::Themes::active $_ } @themes);
	Prima::message("Theme(s) $failed failed to load") if length $failed;
	$t = $playground-> insert( TabbedScrollNotebook => 
		pack => { fill => 'both', expand => 1},
		tabs => ['Tab'],
	);

	$t-> insert( ScrollBar => 
		vertical => 0,
		pack => { side => 'bottom', fill => 'x', },
	);
	$t-> insert( ScrollBar => 
		vertical => 1,
		partial  => 50,
		pack => { side => 'right', fill => 'y', },
	);
	$t-> insert( ListBox => 
		pack => { side => 'right', fill => 'both', expand => 1, padx => 5, pady => 5},
		items => [ qw(1 2 3 4 5)],
		focusedItem => 1,
	);
	$t-> insert( Button => 
		pack => { side => 'top', anchor => 'w', padx => 5, pady => 5},
		text => 'Normal',
	);
	$t-> insert( Button => 
		pack => { side => 'top', anchor => 'w', padx => 5, pady => 5},
		text => 'Disabled',
		enabled => 0,
	);
	$t-> insert( Button => 
		pack => { side => 'top', anchor => 'w', padx => 5, pady => 5},
		text => 'Default',
		default => 1,
	);
	$t-> insert( Radio => 
		pack => { side => 'top', anchor => 'w', padx => 5, pady => 5},
		text => 'Radio',
	);
	$t-> insert( CheckBox => 
		pack => { side => 'top', anchor => 'w', padx => 5, pady => 5},
		text => 'CheckBox',
	);
	$t-> insert( InputLine => 
		pack => { side => 'bottom', anchor => 's', fill => 'x', expand => 1, padx => 5, pady => 5},
	);
}

$w = Prima::MainWindow-> create(
	text => 'Theme selector',
	menuItems => [
		[ 'Options' => [
			[ 'Save configuration' => sub { 
				Prima::message( Prima::Themes::save_rc() ? "Saved ok, restart to reload" : "Error saving:$!");
			} ],
		]],
	],
);

for (@INC) {
	next unless -d "$_/Prima/themes";
	next unless opendir D, "$_/Prima/themes";
	for ( readdir D) { 
		next unless m/^(.*)\.pm$/;
		eval "use Prima::themes::$1";
	}
	closedir D;
}

my @list = Prima::Themes::list;
$c = $w-> insert( Prima::CheckList => 
	pack => { side => 'left', fill => 'y', padx => 5, pady => 5},
	items => \@list,
	onChange => \&test,
	vector => '',
);

$playground = $w-> insert( Widget => 
	size => [ 250, 250],
	pack => { side => 'right', padx => 5, pady => 5, expand => 1, fill => 'both'},
	packPropagate => 0,
);

$w-> set( centered => 1);


# select active themes
my %list = map { $list[$_] => $_ } 0..$#list;
$c-> button( $list{$_}, 1) for Prima::Themes::list_active;

test();

run Prima;
