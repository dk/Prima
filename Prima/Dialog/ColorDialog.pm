package Prima::Dialog::ColorDialog;

use strict;
use warnings;
use Prima qw(Sliders Label Buttons ComboBox ScrollBar);
use vars qw( @ISA $colorWheel $colorWheelShape);
@ISA = qw( Prima::Dialog);

{
my %RNT = (
	%{Prima::Dialog-> notification_types()},
	BeginDragColor => nt::Command,
	EndDragColor   => nt::Command,
);

sub notification_types { return \%RNT; }
}

my $shapext = Prima::Application-> get_system_value( sv::ShapeExtension);

sub hsv2rgb
{
	my ( $h, $s, $v) = @_;
	$v = 1 if $v > 1;
	$v = 0 if $v < 0;
	$s = 1 if $s > 1;
	$s = 0 if $s < 0;
	$v *= 255;
	return $v, $v, $v if $h == -1;
	my ( $r, $g, $b, $i, $f, $w, $q, $t);
	$h -= 360 if $h >= 360;
	$h /= 60;
	$i = int( $h);
	$f = $h - $i;
	$w = $v * (1 - $s);
	$q = $v * (1 - ($s * $f));
	$t = $v * (1 - ($s * (1 - $f)));

	if ( $i == 0) {
		return $v, $t, $w;
	} elsif ( $i == 1) {
		return $q, $v, $w;
	} elsif ( $i == 2) {
		return $w, $v, $t;
	} elsif ( $i == 3) {
		return $w, $q, $v;
	} elsif ( $i == 4) {
		return $t, $w, $v;
	} else {
		return $v, $w, $q;
	}
}

sub rgb2hsv
{
	my ( $r, $g, $b) = @_;
	my ( $h, $s, $v, $max, $min, $delta);
	$r /= 255;
	$g /= 255;
	$b /= 255;
	$max = $r;
	$max = $g if $g > $max;
	$max = $b if $b > $max;
	$min = $r;
	$min = $g if $g < $min;
	$min = $b if $b < $min;
	$v = $max;
	$s = $max ? ( $max - $min) / $max : 0;
	return -1, $s, $v unless $s;

	$delta = $max - $min;
	if ( $r == $max) {
		$h = ( $g - $b) / $delta;
	} elsif ( $g == $max) {
		$h = 2 + ( $b - $r) / $delta;
	} else {
		$h = 4 + ( $r - $g) / $delta;
	}
	$h *= 60;
	$h += 360 if $h < 0;
	return $h, $s, $v;
}

sub xy2hs
{
	my ( $x, $y, $c) = @_;
	my ( $d, $r, $rx, $ry, $h, $s);
	( $rx, $ry) = ( $x - $c, $y - $c);
	my $c2 = $c * $c;
	$d = $c2 * ( $rx*$rx + $ry*$ry - $c2);

	$r = sqrt( $rx*$rx + $ry*$ry);

	$h = $r ? atan2( $rx/$r, $ry/$r) : 0;

	$s = $r / $c;
	$h = $h * 57.295779513 + 180;

	$s = 1 if $s > 1;

	return $h, $s, $d > 0;
}

sub hs2xy
{
	my ( $self, $h, $s) = @_;
	my ( $r, $a) = ( 128 * $s, ($h - 180) / 57.295779513);
	return map { $self->{scaling} * $_ } 128 + $r * sin( $a), 128 + $r * cos( $a);
}

sub _diameter
{
	my ($id, $pix) = @_;
	my $imul = 256 * $pix / $id;
	my $d = int( 256 * $pix - $imul * 2 - 1 + .5);
	$d-- if $d % 2;
	return $d;
}

sub create_wheel
{
	my ($id, $pix, $color)   = @_;
	my $d = _diameter($id, $pix);
	my $imul = 256 * $pix / $id;

	my ( $y1, $x1) = ($id,$id);
	my  $d0 = $id / 2;

	my $i = Prima::Image->new(
		width  => $id,
		height => $id,
		type   => im::RGB,
	);

	for ( my $y = 0; $y < $y1; $y++) {
		for ( my $x = 0; $x < $x1; $x++) {
			my ( $h, $s) = xy2hs( $x, $y, $d0);
			my ( $r, $g, $b) = hsv2rgb( $h, $s, 1);
			$i-> pixel( $x, $y, $b | ($g << 8) | ($r << 16));
		}
	}

	my $quality = $::application->get_bpp > 8;
	$i-> scaling( ist::Gaussian ) if $quality;
	$i-> size( 256 * $pix, 256 * $pix);

	my $xmul = $quality ? 2 : 1;
	my $mask = Prima::Image->new(
		size      => [ map { $_ * $xmul } $i-> size ],
		type      => im::Byte,
		backColor => 0x000000,
		color     => 0xffffff,
	);
	$mask-> scaling( ist::Triangle ) if $quality;
	$mask-> clear;
	my $c = int( 128 * $pix + .5 );
	$mask-> fill_ellipse( map { $_ * $xmul } $c, $c, $d, $d);
	$mask-> size( $i->size );

	my $target = Prima::Image->new(
		type      => im::RGB,
		size      => [$i->size],
		backColor => $color,
	);
	$target->clear;
	$i = Prima::Icon->create_combined( $i, $mask );
	$i-> premultiply_alpha;
	$target-> put_image(0,0,$i);
	return $target->bitmap;
}

