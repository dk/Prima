=pod

=head1 NAME

examples/menu.pl - A menu usage example

=head1 FEATURES

Demonstrates the usage of Prima menu API:

- one-call ( large array ) menu set
- text and image menu item manipulations

Note the "Edit/Kill menu" realisation.

=cut

use strict;
use warnings;
use Prima qw( InputLine Label Application);

package TestWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub create_images_menu
{
	my @ret;
	my $template = shift;

	my $sub = sub {
		my $img = $_[0]-> menu-> icon( $_[1]);
		my @r = @{$img-> palette};
		$img-> palette( [reverse @r]) if @r;
		$_[0]-> menu-> icon( $_[1], $img);
	};

	my $mono = $template->dup;
	$mono->conversion(ict::None);
	$mono->type(im::BW);
	push @ret, [ '1-bit image', $sub, { icon => $mono } ];
	push @ret, [ '-', '1-bit image disabled', $sub, { icon => $mono } ];

	my $mask1 = $template->dup;
	$mask1->set( color => cl::White, backColor => cl::Black, rop2 => rop::CopyPut );
	$mask1->map( $mask1->pixel(0,0) );
	$mask1->conversion(ict::None);
	$mask1->type(im::BW);

	my $mono2 = Prima::Icon->create_combined( $mono, $mask1);
	push @ret, [ '1-bit icon', $sub, { icon => $mono2 } ];
	push @ret, [ '-', '1-bit icon disabled', $sub, { icon => $mono2 } ];

	push @ret, [];
	push @ret, [ 'Color image', $sub, { icon => $template } ];
	push @ret, [ '-', 'Color image disabled', $sub, { icon => $template } ];
	my $color = Prima::Icon->create_combined( $template, $mask1);
	push @ret, [ 'Color icon', $sub, { icon => $color } ];
	push @ret, [ '-', 'Color icon disabled', $sub, { icon => $color } ];
	push @ret, [];

	my $mask8 = $template->dup;
	$mask8->type(im::Byte);
	$mask8->set( color => cl::Black, backColor => 0x808080, rop2 => rop::CopyPut );
	$mask8->map( 0x10101 * $mask8->pixel(0,0) );
	my $argb = Prima::Icon->create_combined( $template, $mask8);
	push @ret, [ 'ARGB icon', $sub, { icon => $argb } ];
	push @ret, [ '-', 'ARGB icon disabled', $sub, { icon => $argb } ];

	return @ret;
}

my $img = Prima::Image-> create;
$0 =~ /^(.*)(\\|\/)[^\\\/]+$/;
$img-> load(( $1 || '.') . '/Hand.gif');

#    Menu item must be an array with up to 6 items in -
# [variable, text or image, accelerator text, shortcut key, sub or command, data]
# see exact rules how these are parsed in L<"Prima::Menu" / "Menu items">.

sub create_menu
{
	my $img = Prima::Image-> create;
	$0 =~ /^(.*)(\\|\/)[^\\\/]+$/;
	$img-> load(( $1 || '.') . '/Hand.gif');
	return [
		[ "~File" => [
			[ "Anonymous" => "Ctrl+D" => '^d' => sub { print "sub!\n";}],   # anonymous sub
			[ '~Images' => [ create_images_menu($img) ]],
			[],                                       # division line
			[ "E~xit" => "Exit"    ]    # calling named function of menu owner
		]],
		[ ef => "~Edit" => [                  # example of system commands usage
			[ "Cop~y"  => sub { $_[0]-> foc_action('copy')}  ],     # try these with input line focused
			[ "Cu~t"   => sub { $_[0]-> foc_action('cut')}   ],
			[ "Pa~ste" => sub { $_[0]-> foc_action('paste')} ],
			[],
			["~Kill menu"=>sub{ $_[0]-> menuItems(
				[
					[ "~Restore all" => sub {
						$_[0]-> menuItems( $_[0]-> create_menu)
					}],
					[ "~Incline" => sub {
						$_[0]-> menu-> insert( $_[0]-> create_menu, '', 1);
					}],
				]);
			}],
			["~Duplicate menu"=>sub{ TestWindow-> create( menu=>$_[0]-> menu)}],
		]],
		[ "~Input line" => [
			[ "Print ~text" => "Text"],
			[ "Print ~selected" => "Selected"],
			[ "Try \"selText\"" => "SelText"],
			[],
			[ "Toggle insert mode" => "InsMode"],
			["Toggle password mode" => "PassMode"],
			["Toggle border existence" => "BorderMode"],
			[ coexistentor => "Coexistentor"=> ""],
		]],
		[],                             # divisor in main menu opens
		[ "~Clusters" => [              # right-adjacent part
		[ "*".checker =>  "Checking Item"   => "Check"     ],
		[],
		[ "-".slave   =>  "Disabled state"   => "PrintText"],
		[ master  =>  "~Enable item above" => "Enable"     ]   # enable/disable and text sample
		]]
	];
}

sub foc_action
{
	my ( $self, $action) = @_;
	my $foc = $self-> selectedWidget;
	return unless $foc and $foc-> alive;
	my $ref = $foc-> can( $action);
	$ref-> ( $foc) if $ref;
}

sub Exit
{
	$::application-> destroy;
}

sub Check
{
	my $menu = $_[ 0]-> menu;
	$menu-> checked( 'checker', ! $menu-> checked( 'checker'));
}

sub PrintText
{
	print $_[ 0]-> menu-> slave-> text;
}

sub Enable
{
	my $slave  = $_[0]-> menu-> slave;
	my $master = $_[0]-> menu-> master;
	if ( $slave-> enabled) {
		$slave -> text( "Disabled state");
		$master-> text( "~Enable item above");
	} else {
		$slave -> text( "Enabled state");
		$master-> text( "~Disable item above");
	}
	$slave-> enabled( ! $slave-> enabled);
}

sub Text
{
	print $_[ 0]-> InputLine1-> text;
}

sub Selected
{
	print $_[ 0]-> InputLine1-> selText;
}

sub SelText
{
	$_[ 0]-> InputLine1-> selText ("SEL");
}

sub InsMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> insertMode ( !$e-> insertMode);
}

sub PassMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> writeOnly ( !$e-> writeOnly);
}


sub BorderMode
{
	my $e = $_[ 0]-> InputLine1;
	$e-> borderWidth (( $e-> borderWidth == 1) ? 0 : 1);
}


package UserInit;

my $w = TestWindow-> create(
	text      => "Menu and input line example",
	bottom    => 300,
	size      => [ 360, 120],
	menuItems => TestWindow::create_menu,
	designScale => [ 7, 16 ],
	onDestroy => sub {$::application-> close},
);
$w-> insert( "InputLine",
	pack      => { pady => 20, padx => 20, fill => 'x', side => 'bottom'},
	text      => $w-> text,
	maxLen    => 200,
	onChange  => sub {
		$_[0]-> owner-> text( $_[0]-> text);
		$_[0]-> owner-> Label1-> text( $_[0]-> text);
		$_[0]-> owner-> menu-> coexistentor-> text( $_[0]-> text)
			if $_[0]-> owner-> menu-> has_item( 'coexistentor');
	},
);

$w-> insert( "Label",
	pack      => { pady => 20, padx => 20, fill => 'both', expand => 1},
	text   => "Type here something",
	backColor => cl::Green,
	valignment => ta::Center,
	focusLink => $w-> InputLine1,
);
run Prima;
