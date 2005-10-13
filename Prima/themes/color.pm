# $Id$
# sample color styles

use strict;
use Prima qw(Themes);
package Prima::Themes::color;

# byte pairs: weak (0xFF00 mask) and strong ( 0x00FF) colors
my %list = (
	backColor         => 0x80c0,
	light3DColor      => 0x80e8, 
	dark3DColor   	   => 0x0080,
	disabledColor     => 0x0040,
	disabledBackColor => 0x90cc,
	color             => 0x0030,
);

my %weak_selection = (
	hiliteColor       => 0x0030,
	hiliteBackColor   => 0x60cc,
);

my %strong_selection = (
	hiliteBackColor   => 0x0010,
	hiliteColor       => 0x00f0,
);

my %strong_classes = map { $_ => 1 } (
	wc::Combo,
	wc::Edit,
	wc::ListBox,
	wc::InputLine,
	wc::Menu,
	wc::Popup
);

my %transparent_classes = map { $_ => 1 } (
	wc::CheckBox,
	wc::Radio,
	wc::Label,
);

sub merger
{
	my ( $object, $profile, $default, $mask) = @_;
	my $class = exists ( $profile->{widgetClass}) ? 
		$profile->{widgetClass} : $default->{widgetClass};
	my %class = (%list, 
		exists($strong_classes{$class}) ? %strong_selection : %weak_selection);
	$class{hiliteBackColor} = $class{disabledBackColor} = $class{backColor}
		if $transparent_classes{$class};
	my ( $r, $g, $b) = (
		( $mask >> 16) & 0xFF,
		( $mask >> 8) & 0xFF,
		$mask & 0xFF,
	);
	for ( keys %class) {
		my ( $weak_color, $strong_color) = (( $class{$_} & 0xFF00) >> 8, $class{$_} & 0xFF);
		$class{$_} = 
			(( $r ? $strong_color : $weak_color) << 16) |
			(( $g ? $strong_color : $weak_color) << 8) |
			( $b ? $strong_color : $weak_color);
	}
	Prima::Themes::merger( $object, $profile, $default, \%class);
}

Prima::Themes::register( 'Prima::themes::color', 'cyan',    ['Prima::Widget' => 0x00FFFF], \&merger);
Prima::Themes::register( 'Prima::themes::color', 'yellow',  ['Prima::Widget' => 0xFFFF00], \&merger);
Prima::Themes::register( 'Prima::themes::color', 'magenta', ['Prima::Widget' => 0xFF00FF], \&merger);
Prima::Themes::register( 'Prima::themes::color', 'gray',    ['Prima::Widget' => 0xFFFFFF], \&merger);