sub create_wheel_shape
{
	return unless $shapext;
	my ($id, $pix) = @_;
	my $quality = $::application->get_bpp > 8;
	my $xmul = $quality ? 2 : 1;
	my $a = Prima::Image-> new(
		width     => $xmul * 256 * $pix,
		height    => $xmul * 256 * $pix,
		type      => im::BW,
		backColor => cl::Black,
		color     => cl::White,
		scaling   => ist::OR,
	);
	my $d = _diameter($id,$pix);
	my $c = int( 128 * $pix + .5 );
	$a-> clear;
	$a-> fill_ellipse( map { $xmul * $_ } $c, $c, $d, $d);
	$a-> size( map { $_ / $xmul } $a->size);
	return $a;
}

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},

		width         => 348,
		height        => 450,
		centered      => 1,
		visible       => 0,
		scaleChildren => 1,
		designScale   => [7, 16],
		text          => 'Select color',

		quality       => 0,
		grayscale     => 0,
		value         => cl::White,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	$self-> {setTransaction} = undef;

	my $c = $self-> {value} = $profile{value};
	$self-> {quality} = 0;
	my ( $r, $g, $b) = cl::to_rgb( $c);
	my ( $h, $s, $v) = rgb2hsv( $r, $g, $b);
	$s *= 255;
	$v *= 255;
	$h = int($h);
	$s = int($s);
	$v = int($v);

	my $dx = $Prima::Widget::default_font_box[0] / ($self-> designScale)[0];
	my $dy = $Prima::Widget::default_font_box[1] / ($self-> designScale)[1];
	my $pix = ( $dx < $dy ) ? $dx : $dy;
	$colorWheel = create_wheel(32, $pix, $self-> map_color($self->backColor)) unless $colorWheel;
	$colorWheelShape = create_wheel_shape(32, $pix) unless $colorWheelShape;

	$self-> {wheel} = $self-> insert( Widget =>
		designScale    => undef,
		origin         => [
			20 * $dx  + ($dx - $pix) * 256 / 2,
			172 * $dy + ($dy - $pix) * 256 / 2
		],
		width          => 256 * $pix,
		height         => 256 * $pix,
		name           => 'Wheel',
		shape          => $colorWheelShape,
		ownerBackColor => 1,
		syncPaint      => 1,
		delegations    => [qw(Paint MouseDown MouseUp MouseMove)],
	);

	$self-> {scaling} = $pix;

	$self-> {roller} = $self-> insert( Widget =>
		origin    => [ 288, 164],
		width     => 48,
		height    => 272,
		buffered  => 1,
		name      => 'Roller',
		ownerBackColor => 1,
		delegations    => [qw(Paint MouseDown MouseUp MouseMove)],
	);

	# RGB
	my %rgbprf = (
		width    => 72,
		max      => 255,
		onChange => sub { RGB_Change( $_[0]-> owner, $_[0]);},
	);
	$self-> {R} = $self-> insert( SpinEdit =>
		origin   => [40,120],
		value    => $r,
		name     => 'R',
		%rgbprf,
	);
	my %labelprf = (
		width      => 20,
		autoWidth  => 0,
		autoHeight => 0,
		valignment => ta::Center,
	);
	$self-> insert( Label =>
		origin     => [ 20, 120],
		focusLink  => $self-> {R},
		text       => 'R:',
		%labelprf,
	);
	$self-> {G} = $self-> insert( SpinEdit =>
		origin   => [148,120],
		value    => $g,
		name     => 'G',
		%rgbprf,
	);
	$self-> insert( Label =>
		origin     => [ 126, 120],
		focusLink  => $self-> {G},
		text       => 'G:',
		%labelprf,
	);
	$self-> {B} = $self-> insert( SpinEdit =>
		origin   => [256,120],
		value    => $b,
		name     => 'B',
		%rgbprf,
	);
	$self-> insert( Label =>
		origin     => [ 236, 120],
		focusLink  => $self-> {B},
		text       => 'B:',
		%labelprf,
	);

	$rgbprf{onChange} = sub { HSV_Change( $_[0]-> owner, $_[0])};
	$self-> {H} = $self-> insert( SpinEdit =>
		origin   => [ 40,78],
		value    => $h,
		name     => 'H',
		%rgbprf,
		max      => 360,
	);
	$self-> insert( Label =>
		origin     => [ 20, 78],
		focusLink  => $self-> {H},
		text       => 'H:',
		%labelprf,
	);
	$self-> {S} = $self-> insert( SpinEdit =>
		origin   => [ 146,78],
		value    => int($s),
		name     => 'S',
		%rgbprf,
	);
	$self-> insert( Label =>
		origin     => [ 126, 78],
		focusLink  => $self-> {S},
		text       => 'S:',
		%labelprf,
	);
	$self-> {V} = $self-> insert( SpinEdit =>
		origin   => [ 256,78],
		value    => int($v),
		name     => 'V',
		%rgbprf,
	);
	$self-> insert( Label =>
		origin     => [ 236, 78],
		focusLink  => $self-> {V},
		text       => 'V:',
		%labelprf,
	);
	$self-> insert( Button =>
		text        => '~OK',
		origin      => [ 20, 20],
		modalResult => mb::OK,
		default     => 1,
	);

	$self-> insert( Button =>
		text        => 'Cancel',
		origin      => [ 126, 20],
		modalResult => mb::Cancel,
	);
	$self-> {R}-> select;
	$self-> quality( $profile{quality});

	$self-> Roller_Repaint if $self-> {quality};
	$self-> grayscale($profile{grayscale});
	return %profile;
}

