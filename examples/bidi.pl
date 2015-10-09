use strict;
use warnings;
use utf8;
use Prima qw(Label InputLine Buttons Application PodView);
use Prima::Bidi qw(:require :rtl);

my $w;
$w = Prima::MainWindow-> create(
	size => [ 430, 200],
	text => "Bidirectional texts",
	menuItems => [
		[ "~Options" => [
			[ "~Toggle direction" => sub {
				$w-> Arabic-> alignment( $w-> Arabic-> alignment == ta::Left ? ta::Right : ta::Left );
				$w-> Hebrew-> textDirection(! $w-> Hebrew-> textDirection);
			} ],
		]],
	],
);

$w->insert( InputLine =>
	name => 'Hebrew',
	origin => [ 10, 10],
	width  => 200,
	text => "אפס123 - תרttttאה מה אני יכול!",
	growMode => gm::Floor,
);

$w-> insert( Button => 
	name => 'Farsi',
	text => 'ترک',
	origin => [ 320, 10 ],
	growMode => gm::GrowLoX,
	onClick => sub { $::application-> close },
);

my $panel = $w->insert( Widget =>
	origin => [ 10, 50 ],
	size   => [ 410, 140 ],
	growMode  => gm::Client,
);

my $arabic = "الفالح حلمه كبير.
طول ساق النبتة وصارت
شجرة في أرض الفالح
وعلى الشجرة غصون و أوراق
بفيتها إ حتمى الفالح
الفالح حلمه كبير";

$panel->insert( Label =>
	packInfo => { fill => 'both', expand => 1, pad => 10, side => 'left' },
	geometry => gt::Pack,
	name  => 'Arabic',
	backColor => cl::Yellow,
	text     => $arabic,
	wordWrap => 1,
	height => 140,
	width  => 200,
	showPartial => 0,
);

my $pod = $panel-> insert( PodView => 
	packInfo => { fill => 'both', expand => 1, pad => 10 , side => 'left'},
	geometry => gt::Pack,
	name => 'Pod',
);

$pod-> open_read( createIndex => 0 );
my $head = $arabic =~ s/(.*?)\n//;
$pod-> read("=head1 $1\n\n" . join("\n\n", split "\n", $arabic) . "\n\n");
$pod-> close_read;

run Prima;
