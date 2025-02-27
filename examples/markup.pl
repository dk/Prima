# XXX textoutbaseline=0 rotated
# XXX document that block is expected to run text on textOutBaseline(1)
# XXX document that block_wrap can workbreak only

=pod

=head1 NAME

examples/markup.pl - markup in widgets

=head1 FEATURES

Demonstrates use of Prima markup in all various contexts

=cut


use strict;
use warnings;
use Prima qw(Application Buttons Edit Notebooks Label DetailedList Outlines MsgBox);
use FindBin qw($Bin);
use Prima::Drawable::Markup q(M);

@Prima::Drawable::Markup::FONTS = (
	{ name   => 'Times New Roman' },
	{ name   => 'Courier New'     },
	{ direction => 4 },
);

@Prima::Drawable::Markup::IMAGES = (
    Prima::Icon->load("$Bin/Hand.gif")
);

my $Main = Prima::MainWindow->new(
	name   => 'Main',
	text   => 'Markup test',
	size   => [500,500],
	designScale => [7, 16],
);

my $tn = $Main->insert('TabbedNotebook',
	pack   => { expand => 1, fill => 'both' },
	tabs   => [ M 'G<White|Basic> Controls', M 'F<0|I<Detailed List>>', M 'U<Outline>', M 'F<2|Rotated> & Bidi>'],
);

$tn->insert_to_page(0,'Label',
	text   => \ "\x{5e9} Some L<tip://$0/tip|F<1|U<m>onospace text>> in a L<pod://Prima::Label/SYNOPSIS|label>",
	autoHeight => 1,
	hotKey => 'm',
	backColor => cl::Yellow,
	wordWrap => 1,
	focusLink => 'List',
	pack   => { side => 'top', fill => 'x', anchor => 'w' },
);

$tn->insert_to_page(0,'Button',
	text   => \ 'Some B<C<LightRed|U<r>ed text>> in a button',
	pack   => { side => 'top', anchor => 'w' },
	hotKey => 'r',
	onClick => sub { message(\ "Hello! This is the B<msgbox> speaking!") },
	hint    => \ "Hints can I<also> be markupified",
);

$tn->insert_to_page(0,'Radio',
	text   => \ 'P<0>Some S<+2|U<b>ig text> in a radio button',
	pack   => { side => 'top' , anchor => 'w'},
	hotKey => 'b',
);

$tn->insert_to_page(0,'CheckBox',
	text   => \ 'P<0>Some S<-2|U<s>mall text> in a checkbox',
	pack   => { side => 'top' , anchor => 'w'},
	hotKey => 's',
);

$tn->insert_to_page(0,'GroupBox',
	text   => \ 'Some B<mixed> I<text> in a groupbox',
	pack   => { side => 'top', fill => 'x' },
);

$tn->insert_to_page(0,'ListBox',
	name   => 'List',
	focusedItem => 0,
	items  => [
		 M 'Some B<bold text>',
		 M 'Some I<italic text>',
		 M 'Some U<underlined text>',
		 M 'Some M<,0.4,m>S<-2|superscript>M<,-0.4,m> and S<-2|subscript>',
		],
	pack   => { side => 'top', fill => 'x' },
);

$tn->insert_to_page(0,'Label',
	wordWrap => 1,
	pack   => { side => 'top', fill => 'both', expand => 1 },
)-> text( \ "Wrappable text: B<bold
W<non-wrappable bold C<Green|and green>>,
but still bold> text
");

$tn->insert_to_page(1,'DetailedList',
	items  => [
		[ M 'Some B<bold text>',  M 'Some I<italic text>',  M 'Some U<underlined text>' ],
		[ M 'Some S<+2|big text>',  M 'Some S<-2|small text>',  M 'Some F<1|monospace text>' ],
		],
	columns => 3,
	headers => [ M 'B<Works>',  M 'in I<headers>',  M 'U<too>'],
	pack   => { expand => 1, fill => 'both' },
);

$tn->insert_to_page(2,'StringOutline',
	items  => [
		[M 'Some B<bold text>', [
			[M 'Some I<italic text>'],
			[M 'Some U<underlined text>'],
			]],
		 [M 'Some S<+2|big text>', [
			[M 'Some S<-2|small text>', [
				[M 'Some F<1|monospace text>' ],
				]],
			]],
		],
	pack   => { expand => 1, fill => 'both' },
);

$tn->insert_to_page(3,'Widget',
	font => { size => 16, direction => 30, name => 'Arial' },
	pack   => { expand => 1, fill => 'both' },
	text   => \ "G<Yellow|B<I<\x{5E9}\x{5DC}\x{5D5}\x{5DD}> C<Green|world>>!>",
	onPaint => sub {
		my ($self, $canvas) = @_;
		$canvas->clear;
		my ( $ox, $oy) = (20, 20);
		$canvas->text_out( $self->text, $ox, $oy );
		$canvas->color(cl::LightRed);
		$canvas->fill_ellipse( $ox, $oy, 5, 5 );
	},
);

Prima->run;

=pod

=head1 tip

This is a tooltip!

=cut