sub on_destroy
{
	$colorWheelShape = undef;
}

sub on_begindragcolor
{
	my ( $self, $property) = @_;
	$self-> {old_text} = $self-> text;
	$self-> {wheel}-> pointer( cr::DragMove);
	$self-> text( "Apply $property...");
}

sub on_enddragcolor
{
	my ( $self, $property, $widget) = @_;

	$self-> {wheel}-> pointer( cr::Default);
	$self-> text( $self-> {old_text});
	if ( $widget) {
		$property = $widget-> can( $property);
		$property-> ( $widget, $self-> value) if $property;
	}
	delete $self-> {old_text};
}

use constant Hue    => 1;
use constant Sat    => 2;
use constant Lum    => 4;
use constant Roller => 8;
use constant Wheel  => 16;
use constant All    => 31;

sub RGB_Change
{
	my ($self, $pin) = @_;
	return if $self-> {setTransaction};
	$self-> {setTransaction} = 1;
	$self-> {RGBPin} = $pin;
	my ( $r, $g, $b) = cl::to_rgb( $self-> {value});
	$r = $self-> {R}-> value if $pin == $self-> {R};
	$g = $self-> {G}-> value if $pin == $self-> {G};
	$b = $self-> {B}-> value if $pin == $self-> {B};
	$self-> value( cl::from_rgb( $r, $g, $b));
	undef $self-> {RGBPin};
	undef $self-> {setTransaction};
}

sub HSV_Change
{
	my ($self, $pin) = @_;
	return if $self-> {setTransaction};
	$self-> {setTransaction} = 1;
	my ( $h, $s, $v);
	$self-> {HSVPin} = Hue | Lum | Sat | ( $pin == $self-> {V} ? (Wheel|Roller) : 0);
	$h = $self-> {H}-> value      ;
	$s = $self-> {S}-> value / 255;
	$v = $self-> {V}-> value / 255;
	$self-> value( cl::from_rgb( hsv2rgb( $h, $s, $v)));
	undef $self-> {HSVPin};
	undef $self-> {setTransaction};
}

sub Wheel_Paint
{
	my ( $owner, $self, $canvas) = @_;
	$canvas-> put_image( 0, 0, $colorWheel);
	my ( $x, $y) = $owner-> hs2xy( $owner-> {H}-> value, $owner-> {S}-> value/273);
	$canvas-> color( cl::White);
	$canvas-> rop( rop::XorPut);
	if ( $shapext) {
		my @sz = $canvas-> size;
		$canvas-> linePattern( lp::DotDot);
		$canvas-> line( $x, 0, $x, $sz[1]);
		$canvas-> line( 0, $y, $sz[0], $y);
	} else {
		$canvas-> lineWidth( 3);
		$canvas-> ellipse( $x, $y, 13, 13);
	}
}

