use strict;
use Const;
use Classes;
use Sliders;
use Label;
use Buttons;

package ColorDialog;
use vars qw( @ISA $colorWheel);
@ISA = qw( Window);

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

sub rgb2value
{
   my ( $r, $g, $b) = @_;
   return $b|($g << 8)|($r << 16);
}

sub value2rgb
{
   my $c = $_[0];
   return ( $c>>16) & 0xFF, ($c>>8) & 0xFF, $c & 0xFF;
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
   my ( $h, $s) = @_;
   my ( $r, $a) = ( 128 * $s, ($h - 180) / 57.295779513);
   return 128 + $r * sin( $a), 128 + $r * cos( $a);
}

sub create_wheel
{
   my ($id, $color)   = @_;
   my $imul = 256 / $id;
   my $i = Image-> create(
      width  => $id,
      height => $id,
      type => im::RGB,
      name => '',
   );

   my ( $y1, $x1) = $i-> size;
   my  $d0 = $i-> width / 2;

   $i-> set_size( 256, 256);

   $i-> begin_paint;
   $i-> color( cl::Black);
   $i-> bar( 0, 0, $i-> width, $i-> height);

   my ( $y, $x);

   for ( $y = 0; $y < $y1; $y++) {
      for ( $x = 0; $x < $x1; $x++) {
         my ( $h, $s, $ok) = xy2hs( $x, $y, $d0);
         next if $ok;
         my ( $r, $g, $b) = hsv2rgb( $h, $s, 1);
         $i-> color( $b|($g<<8)|($r<<16));
         $i-> bar( $x * $imul, $y * $imul, ( $x + 1) * $imul - 1, ( $y + 1) * $imul - 1);
      }
   }
   $i-> end_paint;


   my $a = Image-> create(
      width  => 256,
      height => 256,
      type   => im::RGB,
      name   => 'ColorWheel',
   );

   $a-> begin_paint;
   $a-> color( $color);
   $a-> bar( 0, 0, $a-> size);
   $a-> rop( rop::XorPut);
   $a-> put_image( 0, 0, $i);
   $a-> rop( rop::CopyPut);
   $a-> color( cl::Black);
   $a-> fill_ellipse(
     128, 128,
     128 - $imul - 1,
     128 - $imul - 1);
   $a-> rop( rop::XorPut);
   $a-> put_image( 0, 0, $i);
   $a-> end_paint;

   $i-> destroy;
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
      scaleChildren => 0,
      borderStyle   => bs::Dialog,
      borderIcons   => bi::SystemMenu | bi::TitleBar,
      widgetClass   => wc::Dialog,
      value         => cl::White,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self->{setTransaction} = undef;

   my $c = $self->{value} = $profile{value};
   my ( $r, $g, $b) = value2rgb( $c);
   my ( $h, $s, $v) = rgb2hsv( $r, $g, $b);
   $s *= 255;
   $v *= 255;

   $colorWheel = create_wheel(32, $self->backColor) unless $colorWheel;

   $self->{wheel} = $self->insert( Widget =>
      origin => [ 20, 172],
      width  => 256,
      height => 256,
      name   => 'Wheel',
      ownerBackColor => 1,
   );

   $self->{roller} = $self->insert( Widget =>
      origin    => [ 288, 164],
      width     => 48,
      height    => 272,
      buffered  => 1,
      name      => 'Roller',
      ownerBackColor => 1,
   );

   # RGB
   my %rgbprf = (
      width    => 72,
      max      => 255,
      onChange => sub { RGB_Change( $_[0]-> owner, $_[0]);},
   );
   $self->{R} = $self->insert( SpinEdit =>
      origin   => [40,120],
      value    => $r,
      name     => 'R',
      %rgbprf,
   );
   my %labelprf = (
      width      => 20,
      height     => $self->{R}-> height,
      autoWidth  => 0,
      autoHeight => 0,
      valignment => ta::Center,
   );
   $self-> insert( Label =>
      origin     => [ 20, 120],
      focusLink  => $self->{R},
      text       => 'R:',
      %labelprf,
   );
   $self->{G} = $self->insert( SpinEdit =>
      origin   => [148,120],
      value    => $g,
      name     => 'G',
      %rgbprf,
   );
   $self-> insert( Label =>
      origin     => [ 126, 120],
      focusLink  => $self->{G},
      text       => 'G:',
      %labelprf,
   );
   $self->{B} = $self->insert( SpinEdit =>
      origin   => [256,120],
      value    => $b,
      name     => 'B',
      %rgbprf,
   );
   $self-> insert( Label =>
      origin     => [ 236, 120],
      focusLink  => $self->{B},
      text       => 'B:',
      %labelprf,
   );

   $rgbprf{onChange} = sub { HSV_Change( $_[0]-> owner, $_[0])};
   $self->{H} = $self->insert( SpinEdit =>
      origin   => [ 40,78],
      value    => $h,
      name     => 'H',
      %rgbprf,
      max      => 360,
   );
   $self-> insert( Label =>
      origin     => [ 20, 78],
      focusLink  => $self->{H},
      text       => 'H:',
      %labelprf,
   );
   $self->{S} = $self->insert( SpinEdit =>
      origin   => [ 146,78],
      value    => int($s),
      name     => 'S',
      %rgbprf,
   );
   $self-> insert( Label =>
      origin     => [ 126, 78],
      focusLink  => $self->{S},
      text       => 'S:',
      %labelprf,
   );
   $self->{V} = $self->insert( SpinEdit =>
      origin   => [ 256,78],
      value    => int($v),
      name     => 'V',
      %rgbprf,
   );
   $self-> insert( Label =>
      origin     => [ 236, 78],
      focusLink  => $self->{V},
      text       => 'V:',
      %labelprf,
   );
   $self-> insert( Button =>
      text        => '~OK',
      origin      => [ 20, 20],
      modalResult => cm::OK,
      default     => 1,
   );

   $self-> insert( Button =>
      text        => 'Cancel',
      origin      => [ 126, 20],
      modalResult => cm::Cancel,
   );
   $self->{R}->select;

   return %profile;
}

