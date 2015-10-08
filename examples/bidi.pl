use strict;
use warnings;
use utf8;
use Prima qw(Label InputLine Application);
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

$w->insert( Label =>
	origin => [ 10, 50],
	name  => 'Arabic',
	text => "الفالح حلمه كبير.
طول ساق النبتة وصارت
شجرة في أرض الفالح
وعلى الشجرة غصون و أوراق
بفيتها إ حتمى الفالح
الفالح حلمه كبير",
	backColor => cl::Yellow,
	wordWrap => 1,
	height => 140,
	width  => 200,
	growMode => gm::Client,
	showPartial => 0,
);

$w->insert( InputLine =>
	name => 'Hebrew',
	origin => [ 10, 10],
	width  => 200,
	text => "אפס123 - תרttttאה מה אני יכול!",
	growMode => gm::Floor,
);

run Prima;
