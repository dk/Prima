=pod

=head1 NAME

examples/gt_grid.pl - Demonstrate the grid geometry manager

=head1 FEATURES

Demonstrates some features of the L<Prima::Widget::grid> geometry manager.

=cut
use 5.014;

use strict;
use warnings;
use Prima qw( Application Label InputLine Sliders );
use Prima::Drawable::Markup;

my $w = Prima::MainWindow-> new(
	text      => 'Grid geometry manager example',
	backColor => cl::Gray,
	visible => 0,
);

my ($ipadx,$ipady,$padx,$pady) = (0,0,1,1);
my $text =  <<END =~ s/\n+/ /gr;
This geometry manager arranges widgets in rows and columns.  Cells can
span more than one row or column.
See L<pod://Prima::Widget::grid|Prima::Widget::grid> for its documentation.
END
$text = Prima::Drawable::Markup->new(markup => $text);

my @rows = (
	[
		$w->insert(
			Label =>
			text => "The Prima Grid Geometry Manager",
			backColor => 0xC0C0C0,
		)
	],
	[
		$w->insert(
			Label =>
			text => 'ipadx',
			backColor => cl::White,
		),
		$w->insert(
			InputLine =>
			text => $ipadx,
			onChange => sub { set_ipadx($_[0]->text || 0) },
			backColor => cl::White,
		),
	],
	[
		$w->insert(
			Label =>
			text => 'ipady',
			backColor => cl::White,
		),
		$w->insert(
			CircularSlider =>
			min => 0, max => 10,
			autoTrack => 0,
			onChange => sub { set_ipady($_[0]->value) },
			backColor => cl::White,
		),
	],
	[
		$w->insert(
			Label =>
			text => 'padx',
			backColor => cl::White,
		),
		$w->insert(
			Slider =>
			min => 0, max => 10,
			value => 1,
			onChange => sub { set_padx($_[0]->value) },
			backColor => cl::White,
		),
	],
	[
		$w->insert(
			Label =>
			text => 'pady',
			backColor => cl::White,
		),
		$w->insert(
			InputLine =>
			text => $pady,
			onChange => sub { set_pady($_[0]->text || 0) },
			backColor => cl::White,
		),
		$w->insert(
			Label =>
			text => $text,
			backColor => cl::White,
			autoHeight => 1,
			wordWrap => 1,
			width => 180,
		),
	],
);

$w->gridConfigure($rows[0][0], row => 4, column => 0, colspan => 3, sticky => 'we',);
$w->gridConfigure($rows[1][0], row => 3, column => 0, sticky => 'nse',);
$w->gridConfigure($rows[1][1], row => 3, column => 1,);
$w->gridConfigure($rows[2][0], row => 2, column => 0, sticky => 'e',);
$w->gridConfigure($rows[2][1], row => 2, column => 1,);
$w->gridConfigure($rows[3][0], row => 1, column => 0, sticky => 'e',);
$w->gridConfigure($rows[3][1], row => 1, column => 1,);
$w->gridConfigure($rows[4][0], row => 0, column => 0, sticky => 'e',);
$w->gridConfigure($rows[4][1], row => 0, column => 1,);
$w->gridConfigure($rows[4][2], row => 0, column => 2, rowspan => 4, sticky => 'ns');
reconfigure();

sub set_ipadx { $ipadx = shift; reconfigure() };
sub set_ipady { $ipady = shift; reconfigure() };
sub set_padx  { $padx  = shift; reconfigure() };
sub set_pady  { $pady  = shift; reconfigure() };

sub reconfigure {
	$w->gridLock;
	for my $row (@rows) {
		for my $cell (@$row) {
			$cell->gridConfigure(
				ipadx => $ipadx,  ipady => $ipady,
				padx  => $padx,   pady  => $pady,
			);
		}
	}
	$w->gridUnlock;
}

$w->show;
Prima->run;
