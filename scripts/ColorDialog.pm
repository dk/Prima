# contains:
#    ColorDialog
#    ColorComboBox

use strict;
use Const;
use Classes;
use Sliders;
use Label;
use Buttons;
use ComboBox;
use ScrollBar;

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

      quality       => 0,
      value         => cl::White,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self->{setTransaction} = undef;

   my $c = $self->{value} = $profile{value};
   $self->{quality} = 0;
   my ( $r, $g, $b) = value2rgb( $c);
   my ( $h, $s, $v) = rgb2hsv( $r, $g, $b);
   $s *= 255;
   $v *= 255;
   $h = int($h);
   $s = int($s);
   $v = int($v);

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
   $self-> quality( $profile{quality});

   $self-> Roller_Repaint if $self->{quality};

   return %profile;
}

sub set_color_index
{
   my ( $self, $color, $index) = @_;
   $self-> SUPER::set_color_index( $color, $index);
#   if ( $index == ci::Back and $colorWheel and $self->{wheel}) {
#      $colorWheel-> destroy;
#      $colorWheel = create_wheel(32, $self->backColor);
#      $self->{wheel}->repaint;
#   }
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
   $self->{HSVPin} = Hue | Lum | Sat | ( $pin == $self->{V} ? (Wheel|Roller) : 0);
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
   $canvas-> rectangle( 8, 8, $size[0] - 8, $size[1] - 8);
   $d = int( $v * ($size[1]-16));
   $canvas-> rectangle( 0, $d, $size[0]-1, $d + 15);
   $canvas-> color( $owner->{value});
   $canvas-> bar( 1, $d + 1, $size[0]-2, $d + 14);
   $self-> {paintPoll} = 2 if exists $self-> {paintPoll};
}