sub set_color_index
{
   my ( $self, $color, $index) = @_;
   $self-> SUPER::set_color_index( $color, $index);
   if ( $index == ci::Back and $colorWheel) {
      $colorWheel-> destroy;
      $colorWheel = create_wheel(32, $self->backColor);
      $self->{wheel}->repaint;
   }
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
   my ( $r, $g, $b) = value2rgb( $self->{value});
   $r = $self->{R}->value if $pin == $self->{R};
   $g = $self->{G}->value if $pin == $self->{G};
   $b = $self->{B}->value if $pin == $self->{B};
   $self-> value( rgb2value( $r, $g, $b));
   undef $self->{RGBPin};
   undef $self->{setTransaction};
}

sub HSV_Change
{
   my ($self, $pin) = @_;
   return if $self-> {setTransaction};
   $self-> {setTransaction} = 1;
   my ( $h, $s, $v);
   $self->{HSVPin} = Hue | Lum | Sat | ( $pin == $self->{V} ? Wheel : 0);
   $h = $self->{H}->value      ;
   $s = $self->{S}->value / 255;
   $v = $self->{V}->value / 255;
   $self-> value( rgb2value( hsv2rgb( $h, $s, $v)));
   undef $self->{HSVPin};
   undef $self->{setTransaction};
}

sub Wheel_Paint
{
   my ( $owner, $self, $canvas) = @_;
   $canvas-> put_image( 0, 0, $colorWheel);
   my ( $x, $y) = hs2xy( $owner->{H}-> value, $owner->{S}-> value/273);
   $canvas-> color( cl::White);
   $canvas-> rop( rop::XorPut);
   $canvas-> lineWidth( 3);
   $canvas-> ellipse( $x, $y, 6, 6);
}

sub Wheel_MouseDown
{
   my ( $owner, $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransation};
   return if $btn != mb::Left and $btn != mb::Right;
   my ( $h, $s, $ok) = xy2hs( $x-9, $y-9, 119);
   return if $ok;
   $self->{mouseTransation} = $btn;
   $self-> capture(1);
   $self-> notify( "MouseMove", $mod, $x, $y) if $btn == mb::Left;
}

