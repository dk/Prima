# XXX textoutbaseline=0 rotated
# XXX document that block is expected to run text on textOutBaseline(1)
# XXX document that block_wrap can workbreak only
# XXX geomHeight based on BLK_HEIGHT
# XXX tb:: export
use strict;
use warnings;
use Prima qw(Application Buttons Edit Notebooks Label DetailedList Outlines Drawable::Markup);

my $fp = [
	{ name   => 'Times New Roman', size => 10 },
	{ name   => 'Courier New',     size => 10 },
	{ direction => 4 },
];

sub M($) { Prima::Drawable::Markup->new(markup => $_[0], fontPalette => $fp ) }

my $Main = Prima::MainWindow->create(
	name   => 'Main',
	text   => 'Markup test',
	size   => [500,500],
);

my $tn = $Main->insert('TabbedNotebook',
	pack   => { expand => 1, fill => 'both' },
	tabs   => [ M 'Q<White|Basic> Controls', M 'I<Detailed List>', M 'U<Outline>', M 'F<2|Rotated> & Bidi>'],
);

$tn->insert_to_page(0,'Label',
	text   => M "\x{5e9} Some F<1|U<m>onospace text> in a label",
	autoHeight => 1,
	hotKey => 'm',
	backColor => cl::Yellow,
	wordWrap => 1,
	focusLink => 'List',
	pack   => { side => 'top', fill => 'x', anchor => 'w' },
);

$tn->insert_to_page(0,'Button',
	text   => M 'Some B<C<LightRed|U<r>ed text>> in a button',
	pack   => { side => 'top', anchor => 'w' },
	hotKey => 'r',
	onClick => sub { Prima::message("Hello!") },
);

$tn->insert_to_page(0,'Radio',
	text   => M 'Some S<+2|U<b>ig text> in a radio button',
	pack   => { side => 'top' , anchor => 'w'},
	hotKey => 'b',
);

$tn->insert_to_page(0,'CheckBox',
	text   => M 'Some S<-2|U<s>mall text> in a checkbox',
	pack   => { side => 'top' , anchor => 'w'},
	hotKey => 's',
);

$tn->insert_to_page(0,'GroupBox',
	text   => M 'Some B<mixed> I<text> in a groupbox',
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
	text   => M "Wrappable text: B<bold
W<non-wrappable bold C<Green|and green>>,
but still bold> text
",	
	pack   => { side => 'top', fill => 'both', expand => 1 },
);

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
	text   => M "Q<Yellow|B<I<\x{5E9}\x{5DC}\x{5D5}\x{5DD}> C<Green|world>>!>",
	onPaint => sub {
		my ($self, $canvas) = @_;
		$canvas->clear;
		my ( $ox, $oy) = (20, 20);
		$canvas->text_out( $self->text, $ox, $oy );
		$canvas->color(cl::LightRed);
		$canvas->fill_ellipse( $ox, $oy, 5, 5 );
	},
);

run Prima;