sub Wheel_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransation};
	return if $btn != mb::Left;
	my $scale = $owner->{scaling};
	my ( $h, $s, $ok) = xy2hs( $x-9*$scale, $y-9*$scale, 119*$scale);
	return if $ok;
	$self-> {mouseTransation} = $btn;
	$self-> capture(1);
	if ( $btn == mb::Left) {
		if ( $mod == ( km::Ctrl | km::Alt)) {
			$self-> {drag_color} = 'disabledColor';
		} elsif ( $mod == ( km::Ctrl | km::Alt | km::Shift)) {
			$self-> {drag_color} = 'disabledBackColor';
		} elsif ( $mod == ( km::Ctrl | km::Shift)) {
			$self-> {drag_color} = 'hiliteColor';
		} elsif ( $mod == ( km::Alt | km::Shift)) {
			$self-> {drag_color} = 'hiliteBackColor';
		} elsif ( $mod & km::Ctrl) {
			$self-> {drag_color} = 'color';
		} elsif ( $mod & km::Alt) {
			$self-> {drag_color} = 'backColor';
		} else {
			$self-> notify( "MouseMove", $mod, $x, $y);
		}

		$owner-> notify( 'BeginDragColor', $self-> {drag_color})
			if $self-> {drag_color};
	}
}

sub Wheel_MouseMove
{
	my ( $owner, $self, $mod, $x, $y) = @_;
	return if !$self-> {mouseTransation} or $self-> {drag_color};
	my $scale = $owner->{scaling};
	my ( $h, $s, $ok) = xy2hs( $x-9*$scale, $y-9*$scale, 119*$scale);
	$owner-> {setTransaction} = 1;
	$owner-> {HSVPin} = Lum|Hue|Sat;
	$owner-> {H}-> value( int( $h));
	$owner-> {S}-> value( int( $s * 255));
	$owner-> value( cl::from_rgb( hsv2rgb( int($h), $s, $owner-> {V}-> value/255)));
	$owner-> {HSVPin} = undef;
	$owner-> {setTransaction} = undef;
}

sub Wheel_MouseUp
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransation};
	$self-> {mouseTransation} = undef;
	$self-> capture(0);
	if ( $self-> {drag_color}) {
		$owner-> notify('EndDragColor', $self-> {drag_color},
			$::application-> get_widget_from_point( $self-> client_to_screen( $x, $y)));
		delete $self-> {drag_color};
	}
}

sub Roller_Paint
{
	my ( $owner, $self, $canvas) = @_;
	my @size = $self-> size;
	$canvas-> clear;
	my $i;
	my $step = 8 * $owner->{scaling};
	my ( $h, $s, $v, $d, $dd) = ( $owner-> {H}-> value, $owner-> {S}-> value,
		$owner-> {V}-> value, ($size[1] - $step * 2) / 32, ($size[1] - $step * 2) / 256 );
	$s /= 255;
	$v /= 255;
	my ( $r, $g, $b);

	my $rlim;
	if ( $::application->get_bpp > 8 ) {
		$rlim = 256;
	} else {
		$rlim = 32;
		$dd = $d;
	}
	for $i (0..$rlim-1) {
		( $r, $g, $b) = hsv2rgb( $h, $s, $i / $rlim);
		$canvas-> color( cl::from_rgb( $r, $g, $b));
		$canvas-> bar( $step, $step + $i * $dd, $size[0] - $step, $step + ($i + 1) * $dd);
	}

	$canvas-> color( cl::Black);
	$step = int($step);
	$canvas-> rectangle( $step, $step, $size[0] - $step, $size[1] - $step);
	$d = int( $v * ($size[1]-$step * 2));
	$canvas-> rectangle( 0, $d, $size[0]-1, $d + $step * 2 - 1);
	$canvas-> color( $owner-> {value});
	$canvas-> bar( 1, $d + 1, $size[0]-2, $d + $step * 2 - 2);
	$self-> {paintPoll} = 2 if exists $self-> {paintPoll};
}

sub Roller_Repaint
{
	my $owner = $_[0];
	my $roller = $owner-> {roller};
	if ( $owner-> {quality}) {
		my ( $h, $s, $v) = ( $owner-> {H}-> value, $owner-> {S}-> value, $owner-> {V}-> value);
		$s /= 255;
		$v /= 255;
		my ( $i, $r, $g, $b);
		my @pal;

		for ( $i = 0; $i < 32; $i++) {
			( $r, $g, $b) = hsv2rgb( $h, $s, $i / 31);
			push ( @pal, $b, $g, $r);
		}
		( $r, $g, $b) = cl::to_rgb( $owner-> {value});
		push ( @pal, $b, $g, $r);

		$roller-> {paintPoll} = 1;
		$roller-> palette([@pal]);
		$roller-> repaint if $roller-> {paintPoll} != 2;
		delete $roller-> {paintPoll};
	} else {
		$roller-> repaint;
	}
}


sub Roller_MouseDown
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return if $self-> {mouseTransation};
	$self-> {mouseTransation} = 1;
	$self-> capture(1);
	$self-> notify( "MouseMove", $mod, $x, $y);
}

