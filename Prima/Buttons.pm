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
#

# contains:
#   Button
#   CheckBox
#   Radio
#   SpeedButton
#   RadioGroup
#   GroupBox
#   CheckBoxGroup
#
#   AbstractButton
#   Cluster


use Carp;
use Prima::Const;
use Prima::Classes;
use Prima::IntUtils;
use Prima::StdBitmap;
use strict;


package Prima::AbstractButton;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller);

{
my %RNT = (
   %{Prima::Widget->notification_types()},
   Check => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      pressed      => 0,
      selectable   => 1,
   }
}

sub set_text
{
   my ( $self, $caption) = @_;
   my $cap = $caption;
   $cap =~ s/^([^~]*)\~(.*)$/$1$2/;
   my $ac = $self-> { accel} = length($2) ? lc substr( $2, 0, 1) : undef;
   $self-> SUPER::set_text( $caption);
   $self-> repaint;
}

sub on_translateaccel
{
   my ( $self, $code, $key, $mod) = @_;
   if ( defined $self->{accel} && ($key == kb::NoKey) && lc chr $code eq $self-> { accel})
   {
      $self-> clear_event;
      $self-> notify( 'Click');
   }
   if ( $self-> { default} && $key == kb::Enter)
   {
      $self-> clear_event;
      $self-> notify( 'Click');
   }
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> { pressed} = $profile{ pressed};
   return %profile;
}

sub cancel_transaction
{
   my $self = $_[0];
   if ( $self->{mouseTransaction} || $self->{spaceTransaction}) {
      $self->{spaceTransaction} = undef;
      $self-> capture(0) if $self->{mouseTransaction};
      $self->{mouseTransaction} = undef;
      $self-> pressed( 0);
   }
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;
   if ( $key == kb::Space)
   {
      $self-> clear_event;
      return if $self->{spaceTransaction} || $self->{mouseTransaction};
      $self-> { spaceTransaction} = 1;
      $self-> pressed( 1);
   }
   if ( defined $self->{accel} && ($key == kb::NoKey) && lc chr $code eq $self-> { accel})
   {
      $self-> clear_event;
      $self-> notify( 'Click');
   }
}

sub on_keyup
{
   my ( $self, $code, $key, $mod) = @_;

   if ( $key == kb::Space && $self->{spaceTransaction})
   {
      $self->{spaceTransaction} = undef;
      $self-> capture(0) if $self->{mouseTransaction};
      $self->{mouseTransaction} = undef;
      $self-> pressed( 0);
      $self-> clear_event;
      $self-> notify( 'Click')
   }
}

sub on_leave
{
   my $self = $_[0];
   if ( $self->{spaceTransaction} || $self->{mouseTransaction}) {
      $self-> cancel_transaction;
   } else {
      $self-> repaint;
   }
}

sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction} || $self->{spaceTransaction};
   return if $btn != mb::Left;
   $self->{ mouseTransaction} = 1;
   $self->{ lastMouseOver}  = 1;
   $self-> pressed( 1);
   $self-> capture(1);
   $self-> clear_event;
   $self-> scroll_timer_start if $self->{autoRepeat};
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   return unless $dbl;
   return if $btn != mb::Left;
   return if $self->{mouseTransaction} || $self->{spaceTransaction};
   $self->{ mouseTransaction} = 1;
   $self->{ lastMouseOver}  = 1;
   $self-> pressed( 1);
   $self-> capture(1);
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $btn != mb::Left;
   return unless $self->{mouseTransaction};
   my @size = $self-> size;
   $self-> {mouseTransaction} = undef;
   $self-> {spaceTransaction} = undef;
   $self-> {lastMouseOver}    = undef;
   $self-> capture(0);
   $self-> pressed( 0);
   if ( $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1] ) {
      $self-> clear_event;
      $self-> notify( 'Click');
   }
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;
   return unless $self->{mouseTransaction};
   return if $self-> {autoRepeat} && !$self-> scroll_timer_semaphore;
   my @size = $self-> size;
   my $mouseOver = $x > 0 && $y > 0 && $x < $size[0] && $y < $size[1];
   $self-> pressed( $mouseOver) if $self-> { lastMouseOver} != $mouseOver;
   $self-> { lastMouseOver} = $mouseOver;
   return unless $self->{autoRepeat};
   $self-> scroll_timer_stop, return unless $mouseOver;
   $self-> scroll_timer_start, return unless $self-> scroll_timer_active;
   $self-> scroll_timer_semaphore(0);
   $self-> notify(q(Click));
}

