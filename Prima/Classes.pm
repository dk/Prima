#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
use Prima;
use Prima::Const;

# class Object; base class of all Prima classes
package Prima::Object;
use strict;
use Carp;


sub debug_write
{
   shift if ref $_[0];
   open( F, ">>c:/POPUPHOG.OS2");
   print F @_, "\n";
   close( F);
}

sub CREATE
{
   my $class = shift;
   my $self = {};
   bless( $self, $class);
   return $self;
}

sub DESTROY
{
   my $self = shift;
   my $class = ref( $self);
   ::destroy_mate( $self);
}


sub profile_add
{
   my ($self,$profile) = @_;
   my $default  = $_[0]-> profile_default;
   if (0){
   for ( keys %$default)
   {
      next unless /^-(.*)$/;
      next if exists $default->{$1};      # ignore '-' in this case
      $default->{$1}=$default->{$_};
      delete $default->{$_};
   }
   for ( keys %$profile)
   {
      next unless /^-(.*)$/;
      next if exists $profile->{$1};      # ignore '-' in this case
      $profile->{$1}=$profile->{$_};
      delete $profile->{$_};
   }
   }

   $self-> profile_check_in( $profile, $default);
   delete @$default{keys %$profile};
   @$profile{keys %$default}=values %$default;
   delete $profile->{__ORDER__};
   $profile->{__ORDER__} = [keys %$profile];
#   %$profile = (%$default, %$profile);
}

sub profile_default
{
   return {};
}

sub profile_check_in {};

sub raise_ro { croak "Attempt to write read-only property \"$_[1]\""; }
sub raise_wo { croak "Attempt to read write-only property \"$_[1]\""; }

sub set {
   for ( my $i = 1; $i < @_; $i += 2) {
      my $sub_set = $_[$i];
      $_[0]-> $sub_set( $_[$i+1]);
   }
   return;
}

package Prima::Font;

sub new
{
   my $class = shift;
   my $self = { OWNER=>shift, READ=>shift, WRITE=>shift};
   bless( $self, $class);
   my ($o,$r,$w) = @{$self}{"OWNER","READ","WRITE"};
   my $f = $o->$r();
   $self-> update($f);
   return $self;
}

sub update
{
   my ( $self, $f) = @_;
   for ( keys %{$f}) { $self->{$_} = $f->{$_}; }
}

sub set
{
   my ($o,$r,$w) = @{$_[0]}{"OWNER","READ","WRITE"};
   my ($self, %pr) = @_;
   $self-> update( \%pr);
   $o->$w( \%pr);
}

for ( qw( size name width height direction style pitch)) {
   eval <<GENPROC;
   sub $_
   {
      my (\$o,\$r,\$w) = \@{\$_[0]}{"OWNER","READ","WRITE"};
      my \$font = \$#_ ? {$_ => \$_[1]} : \$o->\$r();
      return \$#_ ? (\$_[0]->update(\$font), \$o->\$w(\$font)) : \$font->{$_};
   }
GENPROC
}

for ( qw( ascent descent family weight maximalWidth internalLeading externalLeading
          xDeviceRes yDeviceRes firstChar lastChar breakChar defaultChar vector
   )) {
   eval <<GENPROC;
   sub $_
   {
      my (\$o,\$r) = \@{\$_[0]}{"OWNER","READ"};
      my \$font = \$o->\$r();
      return \$#_ ? Prima::Object-> raise_ro("Font::$_") : \$font->{$_};
   }
GENPROC
}


sub DESTROY {}

# class Component
package Prima::Component;
use vars qw(@ISA);
@ISA = qw(Prima::Object);

