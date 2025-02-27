=pod

=head1 NAME

examples/menu.pl - menu example

=head1 FEATURES

Demonstrates the usage of the menu API:

=over

=item one-call ( large array ) menu set

=item text and image menu item manipulations

=item menu images on a layered window

=back

Check out the implementation of "Edit/Kill menu"

=cut

use strict;
use warnings;
use Prima qw( InputLine Label Application StdBitmap);
use Prima::Menus;

package TestWindow;
use vars qw(@ISA);
@ISA = qw(Prima::MainWindow);
use Prima::Drawable::Markup q(M);

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

	push @ret, [ $template, $sub ], [];

	my $mono = $template->dup;
	$mono->conversion(ict::None);
	$mono->type(im::BW);

	my $monox = $mono->clone(type => 1);
	$monox->colormap(cl::Yellow, cl::Blue);
	push @ret, [ '1-bit image', $sub, { icon => $monox } ];
	push @ret, [ '-', '1-bit image disabled', $sub, { icon => $monox } ];

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
	$color->maskColor($color->pixel(0,0));
	$color->autoMasking(am::MaskColor);
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

sub create_custom_menu
{
	my @icons = map { Prima::StdBitmap::image($_) } sbmp::CheckBoxUnchecked, sbmp::CheckBoxChecked;
	return [ '@?' => "~Custom" => sub { print "Custom\n" } => {
		onMeasure => sub {
			my ( $self, $menu, $ref) = @_;
			my ($w, $h) = ( $self->owner->get_text_width( $menu-> text, 1 ), $self->owner->popupFont->height );
			my $i = $icons[ $menu->checked ];
			my $isz = $menu-> check_icon_size;
			my $dx = ( $isz > $i-> width ) ? $isz : $i-> width;
			@$ref = ($w + 20 + $dx, $h + 20);
		},
		onPaint => sub {
			my ( $self, $menu, $canvas, $selected, $x1, $y1, $x2, $y2) = @_;
			my @p = ( cl::Black, cl::White );
			@p = reverse @p if $selected;
			$canvas-> new_gradient(palette => \@p)->bar($x1, $y1, $x2, $y2, 1);
			$canvas-> font( $self-> owner->popupFont );
			$canvas-> color(cl::Yellow);

			my $i = $icons[ $menu->checked ];
			my $isz = $menu-> check_icon_size;
			my $dx = ( $isz > $i-> width ) ? $isz : $i-> width;
			$canvas-> draw_text( $menu->text, $x1 + 2 + $dx, $y1, $x2, $y2, dt::VCenter|dt::DrawMnemonic);
			$canvas-> put_image(
				$x1 + (( $isz > $i-> width ) ? ( $menu-> check_icon_size - $i-> width) / 2 : 0),
				($y2 + $y1 - $i->height) / 2, $i);
		},
		hint    => M 'Hints can be B<bold> and I<italic>',
	} ];
}

my $img = Prima::Image-> new;
$0 =~ /^(.*)(\\|\/)[^\\\/]+$/;
$img-> load(( $1 || '.') . '/Hand.gif');

sub test_toplevels
{
	my @img = map { ((ref($$_[-1]) // '') eq 'HASH') ? $$_[-1]->{icon} : () } create_images_menu($img);
	my $i;
	my @menu;
	push @menu, [ '@?', 'Layered', sub {$_[0]->layered( $_[2] ); },  create_custom_menu->[-1] ];

	for ( $i = 0; $i < @img; $i+=2) {
		push @menu, [ "\@t1-$i", $img[$i], sub {
			my $id = $_[1];
			$id =~ s/1/2/;
			my $m = Prima::MenuItem->new( $_[0]->menu, $id);
			$m->enabled(!$m->enabled);
		}];
		push @menu, [ "-\@t2-$i", $img[$i], sub {}];
	};
	Prima::Window->new(
		menuItems => \@menu,
		size => [ 600, $::application->font->height ],
		text => 'Toplevel images',
	);
}

#    Menu item must be an array with up to 6 items in -
# [variable, text or image, accelerator text, shortcut key, sub or command, data]
# see exact rules how these are parsed in L<"Prima::Menu" / "Menu items">.

sub create_menu
{
	return [
		[ "~Window" => [
			[ "Anonymous" => "Ctrl+D" => '^d' => sub { print "sub!\n";}],   # anonymous sub
			[ '~Images' => [ create_images_menu($img) ]],
			create_custom_menu,
			[],                                       # division line
			[ 'Test toplevels' => 'test_toplevels' ],
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
			["~Duplicate menu"=>sub{ TestWindow-> new( menu=>$_[0]-> menu)}],
		]],
		[ il => "~Input line" => [
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
		[ cl => "~Clusters" => [              # right-adjacent part
		[ "*".checker =>  "Checking Item"   => "Check"     ],
		[ "@" =>  "Auto Checking Item"   => sub {print "new state: $_[2]\n" } ],
		[],
		[ '*(' => 'one' => sub {} ],
		[ '' => 'two' => sub {} ],
		[ ')' => 'three' => sub {} ],
		[],
		[ "-@".slave   =>  "Disabled state"   => "PrintText"],
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

sub on_menuselect
{
	my ( $self, $menu, $item ) = @_;
	$self->Hint->text( $menu->hint($item) // '' );
}

sub on_menuenter
{
	my ( $self ) = @_;
	$self->Hint->{saved} = $self->Hint->text;
}

sub on_menuleave
{
	my ( $self ) = @_;
	$self->Hint->text( delete $self->Hint->{saved} );
}

package UserInit;

my $w = TestWindow-> new(
	text      => "Menu and input line example",
	size      => [ 360, 160],
	menuItems => TestWindow::create_menu,
	designScale => [ 7, 16 ],
);
$w->menu->ef->hint("This is menu containing editing commands");
$w->menu->il->hint("Do stuff with the input line");
$w->menu->cl->hint("Various selectors");

$w-> insert( "Prima::Menu::Bar",
	pack  => { pady => 20, padx => 20, fill => 'x', expand => 1},
	menu  => $w->menu,
	onMenuEnter => sub {
		$w->Hint->{saved} //= $w->Hint->text;
	},
	onMenuLeave => sub {
		$w->Hint->text( delete $w->Hint->{saved} );
	},
	onMenuSelect => sub {
		my ( $self, $item ) = @_;
		$w->Hint->text( $self->menu->hint($item) // '' );
	},
);
$w-> insert( "InputLine",
	pack      => { pady => 20, padx => 20, fill => 'x', side => 'bottom'},
	text      => $w-> text,
	maxLen    => 200,
	onChange  => sub {
		$_[0]-> owner-> text( $_[0]-> text);
		$_[0]-> owner-> Hint-> text( $_[0]-> text);
		$_[0]-> owner-> menu-> coexistentor-> text( $_[0]-> text)
			if $_[0]-> owner-> menu-> has_item( 'coexistentor');
	},
);

$w-> insert( "Label",
	name      => 'Hint',
	pack      => { pady => 20, padx => 20, fill => 'both', expand => 1},
	text   => "Type here something",
	backColor => cl::Green,
	valignment => ta::Center,
	focusLink => $w-> InputLine1,
);
$w-> popupItems( $w-> menuItems);

Prima->run;