sub Roller_Repaint
{
   my $owner = $_[0];
   my $roller = $owner->{roller};
   if ( $owner-> {quality}) {
      my ( $h, $s, $v) = ( $owner->{H}-> value, $owner->{S}->value, $owner->{V}-> value);
      $s /= 255;
      $v /= 255;
      my ( $i, $r, $g, $b);
      my @pal = ();

      for ( $i = 0; $i < 32; $i++) {
         ( $r, $g, $b) = hsv2rgb( $h, $s, $i / 31);
         push ( @pal, $b, $g, $r);
      }
      ( $r, $g, $b) = value2rgb( $owner->{value});
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
   $owner-> {HSVPin} = Hue|Sat|Wheel|Roller;
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


sub set_quality
{
   my ( $self, $quality) = @_;
   return if $quality == $self->{quality};
   $self->{quality} = $quality;
   $self->{roller}-> palette([]) unless $quality;
   $self-> Roller_Repaint;
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
   if ( $hsv & Roller) {
      $self->{roller}-> repaint;
   } else {
      $self->Roller_Repaint;
   }
   $self->{setTransaction} = $st;
}

sub value        {($#_)?$_[0]->set_value        ($_[1]):return $_[0]->{value};}
sub quality      {($#_)?$_[0]->set_quality      ($_[1]):return $_[0]->{quality};}

package ColorComboBox;
use vars qw(@ISA);
@ISA = qw(ComboBox);

{
my %RNT = (
   %{Widget->notification_types()},
   Colorify => nt::Action,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   my %sup = %{$_[ 0]-> SUPER::profile_default};
   my @std = Application-> get_default_scrollbar_metrics;
   return {
      %sup,
      style            => cs::DropDownList,
      height           => $sup{ entryHeight},
      value            => cl::White,
      width            => 56,
      literal          => 0,
      colors           => 596,
      editClass        => 'Widget',
      listClass        => 'Widget',
      editProfile      => {
         selectingButtons => 0,
      },
      listProfile      => {
         width    => 78 + $std[0],
         height   => 130,
         growMode => 0,
      },
      onColorify    => undef,
   };
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $p->{ style} = cs::DropDownList;
   $self-> SUPER::profile_check_in( $p, $default);
}

sub init
{
   my $self    = shift;
   my %profile = @_;
   $self->{value} = $profile{value};
   $self->{colors} = $profile{colors};
   %profile = $self-> SUPER::init(%profile);
   $self-> {__DYNAS__}->{onColorify} = $profile{onColorify};
   $self-> colors( $profile{colors});
   $self-> value( $profile{value});
   return %profile;
}

sub set
{
   my ( $self, %profile) = @_;
   $self->{__DYNAS__}->{onColorify} = $profile{onColorify},
      delete $profile{onColorify} if exists $profile{onColorify};
   $self-> SUPER::set( %profile);
}


sub InputLine_KeyDown
{
   my ( $combo, $self, $code, $key) = @_;
   return unless $code;
   $combo-> listVisible(1), $self-> clear_event if $key == kb::Down;
   return if $key != kb::NoKey;
   $self-> clear_event;
}

sub InputLine_Paint
{
   my ( $combo, $self, $canvas, $w, $h, $focused) =
      ($_[0],$_[1],$_[2],$_[1]-> size, $_[1]->focused);
   my $back = $self->enabled ? $self-> backColor : $self-> disabledBackColor;
   my $clr  = $combo-> value;
   $clr = $back if $clr == cl::Invalid;
   $canvas-> rect3d( 0, 0, $w-1, $h-1, 1, $self-> light3DColor, $self-> dark3DColor);
   $canvas-> color( $back);
   $canvas-> rectangle( 1, 1, $w - 2, $h - 2);
   $canvas-> rectangle( 2, 2, $w - 3, $h - 3);
   $canvas-> color( $clr);
   $canvas-> bar( 3, 3, $w - 4, $h - 4);
   if ( $focused) {
      $canvas-> set(
         rop         => rop::XorPut,
         linePattern => lp::Dot,
         color       => cl::White
      );
      $canvas-> rectangle( 2, 2, $w - 3, $h - 3);
   }
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


sub InputLine_Enter { $_[1]-> repaint; }
sub InputLine_Leave { $_[1]-> repaint; }

sub List_Create
{
   my ($combo,$self) = @_;
   $combo-> {btn} = $self-> insert( Button =>
      origin     => [ 3, 3],
      width      => $self-> width - 6,
      height     => 28,
      text       => 'More...',
      selectable => 0,
      name       => 'MoreBtn',
      delegateTo => $combo,
   );
   my $c = $combo-> colors;
   $combo-> {scr} = $self-> insert( ScrollBar =>
      origin     => [ 75, $combo->{btn}-> height + 8],
      top        => $self-> height - 3,
      vertical   => 1,
      name       => 'Scroller',
      max        => $c > 20 ? $c - 20 : 0,
      partial    => 20,
      step       => 4,
      pageStep   => 20,
      whole      => $c,
      delegateTo => $combo,
   );
}


sub List_Paint
{
   my ( $combo, $self, $canvas) = @_;
   my ( $w, $h) = $self-> size;
   my @c3d = ( $self-> light3DColor, $self-> dark3DColor);
   $canvas-> rect3d( 0, 0, $w-1, $h-1, 1, @c3d, $self-> backColor)
      unless exists $self->{inScroll};
   my $i;
   my $pc = 18;
   my $dy = $combo-> {btn}-> height;

   my $maxc = $combo-> colors;
   my $shft = $combo->{scr}-> value;
   for ( $i = 0; $i < 20; $i++) {
      next if $i >= $maxc;
      my ( $x, $y) = (($i % 4) * $pc + 3, ( 4 - int( $i / 4)) * $pc + 9 + $dy);
      my $clr = 0;
      $combo-> notify('Colorify', $i + $shft, \$clr);
      $canvas-> rect3d( $x, $y, $x + $pc - 2, $y + $pc - 2, 1, @c3d, $clr);
   }
}

sub List_MouseDown
{
   my ( $combo, $self, $btn, $mod, $x, $y) = @_;
   $x -= 3;
   $y -= $combo->{btn}-> height + 9;
   return if $x < 0 || $y < 0;
   $x = int($x / 18);
   $y = int($y / 18);
   return if $x > 3 || $y > 4;
   $y = 4 - $y;
   $combo-> listVisible(0);
   my $shft = $combo->{scr}-> value;
   my $maxc = $combo-> colors;
   my $xcol = $shft + $x + $y * 4;
   return if $xcol >= $maxc;
   my $xval = 0;
   $combo-> notify('Colorify', $xcol, \$xval);
   $combo-> value( $xval);
}

sub MoreBtn_Click
{
   my ($combo,$self) = @_;
   my $d;
   $combo-> listVisible(0);
   $d = ColorDialog-> create(
      value => $combo-> value,
   );
   $combo-> value( $d-> value) if $d-> execute;
   $d-> destroy;
}

sub Scroller_Change
{
   my ($combo,$self) = @_;
   $self = $combo-> List;
   $self->{inScroll} = 1;
   $self-> invalidate_rect(
      4, $combo->{btn}->top+6,
      $self->width - $combo-> {scr}-> width,
      $self->height - 3,
   );
   delete $self->{inScroll};
}


sub set_style { $_[0]-> raise_ro('set_style')}

sub set_value
{
   my ( $self, $value) = @_;
   return if $value == $self->{value};
   $self-> {value} = $value;
   $self-> repaint;
}

sub set_colors
{
   my ( $self, $value) = @_;
   return if $value == $self->{colors};
   $self-> {colors} = $value;
   my $scr = $self->{list}->{scr};
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
   if ( $index < 20) {
      $$sref = $palColors[ $index];
   } else {
       my $i = $index - 20;
       my ( $r, $g, $b, $m);
       if ( $i < 64) {
          ( $r, $g, $b) = ( $i & 0x3, ($i >> 2) & 0x3, ( $i >> 4) & 0x3);
          $m = 64;
       } else {
          $i -= 64;
          ( $r, $g, $b) = ( $i & 0x7, ($i >> 3) & 0x7, ( $i >> 6) & 0x7);
          $m = 32;
       }
       $$sref = ($b * $m) | ( $g * $m) << 8 | ( $r * $m) << 16;
   }
   $self-> clear_event;
}


sub value        {($#_)?$_[0]->set_value       ($_[1]):return $_[0]->{value};  }
sub colors       {($#_)?$_[0]->set_colors      ($_[1]):return $_[0]->{colors};  }


1;