sub set_pressed
{
   $_[0]-> { pressed} = $_[1];
   $_[0]-> repaint;
}

sub draw_veil
{
    my ($self,$canvas) = (shift, shift);
    my $back = $self-> backColor;
    $canvas-> set(
       color       => cl::Black,
       backColor   => cl::White,
       fillPattern => fp::SimpleDots,
       rop         => rop::AndPut
    );
    $canvas-> bar( @_);
    $canvas-> set(
       color       => $back,
       backColor   => cl::Black,
       rop         => rop::OrPut
    );
    $canvas-> bar( @_);
    $canvas-> set(
       rop        => rop::CopyPut,
       backColor  => $back,
    );
}

sub draw_caption
{
   my ( $self, $canvas, $x, $y) = @_;
   my $cap = $self-> get_text;
   $cap =~ s/^([^~]*)\~(.*)$/$1$2/;
   my ( $leftPart, $accel) = ( $1, length($2) ? substr( $2, 0, 1) : undef);
   my ( $fw, $fh, $enabled) = (
      $canvas-> get_text_width( $cap),
      $canvas-> font-> height,
      $self-> enabled
   );

   if ( defined $accel)
   {
      my ( $a, $b, $c) = (
         $canvas-> get_text_width( $leftPart),
         $canvas-> get_text_width( $leftPart.$accel),
         $canvas-> get_text_width( $accel)
      );
      unless ( $enabled)
      {
         my $z = $canvas-> color;
         $canvas-> color( cl::White);
         $canvas-> line( $x + $b - $c + 1, $y - 1, $x + $b * 2 - $a - $c, $y - 1);
         $canvas-> color( $z);
      }
      $canvas-> line( $x + $b - $c, $y, $x + $b * 2 - $a - $c - 1, $y);
   }

   unless ( $enabled)
   {
      my $c = $canvas-> color;
      $canvas-> color( cl::White);
      $canvas-> text_out( $cap, $x+1, $y-1);
      $canvas-> color( $c);
   }

   $canvas-> text_out( $cap, $x, $y);
   $canvas-> rect_focus( $x - 2, $y - 2, $x + 2 + $fw, $y + 2 + $fh) if $self-> focused;
}

sub caption_box
{
   my ($self,$canvas) = @_;
   my $cap = $self-> get_text;
   $cap =~ s/~//;
   return $canvas-> get_text_width( $cap), $canvas-> font-> height;
}

sub pressed      {($#_)?$_[0]->set_pressed     ($_[1]):return $_[0]->{pressed}     }

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> cancel_transaction; $_[0]-> repaint; }
sub on_enter   { $_[0]-> repaint; }


package Prima::Button;
use vars qw(@ISA);
@ISA = qw(Prima::AbstractButton);

my %standardGlyphScheme = (
      glyphs => 4,
      defaultGlyph  => 0,
      hiliteGlyph   => 0,
      disabledGlyph => 1,
      pressedGlyph  => 2,
      holdGlyph     => 3,
);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      borderWidth   => 2,
      checkable     => 0,
      checked       => 0,
      default       => 0,
      glyphs        => 1,
      height        => 36,
      image         => undef,
      imageFile     => undef,
      imageScale    => 1,
      modalResult   => 0,
      vertical      => 0,
      width         => 96,
      defaultGlyph  => 0,
      hiliteGlyph   => 0,
      disabledGlyph => 1,
      pressedGlyph  => 2,
      holdGlyph     => 3,
      flat          => 0,
      autoRepeat    => 0,
      widgetClass   => wc::Button,
   }
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   my $checkable = exists $p->{checkable} ? $p->{checkable} : $default->{checkable};
   $p->{ checked} = 0 unless $checkable;
}

