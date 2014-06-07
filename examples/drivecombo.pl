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

examples/drivecombo.pl - File tree widgets

=head1 FEATURES

Use of standard file-listbox and drive-combo box ( the latter
is idle under *nix )

=cut

use strict;
use Prima::ComboBox;
use Prima::FileDialog;

package UserInit;
$::application = Prima::Application-> create( name => "DriveCombo");

my $w = Prima::MainWindow-> create(
	text   => "Combo box",
	left   => 100,
	bottom => 300,
	width  => 250,
	height => 250,
);

$w-> insert( DriveComboBox =>
	pack => { side => 'bottom', padx => 20, pady => 20, fill => 'x' },
	onChange => sub { $w-> DirectoryListBox1-> path( $_[0]-> text); },
);

$w-> insert( DirectoryListBox =>
	pack => { side => 'bottom', padx => 20, pady => 20, fill => 'both', expand => 1, },
	onChange => sub { print $_[0]-> path."\n"},
);

run Prima;
