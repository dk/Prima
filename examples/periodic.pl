=pod

=head1 NAME

examples/periodic.pl - a periodic table of elements

=head1 FEATURES

Demonstrates usage of the grid widget

=cut

use strict;
use warnings;
use Prima qw(Application Grids);

my $scaling = $::application->uiScaling;

my $w = Prima::MainWindow-> new(
	text => "Periodic table of elements",
	size => [ map { $scaling * $_ } 45 * 14 + 5, 45 * 14 + 5],
);

my @layers = ( 2, 8, 8, 10, 9, 10, 9, 10, 9, 10, 10, 0, 14, 14);
my %colors = (
	2 => {
		0 => cl::Blue,
		10 => cl::Red,
	},
	8 => {
		0 => cl::Blue,
		1 => cl::Blue,
		10 => cl::Red,
		default => 0x804000
	},
	9 => {
		0 => cl::Black,
		1 => cl::Black,
		10 => cl::Red,
		default => 0x804000
	},
	10 => {
		0 => cl::Blue,
		1 => cl::Blue,
		default => cl::Black,
	},
	14 => {
		default => cl::Green,
	}
);
my %sides = (
	'9:0' => 4, '9:1' => 4, '9:2' => 4,
	'7:4' => 8, '8:4' => 8, '9:4' => 12,
	'7:6' => 8, '8:6' => 8, '9:6' => 12,
	'7:8' => 8, '8:8' => 8, '9:8' => 12,
	'7:10' => 8, '8:10' => 8, '9:10' => 12,
	'10:3' => 8, '10:5' => 8, '10:7' => 8, '10:9' => 8, '10:11' => 8,
	'0:11' => 8, '1:11' => 8, '2:11' => 8, '3:11' => 8, '4:11' => 8,
	'5:11' => 8, '6:11' => 8, 
	#  '11:4' => 8, '12:4' => 8, '13:4' => 12,
	#  '10:5' => 12, '13:5' => 4, '13:6' => 4,
	#  '11:7' => 8, '12:7' => 8, '13:7' => 8,
);

package Periodic;
use vars qw(@ISA);
@ISA = qw(Prima::GridViewer);