sub init
{
   my $self = shift;
   $self->{$_} = 0 for ( qw(
     borderWidth checkable checked default glyphs
     vertical defaultGlyph hiliteGlyph disabledGlyph pressedGlyph holdGlyph
     flat modalResult autoRepeat
   ));
   $self->{imageScale} = 1;
   $self->{image} = undef;
   my %profile = $self-> SUPER::init(@_);
   defined $profile{image} ?
      $self-> image( $profile{image}) :
      $self-> imageFile( $profile{imageFile});
   $self->$_( $profile{$_}) for ( qw(
     borderWidth checkable checked default imageScale glyphs
     vertical defaultGlyph hiliteGlyph disabledGlyph pressedGlyph holdGlyph
     flat modalResult autoRepeat
   ));
   return %profile;
}

sub on_paint
{
   my ($self,$canvas)  = @_;
   my @clr  = ( $self-> color, $self-> backColor);
   @clr = ( $self-> hiliteColor, $self-> hiliteBackColor)     if ( $self-> { default});
   @clr = ( $self-> disabledColor, $self-> disabledBackColor) if ( !$self-> enabled);
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);
   my @size = $canvas-> size;
   @c3d = reverse @c3d if $_[0]-> { pressed} || $_[0]-> { checked};
   my @fbar = $self-> {default} ?
      ( 1, 1, $size[0] - 2, $size[1] - 2):
      ( 0, 0, $size[0] - 1, $size[1] - 1);
   if ( !$self->{flat} || $self->{hilite})
   {
      $self-> transparent ?
         $canvas-> rect3d( @fbar, $self->{borderWidth}, @c3d) :
         $canvas-> rect3d( @fbar, $self->{borderWidth}, @c3d, $clr[ 1])
   } else {
     $canvas-> color( $clr[ 1]);
     $canvas-> bar( @fbar) unless $self-> transparent;
   }
   if ( $self-> {default}) {
      $canvas-> color( cl::Black);
      $canvas-> rectangle( 0, 0, $size[0]-1, $size[1]-1);
   }
   my $shift  = $self->{checked} ? 1 : 0;
   $shift += $self->{pressed} ? 2 : 0;
   my $capOk = length($self-> text) > 0;
   my ( $fw, $fh) = $capOk ? $self-> caption_box($canvas) : ( 0, 0);
   my ( $textAtX, $textAtY);

   if ( defined $self->{image})
   {
      my $pw = $self->{image}->width / $self-> { glyphs};
      my $ph = $self->{image}->height;
      my $sw = $pw * $self->{imageScale};
      my $sh = $ph * $self->{imageScale};
      my $imgNo = $self->{defaultGlyph};
      my $useVeil = 0;
      if ( $self-> {hilite}) {
        $imgNo = $self->{hiliteGlyph} if $self-> {glyphs} > $self->{hiliteGlyph} && $self->{hiliteGlyph} >= 0;
      }
      if ( $self-> {checked}) {
        $imgNo = $self->{holdGlyph} if $self-> {glyphs} > $self->{holdGlyph} && $self->{holdGlyph} >= 0;
      }
      if ( $self-> {pressed}) {
        $imgNo = $self->{pressedGlyph} if $self-> {glyphs} > $self->{pressedGlyph} && $self->{pressedGlyph} >= 0;
      }
      if ( !$self-> enabled)
      {
        ( $self-> {glyphs} > $self->{disabledGlyph} && $self->{disabledGlyph} >= 0) ?
         $imgNo = $self->{disabledGlyph} : $useVeil = 1;
      }

      my ( $imAtX, $imAtY);
      if ( $capOk)
      {
         if ( $self-> { vertical})
         {
            $imAtX = ( $size[ 0] - $sw) / 2 + $shift;
            $imAtY = ( $size[ 1] - $fh - $sh) / 3;
            $textAtX = ( $size[0] - $fw) / 2 + $shift;
            $textAtY = $size[ 1] - 2 * $imAtY - $fh - $sh - $shift;
            $imAtY   = $size[ 1] - $imAtY - $sh - $shift;
         } else {
            $imAtX = ( $size[ 0] - $fw - $sw) / 3;
            $imAtY = ( $size[ 1] - $sh) / 2 - $shift;
            $textAtX = 2 * $imAtX + $sw + $shift;
            $textAtY = ( $size[1] - $fh) / 2 - $shift;
            $imAtX += $shift;
         }
      } else {
         $imAtX = ( $size[0] - $sw) / 2 + $shift;
         $imAtY = ( $size[1] - $sh) / 2 - $shift;
      }

      $canvas-> put_image_indirect(
         $self-> {image},
         $imAtX, $imAtY,
         $imgNo * $pw, 0,
         $sw, $sh,
         $pw, $ph,
         rop::CopyPut
      );
      $self-> draw_veil( $canvas, $imAtX, $imAtY, $imAtX + $sw, $imAtY + $sh) if $useVeil;
   }
   else
   {
     $textAtX = ( $size[0] - $fw) / 2 + $shift;
     $textAtY = ( $size[1] - $fh) / 2 - $shift;
   }
   $canvas-> color( $clr[0]);
   $self-> draw_caption( $canvas, $textAtX, $textAtY) if $capOk;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;
   if ( $key == kb::Enter)
   {
      $self-> clear_event;
      return $self-> notify( 'Click')
   }
   $self-> SUPER::on_keydown( $code, $key, $mod, $repeat);
}

