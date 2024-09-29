=pod

=head1 NAME

examples/listbox.pl - listbox, combobox, and edit widgets

=head1 FEATURES

Demonstrates the usage of Prima::Edit, Prima::ComboBox,
and Prima::ListBox widgets.

=cut

use strict;
use warnings;
use Prima qw( ComboBox Edit Application );


package TestWindow;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);

sub create_menu
{
	return [
		[ "~ListBox" => [
			["~Add text" => "AddItem"],
			["~Delete current" => sub{$_[0]-> ListBox1-> delete_items( $_[0]-> ListBox1-> focusedItem);}],
			["Delete a~ll" => sub{$_[0]-> ListBox1-> delete_items(0..$_[0]-> ListBox1-> count )}],
			[],
			["~Print all" => "PrintAll"],
			["Print ~selected" => sub{foreach (@{$_[0]-> ListBox1-> selectedItems}){print "$_\n"};}],
			["Print ~focused" => sub{ print $_[0]-> ListBox1-> focusedItem."\n";}],
			[],
			["Toggle ~extended selection"=> sub{$_[0]-> ListBox1-> extendedSelect(!$_[0]-> ListBox1-> extendedSelect)}],
			["Toggle ~multiple selection"=> sub{$_[0]-> ListBox1-> multiSelect(!$_[0]-> ListBox1-> multiSelect)}],
			["~Increase item height"=>sub{$_[0]-> ListBox1-> itemHeight($_[0]-> ListBox1-> itemHeight+2)}],
			["~Decrease item height"=>sub{$_[0]-> ListBox1-> itemHeight($_[0]-> ListBox1-> itemHeight-2)}],
			[],
			['Ali~gn' => [
				['~Left'   => sub { shift->ListBox1->align(ta::Left)   }],
				['~Right'  => sub { shift->ListBox1->align(ta::Right)  }],
				['~Center' => sub { shift->ListBox1->align(ta::Center) }],
			]],
			[],
			["Add~itional"=> sub {
				my $box  = $_[0]-> ListBox1;
				$box-> add_items( 'Hello', 'user', 'from', 'Perl');
			}]
		]],
	[
		"~Edit" => [
			["~VScroll" => sub{$_[0]-> Edit1-> vScroll(!$_[0]-> Edit1-> vScroll)}],
			["~HScroll" => sub{$_[0]-> Edit1-> hScroll(!$_[0]-> Edit1-> hScroll)}],
			["B~oth" => sub{
				$_[0]-> Edit1-> set( hScroll => !$_[0]-> Edit1-> hScroll,
										vScroll => !$_[0]-> Edit1-> vScroll)
			}],
			["~Border" => sub{$_[0]-> Edit1-> borderWidth(!$_[0]-> Edit1-> borderWidth)}],
	]],
	[ "~ComboBox" => [
			["~Add text" => "AddItemC"],
			["Delete a~ll" => sub{$_[0]-> ComboBox1-> List-> delete_items( 0..$_[0]-> ComboBox1-> List-> count) }],
			["Print ~text" => sub{ print $_[0]-> ComboBox1-> text."\n";}],
			[],
			["~Set style" => [
				[ "~Simple" => sub {$_[0]-> ComboBox1-> style(cs::Simple)}],
				[ "~Drop down" => sub {$_[0]-> ComboBox1-> style(cs::DropDown)}],
				[ "Drop down ~list" => sub {$_[0]-> ComboBox1-> style(cs::DropDownList)}],
			]],
			[],
			["Add~itional"=> sub{
				my $box  = $_[0]-> ComboBox1-> List;
				$box-> add_items( 'Hello', 'user', 'from', 'Perl');
			}]
		]],
	];
}


sub AddItem
{
	my $self = shift;
	$self-> ListBox1-> add_items( $self-> InputLine1-> text);
}

sub AddItemC
{
	my $self = shift;
	$self-> ComboBox1-> List-> add_items( $self-> InputLine1-> text);
}


sub PrintAll
{
	my $self = shift;
	print( "$_\n") for @{$self-> ListBox1-> items};
}


my $w = TestWindow-> new(
	name    =>  "Window1",
	origin  => [ 100, 100],
	size    => [ 600, 230],
	designScale => [ 7, 16 ],
	text => "List & edit boxes example",
	menuItems => TestWindow::create_menu,
);

$w-> insert("InputLine", pack => {side => 'bottom', fill => 'x', padx => 20, pady => 20 });

$w-> insert( "ListBox",
	hScroll        => 1,
	multiSelect    => 0,
	extendedSelect => 1,
	draggable       => 1,
	name            => 'ListBox1',
	font            => { size => 24},
	items           => ['Items', 'created', 'indirect'],
	pack            => { side => 'left', expand => 1, fill => 'both', padx => 20, pady => 20},
	align => ta::Right,
);
$w-> insert( "Edit",
	maxLen         => 200,
	name           => 'Edit1',
	hScroll        => 1,
	vScroll        => 1,
	wantReturns    => 0,
	pack            => { side => 'left', expand => 1, fill => 'both', padx => 20, pady => 20},
);
$w-> insert( "ComboBox",
	name           => 'ComboBox1',
	items          => ['Combo', 'box', 'salutes', 'you!'],
	pack            => { side => 'left', expand => 1, fill => 'both', padx => 20, pady => 20},
);

run Prima;