sub Roller_MouseMove
{
	my ( $owner, $self, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransation};
	$owner-> {setTransaction} = 1;
	$owner-> {HSVPin} = Hue|Sat|Wheel|Roller;
	my $step = 8 * $owner->{scaling};
	$owner-> value( cl::from_rgb( hsv2rgb(
		$owner-> {H}-> value, $owner-> {S}-> value/255,
		($y - $step) / ( $self-> height - $step * 2))));
	$owner-> {HSVPin} = undef;
	$owner-> {setTransaction} = undef;
	$self-> update_view;
}

sub Roller_MouseUp
{
	my ( $owner, $self, $btn, $mod, $x, $y) = @_;
	return unless $self-> {mouseTransation};
	$self-> {mouseTransation} = undef;
	$self-> capture(0);
}


sub set_quality
{
	my ( $self, $quality) = @_;
	return if $quality == $self-> {quality};
	$self-> {quality} = $quality;
	$self-> {roller}-> palette([]) unless $quality;
	$self-> Roller_Repaint;
}

sub set_value
{
	my ( $self, $value) = @_;
	return if $value == $self-> {value} and ! $self-> {HSVPin};
	$self-> {value} = $value;
	my $st = $self-> {setTransaction};
	$self-> {setTransaction} = 1;
	my $rgb = $self-> {RGBPin} || 0;
	my $hsv = $self-> {HSVPin} || 0;
	my ( $r, $g, $b) = cl::to_rgb( $value);
	my ( $h, $s, $v) = rgb2hsv( $r, $g, $b);
	$s = int( $s*255);
	$v = int( $v*255);
	$self-> {R}-> value( $r) if $self-> {R} != $rgb;
	$self-> {G}-> value( $g) if $self-> {G} != $rgb;
	$self-> {B}-> value( $b) if $self-> {B} != $rgb;
	$self-> {H}-> value( int($h)) unless $hsv & Hue;
	$self-> {S}-> value( int($s)) unless $hsv & Sat;
	$self-> {V}-> value( int($v)) unless $hsv & Lum;
	$self-> {wheel}-> repaint unless $hsv & Wheel;
	if ( $hsv & Roller) {
		$self-> {roller}-> repaint;
	} else {
		$self-> Roller_Repaint;
	}
	$self-> {setTransaction} = $st;
	$self-> notify(q(Change));
}