sub on_click
{
   my $self = $_[0];
   $self-> checked( !$self-> checked) if $self->{ checkable};
   my $owner = $self-> owner;
   if ( $owner-> isa(q(Prima::Window)) && $owner-> get_modal && $self-> modalResult)
   {
      $owner-> modalResult( $self-> modalResult);
      $owner-> end_modal;
   }
}

sub on_check {}

sub on_mouseenter
{
   my $self = $_[0];
   if ( !$self->{spaceTransaction} && !$self->{mouseTransaction} && $self->enabled)
   {
      $self->{hilite} = 1;
      $self-> repaint if $self->{flat} || $self->{defaultGlyph} != $self->{hiliteGlyph};
   }
}

sub on_mouseleave
{
   my $self = $_[0];
   if ( $self-> {hilite})
   {
      undef $self-> {hilite};
      $self-> repaint if $self->{flat} || $self->{defaultGlyph} != $self->{hiliteGlyph};
   }
}

sub set_default
{
   my $self = $_[0];
   return if $self->{default} == $_[1];
   if ( $self-> { default} = $_[1])
   {
      my @widgets = $self-> owner-> widgets;
      for ( @widgets)
      {
         last if $_ == $self;
         $_-> default(0) if $_->isa(q(Prima::Button)) && $_->default;
      }
   }
   $self-> repaint;
}

sub set_border_width
{
   my ( $self, $bw) = @_;
   $bw = 0 if $bw < 0;
   $bw = int( $bw);
   return if $bw == $self->{borderWidth};
   $self->{borderWidth} = $bw;
   $self-> repaint;
}

sub set_modal_result
{
   my $self = $_[0];
   $self-> { modalResult} = $_[1];
   my $owner = $self-> owner;
   if ( $owner-> isa(q(Prima::Window)) && $owner-> get_modal && $self-> {modalResult})
   {
      $owner-> modalResult( $self-> { modalResult});
      $owner-> end_modal;
   }
}

sub set_glyphs
{
   my $maxG = defined $_[0]->{image} ? $_[0]->{image}-> width : 1;
   $maxG = 1 unless $maxG;
   if ( $_[1] > 0 && $_[1] <= $maxG)
   {
      $_[0]->{glyphs} = $_[1];
      $_[0]-> repaint;
   }
}

sub set_checkable
{
   $_[0]-> checked( 0) unless $_[0]-> {checkable} == $_[1];
   $_[0]-> {checkable} = $_[1];
}

sub set_checked
{
   return unless $_[0]-> { checkable};
   return if $_[0]->{checked}+0 == $_[1]+0;
   $_[0]->{checked} = $_[1];
   $_[0]-> repaint;
   $_[0]-> notify( 'Check', $_[0]->{checked});
}

sub set_image_file
{
   my ($self,$file) = @_;
   $self-> image(undef), return unless defined $file;
   my $img = Prima::Icon-> create;
   my @fp = ($file);
   $fp[0] =~ s/\:(\d+)$//;
   push( @fp, 'index', $1) if defined $1;
   return unless $img-> load(@fp);
   $self->{imageFile} = $file;
   $self->image($img);
}

sub autoRepeat
{
   return $_[0]-> {autoRepeat} unless $#_;
   $_[0]-> {autoRepeat} = $_[1];
}