my %elem_info = (
	H  => { atomic_number => 1, name => "Hydrogen", weight => "1.00794(7)", },
	He => { atomic_number => 2, name => "Helium", weight => "4.002602(2)", },

	Li => { atomic_number => 3, name => "Lithium", weight => "6.941(2)", },
	Be => { atomic_number => 4, name => "Beryllium", weight => "9.012182(3)", },
	B  => { atomic_number => 5, name => "Boron", weight => "10.881(7)", },
	C  => { atomic_number => 6, name => "Carbon", weight => "12.0107(8)", },
	N  => { atomic_number => 7, name => "Nitrogen", weight => "14.00674(7)", },
	O  => { atomic_number => 8, name => "Oxygen", weight => "15.9994(3)", },
	F  => { atomic_number => 9, name => "Fluorine", weight => "18.9984032(5)", },
	Ne => { atomic_number => 10, name => "Neon", weight => "20.1797(6)", },

	Na => { atomic_number => 11, name => "Sodium", weight => "22.989770(2)", },
	Mg => { atomic_number => 12, name => "Magnesium", weight => "24.3050(6)", },
	Al => { atomic_number => 13, name => "Aluminium", weight => "26.981538(2)", },
	Si => { atomic_number => 14, name => "Silicon", weight => "28.0855(3)", },
	P  => { atomic_number => 15, name => "Phosphorus", weight => "30.973761(2)", },
	S  => { atomic_number => 16, name => "Sulfur", weight => "32.066(6)", },
	Cl => { atomic_number => 17, name => "Chlorine", weight => "35.4527(9)", },
	Ar => { atomic_number => 18, name => "Argon", weight => "39.948(1)", },
	K  => { atomic_number => 19, name => "Potassium", weight => "39.0983(1)", },
	Ca => { atomic_number => 20, name => "Calcium", weight => "40.078(4)", },
	Sc => { atomic_number => 21, name => "Scandium", weight => "44.955910(8)", },
	Ti => { atomic_number => 22, name => "Titanium", weight => "47.867(1)", },
	V  => { atomic_number => 23, name => "Vanadium", weight => "50.9415(1)", },
	Cr => { atomic_number => 24, name => "Chromium", weight => "51.9961(6)", },
	Mn => { atomic_number => 25, name => "Manganese", weight => "54.938049(9)", },
	Fe => { atomic_number => 26, name => "Iron", weight => "55.845(2)", },
	Co => { atomic_number => 27, name => "Cobalt", weight => "58.933200(9)", },
	Ni => { atomic_number => 28, name => "Nickel", weight => "58.6934(2)", },
	Cu => { atomic_number => 29, name => "Copper", weight => "63.546(3)", },
	Zn => { atomic_number => 30, name => "Zinc", weight => "65.39(2)", },
	Ga => { atomic_number => 31, name => "Gallium", weight => "69.723(1)", },
	Ge => { atomic_number => 32, name => "Germanium", weight => "72.61(2)", },
	As => { atomic_number => 33, name => "Arsenic", weight => "74.92160(2)", },
	Se => { atomic_number => 34, name => "Selenium", weight => "78.96(3)", },
	Br => { atomic_number => 35, name => "Bromine", weight => "79.904(1)", },
	Kr => { atomic_number => 36, name => "Krypton", weight => "83.80(1)", },

	Rb => { atomic_number => 37, name => "Rubidium", weight => "85.4678(3)", },
	Sr => { atomic_number => 38, name => "Strontium", weight => "87.62(1)", },
	Y  => { atomic_number => 39, name => "Yttrium", weight => "88.90585(2)", },
	Zr => { atomic_number => 40, name => "Zirconium", weight => "91.224(2)", },
	Nb => { atomic_number => 41, name => "Niobium", weight => "92.90638(2)", },
	Mo => { atomic_number => 42, name => "Molybdenum", weight => "95.94(1)", },
	Tc => { atomic_number => 43, name => "Technetium", weight => "[97.9072]", },
	Ru => { atomic_number => 44, name => "Ruthenium", weight => "101.07(2)", },
	Rh => { atomic_number => 45, name => "Rhodium", weight => "102.90550(2)", },
	Pd => { atomic_number => 46, name => "Palladium", weight => "106.42(1)", },
	Ag => { atomic_number => 47, name => "Silver", weight => "107.8682(2)", },
	Cd => { atomic_number => 48, name => "Cadmium", weight => "112.411(8)", },
	In => { atomic_number => 49, name => "Indium", weight => "114.818(3)", },
	Sn => { atomic_number => 50, name => "Tin", weight => "118.710(7)", },
	Sb => { atomic_number => 51, name => "Antimony", weight => "121.760(1)", },
	Te => { atomic_number => 52, name => "Tellurium", weight => "127.60(3)", },
	I  => { atomic_number => 53, name => "Iodine", weight => "126.90447(3)", },
	Xe => { atomic_number => 54, name => "Xenon", weight => "131.29(2)", },

	Cs => { atomic_number => 55, name => "Caesium", weight => "132.90545(2)", },
	Ba => { atomic_number => 56, name => "Barium", weight => "137.327(7)", },
	La => { atomic_number => 57, name => "Lanthanum", weight => "138.9055(2)", },

	Hf => { atomic_number => 72, name => "Hafnium", weight => "178.49(2)", },
	Ta => { atomic_number => 73, name => "Tantalum", weight => "180.9479(1)", },
	W  => { atomic_number => 74, name => "Tungsten", weight => "183.84(1)", },
	Re => { atomic_number => 75, name => "Rhenium", weight => "186.207(1)", },
	Os => { atomic_number => 76, name => "Osmium", weight => "190.23(3)", },
	Ir => { atomic_number => 77, name => "Iridium", weight => "192.217(3)", },
	Pt => { atomic_number => 78, name => "Platinum", weight => "195.078(2)", },
	Au => { atomic_number => 79, name => "Gold", weight => "196.96655(2)", },
	Hg => { atomic_number => 80, name => "Mercury", weight => "200.59(2)", },
	Tl => { atomic_number => 81, name => "Thallium", weight => "204.3383(2)", },
	Pb => { atomic_number => 82, name => "Lead", weight => "207.2(1)", },
	Bi => { atomic_number => 83, name => "Bismuth", weight => "208.98038(2)", },
	Po => { atomic_number => 84, name => "Polonium", weight => "[208.9824]", },
	At => { atomic_number => 85, name => "Astatine", weight => "[209.9871]", },
	Rn => { atomic_number => 86, name => "Radon", weight => "[222.0176]", },

	Fr => { atomic_number => 87, name => "Francium", weight => "[223.0197]", },
	Ra => { atomic_number => 88, name => "Radium", weight => "[226.0254]", },
	Ac => { atomic_number => 89, name => "Actinium", weight => "[227.0277]", },

	Rf => { atomic_number => 104, name => "Rutherfordium", weight => "[263.1125]", },
	Db => { atomic_number => 105, name => "Dubnium", weight => "[262.1144]", },
	Sg => { atomic_number => 106, name => "Seaborgium", weight => "[266.1219]", },
	Bh => { atomic_number => 107, name => "Bohrium", weight => "[264.1247]", },
	Hs => { atomic_number => 108, name => "Hassium", weight => "[269.1341]", },
	Mt => { atomic_number => 109, name => "Meitnerium", weight => "[268.1388]", },
	Ds => { atomic_number => 110, name => "Darmstadtium", weight => "[272.1463]", },
	Rg => { atomic_number => 111, name => "Roentgenium", weight => "[282]", },
	Cn => { atomic_number => 112, name => "Copenicium", weight => "[285]", },
	Nh => { atomic_number => 113, name => "Nihonium", weight => "[286]", },
	Fl => { atomic_number => 114, name => "Flerovium", weight => "[289]", },
	Mc => { atomic_number => 115, name => "Moscovium", weight => "[290]", },
	Lv => { atomic_number => 116, name => "Livermorium", weight => "[293]", },
	Ts => { atomic_number => 117, name => "Tennessine", weight => "[294]", },
	Og => { atomic_number => 118, name => "Oganesson", weight => "[294]", },

	Ce => { atomic_number => 58, name => "Cerium", weight => "140.116(1)", },
	Pr => { atomic_number => 59, name => "Praseodymium", weight => "140.90765(2)", },
	Nd => { atomic_number => 60, name => "Neodymium", weight => "144.24(3)", },
	Pm => { atomic_number => 61, name => "Promethium", weight => "[144.9127]", },
	Sm => { atomic_number => 62, name => "Samarium", weight => "150.36(3)", },
	Eu => { atomic_number => 63, name => "Europium", weight => "151.964(1)", },
	Gd => { atomic_number => 64, name => "Gadolinium", weight => "157.25(3)", },
	Tb => { atomic_number => 65, name => "Terbium", weight => "158.92534(2)", },
	Dy => { atomic_number => 66, name => "Dysprosium", weight => "162.50(3)", },
	Ho => { atomic_number => 67, name => "Holmium", weight => "164.93032(2)", },
	Er => { atomic_number => 68, name => "Erbium", weight => "167.26(3)", },
	Tm => { atomic_number => 69, name => "Thulium", weight => "168.93421(2)", },
	Yb => { atomic_number => 70, name => "Ytterbium", weight => "173.04(3)", },
	Lu => { atomic_number => 71, name => "Lutetium", weight => "174.967(1)", },

	Th => { atomic_number => 90, name => "Thorium", weight => "232.0381(1)", },
	Pa => { atomic_number => 91, name => "Protactinium", weight => "231.03588(2)", },
	U  => { atomic_number => 92, name => "Uranium", weight => "238.0289(1)", },
	Np => { atomic_number => 93, name => "Neptunium", weight => "[237.0482]", },
	Pu => { atomic_number => 94, name => "Plutonium", weight => "[244.0642]", },
	Am => { atomic_number => 95, name => "Americium", weight => "[243.0614]", },
	Cm => { atomic_number => 96, name => "Curium", weight => "[247.0703]", },
	Bk => { atomic_number => 97, name => "Berkelium", weight => "[247.0703]", },
	Cf => { atomic_number => 98, name => "Californium", weight => "[251.0796]", },
	Es => { atomic_number => 99, name => "Einsteinium", weight => "[252.0830]", },
	Fm => { atomic_number => 100, name => "Fermium", weight => "[257.0951]", },
	Md => { atomic_number => 101, name => "Mendelevium", weight => "[258.0984]", },
	No => { atomic_number => 102, name => "Nobelium", weight => "[259.1011]", },
	Lr => { atomic_number => 103, name => "Lawrencium", weight => "[262.110]", },
);

