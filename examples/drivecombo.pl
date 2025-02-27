=pod

=head1 NAME

examples/drivecombo.pl - file tree widgets

=head1 FEATURES

Use of standard file-listbox and drive-combo box (the latter though is useless under
unix)

=cut

use strict;
use warnings;
use Prima::ComboBox;
use Prima::Dialog::FileDialog;

package UserInit;
$::application = Prima::Application-> new( name => "DriveCombo");

my $w = Prima::MainWindow-> new(
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

Prima->run;
