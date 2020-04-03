use strict;
use warnings;
use utf8;
use Prima qw(Label InputLine Buttons Application PodView Edit FontDialog);
use Prima::Bidi qw(:require);

my $w;
my $pod;
my $arabic;
my $editor;
my $pod_text;
my $font_dialog;

Prima::Bidi::language('ar_AR');
$::application->textDirection(1);

$w = Prima::MainWindow-> create(
	size => [ 430, 200],
	designScale => [7, 16],
	text => "Bidirectional texts",
	menuItems => [
		[ "~Options" => [
			[ "~Toggle direction" => sub {
				my $td = !$w-> Hebrew-> textDirection;
				$w-> Hebrew-> textDirection($td);
				$arabic-> textDirection( $td);
				$pod->textDirection($td);
				$pod->format(1);
			} ],
			[ "~Set font" => sub {
				$font_dialog //= Prima::FontDialog-> create(logFont => $w->font);
				$w->font($font_dialog-> logFont) if $font_dialog-> execute == mb::OK;
			} ],
		]],
	],
);

sub can_rtl
{
	$::application->font(shift);
	my @r = @{ $::application->get_font_ranges };
	my $hebrew = ord('א');
	my $arabic = ord('ر');
	my $found_hebrew;
	my $found_arabic;
	for ( my $i = 0; $i < @r; $i += 2 ) {
		my ( $l, $r ) = @r[$i, $i+1];
		$found_hebrew = 1 if $l <= $hebrew && $r >= $hebrew;
		$found_arabic = 1 if $l <= $arabic && $r >= $arabic;
	}
	return $found_hebrew && $found_arabic;
}

# try to find font with arabic and hebrew letters
$::application->begin_paint_info;
unless (can_rtl($w->font)) {
	my $found;
	my @f = @{$::application->fonts};

	# fontconfig fonts
	for my $f ( @f ) {
		next unless $f->{vector};
		next unless $f->{name} =~ /^[A-Z]/;
		next unless can_rtl($f);
		$found = $f;
		goto FOUND;
	}

	# x11/core vector fonts
	for my $f ( @f ) {
		next unless $f->{vector};
		next if $f->{name} =~ /^[A-Z]/;
		next unless can_rtl($f);
		$found = $f;
		goto FOUND;
	}

	# bitmap fonts
	for my $f ( @f ) {
		next if $f->{vector};
		next unless can_rtl($f);
		$found = $f;
		goto FOUND;
	}
FOUND:
	$w->font->name($found->{name}) if $found;

}
$::application->end_paint_info;

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

my $arabic_text = "الفالح حلمه كبير.
طول ساق النبتة وصارت
شجرة في أرض الفالح
وعلى الشجرة غصون و أوراق
بفيتها إ حتمى الفالح
الفالح حلمه كبير";

$editor = $panel-> insert( Edit =>
	packInfo => { fill => 'both', expand => 1, pad => 10 , side => 'left'},
	geometry => gt::Pack,
	name => 'Editor',
	text     => $arabic_text,
);


$arabic = $panel->insert( Label =>
	packInfo => { fill => 'both', expand => 1, pad => 10, side => 'left' },
	geometry => gt::Pack,
	name  => 'Arabic',
	backColor => cl::Yellow,
	text     => $arabic_text,
	wordWrap => 1,
	showPartial => 0,
);

$pod = $panel-> insert( PodView =>
	packInfo => { fill => 'both', expand => 1, pad => 10 , side => 'left'},
	geometry => gt::Pack,
	name => 'Pod',
);

$arabic_text =~ s/(.*?)\n//;
$pod_text = "=head1 $1\n\n" . join("\n\n", split "\n", $arabic_text) . "\n\n";
$pod-> open_read( createIndex => 0 );
$pod-> read($pod_text);
$pod-> close_read;

run Prima;