sub value        {($#_)?$_[0]-> set_value        ($_[1]):return $_[0]-> {value};}
sub quality      {($#_)?$_[0]-> set_quality      ($_[1]):return $_[0]-> {quality};}

sub grayscale
{
	return $_[0]->{grayscale} unless $#_;
	my ( $self, $gs ) = @_;
	$self-> {$_}-> enabled(!$gs) for qw(H S R G B wheel);
	if ( $gs ) {
		$self-> {$_}-> value(0) for qw(H S);
	}
}

package Prima::ColorComboBox;
use vars qw(@ISA);
@ISA = qw(Prima::ComboBox);

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	Colorify => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
	my %sup = %{$_[ 0]-> SUPER::profile_default};
	my @std = Prima::Application-> get_default_scrollbar_metrics;
	my $scaling = $::application-> uiScaling;
	return {
		%sup,
		style            => cs::DropDownList,
		height           => $sup{ editHeight},
		value            => cl::White,
		width            => 56 * $scaling,
		literal          => 0,
		colors           => 20 + 128,
		grayscale        => 0,
		editClass        => 'Prima::Widget',
		listClass        => 'Prima::Widget',
		editProfile      => {
			selectingButtons => 0,
		},
		listProfile      => {
			width    => $scaling * 78 + $std[0],
			height   => $scaling * 130,
			growMode => 0,
		},
	};
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { style} = cs::DropDownList;
	$self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
	my $self    = shift;
	my %profile = @_;
	$self-> {value} = $profile{value};
	$self-> {colors} = $profile{colors};
	@{$profile{listDelegations}} = grep { $_ ne 'SelectItem' } @{$profile{listDelegations}};
	push ( @{$profile{listDelegations}}, qw(Create Paint MouseDown MouseMove MouseLeave));
	push ( @{$profile{editDelegations}}, qw(Paint MouseDown Enter Leave Enable Disable KeyDown MouseEnter MouseLeave));
	%profile = $self-> SUPER::init(%profile);
	$self-> colors( $profile{colors});
	$self-> value( $profile{value});
	$self-> grayscale( $profile{grayscale});
	return %profile;
}

sub InputLine_KeyDown
{
	my ( $combo, $self, $code, $key) = @_;
	$combo-> listVisible(1), $self-> clear_event if $key == kb::Down;
	return if $key != kb::NoKey;
	$self-> clear_event;
}

sub InputLine_Paint
{
	my ( $combo, $self, $canvas, $w, $h, $focused) =
		($_[0],$_[1],$_[2],$_[1]-> size, $_[1]-> focused);
	my $back = $self-> enabled ? $self-> backColor : $self-> disabledBackColor;
	my $clr  = $combo-> value;
	$clr = $combo->prelight_color($clr) if $self->{prelight};
	$clr = $back if $clr == cl::Invalid;
	$canvas-> rect3d( 0, 0, $w-1, $h-1, 1, $self-> light3DColor, $self-> dark3DColor);
	$canvas-> color( $back);
	$canvas-> rectangle( 1, 1, $w - 2, $h - 2);
	$canvas-> rectangle( 2, 2, $w - 3, $h - 3);
	$canvas-> color( $clr);
	$canvas-> rop2(rop::CopyPut);
	$canvas-> fillPattern([(0xEE, 0xBB) x 4]) unless $self-> enabled;
	$canvas-> bar( 3, 3, $w - 4, $h - 4);
	$canvas-> rect_focus(2, 2, $w - 3, $h - 3) if $focused;
}

sub InputLine_MouseDown
{
	# this code ( instead of listVisible(!listVisible)) is formed so because
	# ::InputLine is selectable, and unwilling focus() could easily hide
	# listBox automatically. Manual focus is also supported by
	# selectingButtons == 0.
	my ( $combo, $self)  = @_;
	my $lv = $combo-> listVisible;
	$combo-> listVisible(!$lv);
	$self-> focus if $lv;
	$self-> clear_event;
}

sub InputLine_Enable  { $_[1]-> repaint };
sub InputLine_Disable { $_[1]-> repaint };
sub InputLine_Enter   { $_[1]-> repaint; }

sub InputLine_Leave
{
	$_[0]-> listVisible(0) if $Prima::ComboBox::capture_mode;
	$_[1]-> repaint;
}


sub InputLine_MouseWheel
{
	my ( $self, $widget, $mod, $x, $y, $z) = @_;

	my $v = $self-> value;
	$z = $z / 120 * 16;
	my ( $r, $g, $b) = ( $v >> 16, ($v >> 8) & 0xff, $v & 0xff);
	if ( $mod & km::Shift) {
		$r += $z;
	} elsif ( $mod & km::Ctrl) {
		$g += $z;
	} elsif ( $mod & km::Alt) {
		$b += $z;
	} else {
		$r += $z;
		$g += $z;
		$b += $z;
	}
	for ( $r, $g, $b) {
		$_ = 0 if $_ < 0;
		$_ = 255 if $_ > 255;
	}
	$self-> value( $r * 65536 + $g * 256 + $b);
	$widget-> clear_event;
}

sub InputLine_MouseEnter
{
	my ($self, $widget) = @_;
	if ( !$widget->capture && $self->enabled) {
		$widget->{prelight} = 1;
		$widget->repaint;
	}
}

sub InputLine_MouseLeave
{
	my ($self, $widget) = @_;
	if ( !$widget->capture && $self->enabled) {
		delete $widget->{prelight};
		$widget->repaint;
	}
}

sub List_Create
{
	my ($combo,$self) = @_;
	$self-> {scaling} = $::application-> uiScaling;
	$combo-> {btn} = $self-> insert( Button =>
		origin     => [ map { $_ * $self->{scaling} } 3, 3],
		width      => $self-> width - 6 * $self->{scaling},
		height     => $self->{scaling} * 28,
		text       => '~More...',
		selectable => 0,
		name       => 'MoreBtn',
		onClick    => sub { $combo-> MoreBtn_Click( @_)},
	);

	my $c = $combo-> colors;
	$combo-> {scr} = $self-> insert( ScrollBar =>
		origin     => [ 75 * $self->{scaling}, $combo-> {btn}-> height + 8 * $self->{scaling}],
		top        => $self-> height - 3 * $self->{scaling},
		vertical   => 1,
		name       => 'Scroller',
		max        => $c > 20 ? $c - 20 : 0,
		partial    => 20,
		step       => 4,
		pageStep   => 20,
		whole      => $c,
		delegations=> [ $combo, 'Change'],
	);
}


sub List_Paint
{
	my ( $combo, $self, $canvas) = @_;
	my ( $w, $h) = $self-> size;
	my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
	$canvas-> rect3d( 0, 0, $w-1, $h-1, 1, @c3d, cl::Back)
		unless exists $self-> {inScroll};
	my $i;
	my $sc = $self->{scaling};
	my $pc = 18 * $sc;
	my $dy = $combo-> {btn}-> height;

	my $maxc = $combo-> colors;
	my $shft = $combo-> {scr}-> value;
	for ( $i = 0; $i < 20; $i++) {
		next if $i >= $maxc;
		my $X = $i % 4;
		my $Y = int($i / 4);
		my ( $x, $y) = ($X * $pc + 3 * $sc, (4 - $Y) * $pc + 9 * $sc + $dy);
		my $clr = 0;
		$combo-> notify('Colorify', $i + $shft, \$clr);

		my @c = @c3d;
		@c = reverse @c if
			$self->{prelight} &&
			$self->{prelight}->[0] == $X &&
			$self->{prelight}->[1] == $Y;
		$canvas-> rect3d( $x, $y, $x + $pc - 2 * $sc, $y + $pc - 2 * $sc, 1, @c, $clr);
	}
}

sub list_pos2xy
{
	my ( $combo, $self, $x, $y) = @_;
	$x -= 3 * $self->{scaling};
	$y -= $combo-> {btn}-> height + 9 * $self->{scaling};
	return if $x < 0 || $y < 0;
	my $pc = 18 * $self->{scaling};
	$x = int($x / $pc);
	$y = int($y / $pc);
	return if $x > 3 * $self->{scaling} || $y > 4 * $self->{scaling};
	$y = 4 - $y;
	my $shft = $combo-> {scr}-> value;
	my $maxc = $combo-> colors;
	my $xcol = $shft + $x + $y * 4;
	return if $xcol >= $maxc;

	return $x, $y, $xcol;
}

sub List_MouseDown
{
	my ( $combo, $self, $btn, $mod, $x, $y) = @_;
	my $xcol;
	($x, $y, $xcol) = $combo->list_pos2xy($self, $x, $y);
	return unless defined $x;
	$combo-> listVisible(0);
	my $xval = 0;
	$combo-> notify('Colorify', $xcol, \$xval);
	$combo-> value( $xval);
}

sub List_MouseMove
{
	my ( $combo, $self, $mod, $x, $y) = @_;
	my $xcol;
	($x, $y, $xcol) = $combo->list_pos2xy($self, $x, $y);
	if ( defined $xcol ) {
		return if
			defined $self->{prelight} &&
			$self->{prelight}->[0] == $x &&
			$self->{prelight}->[1] == $y;
		$self->{prelight} = [ $x, $y ];
	} else {
		return unless defined $self->{prelight};
		delete $self->{prelight};
	}
	$self->repaint;
}

sub List_MouseLeave
{
	my ($self, $widget) = @_;
	if ( !$widget->capture && $self->enabled) {
		delete $widget->{prelight};
		$widget->repaint;
	}
}

sub MoreBtn_Click
{
	my ($combo,$self) = @_;
	my $d;
	$combo-> listVisible(0);
	$d = Prima::Dialog::ColorDialog-> new(
		text      => 'Mixed color palette',
		value     => $combo-> value,
		grayscale => $combo->grayscale,
	);
	$combo-> value( $d-> value) if $d-> execute != mb::Cancel;
	$d-> destroy;
}

sub Scroller_Change
{
	my ($combo,$self) = @_;
	$self = $combo-> List;
	$self-> {inScroll} = 1;
	my $s = $::application-> uiScaling;
	$self-> invalidate_rect(
		3*$s, $combo-> {btn}-> top+6*$s,
		$self-> width - $combo-> {scr}-> width,
		$self-> height - 3*$s,
	);
	delete $self-> {inScroll};
}


sub set_style { $_[0]-> raise_ro('set_style')}

sub set_value
{
	my ( $self, $value) = @_;
	return if $value == $self-> {value};
	$self-> {value} = $value;
	$self-> notify(q(Change));
	$self-> {edit}-> repaint;
}

sub set_colors
{
	my ( $self, $value) = @_;
	return if $value == $self-> {colors};
	$self-> {colors} = $value;
	my $scr = $self-> {list}-> {scr};
	$scr-> set(
		max        => $value > 20 ? $value - 20 : 0,
		whole      => $value,
	) if $scr;
	$self-> {list}-> repaint;
}


my @palColors = (
	0xffffff,0x000000,0xc6c3c6,0x848284,
	0xff0000,0x840000,0xffff00,0x848200,
	0x00ff00,0x008200,0x00ffff,0x008284,
	0x0000ff,0x000084,0xff00ff,0x840084,
	0xc6dfc6,0xa5cbf7,0xfffbf7,0xa5a2a5,
);


sub on_colorify
{
	my ( $self, $index, $sref) = @_;
	if ( $self->{grayscale}) {
		my $g;
		if ( $index < 20 ) {
			$g = $index * 255 / 20;
		} else {
			$g = ($index - 20) * 255 / 128;
		}
		$$sref = cl::from_rgb($g,$g,$g);
	} elsif ( $index < 20) {
		$$sref = $palColors[ $index];
	} else {
		my $i = $index - 20;
		my ( $r, $g, $b);
		if ( $i < 64) {
			( $r, $g, $b) = Prima::Dialog::ColorDialog::hsv2rgb(
				$i * 4, 0.25 + ($i % 4) * 0.25, 1
			);
		} else {
			( $r, $g, $b) = Prima::Dialog::ColorDialog::hsv2rgb(
				$i * 4, 1, 0.25 + ($i % 4) * 0.25
			);
		}
		$$sref = $b | $g << 8 | $r << 16;
	}
	$self-> clear_event;
}


sub value        {($#_)?$_[0]-> set_value       ($_[1]):return $_[0]-> {value};  }
sub colors       {($#_)?$_[0]-> set_colors      ($_[1]):return $_[0]-> {colors};  }

sub grayscale
{
	return $_[0]->{grayscale} unless $#_;
	my ( $self, $gs ) = @_;
	$self->{grayscale} = $gs;
	$self->value(cl::to_gray_rgb($self->value)) if $gs;
}

1;

=pod

=head1 NAME

Prima::Dialog::ColorDialog - standard color selection facilities

=head1 SYNOPSIS

	use Prima qw(Dialog::ColorDialog Application);

	my $p = Prima::Dialog::ColorDialog-> new(
		quality => 1,
	);
	printf "color: %06x", $p-> value if $p-> execute == mb::OK;

=for podview <img src="colordlg.png">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/Dialog/colordlg.png">

=head1 DESCRIPTION

The module contains two packages, C<Prima::Dialog::ColorDialog> and C<Prima::ColorComboBox>,
used as standard tools for the interactive color selection. C<Prima::ColorComboBox> is
a modified combo widget that provides selecting colors from a predefined palette, but also can
invoke a C<Prima::Dialog::ColorDialog> window.

=head1 Prima::Dialog::ColorDialog

=head2 Properties

=over

=item grayscale BOOLEAN

If set, allows only gray colors

=item quality BOOLEAN

The setting can increase the visual quality of the dialog if run on paletted displays.

Default value: 0

=item value COLOR

Selects the color represented by the color wheel and other dialog controls.

Default value: C<cl::White>

=back

=head2 Methods

=over

=item hsv2rgb HUE, SATURATION, LUMINOSITY

Converts a color from HSV to RGB format and returns three 8-bit integer values, red, green,
and blue components.

=item rgb2hsv RED, GREEN, BLUE

Converts color from RGB to HSV format and returns three numerical values, hue, saturation,
and luminosity components.

=item xy2hs X, Y, RADIUS

Maps X and Y coordinate values onto a color wheel with RADIUS in pixels.
The code uses RADIUS = 119 for mouse position coordinate mapping.
Returns three values, - hue, saturation, and error flag. If the error flag
is set, the conversion is failed.

=item hs2xy HUE, SATURATION

Maps hue and saturation onto a 256-pixel wide color wheel, and
returns X and Y coordinates of the corresponding point.

=item create_wheel SHADES, BACK_COLOR

Creates a color wheel with the number of SHADES given,
drawn on a BACK_COLOR background. Returns a C<Prima::DeviceBitmap> object.

=item create_wheel_shape SHADES

Creates a circular 1-bit mask with a radius derived from SHAPES.
SHAPES must be the same as passed to L<create_wheel>.
Returns a C<Prima::Image> object.

=back

=head2 Events

=over

=item BeginDragColor $PROPERTY

Called when the user starts dragging a color from the color wheel by the left
mouse button and an optional combination of Alt, Ctrl, and Shift keys.
$PROPERTY is one of the C<Prima::Widget> color properties, and depends on
a combination of the following keys:

	Alt              backColor
	Ctrl             color
	Alt+Shift        hiliteBackColor
	Ctrl+Shift       hiliteColor
	Ctrl+Alt         disabledColor
	Ctrl+Alt+Shift   disabledBackColor

The default action reflects the property to be changed in the dialog title

=item Change

The notification is called when the L<value> property is changed, either
interactively or as a result of a direct call.

=item EndDragColor $PROPERTY, $WIDGET

Called when the user releases the mouse button over a Prima widget.
The default action sets C<< $WIDGET->$PROPERTY >> to the selected color value.

=back

=head2 Variables

=over

=item $colorWheel

Contains the cached result of the L<create_wheel> method.

=item $colorWheelShape

Contains the cached result of the L<create_wheel_shape> method.

=back

=head1 Prima::ColorComboBox

=head2 Events

=over

=item Colorify INDEX, COLOR_PTR

C<nt::Action> callback, designed to map combo palette index into an RGB color.
INDEX is an integer from 0 to L<colors> - 1, COLOR_PTR is a reference to the
result scalar where the notification is expected to store the resulting color.

=back

=head2 Properties

=over

=item colors INTEGER

Defines the amount of colors in the fixed palette of the combo box.

=item grayscale BOOLEAN

If set, allows only gray colors

=item value COLOR

Contains the color selection as a 24-bit integer value.

=back

=head1 SEE ALSO

L<Prima>, L<Prima::ComboBox>, F<examples/cv.pl>.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=cut