sub borderWidth  {($#_)?$_[0]->set_border_width($_[1]):return $_[0]->{borderWidth} }
sub checkable    {($#_)?$_[0]->set_checkable   ($_[1]):return $_[0]->{checkable}   }
sub checked      {($#_)?$_[0]->set_checked     ($_[1]):return $_[0]->{checked}     }
sub default      {($#_)?$_[0]->set_default     ($_[1]):return $_[0]->{default}     }
sub defaultGlyph {($#_)?($_[0]->{defaultGlyph} = $_[1],$_[0]->repaint) :return $_[0]->{defaultGlyph}     }
sub hiliteGlyph  {($#_)?($_[0]->{hiliteGlyph}  = $_[1],$_[0]->repaint) :return $_[0]->{hiliteGlyph}     }
sub disabledGlyph{($#_)?($_[0]->{disabledGlyph} = $_[1],$_[0]->repaint) :return $_[0]->{disabledGlyph}     }
sub pressedGlyph {($#_)?($_[0]->{pressedGlyph} = $_[1],$_[0]->repaint) :return $_[0]->{pressedGlyph}     }
sub holdGlyph    {($#_)?($_[0]->{holdGlyph} = $_[1],$_[0]->repaint) :return $_[0]->{holdGlyph}     }
sub flat         {($#_)?($_[0]->{flat}      = $_[1],$_[0]->repaint) :return $_[0]->{flat}          }
sub image        {($#_)?($_[0]->{image}=$_[1], $_[0]->repaint):return $_[0]->{image} }
sub imageFile    {($#_)?$_[0]->set_image_file($_[1]):return $_[0]->{imageFile}}
sub imageScale   {($#_)?($_[0]->{imageScale}=$_[1], $_[0]->repaint):return $_[0]->{imageScale} }
sub vertical     {($#_)?($_[0]->{vertical}=$_[1], $_[0]->repaint):return $_[0]->{vartical} }
sub modalResult  {($#_)?$_[0]->set_modal_result($_[1]):return $_[0]->{modalResult} }
sub glyphs       {($#_)?$_[0]->set_glyphs      ($_[1]):return $_[0]->{glyphs}      }


package Prima::Cluster;
use vars qw(@ISA @images);
@ISA = qw(Prima::AbstractButton);

my @images;

{
   my $i = 0;
   my @idx = ( sbmp::CheckBoxUnchecked, sbmp::CheckBoxUncheckedPressed,
               sbmp::CheckBoxChecked, sbmp::CheckBoxCheckedPressed,
               sbmp::RadioUnchecked, sbmp::RadioUncheckedPressed,
               sbmp::RadioChecked, sbmp::RadioCheckedPressed );
   for ( @idx)
   {
      $images[ $i] = ( $i > 3) ? Prima::StdBitmap::icon( $_) : Prima::StdBitmap::image( $_);
      $i++;
   }
}


sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      auto           => 1,
      checked        => 0,
      ownerBackColor => 1,
      height         => 36,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> { auto   } = $profile{ auto   };
   $self-> { checked} = $profile{ checked};
   return %profile;
}

sub on_click
{
   my $self = $_[0];
   $self-> focus;
   $self-> checked( !$self-> checked);
}

sub on_enter
{
   my $self = $_[0];
   $self-> check if $self-> auto;
   $self-> SUPER::on_enter;
}

sub set_checked
{
   my $old = $_[0]-> {checked};
   my $new = $_[1] ? 1 : 0;
   if ( $old != $new) {
      $_[0]-> {checked} = $new;
      $_[0]-> repaint;
      $_[0]-> notify( 'Check', $_[0]->{checked});
   }
}

sub auto         { ($#_)?$_[0]->{auto}          =$_[1] :return $_[0]->{auto}}
sub checked      { ($#_)?$_[0]->set_checked     ($_[1]):return $_[0]->{checked}}
sub toggle       { my $i = $_[0]-> checked; $_[0]-> checked( !$i); return !$i;}
sub check        { $_[0]->set_checked(1)}
sub uncheck      { $_[0]->set_checked(0)}


package Prima::CheckBox;
use vars qw(@ISA);
@ISA = qw(Prima::Cluster);

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      auto        => 0,
      widgetClass => wc::CheckBox,
   }
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @clr;
   if ( $self-> enabled)
   {
     if ( $self-> focused) {
        @clr = ($self-> hiliteColor, $self-> hiliteBackColor);
     } else { @clr = ($self-> color, $self-> backColor); }
   } else { @clr = ($self-> disabledColor, $self-> disabledBackColor); }

   my @size = $canvas-> size;
   unless ( $self-> transparent)
   {
     $canvas-> color( $clr[ 1]);
     $canvas-> bar( 0, 0, @size);
   }
   my ( $image, $imNo);
   if ( $self-> { checked})
   {
      $imNo = $self-> { pressed} ? 3 : 2;
   } else {
      $imNo = $self-> { pressed} ? 1 : 0;
   };
   my $xStart;
   $image = $images[ $imNo];
   my @c3d  = ( $self-> light3DColor, $self-> dark3DColor);

   if ( $image)
   {
      $canvas-> put_image( 0, ( $size[1] - $image-> height) / 2, $image);
      $xStart = $image-> width;
   } else {
      $xStart = 16;
      push ( @c3d, shift @c3d) if $self-> { pressed};
      $canvas-> rect3d( 1, ( $size[1] - 14) / 2, 15, ( $size[1] + 14) / 2, 1, @c3d, $clr[ 1]);
      if ( $self-> { checked})
      {
         my $at = $self-> { pressed} ? 1 : 0;
         $canvas-> color( cl::Black);
         $canvas-> lineWidth( 2);
         my $yStart = ( $size[1] - 14) / 2;
         $canvas-> line( $at + 4, $yStart - $at +  8, $at + 5 , $yStart - $at + 3, );
         $canvas-> line( $at + 5 , $yStart - $at + 3, $at + 12, $yStart - $at + 12 );
         $canvas-> lineWidth( 0);
      }
   }
   $canvas-> color( $clr[ 0]);
   my ( $fw, $fh) = $self-> caption_box( $canvas);
   $self-> draw_caption( $canvas, $xStart * 1.5, ( $size[1] - $fh) / 2 );

}

package Prima::Radio;
use vars qw(@ISA @images);
@ISA = qw(Prima::Cluster);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   @$def{qw(widgetClass)} = (wc::Radio, undef);
   return $def;
}

sub on_paint
{
   my ($self,$canvas) = @_;
   my @clr;
   if ( $self-> enabled)
   {
     if ( $self-> focused) {
        @clr = ($self-> hiliteColor, $self-> hiliteBackColor);
     } else { @clr = ($self-> color, $self-> backColor); }
   } else { @clr = ($self-> disabledColor, $self-> disabledBackColor); }

   my @size = $canvas-> size;
   unless ( $self-> transparent)
   {
      $canvas-> color( $clr[ 1]);
      $canvas-> bar( 0, 0, @size);
   }

   my ( $image, $imNo);
   if ( $self-> { checked})
   {
      $imNo = $self-> { pressed} ? 7 : 6;
   } else {
      $imNo = $self-> { pressed} ? 5 : 4;
   };
   my $xStart;
   $image = $images[ $imNo];
   if ( $image)
   {
      $canvas-> put_image( 0, ( $size[1] - $image-> height) / 2, $image);
      $xStart = $image-> width;
   } else {
      $xStart = 16;
      my $y = ( $size[1] - 16) / 2;
      my @xs = ( 0, 8, 16, 8);
      my @ys = ( 8, 16, 8, 0);
      for ( @ys) {$_+=$y};
      my $i;
      if ( $self-> { pressed})
      {
         $canvas-> color( cl::Black);
         for ( $i = -1; $i < 3; $i++) { $canvas-> line($xs[$i],$ys[$i],$xs[$i+1],$ys[$i+1])};
      } else {
         my @clr = $self->{checked} ?
           ( $self-> light3DColor, $self-> dark3DColor) :
           ( $self-> dark3DColor, $self-> light3DColor);
         $canvas-> color( $clr[1]);
         for ( $i = -1; $i < 1; $i++) { $canvas-> line($xs[$i],$ys[$i],$xs[$i+1],$ys[$i+1])};
         $canvas-> color( $clr[0]);
         for ( $i = 1; $i < 3; $i++)  { $canvas-> line($xs[$i],$ys[$i],$xs[$i+1],$ys[$i+1])};
      }
      if ( $self-> checked)
      {
         $canvas-> color( cl::Black);
         $canvas-> fillpoly( [ 6, $y+8, 8, $y+10, 10, $y+8, 8, $y+6]);
      }
   }
   $canvas-> color( $clr[ 0]);
   my ( $fw, $fh) = $self-> caption_box( $canvas);
   $self-> draw_caption( $canvas, $xStart * 1.5, ( $size[1] - $fh) / 2 );
}

sub on_click
{
   my $self = $_[0];
   $self-> focus;
   $self-> checked( 1) unless $self-> checked;
}

sub set_checked
{
   my $self = $_[0];
   my $chkOk = $self->{checked};

   my $old = $self-> {checked} + 0;
   $self-> {checked} = $_[1] + 0;
   if ( $old != $_[1] + 0) {
      $self-> repaint;
      $chkOk = ( $self->{checked} != $chkOk) && $self->{checked};
      my $owner = $self-> owner;
      $owner-> notify( 'RadioClick', $self) if $chkOk && exists $owner-> notification_types->{RadioClick};
      $self-> notify( 'Check', $self->{checked});
   }
}

sub checked  { ($#_)?$_[0]->set_checked ($_[1]):return $_[0]->{checked}     }

package Prima::SpeedButton;
use vars qw(@ISA);
@ISA = qw(Prima::Button);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   @$def{qw(selectable width height text)} = (0, 36, 36, "");
   return $def;
}

package Prima::GroupBox;
use vars qw(@ISA);
@ISA=qw(Prima::Widget);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   @$def{qw(ownerBackColor)} = (1);
   return $def;
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size   = $canvas-> size;
   my @clr    = $self-> enabled ?
   ( $self-> color, $self-> backColor) :
   ( $self-> disabledColor, $self-> disabledBackColor);
   unless ( $self-> transparent) {
      $canvas-> color( $clr[1]);
      $canvas-> bar( 0, 0, @size);
   }
   my $fh = $canvas-> font-> height;
   $canvas-> color( $self-> light3DColor);
   $canvas-> rectangle( 1, 0, $size[0] - 1, $size[1] - $fh / 2 - 2);
   $canvas-> color( $self-> dark3DColor);
   $canvas-> rectangle( 0, 1, $size[0] - 2, $size[1] - $fh / 2 - 1);
   my $c = $self-> text;
   if ( length( $c) > 0) {
      $canvas-> color( $clr[1]);
      $canvas-> bar  ( 8, $size[1] - $fh - 1, 16 + $canvas-> get_text_width( $c), $size[1] - 1);
      $canvas-> color( $clr[0]);
      $canvas-> text_out( $c, 12, $size[1] - $fh - 1);
   }
}

package Prima::RadioGroup;
no strict; @ISA=qw(Prima::GroupBox); use strict;

{
my %RNT = (
   %{Prima::Cluster->notification_types()},
   RadioClick => nt::Default,
);

sub notification_types { return \%RNT; }
}


sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      index => 0,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> index( $profile{index});
   return %profile;
}

sub on_radioclick
{
   my ($me,$rd) = @_;
   for ($me->widgets)
   {
      next if "$rd" eq "$_";
      next unless $_->isa(q(Prima::Radio));
      $_-> checked(0);
   }
}


sub index
{
   my $self = $_[0];
   my @c    = $self-> widgets;
   if ( $#_) {
      my $i = $_[1];
      $i = 0 if $i < 0;
      $i = scalar @c if $i > scalar @c;
      $c[$i]-> check if $c[$i];
   } else {
      my $i;
      for ( $i = 0; $i < scalar @c; $i++) {
         return $i if $c[$i]-> checked;
      }
      return -1;
   }
}

package Prima::CheckBoxGroup;
no strict; @ISA=qw(Prima::GroupBox); use strict;

sub profile_default
{
   return {
      %{$_[ 0]-> SUPER::profile_default},
      value => 0,
   }
}


sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> value( $profile{value});
   return %profile;
}


sub value
{
   my $self = $_[0];
   my @c    = $self-> widgets;
   my $i;
   if ( $#_) {
      my $value = $_[1];
      for ( $i = 0; $i < scalar @c; $i++) {
         $c[$i]-> checked( $value & ( 1 << $i));
      }
   } else {
      my $value = 0;
      for ( $i = 0; $i < scalar @c; $i++) {
         $value |= 1 << $i if $c[$i]-> checked;
      }
      return $value;
   }
}


1;