sub on_keydown
{
	my $self = shift;
	local $self->{inside_keydown} = 1;
	$self->SUPER::on_keydown(@_);
}

sub focusedCell
{
	return $_[0]-> SUPER::focusedCell unless $#_;
	my ($self,$x,$y) = @_;
	($x, $y) = @$x if !defined $y && ref($x) eq 'ARRAY';

	my (@last, @dir);
	if ( $self-> {inside_keydown}) {
		@last = $self->SUPER::focusedCell;
		@dir = ($x - $last[0], $y - $last[1]);
		@dir = () unless
			(abs($dir[0]) == 1 && $dir[1] == 0) ||
			(abs($dir[1]) == 1 && $dir[0] == 0);
	}

	if ( @dir ) {
		my $text = $self-> {cells}-> [$last[1]]-> [$last[0]];
		if ( $dir[0] == 1 ) {
			if ($text eq 'La' ) { 
				($x, $y) = (0, 12);
			} elsif ( $text eq 'Ac') {
				($x, $y) = (0, 13);
			} elsif ( $text eq 'Lu') {
				($x, $y) = (3, 7);
			} elsif ( $text eq 'Lr') {
				($x, $y) = (3, 9);
			}
		} elsif ( $dir[0] == -1 ) {
			if ($text eq 'Hf' ) { 
				($x, $y) = (13, 12);
			} elsif ( $text eq 'Rf') {
				($x, $y) = (13, 13);
			} elsif ( $text eq 'Ce') {
				($x, $y) = (2, 7);
			} elsif ( $text eq 'Th') {
				($x, $y) = (2, 9);
			}
		}
	}


	unless ($y >= 0 && $x >= 0 && $self-> {cells}-> [$y] &&
		$self-> {cells}-> [$y]-> [$x] && length $self-> {cells}-> [$y]-> [$x]
	) {
		return unless @dir;
		my $n = 9;
		my @p = ($x, $y);
		my $c = $self->{cells};
		while ($n-- > 0) {
			$p[$_] += $dir[$_] for 0,1;
			last if $p[0] < 0 || $p[1] < 0 || $p[1] >= @$c || $p[0] >= @{$c->[$p[1]]};
			my $item = $c-> [$p[1]]-> [$p[0]];
			next unless defined($item) and defined ($elem_info{$item}-> {name});
			($x,$y) = @p;
			goto SETFOC;
		}

		if ( $dir[0] == 1 && $y < $#$c ) {
			my $item = $c-> [$y + 1]-> [0];
			if ( defined($item) and defined ($elem_info{$item}-> {name})) {
				$x = 0;
				$y++;
				goto SETFOC;
			}
		} elsif ( $dir[0] == -1 && $y > 0 ) {
			my $n = 5;
			my $nx = @{$c->[$y - 1]} - 1;
			while ( $n-- > 0 ) {
				my $item = $c-> [$y - 1]-> [$nx];
				if ( defined($item) and defined ($elem_info{$item}-> {name})) {
					$x = $nx;
					$y--;
					goto SETFOC;
				}
				$nx--;
			}
		}

		return;
	}
		
SETFOC:
	$self-> SUPER::focusedCell( $x, $y);
}

