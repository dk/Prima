use strict;
use warnings;
use utf8;
use Prima qw(Label InputLine Application);
use Prima::Bidi qw(:require :ltr);

#$Prima::Bidi::default_direction_rtl = 1;

sub _b($) { Prima::Bidi::visual(shift) }

my $w = Prima::MainWindow-> create(
	size => [ 430, 200],
	text => "Bidirectional texts",
);

$w->insert( Label =>
	origin => [ 10, 50],
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

my $i = $w->insert( InputLine =>
	origin => [ 10, 10],
	width  => 200,
	text => "אפס123 - תראה מה אני יכול!",
	growMode => gm::Floor,
);

run Prima;
