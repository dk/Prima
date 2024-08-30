=pod

=head1 NAME

examples/grid.pl - grid widget example

=head1 FEATURES

Demonstrates the usage of grid widgets - Prima::AbstractGrid and
its text-oriented descendant, Prima::Grid. To switch between
these, toggle the C<$abstract_grid> flag.

=cut


use strict;
use warnings;
use Prima;
use Prima::Application;
use Prima::Grids;


my $g;
my $w = Prima::MainWindow-> new(
	text => 'Grid example',
	packPropagate => 0,
	menuItems => [
		['~Options' => [
			['*dhg', 'Draw HGrid'=> sub { $g-> drawHGrid( $_[0]-> menu-> toggle( $_[1])) }],
			['*dvg', 'Draw VGrid'=> sub { $g-> drawVGrid( $_[0]-> menu-> toggle( $_[1])) }],
			['mse', 'Multi select'=> sub { $g-> multiSelect( $_[0]-> menu-> toggle( $_[1])) }],
			['*ind', 'Use indents' => sub { $g-> cellIndents(($_[0]-> menu-> toggle( $_[1])) x 4); }],
			['ccw', 'Constant cell width' => sub { $g-> constantCellWidth($_[0]-> menu-> toggle( $_[1]) ? 100 : undef); }],
			['cch', 'Constant cell height' => sub { $g-> constantCellHeight($_[0]-> menu-> toggle( $_[1]) ? 100 : undef); }],
		]
	]],
);

# change this to 0 and Prima::Grid will be created instead
my $abstract_grid = 1;

my @user_breadths=({},{});

$g = $w-> insert(
( $abstract_grid ? 'Prima::AbstractGrid' : 'Prima::Grid' ),
( $abstract_grid ? () : ('cells', [ map { my $a = $_; [ map {"$a.$_"} 0 .. 300]} 0 .. 300])),
	onMeasure => sub {
		my ( $self, $axis, $index, $ref) = @_;
		if ( defined $user_breadths[$axis]-> {$index} ) {
			$$ref = $user_breadths[$axis]-> {$index};
		} else {
			$$ref = ( 11 - ( $index % 11)) * 15;
			$$ref = 15 if $$ref < 15;
		}
	},
	onSetExtent => sub {
		my ( $self, $axis, $index, $breadth) = @_;
		$user_breadths[$axis]-> {$index} = $breadth;
	},
	onDrawCell => sub {
		my ( $self, $canvas,
				$col, $row, $type,
				$x1, $y1, $x2, $y2,
				$X1, $Y1, $X2, $Y2,
				$sel, $foc, $pre) = @_;

		my $bk = $sel ? $self-> hiliteBackColor :
						( $type ? $self-> indentCellBackColor : cl::Back);
		$bk = $self-> prelight_color($bk) if $pre;
		$canvas-> backColor( $bk );
		$canvas-> clear( $x1, $y1, $x2, $y2);
		$canvas-> color( $sel ? $self-> hiliteColor :
						( $type ? $self-> indentCellColor : cl::Fore));
		$canvas-> text_out( "$col.$row", $X2-50, $Y1+3);
		$canvas-> rect_focus( $x1, $y1, $x2, $y2 ) if $foc;
	},
	onGetRanges => sub {
		my ( $self, $axis, $index, $min, $max) = @_;
		$$min = 50;
	},
	onStringify => sub {
		my ( $self, $col, $row, $ref) = @_;
		$$ref = "$col.$row";
	},
	onGetAlignment => sub {
		my ( $self, $col, $row, $ha, $va) = @_;
		$$ha = ta::Center if $col == 0 || $row == 0 || $col == 9999 || $row == 9999;
	},
	allowChangeCellWidth => 1,
	allowChangeCellHeight => 1,
	clipCells => 2,
	pack => { expand => 1, fill => 'both' },
);
if ( $abstract_grid) {
	$g-> columns(10000);
	$g-> rows(10000);
}
$g-> cellIndents(1,1,1,1);

#$g-> insert_row( 0, 0..300 );
#$g-> insert_column( 100, 0..300 );
#$g-> delete_columns( 100, 1);
#$g-> add_column( 100, [0..300] );

run Prima;
