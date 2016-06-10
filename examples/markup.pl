use strict;
use warnings;
use Prima qw(Application Buttons Edit Notebooks Label DetailedList Outlines Drawable::Markup);

my $fp = [
	{ name   => 'Times New Roman', size => 10 },
	{ name   => 'Courier New',     size => 10 },
	{ direction => 4 },
];

my $enc = $::application->font_encodings;

my $esc = {
	char_at_c0 => [1,0xC0],
	sub => sub { 'retval' },
};

sub M($) { Prima::Drawable::Markup->new($_[0], fontPalette => $fp, encodings => $enc, escapes => $esc ) }

my $Main = Prima::MainWindow->create(
	name   => 'Main',
	text   => 'Markup test',
	size   => [500,500],
);

my $tn = $Main->insert('TabbedNotebook',
	pack   => { expand => 1, fill => 'both' },
	tabs   => [ M 'B<Basic Controls>', M 'I<Detailed List>', M 'U<Outline>', M 'F<2|Custom Paint>'],
);

$tn->insert_to_page(0,'Label',
	text   => M 'Some F<1|monospace text> in a label',
	pack   => { side => 'top', fill => 'x', anchor => 'w' },
);

$tn->insert_to_page(0,'Button',
	text   => M 'Some B<C<cl::LightRed|red text>> in a button',
	pack   => { side => 'top', anchor => 'w' },
);

$tn->insert_to_page(0,'Radio',
	text   => M 'Some S<+2|big text> in a radio button',
	pack   => { side => 'top' , anchor => 'w'},
);

$tn->insert_to_page(0,'CheckBox',
	text   => M 'Some S<-2|small text> in a checkbox',
	pack   => { side => 'top' , anchor => 'w'},
);

$tn->insert_to_page(0,'GroupBox',
	text   => M 'Some B<mixed> I<text> in a groupbox',
	pack   => { side => 'top', fill => 'x' },
);

$tn->insert_to_page(0,'ListBox',
	items  => [
		 M 'Some B<bold text>',
		 M 'Some I<italic text>',
		 M 'Some U<underlined text>',
		 M 'Some escapes: E<char_at_c0>, E<sub>',
		],
	pack   => { side => 'top', fill => 'x' },
);

$tn->insert_to_page(0,'Label',
	name   => 'EditTest',
	text   => M "Some different encodings in an Edit:
The character at 0xC0:
N<0|\xC0> ($enc->[0])
N<1|\xC0> ($enc->[1])
N<2|\xC0> ($enc->[2])
N<3|\xC0> ($enc->[3])",
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
	font => { size => 16, direction => 30 },
	pack   => { expand => 1, fill => 'both' },
	text   => M "B<Hello> C<cl::Green|world>!",
	onPaint => sub {
		my ($self, $canvas) = @_;
		$canvas->clear;
		my $t = $self-> text;
		my @box = @{ $canvas->get_text_box( $t ) };
		pop @box;
		pop @box;
		my ( $ox, $oy) = (20, 20);
		$box[$_] += $ox for 0,2,4,6; 
		$box[$_] += $oy for 1,3,5,7; 
		@box[4,5,6,7] = @box[6,7,4,5];
		$canvas-> color( cl::Yellow);
		$canvas-> fillpoly(\@box);
		$canvas-> color( cl::Black);
		$canvas->text_out( $t, $ox, $oy );
	},
);

run Prima;