my @small_font_metrics;

my $g = $w-> insert( Periodic =>
	origin => [0,0],
	size   => [$w-> size],
	growMode => gm::Client,
	cells  => [
		['H',  ('')x9,                    'He',   ('')x3],
		[qw(Li Be B  C  N  O  F),  ('')x3,'Ne',   ('')x3],
		[qw(Na Mg Al Si P  S  Cl), ('')x3,'Ar',   ('')x3],
		[qw(K  Ca Sc Ti V  Cr Mn Fe Co Ni),       ('')x4],
		[qw(Cu Zn Ga Ge As Se Br), ('')x3,'Kr',   ('')x3],
		[qw(Rb Sr Y  Zr Nb Mo Tc Ru Rh Pd),       ('')x4],
		[qw(Ag Cd In Sn Sb Te I),  ('')x3,'Xe',   ('')x3],
		[qw(Cs Ba La Hf Ta W  Re Os Ir Pt),       ('')x4],
		[qw(Au Hg Tl Pb Bi Po At), ('')x3,'Rn',   ('')x3],
		[qw(Fr Ra Ac Rf Db Sg Bh Hs Mt Ds),       ('')x4],
		[qw(Rg Cn Nh Fl Mc Lv Ts), ('')x3,'Og',   ('')x3],
		[('') x 14],
		[qw(Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu)],
		[qw(Th Pa U  Np Pu Am Cm Bk Cf Es Fm Md No Lr)],
	],
	drawHGrid => 0,
	drawVGrid => 0,
	constantCellWidth => int( 45 * $scaling + .5),
	constantCellHeight => int( 45 * $scaling + .5),
	multiSelect => 0,
	onDrawCell => sub {
		my ( $self, $canvas,
			$column, $row, $indent,
			$sx1, $sy1, $sx2, $sy2,
			$cx1, $cy1, $cx2, $cy2,
			$selected, $focused, $prelight) = @_;
		$canvas-> clear($sx1, $sy1, $sx2, $sy2);
		my $item = $self-> {cells}-> [$row]-> [$column];
		my $color = $colors{$layers[$row]};
		if ( length $item) {
			return unless $color;
			return unless defined
				($color = ( exists $color-> {$column}) ?
					$color-> {$column} :
					$color-> {default});
			$canvas-> color( cl::Black);
			$canvas-> rectangle( $cx1-1, $cy1-1, $cx2, $cy2);
			if ( $focused || $prelight ) {
				$self-> draw_item_background($canvas, $cx1, $cy1, $cx2-1, $cy2-1, $prelight, $focused ? $self-> hiliteBackColor : cl::Back);
				$canvas-> backColor( cl::Back );
				$canvas-> color( $focused ? $self-> hiliteColor : cl::Fore);
			} else {
				$canvas-> color( $color);
			}
			my $f = $canvas-> font;
			$f->style(fs::Bold);
			$canvas-> text_out( $item, $cx1 + 10 * $scaling, $cy1 + 10 * $scaling);
			$f->style(fs::Normal);
			$canvas-> font( height => 12 * $scaling );
				@small_font_metrics = ( $canvas-> get_text_width('3'), $f-> height)
					unless @small_font_metrics;

			my $text = $elem_info{$item}-> {atomic_number}||"";
			$canvas-> text_out( $text,
				$cx2 - $small_font_metrics[0] * length($text) - 4,
				$cy2 - $small_font_metrics[1] - 4);
			$canvas-> font($f);
			$canvas-> rect_focus( $sx1, $sy1, $sx2-1, $sy2-1) if $focused;
		} elsif ( exists $sides{"$column:$row"}) {
			my $side = $sides{"$column:$row"};
			$canvas-> color( cl::Black);
			$canvas-> line( $cx1-1,$cy1-1,$cx2,$cy1-1 ) if $side & 1;
			$canvas-> line( $cx1-1,$cy1-1,$cx1-1,$cy2 ) if $side & 2;
			$canvas-> line( $cx2,$cy1-1,$cx2,$cy2 ) if $side & 4;
			$canvas-> line( $cx1-1,$cy2,$cx2,$cy2 ) if $side & 8;
		}
	},
	onClick => sub {
		my ($self) = @_;
		my @foc = $self-> focusedCell;
		my $text = $self-> {cells}-> [$foc[1]]-> [$foc[0]];
		return unless $text;
		if ($text eq "La") {
			$self-> focusedCell(0, 11);
		} elsif ($text eq "Ac") {
			$self-> focusedCell(0, 12);
		}
	},
	onSelectCell => sub {
		my ( $self, $col, $row) = @_;
		my $item = $self-> {cells}-> [$row]-> [$col];
		return unless defined($item) and defined ($elem_info{$item}-> {name});
		$w-> text("Periodic table of elements - $elem_info{$item}->{name} $elem_info{$item}->{atomic_number} $elem_info{$item}->{weight}");
	},
);

run Prima;