{
my %RNT = (
   Create      => nt::Default,
   Destroy     => nt::Default,
   PostMessage => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      name        => ref $_[ 0],
      owner       => $::application,
      delegations => undef,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   my $owner = $p-> { owner} ? $p-> { owner} : $default->{ owner};
   $self-> SUPER::profile_check_in( $p, $default);
   if ( defined $owner) {
     unless ( exists( $p-> { name}) || $default-> { name} ne ref $self) {
        $p-> { name} = ( ref $self) . ( 1 + map { (ref $self) eq (ref $_) ? 1 : () } $owner-> get_components);
        $p-> { name} =~ s/(.*):([^:]+)$/$2/;
     }
   }
}

sub set_owner  { $_[0]->set( owner => $_[1]);}

sub name       {($#_)?$_[0]->set_name        ($_[1]):return $_[0]->get_name;        }
sub owner      {($#_)?$_[0]->set_owner       ($_[1]):return $_[0]->get_owner;       }
sub delegations{($#_)?$_[0]->set_delegations ($_[1]):return $_[0]->get_delegations; }

sub get_notify_sub
{
   my ($self, $note) = @_;
   my $rnt = $self->notification_types->{$note};
   $rnt = nt::Default unless defined $rnt;
   if ( $rnt & nt::CustomFirst) {
      my ( $referer, $sub, $id) = $self-> get_notification( $note, ($rnt & nt::FluxReverse) ? -1 : 0);
      if ( defined $referer) {
         return ( $sub, $referer, $self) if $referer != $self;
         return ( $sub, $self);
      }
      my $method = "on_" . lc $note;
      return ( $sub, $self) if $sub = $self-> can( $method, 0);
   } else {
      my ( $sub, $method) = ( undef, "on_" . lc $note);
      return ( $sub, $self) if $sub = $self-> can( $method, 0);
      my ( $referer, $sub2, $id) = $self-> get_notification( $note, ($rnt & nt::FluxReverse) ? -1 : 0);
      if ( defined $referer) {
         return ( $sub, $referer, $self) if $referer != $self;
         return ( $sub, $self);
      }
   }
   return undef;
}

sub AUTOLOAD
{
   no strict;
   my $self = shift;
   my $expectedMethod = $AUTOLOAD;
   Carp::confess "There is no such thing as \"$expectedMethod\"\n" if scalar @_;
   my ($componentName) = $expectedMethod =~ /::([^:]+)$/;
   my $component = $self-> bring( $componentName);
   Carp::croak("Unknown widget or method \"$expectedMethod\"") unless $component && ref($component);
   return $component;
}

package Prima::Clipboard;
use vars qw(@ISA);
@ISA = qw(Prima::Component);

sub text  { $#_ ? $_[0]-> store( 'Text',  $_[1]) : return $_[0]-> fetch('Text')  }
sub image { $#_ ? $_[0]-> store( 'Image', $_[1]) : return $_[0]-> fetch('Image') }

# class Drawable
package Prima::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::Component);

my $sysData = Prima::Application-> get_system_info;

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      color           => cl::Black,
      backColor       => cl::White,
      fillPattern     => fp::Solid,
      font            => {
         height      => 16,
         width       => 0,
         pitch       => fp::Default,
         style       => fs::Normal,
         aspect      => 1,
         direction   => 0,
         name        => "Helv"
      },
      lineEnd         => le::Round,
      linePattern     => lp::Solid,
      lineWidth       => 0,
      palette         => [],
      region          => undef,
      rop             => rop::CopyPut,
      rop2            => rop::NoOper,
      textOutBaseline => 0,
      textOpaque      => 0,
      transform       => [ 0, 0],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   $p-> { font} = {} unless exists $p->{ font};
   $p->{ font} = Prima::Drawable-> font_match( $p->{ font}, $default->{ font});
}

sub color      {($#_)?$_[0]->set_color       ($_[1]):return $_[0]->get_color;       }
sub clipRect   {($#_)?shift->set_clip_rect   (@_   ):return $_[0]->get_clip_rect;   }
sub backColor  {($#_)?$_[0]->set_back_color  ($_[1]):return $_[0]->get_back_color;  }
sub fillPattern{($#_)?($#_>1?shift->set_fill_pattern(@_):shift->set_fill_pattern_id(@_)):return $_[0]->get_fill_pattern;}
sub lineEnd    {($#_)?$_[0]->set_line_end    ($_[1]):return $_[0]->get_line_end;    }
sub linePattern{($#_)?$_[0]->set_line_pattern($_[1]):return $_[0]->get_line_pattern;}
sub lineWidth  {($#_)?$_[0]->set_line_width  ($_[1]):return $_[0]->get_line_width;  }
sub textOutBaseline{($#_)?$_[0]->set_text_out_baseline($_[1]):return $_[0]->get_text_out_baseline;  }
sub textOpaque {($#_)?$_[0]->set_text_opaque ($_[1]):return $_[0]->get_text_opaque; }
sub font       {($#_)?$_[0]->set_font($#_>1?{@_[1..$#_]}:$_[1]):return Prima::Font->new($_[0], "get_font", "set_font")}
sub palette    {($#_)?$_[0]->set_palette      ($_[1]):return $_[0]->get_palette;    }
sub region     {($#_)?$_[0]->set_region      ($_[1]):return $_[0]->get_region;      }
sub rop        {($#_)?$_[0]->set_rop         ($_[1]):return $_[0]->get_rop;         }
sub rop2       {($#_)?$_[0]->set_rop2        ($_[1]):return $_[0]->get_rop2;        }
sub resolution {($#_)?$_[0]->raise_ro("resolution") :return $_[0]->get_resolution;  }
sub width      {($#_)?$_[0]->raise_ro("width")      :return $_[0]->get_width;       }
sub height     {($#_)?$_[0]->raise_ro("height")     :return $_[0]->get_height;      }
sub size       {($#_)?$_[0]->raise_ro("size")       :return $_[0]->get_size;        }
sub transform  {($#_)? ($_[0]->set_transform($#_>1?@_[1..$#_]:@{$_[1]})):return $_[0]->get_transform;    }
sub pixel      {($#_>2)? shift->set_pixel(@_) : return shift->get_pixel(@_);        }

sub rect3d
{
   my ( $self, $x, $y, $x1, $y1, $width, $lColor, $rColor, $backColor) = @_;
   my $c = $self-> color;
   if ( defined $backColor)
   {
      $self-> color( $backColor);
      $self-> bar( $x + $width, $y + $width, $x1 - $width, $y1 - $width);
   }
   $lColor = $rColor = cl::Black if $self-> get_bpp == 1;
   $self-> color( $c), return if $width <= 0;
   $self-> color( $lColor);
   $width = ( $y1 - $y) / 2 if $width > ( $y1 - $y) / 2;
   $width = ( $x1 - $x) / 2 if $width > ( $x1 - $x) / 2;
   $self-> lineWidth( 0);
   my $i;
   for ( $i = 0; $i < $width; $i++)
   {
      $self-> line( $x + $i, $y + $i, $x + $i, $y1 - $i);
      $self-> line( $x + $i + 1, $y1 - $i, $x1 - $i, $y1 - $i);
   }
   $self-> color( $rColor);
   for ( $i = 0; $i < $width; $i++)
   {
      $self-> line( $x1 - $i, $y + $i, $x1 - $i, $y1 - $i);
      $self-> line( $x + $i + 1, $y + $i, $x1 - $i, $y + $i);
   }
   $self-> color( $c);
}

sub rect_focus
{
   my ( $canvas, $x, $y, $x1, $y1, $width) = @_;
   ( $x, $x1) = ( $x1, $x) if $x > $x1;
   ( $y, $y1) = ( $y1, $y) if $y > $y1;

   $width = 1 if !defined $width || $width < 1;
   if (( $width == 1) && (( $sysData->{apc} != apc::Win32) or ( $sysData->{system} eq 'Windows NT'))) {
      my ( $lp, $cl, $cl2, $rop) = (
         $canvas-> linePattern, $canvas-> color,
         $canvas-> backColor, $canvas-> rop
      );
      $canvas-> set(
         linePattern => lp::DotDot,
         color       => cl::White,
         backColor   => cl::Black,
         rop         => rop::XorPut,
      );
      $canvas-> rectangle( $x, $y, $x1, $y1);
      $canvas-> set(
         linePattern => $lp,
         backColor   => $cl2,
         color       => $cl,
         rop         => $rop
      );
   } else {
      my ( $cl, $cl2, $rop) = (
         $canvas-> color, $canvas-> backColor, $canvas-> rop
      );
      my @fp = $canvas-> fillPattern;
      $canvas-> set(
         fillPattern => fp::SimpleDots,
         color       => cl::White,
         backColor   => cl::Black,
         rop         => rop::XorPut,
      );

      if ( $width * 2 >= $x1 - $x or $width * 2 >= $y1 - $y) {
         $canvas-> bar( $x, $y, $x1, $y1);
      } else {
         $width -= 1;
         $canvas-> bar( $x, $y, $x1, $y + $width);
         $canvas-> bar( $x, $y1 - $width, $x1, $y1);
         $canvas-> bar( $x, $y + $width + 1, $x + $width, $y1 - $width - 1);
         $canvas-> bar( $x1 - $width, $y + $width + 1, $x1, $y1 - $width - 1);
      }

      $canvas-> set(
         fillPattern => [@fp],
         backColor   => $cl2,
         color       => $cl,
         rop         => $rop,
      );
   }
}

sub draw_text
{
   my ( $canvas, $string, $x, $y, $x2, $y2, $flags, $tabIndent) = @_;

   $flags     = dt::Default unless defined $flags;
   $tabIndent = 1 if !defined( $tabIndent) || $tabIndent < 0;

   $x2 = int( $x2);
   $x  = int( $x);
   $y2 = int( $y2);
   $y  = int( $y);

   my ( $w, $h) = ( $x2 - $x + 1, $y2 - $y + 1);

   return 0 if $w <= 0 || $h <= 0;

   my $twFlags = tw::ReturnLines |
      (( $flags & dt::DrawMnemonic  ) ? ( tw::CalcMnemonic | tw::CollapseTilde) : 0) |
      (( $flags & dt::DrawSingleChar) ? 0 : tw::BreakSingle ) |
      (( $flags & dt::NewLineBreak  ) ? tw::NewLineBreak : 0) |
      (( $flags & dt::SpaceBreak    ) ? tw::SpaceBreak   : 0) |
      (( $flags & dt::WordBreak     ) ? tw::WordBreak    : 0) |
      (( $flags & dt::ExpandTabs    ) ? ( tw::ExpandTabs | tw::CalcTabs) : 0)
   ;

   my @lines = @{$canvas-> text_wrap( $string, ( $flags & dt::NoWordWrap) ? -1 : $w, $twFlags, $tabIndent)};
   my $tildes;
   $tildes = pop @lines if $flags & dt::DrawMnemonic;

   return 0 unless scalar @lines;

   my @clipSave;
   my $fh = $canvas-> font-> height +
            (( $flags & dt::QueryHeight) ? $canvas-> font-> externalLeading : 0);
   my ( $linesToDraw, $retVal);
   my $valign = $flags & 0xC;

   if ( $flags & dt::QueryHeight) {
      $linesToDraw = scalar @lines;
      $h = $retVal = $linesToDraw * $fh;
   } else {
      $linesToDraw = int( $retVal = ( $h / $fh));
      $linesToDraw++ if (( $h % $fh) > 0) and ( $flags & dt::DrawPartial);
      $valign      = dt::Top if $linesToDraw < scalar @lines;
      $linesToDraw = $retVal = scalar @lines if $linesToDraw > scalar @lines;
   }

   if ( $flags & dt::UseClip) {
      @clipSave = $canvas-> clipRect;
      $canvas-> clipRect( $x, $y, $x + $w, $y + $h);
   }

   if ( $valign == dt::Top) {
      $y = $y2;
   } elsif ( $valign == dt::VCenter) {
      $y = $y2 - int(( $h - $linesToDraw * $fh) / 2);
   } else {
      $y += $linesToDraw * $fh;
   }

   my ( $starty, $align) = ( $y, $flags & 0x3);

   for ( @lines) {
      last unless $linesToDraw--;
      my $xx;
      if ( $align == dt::Left) {
         $xx = $x;
      } elsif ( $align == dt::Center) {
         $xx = $x + int(( $w - $canvas-> get_text_width( $_)) / 2);
      } else {
         $xx = $x2 - $canvas-> get_text_width( $_);
      }
      $y -= $fh;
      $canvas-> text_out( $_, $xx, $y);
   }

   if (( $flags & dt::DrawMnemonic) and ( $tildes->{tildeLine} >= 0)) {
      my $tl = $tildes->{tildeLine};
      my $xx = $x;
      if ( $align == dt::Center) {
         $xx = $x + int(( $w - $canvas-> get_text_width( $lines[ $tl])) / 2);
      } elsif ( $align == dt::Right) {
         $xx = $x2 - $canvas-> get_text_width( $lines[ $tl]);
      }
      $tl++;
      $canvas-> line( $xx + $tildes->{tildeStart}, $starty - $fh * $tl,
                      $xx + $tildes->{tildeEnd}  , $starty - $fh * $tl);
   }

   $canvas-> clipRect( @clipSave) if $flags & dt::UseClip;

   return $retVal;
}

# class Image
package Prima::Image;
use vars qw( @ISA);
@ISA = qw(Prima::Drawable);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      conversion    => ict::Halftone,
      height        => 0,
      palette       => [0,0,0,0xFF,0xFF,0xFF],
      preserveType  => 0,
      rangeLo       => 0,
      rangeHi       => 1,
      type          => im::Mono,
      width         => 0,
      hScaling      => 1,
      vScaling      => 1,
      data          => '',
      resolution    => [ 0, 0],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub set_type       {$_[0]->set( type    =>$_[1])}

sub data         {($#_)?$_[0]->set_data         ($_[1]):return $_[0]->get_data;         }
sub conversion   {($#_)?$_[0]->set_conversion   ($_[1]):return $_[0]->get_conversion;   }
sub height       {($#_)?$_[0]->set_height       ($_[1]):return $_[0]->get_height;       }
sub rangeLo      {($#_)?$_[0]->set_stats  ($_[1], is::RangeLo):return $_[0]->get_stats(is::RangeLo); }
sub rangeHi      {($#_)?$_[0]->set_stats  ($_[1], is::RangeHi):return $_[0]->get_stats(is::RangeHi); }
sub sum          {($#_)?$_[0]->set_stats  ($_[1], is::Sum    ):return $_[0]->get_stats(is::Sum); }
sub sum2         {($#_)?$_[0]->set_stats  ($_[1], is::Sum2   ):return $_[0]->get_stats(is::Sum2); }
sub mean         {($#_)?$_[0]->set_stats  ($_[1], is::Mean   ):return $_[0]->get_stats(is::Mean); }
sub variance     {($#_)?$_[0]->set_stats  ($_[1], is::Variance):return $_[0]->get_stats(is::Variance); }
sub stdDev       {($#_)?$_[0]->set_stats  ($_[1], is::StdDev ):return $_[0]->get_stats(is::StdDev); }
sub type         {($#_)?$_[0]->set_type         ($_[1]):return $_[0]->get_type;         }
sub width        {($#_)?$_[0]->set_width        ($_[1]):return $_[0]->get_width;        }
sub size         {($#_)?$_[0]->set_size   ($_[1],$_[2]):return $_[0]->get_size;         }
sub palette      {($#_)?$_[0]->set_palette      ($_[1]):return $_[0]->get_palette;      }
sub preserveType {($#_)?$_[0]->set_preserve_type($_[1]):return $_[0]->get_preserve_type;}
sub hScaling     {($#_)?$_[0]->set_h_scaling    ($_[1]):return $_[0]->get_h_scaling    ;}
sub vScaling     {($#_)?$_[0]->set_v_scaling    ($_[1]):return $_[0]->get_v_scaling    ;}
sub resolution   {($#_)?shift->set_resolution   (@_)   :return $_[0]->get_resolution   ;}

# class Icon
package Prima::Icon;
use vars qw( @ISA);
@ISA = qw(Prima::Image);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      mask  => '',
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub mask         {($#_)?$_[0]->set_mask         ($_[1]):return $_[0]->get_mask;         }

# class DeviceBitmap
package Prima::DeviceBitmap;
use vars qw( @ISA);
@ISA = qw(Prima::Drawable);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      height       => 0,
      width        => 0,
      monochrome   => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub monochrome {($#_)?$_[0]->raise_ro("monochrome") :return $_[0]->get_monochrome;}

# class Timer
package Prima::Timer;
use vars qw(@ISA);
@ISA = qw(Prima::Component);

{
my %RNT = (
   %{Prima::Component->notification_types()},
   Tick => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      timeout => 1000,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub timeout    {($#_)?$_[0]->set_timeout     ($_[1]):return $_[0]->get_timeout;     }

# class Printer
package Prima::Printer;
use vars qw(@ISA);
@ISA = qw(Prima::Drawable);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      printer => '',
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

# class Widget
package Prima::Widget;
use vars qw(@ISA %WidgetProfile);
@ISA = qw(Prima::Drawable);

{
my %RNT = (
   %{Prima::Drawable->notification_types()},
   Change         => nt::Default,
   Click          => nt::Default,
   Close          => nt::Command,
   ColorChanged   => nt::Default,
   Disable        => nt::Default,
   DragDrop       => nt::Default,
   DragOver       => nt::Default,
   Enable         => nt::Default,
   EndDrag        => nt::Default,
   Enter          => nt::Default,
   FontChanged    => nt::Default,
   Help           => nt::Default,
   Hide           => nt::Default,
   Hint           => nt::Default,
   KeyDown        => nt::Command,
   KeyUp          => nt::Command,
   Leave          => nt::Default,
   Menu           => nt::Default,
   MouseClick     => nt::Command,
   MouseDown      => nt::Command,
   MouseUp        => nt::Command,
   MouseMove      => nt::Command,
   MouseWheel     => nt::Command,
   MouseEnter     => nt::Command,
   MouseLeave     => nt::Command,
   Move           => nt::Default,
   Paint          => nt::Action,
   Popup          => nt::Command,
   Setup          => nt::Default,
   Show           => nt::Default,
   Size           => nt::Default,
   TranslateAccel => nt::Default,
   ZOrderChanged  => nt::Default,
);

sub notification_types { return \%RNT; }
}

%WidgetProfile = (
   accelTable        => undef,
   accelItems        => undef,
   backColor         => cl::Normal,
   briefKeys         => 1,
   buffered          => 0,
   capture           => 0,
   clipOwner         => 1,
   color             => cl::NormalText,
   bottom            => 100,
   centered          => 0,
   current           => 0,
   currentWidget     => undef,
   cursorVisible     => 0,
   dark3DColor       => cl::Dark3DColor,
   disabledBackColor => cl::Disabled,
   disabledColor     => cl::DisabledText,
   enabled           => 1,
   firstClick        => 0,
   focused           => 0,
   growMode          => 0,
   height            => 100,
   helpContext       => hmp::Owner,
   hiliteBackColor   => cl::Hilite,
   hiliteColor       => cl::HiliteText,
   hint              => '',
   hintVisible       => 0,
   light3DColor      => cl::Light3DColor,
   left              => 100,
   ownerColor        => 0,
   ownerBackColor    => 0,
   ownerFont         => 1,
   ownerHint         => 1,
   ownerShowHint     => 1,
   ownerPalette      => 1,
   pointerIcon       => undef,
   pointer           => cr::Default,
   pointerType       => cr::Default,
   pointerVisible    => 1,
   popup             => undef,
   popupColor             => cl::NormalText,
   popupBackColor         => cl::Normal,
   popupHiliteColor       => cl::HiliteText,
   popupHiliteBackColor   => cl::Hilite,
   popupDisabledColor     => cl::DisabledText,
   popupDisabledBackColor => cl::Disabled,
   popupLight3DColor      => cl::Light3DColor,
   popupDark3DColor       => cl::Dark3DColor,
   popupItems        => undef,
   right             => 200,
   scaleChildren     => 1,
   selectable        => 0,
   selected          => 0,
   selectedWidget    => undef,
   selectingButtons  => mb::Left,
   shape             => undef,
   showHint          => 1,
   syncPaint         => 1,
   tabOrder          => -1,
   tabStop           => 1,
   text              => undef,
   textOutBaseline   => 0,
   top               => 200,
   transparent       => 0,
   visible           => 1,
   widgetClass       => wc::Custom,
   width             => 100,
   x_centered        => 0,
   y_centered        => 0,
);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;

   @$def{keys %WidgetProfile} = values %WidgetProfile;

   my %WidgetProfile = (     # secondary; contains anonymous array; must be generated at every invocation
      cursorPos         => [ 0, 0],
      cursorSize        => [ 12, 3],
      designScale       => [ 0, 0],
      origin            => [ 0, 0],
      pointerHotSpot    => [ 0, 0],
      rect              => [ 0, 0, 100, 100],
      size              => [ 100, 100],
      sizeMin           => [ 0, 0],
      sizeMax           => [ 16384, 16384],
   );
   @$def{keys %WidgetProfile} = values %WidgetProfile;
   @$def{qw( font popupFont)} = ( $_[ 0]-> get_default_font, $_[ 0]-> get_default_popup_font);
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   my $orgFont = exists $p->{ font} ? $p->{ font} : undef;
   my $owner = exists $p-> { owner} ? $p-> { owner} : $default-> { owner};
   $self-> SUPER::profile_check_in( $p, $default);
   delete $p->{ font} unless defined $orgFont;

   $p-> {text} = ( defined $p-> {name} ? $p-> {name} : $default->{name})
      if !defined $p-> { text} and !defined $default->{text};

   $p->{showHint} = 1 if ( defined $owner) && ( defined $::application) && ( $owner == $::application) &&
      ( exists $p->{ ownerShowHint} ? $p->{ ownerShowHint} : $default->{ ownerShowHint});

   for ( $owner ? qw( color backColor showHint hint font): ())
   {
      my $o_ = 'owner' . ucfirst $_;
      $p->{ $_} = $owner-> $_() if
         ( $p->{ $o_} = exists $p->{ $_} ? 0 :
             ( exists $p-> { $o_} ? $p->{ $o_} : $default-> {$o_}));
   }
   for ( qw( font popupFont))
   {
      $p-> { $_} = {} unless exists $p->{ $_};
      $p->{ $_} = Prima::Widget-> font_match( $p->{ $_}, $default->{ $_});
   }

   if ( exists( $p-> { origin}))
   {
      $p-> { left  } = $p-> { origin}-> [ 0];
      $p-> { bottom} = $p-> { origin}-> [ 1];
   }

   if ( exists( $p-> { rect})) {
      my $r = $p-> { rect};
      $p-> { left  } = $r-> [ 0];
      $p-> { bottom} = $r-> [ 1];
      $p-> { right } = $r-> [ 2];
      $p-> { top   } = $r-> [ 3];
   }

   if ( exists( $p-> { size})) {
      $p-> { width } = $p-> { size}-> [ 0];
      $p-> { height} = $p-> { size}-> [ 1];
   }

   my $designScale = exists $p->{designScale} ? $p->{designScale} : $default->{designScale};
   if ( defined $designScale) {
      my @defScale = @$designScale;
      if (( $defScale[0] > 0) && ( $defScale[1] > 0)) {
         @{$p-> { designScale}} = @defScale;
         for ( qw ( left right top bottom width height)) {
            $p->{$_} = $default->{$_} unless exists $p->{$_};
         }
      } else {
         @defScale = $owner-> get_design_scale if defined $owner && $owner-> scaleChildren;
         @{$p-> { designScale}} = @defScale if ( $defScale[0] > 0) && ( $defScale[1] > 0);
      }
      if ( exists $p-> { designScale}) {
         my @d = @{$p->{ designScale}};
         my @a = ( $p->{ font}->{ width}, $p->{ font}->{ height});
         $p->{left}    *= $a[0] / $d[0] if exists $p->{left};
         $p->{right}   *= $a[0] / $d[0] if exists $p->{right};
         $p->{top}     *= $a[1] / $d[1] if exists $p->{top};
         $p->{bottom}  *= $a[1] / $d[1] if exists $p->{bottom};
         $p->{width}   *= $a[0] / $d[0] if exists $p->{width};
         $p->{height}  *= $a[1] / $d[1] if exists $p->{height};
      }
   } else {
      $p-> {designScale} = [0,0];
   }

   $p-> { top} = $default->{ bottom} + $p-> { height}
     if ( !exists ( $p-> { top}) && !exists( $p-> { bottom}) && exists( $p-> { height}));
   $p-> { height} = $p-> { top} - $p-> { bottom}
     if ( !exists( $p-> { height}) && exists( $p-> { top}) && exists( $p-> { bottom}));
   $p-> { top} = $p-> { bottom} + $p-> { height}
     if ( !exists( $p-> { top}) && exists( $p-> { height}) && exists( $p-> { bottom}));
   $p-> { bottom} = $p-> { top} - $p-> { height}
     if ( !exists( $p-> { bottom}) && exists( $p-> { height}) && exists( $p-> { top}));
   $p-> { bottom} = $p-> { top} - $default->{ height}
     if ( !exists( $p-> { bottom}) && !exists( $p-> { height}) && exists( $p-> { top}));
   $p-> { top} = $p-> { bottom} + $default-> { height}
     if ( !exists( $p-> { top}) && !exists( $p-> { height}) && exists( $p-> { bottom}));


   $p-> { right} = $default->{ left} + $p-> { width}
     if ( !exists( $p-> { right}) && !exists( $p-> { left}) && exists( $p-> { width}));
   $p-> { width} = $p-> { right} - $p-> { left}
     if ( !exists( $p-> { width}) && exists( $p-> { right}) && exists( $p-> { left}));
   $p-> { right} = $p-> { left} + $p-> { width}
     if ( !exists( $p-> { right}) && exists( $p-> { width}) && exists( $p-> { left}));
   $p-> { left}  = $p-> { right} - $p-> { width}
     if ( !exists( $p-> { left}) && exists( $p-> { right}) && exists( $p-> { width}));
   $p-> { left}  = $p-> { right} - $default->{width}
     if ( !exists( $p-> { left}) && !exists( $p->{ width}) && exists($p->{right}));
   $p-> { right} = $p-> { left} + $default-> { width}
     if ( !exists( $p-> { right}) && !exists( $p-> { width}) && exists( $p-> { left}));

   if ( $p-> { popup}) {
      $p->{ popupItems} = $p->{popup}->get_items('');
      delete $p-> {popup};
   }

   my $current = exists $p-> { current} ? $p-> { current} : $default-> { current};
   if ( defined($owner) && !$current && !$owner-> currentWidget) {
      my $ena = exists $p-> { enabled} ? $p-> { enabled} : $default-> { enabled};
      my $vis = exists $p-> { visible} ? $p-> { visible} : $default-> { visible};
      $p->{current} = 1 if $ena && $vis;
   }

   if ( exists $p-> {pointer}) {
      my $pt = $p-> {pointer};
      $p->{pointerType}    = ( ref($pt) ? cr::User : $pt) if !exists $p->{pointerType};
      $p->{pointerIcon}    = $pt if !exists $p->{pointerIcon} && ref( $pt);
      $p->{pointerHotSpot} = $pt->{__pointerHotSpot}
         if !exists $p->{pointerHotSpot} && ref( $pt) && exists $pt->{__pointerHotSpot};
   }
}

sub set_sync_paint {$_[0]->set(syncPaint  =>$_[1])};
sub set_clip_owner {$_[0]->set(clipOwner  =>$_[1])};
sub set_transparent{$_[0]->set(transparent=>$_[1])};

sub accelTable       {($#_)?$_[0]->set_accel_table     ($_[1]):return $_[0]->get_accel_table;     }
sub accelItems       {($#_)?$_[0]->set_accel_items     ($_[1]):return $_[0]->get_accel_items;     }
sub briefKeys        {($#_)?$_[0]->set_brief_keys  ($_[1]):return $_[0]->get_brief_keys;  }
sub bottom           {($#_)?$_[0]->set_bottom      ($_[1]):return $_[0]->get_bottom;      }
sub buffered         {($#_)?$_[0]->set_buffered    ($_[1]):return $_[0]->get_buffered;    }
sub capture          {($#_)?shift->set_capture     (@_)   :return $_[0]->get_capture;     }
sub centered         {($#_)?$_[0]->set_centered(1,1)      :$_[0]->raise_wo("centered");   }
sub clipOwner        {($#_)?$_[0]->set_clip_owner  ($_[1]):return $_[0]->get_clip_owner;  }
sub current          {($#_)?$_[0]->set_current($_[1])     :$_[0]->get_current;            }
sub currentWidget   {($#_)?$_[0]->set_current_widget ($_[1]):return $_[0]->get_current_widget; }
sub cursorPos        {($#_)? ($_[0]->set_cursor_pos($#_ > 1 ? @_[1..$#_] : @{$_[1]}))   :return $_[0]->get_cursor_pos;    }
sub cursorSize       {($#_)? ($_[0]->set_cursor_size($#_ > 1 ? @_[1..$#_] : @{$_[1]}))   :return $_[0]->get_cursor_size;    }
sub cursorVisible    {($#_)?$_[0]->set_cursor_visible($_[1]):return $_[0]->get_cursor_visible;      }
sub enabled          {($#_)?$_[0]->set_enabled     ($_[1]):return $_[0]->get_enabled;     }
sub growMode         {($#_)?$_[0]->set_grow_mode   ($_[1]):return $_[0]->get_grow_mode;   }
sub dark3DColor      {($#_)?$_[0]->set_color_index ($_[1], ci::Dark3DColor):return $_[0]->get_color_index(ci::Dark3DColor)}
sub designScale      {($#_)?shift->set_design_scale(@_)                 :return $_[0]->get_design_scale;       }
sub disabledBackColor{($#_)?$_[0]->set_color_index ($_[1], ci::Disabled):return $_[0]->get_color_index(ci::Disabled)}
sub disabledColor    {($#_)?$_[0]->set_color_index ($_[1], ci::DisabledText):return $_[0]->get_color_index(ci::DisabledText)}
sub firstClick       {($#_)?$_[0]->set_first_click ($_[1]):return $_[0]->get_first_click; }
sub focused          {($#_)?$_[0]->set_focused     ($_[1]):return $_[0]->get_focused;     }
sub height           {($#_)?$_[0]->set_height      ($_[1]):return $_[0]->get_height;      }
sub helpContext      {($#_)?$_[0]->set_help_context($_[1]):return $_[0]->get_help_context;}
sub hiliteBackColor  {($#_)?$_[0]->set_color_index ($_[1], ci::Hilite  ):return $_[0]->get_color_index(ci::Hilite  )}
sub hiliteColor      {($#_)?$_[0]->set_color_index ($_[1], ci::HiliteText  ):return $_[0]->get_color_index(ci::HiliteText  )}
sub hint             {($#_)?$_[0]->set_hint        ($_[1]):return $_[0]->get_hint;        }
sub hintVisible      {($#_)?$_[0]->set_hint_visible($_[1]):return $_[0]->get_hint_visible;}
sub left             {($#_)?$_[0]->set_left        ($_[1]):return $_[0]->get_left;        }
sub light3DColor     {($#_)?$_[0]->set_color_index ($_[1], ci::Light3DColor):return $_[0]->get_color_index(ci::Light3DColor)}
sub menu             {($#_)?$_[0]->set_menu        ($_[1]):return $_[0]->get_menu;        }
sub menuItems        {($#_)?$_[0]->set_menu_items  ($_[1]):return $_[0]->get_menu_items;  }
sub ownerBackColor   {($#_)?$_[0]->set_owner_back_color($_[1]) :return $_[0]->get_owner_back_color; }
sub ownerColor       {($#_)?$_[0]->set_owner_color($_[1]) :return $_[0]->get_owner_color; }
sub ownerFont        {($#_)?$_[0]->set_owner_font ($_[1]) :return $_[0]->get_owner_font;  }
sub ownerHint        {($#_)?$_[0]->set_owner_hint ($_[1]) :return $_[0]->get_owner_hint;  }
sub ownerPalette     {($#_)?$_[0]->set_owner_palette($_[1]) :return $_[0]->get_owner_palette;  }
sub ownerShowHint    {($#_)?$_[0]->set_owner_show_hint($_[1]):return $_[0]->get_owner_show_hint;}
sub pointerIcon      {($#_)?shift->set_pointer_icon(@_):return $_[0]->get_pointer_icon;}
sub pointerHotSpot   {($#_)?shift->set_pointer_hot_spot(@_):return $_[0]->get_pointer_hot_spot;}
sub pointerPos       {($#_)?shift->set_pointer_pos (@_):return $_[0]->get_pointer_pos; }
sub pointerType      {($#_)?shift->set_pointer_type(@_):return $_[0]->get_pointer_type;}
sub pointerVisible   {($#_)?$_[0]->set_pointer_visible($_[1]):return $_[0]->get_pointer_visible;     }
sub popup            {($#_)?$_[0]->set_popup       ($_[1]):return $_[0]->get_popup;       }
sub popupFont             {($#_)?$_[0]->set_popup_font ($_[1])  :return Prima::Font->new($_[0], "get_popup_font", "set_popup_font")}
sub popupColor            {($#_)?$_[0]->set_popup_color  ($_[1], ci::NormalText)        :return $_[0]->get_popup_color(ci::NormalText);}
sub popupBackColor        {($#_)?$_[0]->set_popup_color  ($_[1], ci::Normal)            :return $_[0]->get_popup_color(ci::Normal);}
sub popupDisabledBackColor{($#_)?$_[0]->set_popup_color  ($_[1], ci::Disabled)          :return $_[0]->get_popup_color(ci::Disabled);}
sub popupHiliteBackColor  {($#_)?$_[0]->set_popup_color  ($_[1], ci::Hilite)            :return $_[0]->get_popup_color(ci::Hilite);}
sub popupDisabledColor    {($#_)?$_[0]->set_popup_color  ($_[1], ci::DisabledText)      :return $_[0]->get_popup_color(ci::DisabledText);}
sub popupHiliteColor      {($#_)?$_[0]->set_popup_color  ($_[1], ci::HiliteText)        :return $_[0]->get_popup_color(ci::HiliteText);}
sub popupDark3DColor      {($#_)?$_[0]->set_popup_color  ($_[1], ci::Dark3DColor)       :return $_[0]->get_popup_color(ci::Dark3DColor);}
sub popupLight3DColor     {($#_)?$_[0]->set_popup_color  ($_[1], ci::Light3DColor)      :return $_[0]->get_popup_color(ci::Light3DColor);}
sub popupItems       {($#_)?$_[0]->set_popup_items ($_[1]):return $_[0]->get_popup_items; }
sub origin           {($#_)?$_[0]->set_pos  ($_[1], $_[2]):return $_[0]->get_pos;         }
sub right            {($#_)?$_[0]->set_right       ($_[1]):return $_[0]->get_right;       }
sub rect             {($#_)? ($_[0]->set_rect($#_>1?@_[1..$#_]:@{$_[1]})):return $_[0]->get_rect}
sub scaleChildren    {($#_)?$_[0]->set_scale_children  ($_[1]):return $_[0]->get_scale_children;  }
sub selectable       {($#_)?$_[0]->set_selectable  ($_[1]):return $_[0]->get_selectable}
sub selected         {($#_)?$_[0]->set_selected    ($_[1]):return $_[0]->get_selected  }
sub selectedWidget  {($#_)?$_[0]->set_selected_widget($_[1]):return $_[0]->get_selected_widget;}
sub selectingButtons {($#_)?$_[0]->set_selecting_buttons($_[1]):return $_[0]->get_selecting_buttons}
sub shape            {($#_)?$_[0]->set_shape       ($_[1]):return $_[0]->get_shape;       }
sub showHint         {($#_)?$_[0]->set_show_hint   ($_[1]):return $_[0]->get_show_hint;   }
sub size             {($#_)?$_[0]->set_size ($_[1], $_[2]):return $_[0]->get_size;        }
sub sizeMax          {($#_)? ($_[0]->set_size_max($#_ > 1 ? @_[1..$#_] : @{$_[1]}))   :return $_[0]->get_size_max;    }
sub sizeMin          {($#_)? ($_[0]->set_size_min($#_ > 1 ? @_[1..$#_] : @{$_[1]}))   :return $_[0]->get_size_min;    }
sub syncPaint        {($#_)?$_[0]->set_sync_paint  ($_[1]):return $_[0]->get_sync_paint;  }
sub tabOrder         {($#_)?$_[0]->set_tab_order   ($_[1]):return $_[0]->get_tab_order;   }
sub tabStop          {($#_)?$_[0]->set_tab_stop    ($_[1]):return $_[0]->get_tab_stop;    }
sub text             {($#_)?$_[0]->set_text        ($_[1]):return $_[0]->get_text;     }
sub top              {($#_)?$_[0]->set_top         ($_[1]):return $_[0]->get_top;         }
sub transparent      {($#_)?$_[0]->set_transparent ($_[1]):return $_[0]->get_transparent; }
sub visible          {($#_)?$_[0]->set_visible     ($_[1]):return $_[0]->get_visible;     }
sub widgetClass      {($#_)?$_[0]->set_widget_class  ($_[1]):return $_[0]->get_widget_class;  }
sub width            {($#_)?$_[0]->set_width       ($_[1]):return $_[0]->get_width;       }
sub x_centered       {($#_)?$_[0]->set_centered(1,0)      :$_[0]->raise_wo("x_centered"); }
sub y_centered       {($#_)?$_[0]->set_centered(0,1)      :$_[0]->raise_wo("y_centered"); }

sub set_commands
{
   my ( $self, $enable, @commands) = @_;
   foreach ( $self-> get_components)
   {
      if ( $_->isa("Prima::AbstractMenu")) {
        my $menu = $_;
        foreach ( @commands) { $menu-> set_command( $_, $enable); }
      };
   }
}

sub insert
{
   my $self = shift;
   my @e = ();
   while (ref $_[0]) {
      my $cl = shift @{$_[0]};
      $cl = "Prima::$cl" unless $cl =~ /^Prima::/ || $cl-> isa("Prima::Component");
      push @e, $cl-> create(@{$_[0]}, owner=> $self);
      shift;
   }
   if (@_) {
      my $cl = shift @_;
      $cl = "Prima::$cl" unless $cl =~ /^Prima::/ || $cl-> isa("Prima::Component");
      push @e, $cl-> create(@_, owner=> $self);
   }
   return wantarray ? @e : $e[0];
}

sub pointer
{
   if ( $#_) {
      $_[0]-> set_pointer_type( $_[1]), return unless ref( $_[1]);
      defined $_[1]-> {__pointerHotSpot} ?
         $_[0]-> set(
            pointerIcon    => $_[1],
            pointerHotSpot => $_[1]-> {__pointerHotSpot},
         ) :
            $_[0]-> set_pointer_icon( $_[1]);
      $_[0]-> set_pointer_type( cr::User);
   } else {
      my $i = $_[0]-> get_pointer_type;
      return $i if $i != cr::User;
      $i = $_[0]-> get_pointer_icon;
      $i-> {__pointerHotSpot} = [ $_[0]-> get_pointer_hot_spot];
      return $i;
   }
}

sub widgets    { return shift->get_widgets};
sub enable_commands  { shift->set_commands(1,@_)}
sub disable_commands { shift->set_commands(0,@_)}
sub key_up      { shift-> key_event( cm::KeyUp,   @_)}
sub key_down    { shift-> key_event( cm::KeyDown, @_)}
sub mouse_up    { splice( @_,5,0,0) if $#_ > 4; shift-> mouse_event( cm::MouseUp, @_); }
sub mouse_move  { splice( @_,5,0,0) if $#_ > 4; splice( @_,1,0,0); shift-> mouse_event( cm::MouseMove, @_) }
sub mouse_wheel { splice( @_,5,0,0) if $#_ > 4; shift-> mouse_event( cm::MouseWheel, @_) }
sub mouse_down  { splice( @_,5,0,0) if $#_ > 4;
                  splice( @_,2,0,0) if $#_ < 4;
                  shift-> mouse_event( cm::MouseDown, @_);}
sub mouse_click { shift-> mouse_event( cm::MouseClick, @_) }
sub select      { $_[0]-> set_selected(1); }
sub deselect    { $_[0]-> set_selected(0); }
sub focus       { $_[0]-> set_focused(1); }
sub defocus     { $_[0]-> set_focused(0); }


# class Window
package Prima::Window;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   Activate      => nt::Default,
   Deactivate    => nt::Default,
   EndModal      => nt::Default,
   Execute       => nt::Default,
   WindowState   => nt::Default,
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      borderIcons           => bi::All,
      borderStyle           => bs::Sizeable,
      clipOwner             => 0,
      growMode              => gm::DontCare,
      icon                  => 0,
      menu                  => undef,
      menuItems             => undef,
      menuColor             => cl::NormalText,
      menuBackColor         => cl::Normal,
      menuHiliteColor       => cl::HiliteText,
      menuHiliteBackColor   => cl::Hilite,
      menuDisabledColor     => cl::DisabledText,
      menuDisabledBackColor => cl::Disabled,
      menuLight3DColor      => cl::Light3DColor,
      menuDark3DColor       => cl::Dark3DColor,
      menuFont              => $_[ 0]-> get_default_menu_font,
      modalResult           => cm::Cancel,
      modalHorizon          => 1,
      ownerFont             => 0,
      originDontCare        => 1,
      sizeDontCare          => 1,
      tabStop               => 0,
      taskListed            => 1,
      transparent           => 0,
      widgetClass           => wc::Window,
      windowState           => ws::Normal,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   if ( $p-> { menu})
   {
      $p->{ menuItems} = $p->{menu}->get_items("");
      delete $p-> {menu};
   }
   $p->{ menuFont} = {} unless exists $p->{ menuFont};
   $p->{ menuFont} = Prima::Drawable-> font_match( $p->{ menuFont}, $default->{ menuFont});
   $p->{ modalHorizon} = 0 if $p->{clipOwner} || $default->{clipOwner};
   $p->{ growMode} = 0 if !exists $p->{growMode} and $default->{growMode} == gm::DontCare and
     (( exists $p->{clipOwner} && ($p->{clipOwner}==1)) or ( $default->{clipOwner} == 1));
   my $shp = exists $p->{originDontCare} ? $p->{originDontCare} : $default-> {originDontCare};
   my $shs = exists $p->{sizeDontCare  } ? $p->{sizeDontCare  } : $default-> {sizeDontCare  };
   $p->{originDontCare} = 0 if $shp and
      exists $p->{left}   or exists $p->{bottom} or
      exists $p->{origin} or exists $p->{rect};
   $p->{sizeDontCare} = 0 if $shs and
      exists $p->{width}  or exists $p->{height} or
      exists $p->{size}   or exists $p->{rect} or
      exists $p->{right}  or exists $p->{top};
}

sub maximize    { $_[0]->windowState( ws::Maximized)}
sub minimize    { $_[0]->windowState( ws::Minimized)}
sub restore     { $_[0]->windowState( ws::Normal)}

sub set_border_icons     {$_[0]->set(borderIcons  =>$_[1])}
sub set_border_style     {$_[0]->set(borderStyle  =>$_[1])}
sub set_task_listed      {$_[0]->set(taskListed   =>$_[1])}

sub borderIcons          {($#_)?$_[0]->set_border_icons($_[1])                        :return $_[0]->get_border_icons;}
sub borderStyle          {($#_)?$_[0]->set_border_style($_[1])                        :return $_[0]->get_border_style;}
sub frameOrigin          {($#_)?$_[0]->set_frame_pos($_[1], $_[2])                    :return $_[0]->get_frame_pos;   }
sub frameSize            {($#_)?$_[0]->set_frame_size($_[1], $_[2])                   :return $_[0]->get_frame_size;  }
sub frameWidth           {($#_)?$_[0]->set_frame_size($_[1], ($_[0]->get_frame_size)[1]):return ($_[0]->get_frame_size)[0];  }
sub frameHeight          {($#_)?$_[0]->set_frame_size(($_[0]->get_frame_size)[0], $_[1]):return ($_[0]->get_frame_size)[1];  }
sub taskListed           {($#_)?$_[0]->set_task_listed ($_[1])                        :return $_[0]->get_task_listed; }
sub windowState          {($#_)?$_[0]->set_window_state($_[1])                        :return $_[0]->get_window_state;}
sub icon                 {($#_)?$_[0]->set_icon        ($_[1])  :                      return $_[0]->get_icon;        }
sub menu                 {($#_)?$_[0]->set_menu        ($_[1])                        :return $_[0]->get_menu; }
sub menuFont             {($#_)?$_[0]->set_menu_font   ($_[1])  :return Prima::Font->new($_[0], "get_menu_font", "set_menu_font")}
sub menuColor            {($#_)?$_[0]->set_menu_color  ($_[1], ci::NormalText)        :return $_[0]->get_menu_color(ci::NormalText);}
sub menuBackColor        {($#_)?$_[0]->set_menu_color  ($_[1], ci::Normal)            :return $_[0]->get_menu_color(ci::Normal);}
sub menuDisabledBackColor{($#_)?$_[0]->set_menu_color  ($_[1], ci::Disabled)          :return $_[0]->get_menu_color(ci::Disabled);}
sub menuHiliteBackColor  {($#_)?$_[0]->set_menu_color  ($_[1], ci::Hilite)            :return $_[0]->get_menu_color(ci::Hilite);}
sub menuDisabledColor    {($#_)?$_[0]->set_menu_color  ($_[1], ci::DisabledText)      :return $_[0]->get_menu_color(ci::DisabledText);}
sub menuHiliteColor      {($#_)?$_[0]->set_menu_color  ($_[1], ci::HiliteText)        :return $_[0]->get_menu_color(ci::HiliteText);}
sub menuDark3DColor      {($#_)?$_[0]->set_menu_color  ($_[1], ci::Dark3DColor)       :return $_[0]->get_menu_color(ci::Dark3DColor);}
sub menuLight3DColor     {($#_)?$_[0]->set_menu_color  ($_[1], ci::Light3DColor)      :return $_[0]->get_menu_color(ci::Light3DColor);}
sub modalHorizon         {($#_)?$_[0]->set_modal_horizon($_[1]):return $_[0]->get_modal_horizon;  }
sub modalResult          {($#_)?$_[0]->set_modal_result($_[1]):return $_[0]->get_modal_result;}


# class Dialog
package Prima::Dialog;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      borderStyle    => bs::Dialog,
      borderIcons    => bi::SystemMenu | bi::TitleBar,
      widgetClass    => wc::Dialog,
      originDontCare => 0,
      sizeDontCare   => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

# class MenuItem
package Prima::MenuItem;

sub create
{
   my $class = $_[0];
   my $self = {};
   bless( $self, $class);
   $self-> {menu} = $_[1];
   $self-> {id}   = $_[2];
   return $self;
}

sub action  { ( $#_) ? $_[ 0]->{ menu}-> set_action ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_action ( $_[0]->{ id}); }
sub accel   { ( $#_) ? $_[ 0]->{ menu}-> set_accel  ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_accel  ( $_[0]->{ id}); }
sub key     { ( $#_) ? $_[ 0]->{ menu}-> set_key    ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_key    ( $_[0]->{ id}); }
sub text    { ( $#_) ? $_[ 0]->{ menu}-> set_text   ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_text   ( $_[0]->{ id}); }
sub enabled { ( $#_) ? $_[ 0]->{ menu}-> set_enabled( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_enabled( $_[0]->{ id}); }
sub image   { ( $#_) ? $_[ 0]->{ menu}-> set_image  ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_image  ( $_[0]->{ id}); }
sub items   { my $i = shift; ( @_) ? $i-> { menu}-> set_items  ( $i->{ id}, @_):return $i->{menu}-> get_items  ( $i->{ id}); }
sub checked { ( $#_) ? $_[ 0]->{ menu}-> set_check  ( $_[0]->{ id}, $_[1]) : return $_[0]->{ menu}-> get_check  ( $_[0]->{ id}); }
sub enable  { $_[0]->{menu}-> set_enabled( $_[0]->{ id}, 1) };
sub disable { $_[0]->{menu}-> set_enabled( $_[0]->{ id}, 0) };
sub check   { $_[0]->{menu}-> set_check( $_[0]->{ id}, 1) };
sub uncheck { $_[0]->{menu}-> set_check( $_[0]->{ id}, 0) };
sub delete  { $_[ 0]->{ menu}-> delete( $_[0]->{ id}) }
sub toggle  {
   my $i = !$_[0]->{ menu}-> get_check($_[0]->{ id});
   $_[0]->{ menu}-> set_check($_[0]->{ id}, $i);
   return $i
}

# class AbstractMenu
package Prima::AbstractMenu;
use vars qw(@ISA);
@ISA = qw(Prima::Component);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      selected => 1,
      items    => undef
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub select     {$_[0]->set_selected(1)}

sub accel      {($#_>1)?shift->set_accel     (@_)   :return $_[0]->get_accel   ($_[1]);}
sub action     {($#_>1)?shift->set_action    (@_)   :return $_[0]->get_action  ($_[1]);}
sub checked    {($#_>1)?shift->set_check     (@_)   :return $_[0]->get_check   ($_[1]);}
sub image      {($#_>1)?shift->set_image     (@_)   :return $_[0]->get_image   ($_[1]);}
sub enable     {$_[0]->set_enabled($_[1],1);}
sub disable    {$_[0]->set_enabled($_[1],0);}
sub check      {$_[0]->set_check($_[1],1);}
sub uncheck    {$_[0]->set_check($_[1],0);}
sub enabled    {($#_>1)?shift->set_enabled   (@_)   :return $_[0]->get_enabled ($_[1]);}
sub key        {($#_>1)?shift->set_key       (@_)   :return $_[0]->get_key     ($_[1]);}
sub items      {($#_)?$_[0]->set_items       ($_[1]):return $_[0]->get_items("");      }
sub selected   {($#_)?$_[0]->set_selected    ($_[1]):return $_[0]->get_selected;       }
sub text       {($#_>1)?shift->set_text      (@_)   :return $_[0]->get_text    ($_[1]);}
sub variable   {($#_>1)?shift->set_variable  (@_)   :return $_[0]->get_variable($_[1]);}
sub toggle     {
   my $i = !$_[0]-> get_check($_[1]);
   $_[0]-> set_check($_[1], $i);
   return $i;
}



sub AUTOLOAD
{
   no strict;
   my $self = shift;
   my $expectedMethod = $AUTOLOAD;
   die "There is no such method as \"$expectedMethod\"" if scalar @_;
   my ($itemName) = $expectedMethod =~ /::([^:]+)$/;
   die "Unknown menu item identifier \"$itemName\"" unless defined $itemName && $self-> has_item( $itemName);
   return Prima::MenuItem-> create( $self, $itemName);
}

# class AccelTable
package Prima::AccelTable;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractMenu);

# class Menu
package Prima::Menu;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractMenu);

# class Popup
package Prima::Popup;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractMenu);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   $def->{auto} = 1;
   return $def;
}

sub auto       {($#_)?$_[0]->set_auto        ($_[1]):return $_[0]->get_auto;        }

package Prima::HintWidget;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      showHint      => 0,
      ownerShowHint => 0,
      visible       => 0,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}


sub set_text
{
   my $self = $_[0];
   $self-> SUPER::set_text( $_[1]);
   $self-> notify( 'Change');
   $self-> repaint;
}

sub on_change
{
   my $self = $_[0];
   my @ln   = split( '\n', $self-> text);
   my $maxLn = 0;
   for ( @ln) {
      my $x = $self-> get_text_width( $_);
      $maxLn = $x if $maxLn < $x;
   }
   $self-> size(
      $maxLn + 6,
      ( $self-> font-> height * scalar @ln) + 2
   );
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my $cl = $self-> color;
   my @size = $canvas-> size;
   $canvas-> color( $self-> backColor);
   $canvas-> bar( 1, 1, $size[0]-2, $size[1]-2);
   $canvas-> color( $cl);
   $canvas-> rectangle( 0, 0, $size[0]-1, $size[1]-1);
   my @ln = split( '\n', $self-> text);
   my $fh = $canvas-> font-> height;
   my ( $x, $y) = ( 3, $size[1] - 1 - $fh);
   for ( @ln) {
       $canvas-> text_out( $_, $x, $y);
       $y -= $fh;
   }
}

package Prima::Application;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      autoClose      => 1,
      pointerType    => cr::Arrow,
      owner          => undef,
      scaleChildren  => 0,
      ownerColor     => 0,
      ownerBackColor => 0,
      ownerFont      => 0,
      ownerShowHint  => 0,
      ownerPalette   => 0,
      showHint       => 1,
      clipboardClass => 'Prima::Clipboard',
      helpFile       => '',
      helpContext    => hmp::None,
      hintClass      => 'Prima::HintWidget',
      hintColor      => cl::Black,
      hintBackColor  => 0xffff80,
      hintPause      => 800,
      hintFont       => Prima::Widget::get_default_font,
      modalHorizon   => 1,
      printerClass   => 'Prima::Printer',
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   delete $p-> { owner};
   delete $p-> { ownerColor};
   delete $p-> { ownerBackColor};
   delete $p-> { ownerFont};
   delete $p-> { ownerShowHint};
   delete $p-> { ownerPalette};
}


sub autoClose     {($#_)?$_[0]->set_auto_close   ($_[1]):return $_[0]->get_auto_close;     }
sub size          { return $_[0]-> get_size; }
sub width         { return ($_[0]-> get_size)[0];}
sub height        { return ($_[0]-> get_size)[1];}
sub clipboard     { return $_[0]-> get_clipboard }
sub hintColor     {($#_)?$_[0]->set_hint_color  ($_[1]):return $_[0]->get_hint_color;   }
sub hintBackColor {($#_)?$_[0]->set_hint_back_color  ($_[1]):return $_[0]->get_hint_back_color;   }
sub hintFont      {($#_)?$_[0]->set_hint_font        ($_[1])  :return Prima::Font->new($_[0], "get_hint_font", "set_hint_font")}
sub hintPause     {($#_)?$_[0]->set_hint_pause       ($_[1]):return $_[0]->get_hint_pause;        }
sub helpFile      {($#_)?$_[0]->set_help_file        ($_[1]):return $_[0]->get_help_file;         }
sub modalHorizon  {($#_)?$_[0]->set_modal_horizon($_[1]):return $_[0]->get_modal_horizon;  }

1;