sub Wheel_MouseMove
{
   my ( $owner, $self, $mod, $x, $y) = @_;
   return if !$self->{mouseTransation} || $self->{mouseTransation} != mb::Left;
   my ( $h, $s, $ok) = xy2hs( $x-9, $y-9, 119);
   $owner-> {setTransaction} = 1;
   $owner-> {HSVPin} = Lum|Hue|Sat;
   $owner-> {H}-> value( int( $h));
   $owner-> {S}-> value( int( $s * 255));
   $owner-> value( rgb2value( hsv2rgb( int($h), $s, $owner->{V}->value/255)));
   $owner-> {HSVPin} = undef;
   $owner-> {setTransaction} = undef;
}

sub Wheel_MouseUp
{
   my ( $owner, $self, $btn, $mod, $x, $y) = @_;
   return unless $self->{mouseTransation};
   $self->{mouseTransation} = undef;
   $self-> capture(0);
   if ( $btn == mb::Right) {
      my $x = $::application-> get_view_from_point( $self-> client_to_screen( $x, $y));
      return unless $x;
      if ( $mod & kb::Shift) {
         $x-> backColor( $owner-> value);
      } else {
         $x-> color( $owner-> value);
      }
   }
}

sub Roller_Paint
{
   my ( $owner, $self, $canvas) = @_;
   my @size = $self-> size;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 0, 0, @size);
   my $i;
   my ( $h, $s, $v, $d) = ( $owner->{H}-> value, $owner->{S}->
                           value, $owner->{V}-> value, ($size[1]-16) / 32);
   $s /= 255;
   $v /= 255;
   my ( $r, $g, $b);

   for ( $i = 0; $i < 32; $i++) {
      ( $r, $g, $b) = hsv2rgb( $h, $s, $i / 31);
      $canvas-> color( rgb2value( $r, $g, $b));
      $canvas-> bar( 8, 8 + $i * $d, $size[0] - 8, 8 + ($i + 1) * $d);
   }

   $canvas-> color( cl::Black);
   $d = int( $v * ($size[1]-16));
   $canvas-> rectangle( 0, $d, $size[0]-1, $d + 15);
   $canvas-> color( $owner->{value});
   $canvas-> bar( 1, $d + 1, $size[0]-2, $d + 14);
}

sub Roller_MouseDown
{
   my ( $owner, $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransation};
   $self->{mouseTransation} = 1;
   $self-> capture(1);
   $self-> notify( "MouseMove", $mod, $x, $y);
}

sub Roller_MouseMove
{
   my ( $owner, $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransation};
   $owner-> {setTransaction} = 1;
   $owner-> {HSVPin} = Hue|Sat|Wheel;
   $owner-> value( rgb2value( hsv2rgb(
      $owner->{H}->value, $owner->{S}->value/255,
      ($y - 8) / ( $self-> height - 16))));
   $owner-> {HSVPin} = undef;
   $owner-> {setTransaction} = undef;
}

sub Roller_MouseUp
{
   my ( $owner, $self, $btn, $mod, $x, $y) = @_;
   return unless $self->{mouseTransation};
   $self->{mouseTransation} = undef;
   $self-> capture(0);
}


sub set_value
{
   my ( $self, $value) = @_;
   return if $value == $self->{value} and ! $self->{HSVPin};
   $self->{value} = $value;
   my $st = $self->{setTransaction};
   $self->{setTransaction} = 1;
   my $rgb = $self->{RGBPin} || 0;
   my $hsv = $self->{HSVPin} || 0;
   my ( $r, $g, $b) = value2rgb( $value);
   my ( $h, $s, $v) = rgb2hsv( $r, $g, $b);
   $s = int( $s*255);
   $v = int( $v*255);
   $self->{R}->value( $r) if $self->{R} != $rgb;
   $self->{G}->value( $g) if $self->{G} != $rgb;
   $self->{B}->value( $b) if $self->{B} != $rgb;
   $self->{H}->value( int($h)) unless $hsv & Hue;
   $self->{S}->value( int($s)) unless $hsv & Sat;
   $self->{V}->value( int($v)) unless $hsv & Lum;
   $self->{wheel}->repaint unless $hsv & Wheel;
   $self->{roller}->repaint unless $hsv & Roller;
   $self->{setTransaction} = $st;
}

sub value        {($#_)?$_[0]->set_value        ($_[1]):return $_[0]->{value};}


1;


