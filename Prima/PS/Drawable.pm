#
#  Copyright (c) 1997-2000 The Protein Laboratory, University of Copenhagen
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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#
use strict;
use POSIX q(setlocale);
use Prima;
use Prima::PS::Fonts;
use Prima::PS::Encodings;

package Prima::PS::Drawable;
use vars qw(@ISA);
@ISA = qw(Prima::Drawable);

=head1 NAME

Prima::PS::Drawable -  PostScript interface to Prima::Drawable

=head1 SYNOPSIS

   use Prima;
   use Prima::PS::Drawable;

   package FileSpoolPrinter;
   use vars qw(@ISA);
   @ISA = qw(Prima::PS::Drawable);

   sub spool {
      open F, "> ./test.ps";
      print F ($_, "\n") for @{$_[1]};
      close F;
   }
   
   my $x = FileSpoolPrinter-> create;
   $x-> begin_doc;
   $x-> font-> size( 30);
   $x-> text_out( "hello!", 100, 100);
   $x-> end_doc;


=head1 DESCRIPTION

Realizes the Prima library interface to PostScript level 2 document language.
The module is designed to be compliant with Prima::Drawable interface.
All properties' behavior is as same as Prima::Drawable's, except those 
described below. 

=item ::resolution

Can be set while object is in normal stage - cannot be changed if document
is opened. Applies to fillPattern realization and general pixel-to-point
and vice versa calculations

=item ::region

- ::region is not realized ( yet?)

=head2 Specific properties

=item ::copies

amount of copies that PS interpreter should print

=item ::grayscale

could be 0 or 1

=item ::locale

used by default for optimized text operations. Not used if ::useDeviceFonts is 0.

=item ::pageSize 

physical page dimension, in points

=item ::pageMargins

non-printable page area, an array of 4 integers:
left, bottom, right and top margins in points.

=item ::reversed

if 1, a 90 degrees rotated document layout is assumed 

=item ::rotate and ::scale

along with Prima::Drawable::translate provide PS-specific
transformation matrix manipulations. ::rotate is number,
measured in degrees, counter-clockwise. ::scale is array of
two numbers, respecively x- and y-scale. 1 is 100%, 2 is 200% 
etc.

=item ::useDeviceFonts

1 by default; optimizes greatly text operaions, but takes the risk
that a character could be drawn incorrectly or not drawn at all -
thsi behavior depends on a particular PS interpreter.

=cut

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      copies           => 1,
      grayscale        => 0,
      locale           => undef,
      pageDevice       => undef,
      pageSize         => [ 598, 845],
      pageMargins      => [ 12, 12, 12, 12],
      resolution       => [ 300, 300],
      reversed         => 0,
      rotate           => 0,
      scale            => [ 1, 1],
      textOutBaseline  => 1,
      useDeviceFonts   => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   $self-> {clipRect}    = [0,0,0,0];
   $self-> {pageSize}    = [0,0];
   $self-> {pageMargins} = [0,0,0,0];
   $self-> {resolution}  = [72,72];
   $self-> {scale}       = [ 1, 1];
   $self-> {copies}      = 1;
   $self-> {rotate}      = 1;
   $self-> {font}        = {};
   my %profile = $self-> SUPER::init(@_);
   $self-> $_( $profile{$_}) for qw( grayscale copies pageDevice 
      useDeviceFonts rotate reversed locale);
   $self-> $_( @{$profile{$_}}) for qw( pageSize pageMargins resolution scale);
   return %profile;
}

# internal routines

sub cmd_rgb
{
   my ( $r, $g, $b) = (
      int((($_[1] & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100, 
      int((($_[1] & 0xff00) >> 8) * 100 / 256 + 0.5) / 100, 
      int(($_[1] & 0xff)*100/256 + 0.5) / 100);
   unless ( $_[0]-> {grayscale}) {
      return "$r $g $b setrgbcolor";
   } else {
      my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100;
      return "$i setgray";
   }
}

sub color_check
{
   return $_[1] unless $_[0]-> {grayscale};
   my ( $r, $g, $b) = ((($_[1] & 0xff0000) >> 16) / 255, (($_[1] & 0xff00) >> 8) / 255, ($_[1] & 0xff)/255);
   return 65793 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b); 
}

=head2 Internal routines

=item emit

Can be called for direct PostScript code injection. Example:

  $x-> emit('0.314159 setgray');
  $x-> bar( 10, 10, 20, 20);

=cut

sub emit
{
   my $self = $_[0];
   push( @{$self-> {psData}}, $_[1]) if $self-> {canDraw};
}

sub save_state
{
   my $self = $_[0];
   
   $self-> {saveState} = {};
   $self-> {saveState}-> {$_} = $self-> $_() for qw( 
      color backColor fillPattern lineEnd linePattern lineWidth
      rop rop2 textOpaque textOutBaseline font locale
   );
   $self-> {saveState}-> {$_} = [$self-> $_()] for qw( 
      transform clipRect
   );
   $self-> {saveState}-> {localeEncoding} = 
      $self-> {useDeviceFonts} ? [ @{$self-> {localeEncoding}}] : {};
}

sub restore_state
{
   my $self = $_[0];
   for ( qw( color backColor fillPattern lineEnd linePattern lineWidth
         rop rop2 textOpaque textOutBaseline font locale)) {
       $self-> $_( $self-> {saveState}->{$_});     
   }      
   for ( qw( transform clipRect)) {
       $self-> $_( @{$self-> {saveState}->{$_}});
   }      
   $self-> {localeEncoding} = $self-> {saveState}-> {localeEncoding};
}

=item pixel2point and point2pixel

Helpers for transformation from pixel to points and vice versa.

=cut

sub pixel2point
{
   my $self = shift;
   my $i;
   my @res;
   for ( $i = 0; $i < scalar @_; $i+=2) {
      my ( $x, $y) = @_[$i,$i+1];
      push( @res, int( $x * 7227 / $self-> {resolution}-> [0] + 0.5) / 100 );
      push( @res, int( $y * 7227 / $self-> {resolution}-> [1] + 0.5) / 100 ) if defined $y;
   }
   return @res;
}

sub point2pixel
{
   my $self = shift;
   my $i;
   my @res;
   for ( $i = 0; $i < scalar @_; $i+=2) {
      my ( $x, $y) = @_[$i,$i+1];
      push( @res, $x * $self-> {resolution}-> [0] / 72.27);
      push( @res, $y * $self-> {resolution}-> [1] / 72.27) if defined $y;
   }
   return @res;
}


sub change_transform
{
   return if $_[0]-> {delay};
   
   my @tp = $_[0]-> transform;
   my @cr = $_[0]-> clipRect;
   my @sc = $_[0]-> scale;
   my $ro = $_[0]-> rotate;
   $cr[2] -= $cr[0];
   $cr[3] -= $cr[1];
   my $mcr2 = -$cr[2];
   my $doClip = grep { $_ != 0 } @cr;
   my $doTR   = grep { $_ != 0 } @tp; 
   my $doSC   = grep { $_ != 0 } @sc; 

   if ( !$doClip && !$doTR && !$doSC && !$ro) {
      $_[0]-> emit('gsave') if $_[1];
      return;
   }

   @cr = $_[0]-> pixel2point( @cr);
   @tp = $_[0]-> pixel2point( @tp);
   
   $_[0]-> emit('grestore') unless $_[1];
   $_[0]-> emit('gsave');
   $_[0]-> emit(<<CLIP) if $doClip;
newpath $cr[0] $cr[1] moveto 0 $cr[2] rlineto $cr[3] 0 rlineto 0 $mcr2 rlineto closepath clip
CLIP
   $_[0]-> emit("@tp translate") if $doTR;
   $_[0]-> emit("@sc scale") if $doSC;
   $_[0]-> emit("$ro rotate") if $ro != 0;
   $_[0]-> {changed}-> {$_} = 1 for qw(fill linePattern lineWidth lineEnd font);
}

=item fill and stroke

Wrappers for PS outline that is expected to be filled or stroked.
Apply colors, line and fill styles if necessary.

=cut

sub fill
{
   my ( $self, $start, $code, $end) = @_;
   my ( $r1, $r2) = ( $self-> rop, $self-> rop2);
   return if 
      $r1  == rop::NoOper &&
      $r1 == rop::NoOper;
   
   $self-> emit( $start) if length $start;
   if ( $r2 != rop::NoOper && $self-> {fpType} ne 'F') {
      my $bk = 
         ( $r2 == rop::Blackness) ? 0 :
         ( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;
      
      $self-> {changed}-> {fill} = 1;
      $self-> emit( $self-> cmd_rgb( $bk)); 
      $self-> emit( $code);
   }
   if ( $r1 != rop::NoOper && $self-> {fpType} ne 'B') {
      my $c = 
         ( $r1 == rop::Blackness) ? 0 :
         ( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;
      if ($self-> {changed}-> {fill}) {
         if ( $self-> {fpType} eq 'F') {
            $self-> emit( $self-> cmd_rgb( $c));
         } else {
            my ( $r, $g, $b) = (
               int((($c & 0xff0000) >> 16) * 100 / 256 + 0.5) / 100, 
               int((($c & 0xff00) >> 8) * 100 / 256 + 0.5) / 100, 
               int(($c & 0xff)*100/256 + 0.5) / 100);
            if ( $self-> {grayscale}) {
               my $i = int( 100 * ( 0.31 * $r + 0.5 * $g + 0.18 * $b) + 0.5) / 100; 
               $self-> emit(<<GRAYPAT);
[\/Pattern \/DeviceGray] setcolorspace
$i Pat_$self->{fpType} setcolor
GRAYPAT
            } else {
               $self-> emit(<<RGBPAT);
[\/Pattern \/DeviceRGB] setcolorspace
$r $g $b Pat_$self->{fpType} setcolor
RGBPAT
            }
         }
         $self-> {changed}-> {fill} = 0;
      }
      $self-> emit( $code);
   }
   $self-> emit( $end) if length $end;
}

sub stroke
{
   my ( $self, $start, $code, $end) = @_; 

   my ( $r1, $r2) = ( $self-> rop, $self-> rop2);
   my $lp = $self-> linePattern;
   return if 
      $r1  == rop::NoOper &&
      $r1 == rop::NoOper;

   $self-> emit( $start) if length $start;

   if ( $r2 != rop::NoOper && $lp ne lp::Solid ) {
      my $bk = 
         ( $r2 == rop::Blackness) ? 0 :
         ( $r2 == rop::Whiteness) ? 0xffffff : $self-> backColor;
      
      $self-> {changed}-> {linePattern} = 1;
      $self-> {changed}-> {fill}        = 1;
      $self-> emit( $self-> cmd_rgb( $bk)); 
      $self-> emit( $code);
   }
   
   if ( $r1 != rop::NoOper && length( $lp)) {
      my $fk = 
         ( $r1 == rop::Blackness) ? 0 :
         ( $r1 == rop::Whiteness) ? 0xffffff : $self-> color;
         
      if ( $self-> {changed}-> {linePattern}) {
         if ( length( $lp) == 1) {
            $self-> emit('[] 0 setdash');
         } else {
            my @x = split('', $lp);
            push( @x, 0) if scalar(@x) % 1;
            @x = map { ord($_) } @x;
            $self-> emit("[@x] 0 setdash");
         }
         $self-> {changed}-> {linePattern} = 0;
      }

      if ( $self-> {changed}-> {lineWidth}) {
         $self-> emit( $self-> lineWidth . ' setlinewidth');       
         $self-> {changed}-> {lineWidth} = 0;
      }

      if ( $self-> {changed}-> {lineEnd}) { 
         my $le = $self-> lineEnd;
         my $id = ( $le == le::Round) ? 1 : (( $le == le::Square) ? 2 : 0);
         $self-> emit( "$id setlinecap");
      }

      if ( $self-> {changed}-> {fill}) {
         $self-> emit( $self-> cmd_rgb( $fk));
      }
      $self-> emit( $code);
   }
   $self-> emit( $end) if length $end;
   
}

# Prima::Printer interface

sub begin_doc
{
   my ( $self, $docName) = @_;
   return 0 if $self-> get_paint_state;
   $self-> {psData}  = [];
   $self-> {canDraw} = 1;

   $docName = $::application ? $::application-> name : "Prima::PS::Printer"
      unless defined $docName;
   my $data = scalar localtime;
   my @b2 = (
      $self-> {pageSize}-> [0] - $self-> {pageMargins}-> [2],
      $self-> {pageSize}-> [1] - $self-> {pageMargins}-> [3]
   );
   
   $self-> {fpHash}  = {};
   $self-> {pages}   = 1;

   my ($x,$y) = (
      $self-> {pageSize}-> [0] - $self-> {pageMargins}-> [0] - $self-> {pageMargins}-> [2],
      $self-> {pageSize}-> [1] - $self-> {pageMargins}-> [1] - $self-> {pageMargins}-> [3]
   );

   my $extras = '';
   my $setup = '';
   my %pd = defined( $self-> {pageDevice}) ? %{$self-> {pageDevice}} : ();
   
   if ( $self-> {copies} > 1) {
      $pd{NumCopies} = $self-> {copies};
      $extras .= "\%\%Requirements: numcopies($self->{copies})\n";
   }
   
   if ( scalar keys %pd) {
      my $jd = join( "\n", map { "/$_ $pd{$_}"} keys %pd);
      $setup .= <<NUMPAGES; 
%%BeginFeature
<< $jd >> setpagedevice
%%EndFeature
NUMPAGES
   }
   $self-> {localeData} = {};
   $self-> {fontLocaleData} = {};
   
   $self-> emit( <<PSHEADER);
%!PS-Adobe-2.0
%%Title: $docName
%%Creator: Prima::PS::Printer
%%CreationDate: $data
%%Pages: (atend)
%%BoundingBox: @{$self->{pageMargins}}[0,1] @b2
$extras
%%LanguageLevel: 2
%%DocumentNeededFonts: (atend)
%%DocumentSuppliedFonts: (atend)
%%EndComments

%%BeginSetup
$setup
%%EndSetup

%%Page: 1 1
PSHEADER

   $self-> {pagePrefix} = <<PREFIX;
@{$self->{pageMargins}}[0,1] translate
newpath 0 0 moveto 0 $y rlineto $x 0 rlineto 0 -$y rlineto closepath clip
PREFIX

   $self-> {pagePrefix} .= <<REV if $self-> {reversed};
0 0 moveto   
90 rotate
0 -$x translate
REV

   $self-> {changed} = { map { $_ => 0 } qw(fill lineEnd linePattern lineWidth font)};
   $self-> {docFontMap} = {};
   $self-> {setupEmitIndex} = scalar @{$self->{psData}} - 1;
   
   $self-> SUPER::begin_paint;
   $self-> save_state;
   
   $self-> {delay} = 1;
   $self-> restore_state;
   $self-> {delay} = 0;
   
   $self-> emit( $self-> {pagePrefix});
   $self-> change_transform( 1);
   $self-> {changed}-> {linePattern} = 0; 
   
   return 1;
}

sub abort_doc
{
   my $self = $_[0];
   return unless $self-> {canDraw};
   $self-> {canDraw} = 0; 
   $self-> SUPER::end_paint;
   $self-> restore_state;
   delete $self-> {$_} for 
      qw (saveState localeData psData changed fontLocaleData pagePrefix);
   $self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
}

sub end_doc
{
   my $self = $_[0];
   return unless $self-> {canDraw};

   $self-> emit(<<PSFOOTER);
grestore      
showpage   

%%Trailer
%%DocumentNeededFonts:
%%DocumentSuppliedFonts:
%%Pages: $_[0]->{pages}
%%EOF
PSFOOTER

   if ( $self-> {locale}) {
      my @z = map { '/' . $_ } keys %{$self-> {docFontMap}};
      my $xcl = "/FontList [@z] def\n";
      
   }
   
   $self-> {canDraw} = 0; 
   $self-> SUPER::end_paint;
   $self-> restore_state;
   my $p = $self-> {psData};
   delete $self-> {$_} for 
      qw (saveState localeData changed fontLocaleData psData pagePrefix);
   $self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
   $self-> spool( $p);
}

# Prima::Drawable interface

sub begin_paint { return $_[0]-> begin_doc; }
sub end_paint   {        $_[0]-> abort_doc; }

sub begin_paint_info
{
   my $self = $_[0];
   return 0 if $self-> get_paint_state;
   my $ok = $self-> SUPER::begin_paint_info;
   return 0 unless $ok;
   $self-> save_state;
}

sub end_paint_info
{
   my $self = $_[0];
   return if $self-> get_paint_state != 2;
   $self-> SUPER::end_paint_info;
   $self-> restore_state;
}

sub new_page
{
   return unless $_[0]-> {canDraw};
   my $self = $_[0];
   $self-> {pages}++;
   $self-> emit('grestore');
   $self-> emit("showpage\n%%Page: $self->{pages} $self->{pages}\n");
   $self-> $_( @{$self-> {saveState}->{$_}}) for qw( transform clipRect);
   $self-> change_transform(1);
   $self-> emit( $self-> {pagePrefix});
}

sub pages { $_[0]-> {pages} }

=item spool

Prima::PS::Drawable is not responsible for output of
generated document, it just calls ::spool when document
is closed through ::end_doc. By default just skips data.
Prima::PS::Printer handles spooling logic.

=cut

sub spool
{
 #  my $p = $_[1];
 #  open F, "> ./test.ps";
 #  print F ($_, "\n") for @{$p};
 #  close F;
}   

# properties

sub color
{
   return $_[0]-> SUPER::color unless $#_;
   $_[0]-> SUPER::color($_[0]-> color_check( $_[1]));
   return unless $_[0]-> {canDraw};
   $_[0]-> {changed}-> {fill} = 1;
}

sub fillPattern
{
   return $_[0]-> SUPER::fillPattern unless $#_;
   $_[0]-> SUPER::fillPattern( $_[1]);
   return unless $_[0]-> {canDraw};
   
   my $self = $_[0];
   my @fp  = @{$self-> SUPER::fillPattern};
   my $solidBack = ! grep { $_ != 0 } @fp;
   my $solidFore = ! grep { $_ != 0xff } @fp;
   my $fpid;
   my @scaleto = $self-> pixel2point( 8, 8);
   if ( !$solidBack && !$solidFore) {
      $fpid = join( '', map { sprintf("%02x", $_)} @fp);
      unless ( exists $self-> {fpHash}-> {$fpid}) {
         $self-> emit( <<PATTERNDEF);
<< 
\/PatternType 1 \% Tiling pattern
\/PaintType 2 \% Uncolored
\/TilingType 1
\/BBox [ 0 0 @scaleto]
\/XStep $scaleto[0]
\/YStep $scaleto[1] 
\/PaintProc { begin 
gsave 
@scaleto scale
8 8 true
[8 0 0 8 0 0]
< $fpid > imagemask
grestore
end 
} bind
>> matrix makepattern 
\/Pat_$fpid exch def
      
PATTERNDEF
         $self-> {fpHash}-> {$fpid} = 1;
      }
   }
   $self-> {fpType} = $solidBack ? 'B' : ( $solidFore ? 'F' : $fpid);
   $self-> {changed}-> {fill} = 1; 
}

sub lineEnd
{
   return $_[0]-> SUPER::lineEnd unless $#_;
   $_[0]-> SUPER::lineEnd($_[1]);
   return unless $_[0]-> {canDraw};
   $_[0]-> {changed}-> {lineEnd} = 1; 
}

sub linePattern
{
   return $_[0]-> SUPER::linePattern unless $#_;
   $_[0]-> SUPER::linePattern($_[1]);
   return unless $_[0]-> {canDraw};
   $_[0]-> {changed}-> {linePattern} = 1; 
}

sub lineWidth
{
   return $_[0]-> SUPER::lineWidth unless $#_;
   $_[0]-> SUPER::lineWidth($_[1]);
   return unless $_[0]-> {canDraw};
   $_[0]-> {changed}-> {lineWidth} = 1; 
}

sub rop
{
   return $_[0]-> SUPER::rop unless $#_;
   my ( $self, $rop) = @_;
   $rop = rop::CopyPut if 
      $rop != rop::Blackness || $rop != rop::Whiteness || $rop != rop::NoOper;
   $self-> SUPER::rop( $rop);
}

sub rop2
{
   return $_[0]-> SUPER::rop2 unless $#_;
   my ( $self, $rop) = @_;
   $rop = rop::CopyPut if 
      $rop != rop::Blackness || $rop != rop::Whiteness || $rop != rop::NoOper;
   $self-> SUPER::rop2( $rop);
}

sub transform
{
   return $_[0]-> SUPER::transform unless $#_;
   my $self = shift;
   $self-> SUPER::transform(@_);
   $self-> change_transform;
}

sub clipRect
{
   return @{$_[0]-> {clipRect}} unless $#_;
   $_[0]-> {clipRect} = [@_[1..4]];
   $_[0]-> change_transform;
}

sub region
{
   return undef;
}

sub scale
{
   return @{$_[0]-> {scale}} unless $#_;
   my $self = shift;
   $self-> {scale} = [@_[0,1]];
   $self-> change_transform;
}

sub reversed
{
   return @{$_[0]-> {reversed}} unless $#_;
   my $self = $_[0];
   $self-> {reversed} = $_[1] unless $self-> get_paint_state;
   $self-> calc_page;
}


sub rotate
{
   return $_[0]-> {rotate} unless $#_;
   my $self = $_[0];
   $self-> {rotate} = $_[1];
   $self-> change_transform;
}


sub resolution
{
   return @{$_[0]-> {resolution}} unless $#_;
   return if $_[0]-> get_paint_state;
   my ( $x, $y) =  @_[1..2];
   return if $x <= 0 || $y <= 0;
   $_[0]-> {resolution} = [$x, $y];
   $_[0]-> calc_page;
}

sub copies
{
   return $_[0]-> {copies} unless $#_;
   $_[0]-> {copies} = $_[1] unless $_[0]-> get_paint_state;
}

sub pageDevice
{
   return $_[0]-> {pageDevice} unless $#_;
   $_[0]-> {pageDevice} = $_[1] unless $_[0]-> get_paint_state;
}

sub useDeviceFonts
{
   return $_[0]-> {useDeviceFonts} unless $#_;
   $_[0]-> {useDeviceFonts} = $_[1] unless $_[0]-> get_paint_state;
}

sub grayscale 
{
   return $_[0]-> {grayscale} unless $#_;
   $_[0]-> {grayscale} = $_[1] unless $_[0]-> get_paint_state;
}

sub locale
{
   return $_[0]-> {locale} unless $#_;
   my ( $self, $loc) = @_;
   $self-> {locale} = $loc;
   return unless $self-> {useDeviceFonts};
   my $lzc = defined($loc) ? $loc : POSIX::setlocale(1);
   my $le  = $self-> {localeEncoding} = Prima::PS::Encodings::load( $lzc);
   return unless $self-> {canDraw};
   
   unless ( scalar keys %{$self-> {localeData}}) {
      return if ! defined($loc);
      $self-> emit( <<ENCODER);
\/reencode_font { exch \/enco exch def
dup dup findfont dup length dict begin { 1 index 
\/FID ne{def}{pop pop}ifelse} forall \/Encoding 
enco def currentdict end definefont } bind def
ENCODER
   }

   unless ( exists $self-> {localeData}-> {$loc}) {
      $self-> {localeData}-> {$loc} = 1;
      $self-> emit( "/Encoding_$loc [");
      my $i = 0;
      for ( $i = 0; $i < 16; $i++) {
         $self-> emit( join('', map {'/' . $_ } @$le[$i * 16 .. $i * 16 + 15]));
      }
      $self-> emit("] def\n");
   }
}

sub calc_page
{
   my $self = $_[0];
   my @s =  @{$self-> {pageSize}};
   my @m =  @{$self-> {pageMargins}};
   if ( $self-> {reversed}) {
      @s = @s[1,0];
      @m = @m[1,0,3,2];
   }
   $self-> {size} = [
      int(( $s[0] - $m[0] - $m[2]) * $self-> {resolution}-> [0] / 72.27 + 0.5),
      int(( $s[1] - $m[1] - $m[3]) * $self-> {resolution}-> [1] / 72.27 + 0.5),
   ];
}

sub pageSize
{
   return @{$_[0]-> {pageSize}} unless $#_;
   my ( $self, $px, $py) = @_;
   return if $self-> get_paint_state;
   $px = 1 if $px < 1;
   $py = 1 if $py < 1;
   $self-> {pageSize} = [$px, $py];
   $self-> calc_page;
}

sub pageMargins
{
   return @{$_[0]-> {pageMargins}} unless $#_;
   my ( $self, $px, $py, $px2, $py2) = @_;
   return if $self-> get_paint_state;
   $px = 0 if $px < 0;
   $py = 0 if $py < 0;
   $px2 = 0 if $px2 < 0;
   $py2 = 0 if $py2 < 0;
   $self-> {pageMargins} = [$px, $py, $px2, $py2];
   $self-> calc_page;
}

sub size
{
   return @{$_[0]-> {size}} unless $#_;
   $_[0]-> raise_ro("size");
}

# primitives

sub arc
{
   my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $end -= $start;
   $self-> stroke( <<ARC,
$x $y moveto
gsave   
$x $y translate
1 $try scale $start rotate
ARC
      "newpath $rx 0 moveto 0 0 $rx 0 $end arc stroke",
      "grestore");
}

sub chord
{
   my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $end -= $start;
   $self-> stroke(<<ARC,
$x $y moveto
gsave
$x $y translate
1 $try scale $start rotate
ARC
      "newpath $rx 0 moveto 0 0 $rx 0 $end arc closepath stroke",
      "grestore");
}

sub ellipse
{
   my ( $self, $x, $y, $dx, $dy) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $self-> stroke(<<ARC,
$x $y moveto
gsave   
$x $y translate
1 $try scale
ARC
        "newpath $rx 0 moveto 0 0 $rx 0 360 arc stroke",
        "grestore");
}

sub fill_chord
{
   my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $end -= $start;
   $self-> fill( <<START,
$x $y moveto
gsave   
$x $y translate
1 $try scale
START
     "newpath $rx 0 moveto 0 0 $rx 0 $end arc closepath fill",
     "grestore");
}

sub fill_ellipse
{
   my ( $self, $x, $y, $dx, $dy) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $self-> fill(<<ARC,
$x $y moveto
gsave   
$x $y translate
1 $try scale
ARC
   "newpath $rx 0 moveto 0 0 $rx 0 360 arc fill",
   "grestore");
}

sub sector
{
   my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $end -= $start;
   $self-> stroke(<<ARC,
$x $y moveto
gsave   
$x $y translate
1 $try scale $start rotate
ARC
      "newpath 0 0 moveto 0 0 $rx 0 $end arc 0 0 lineto stroke",
      "grestore");
}

sub fill_sector
{
   my ( $self, $x, $y, $dx, $dy, $start, $end) = @_;
   my $try = $dy / $dx;
   ( $x, $y, $dx, $dy) = $self-> pixel2point( $x, $y, $dx, $dy);
   my $rx = $dx / 2;
   $end -= $start;
   $self-> fill(<<ARC,
$x $y moveto
gsave   
$x $y translate
1 $try scale $start rotate
ARC

     "newpath 0 0 moveto 0 0 $rx 0 $end arc 0 0 lineto fill",
     "grestore");
}

sub text_out
{
   my ( $self, $text, $x, $y, $len) = @_;
   return unless $self-> {canDraw};
   $y += $self-> {font}-> {descent} if !$self-> textOutBaseline;
   ( $x, $y) = $self-> pixel2point( $x, $y); 
   if ( !defined( $len) || $len < 0) {
      $len = length( $text);
   } else {
      $text = substr( $text, 0, $len);
   }
  
   my $n = $self-> {typeFontMap}-> {$self-> {font}-> {name}};
   my $spec = exists ( $self-> {font}-> {encoding}) ? ( $self-> {font}-> {encoding} eq 'Specific') : 0;
   if ( $n == 1) {
       my $fn = $self-> {font}-> {docname};
       unless ( $spec || 
           ( !defined( $self-> {locale}) && !defined($self->{fontLocaleData}->{$fn})) ||
           ( defined( $self-> {locale}) && defined($self->{fontLocaleData}->{$fn}) && 
               ($self->{fontLocaleData}->{$fn} eq $self-> {locale}))) {
           $self-> {fontLocaleData}-> {$fn} = $self-> {locale};
           $self-> emit( "Encoding_$self->{locale} /$fn reencode_font");
           $self-> {changed}-> {font} = 1;
       }      

       if ( $self-> {changed}-> {font}) {
          my $szf = int( 7227 * $self->{font}->{size} / $self-> {resolution}-> [0] + 0.5) / 100;
          $self-> emit( "/$fn findfont $szf scalefont setfont");
          $self-> {changed}-> {font} = 0;
       }
   }
   my $wmul = $self-> {font}-> {width} / $self-> {fontWidthDivisor};
   $self-> emit("gsave");
   $self-> emit("$x $y translate");
   $self-> emit("$wmul 1 scale") if $wmul != 1;
   $self-> emit("0 0 moveto");
   if ( $self->{font}->{direction} != 0) {
      my $r = $self->{font}->{direction} / 10;
      $self-> emit("$r rotate");
   }
   my @rb;
   if ( $self-> textOpaque || $self-> {font}-> {style} & fs::Underlined) {
      my ( $ds, $bs) = ( $self-> {font}-> {direction}, $self-> textOutBaseline);
      $self-> {font}-> {direction} = 0;
      $self-> textOutBaseline(1) unless $bs;
      @rb = $self-> pixel2point( @{$self-> get_text_box( $text, $len)});
      $self-> {font}-> {direction} = $ds;
      $self-> textOutBaseline($bs) unless $bs;
   }
   if ( $self-> textOpaque) {
      $self-> emit( $self-> cmd_rgb( $self-> backColor)); 
      $self-> emit( "gsave newpath @rb[0,1] moveto @rb[2,3] lineto @rb[6,7] lineto @rb[4,5] lineto closepath fill grestore");
   }
   
   $self-> emit( $self-> cmd_rgb( $self-> color));
   my $i;
   my $m = length $text;
   my ( $rm, $nd) = $self-> get_rmap;
   my ( $xp, $yp) = ( $x, $y);
   my $c  = $self-> {font}-> {chardata}; 
   my $le = $self-> {localeEncoding};
   my $adv = 0;

   for ( $i = 0; $i < $m; $i++) {
      my $j = substr( $text, $i, 1);
      my $xr = $rm-> [ ord $j] || $nd;
      if ( $n == 1 && ( $le-> [ ord $j] ne '.notdef') && 
         ( $spec || exists ( $c-> {$le-> [ ord $j]}))
      ) {
         $j =~ s/([\\()])/\\$1/g; 
         my $adv2 = int( $adv * 100 + 0.5) / 100;
         $self-> emit( "$adv2 0 moveto") if $adv2 != 0;
         $self-> emit("($j) show");
      } elsif ( defined $rm-> [ord $j]) {
         my $adv2 = $adv + $$xr[1] * 72.27 / $self-> {resolution}-> [0]; 
         $adv2 = int( $adv * 100 + 0.5) / 100;
         my $pg = $self-> plate_glyph( ord $j);
         if ( length $pg) {
            $self-> emit( "$adv2 $self->{plate}->{yd} moveto");
            $self-> emit("gsave currentpoint translate");
            $self-> emit( $pg);
            $self-> emit("grestore");
         }
      } 
      $adv += ( $$xr[1] + $$xr[2] + $$xr[3]) * 72.27 / $self-> {resolution}-> [0];
   }
   
   #$text =~ s/([\\()])/\\$1/g;
   #$self-> emit("($text) show");
   
   if ( $self-> {font}-> {style} & fs::Underlined) {
      $self-> emit("[] 0 setdash 0 setlinecap 0 setlinewidth");
      $self-> emit("newpath $rb[0] 0 moveto $rb[4] 0 rlineto stroke");
   }
   $self-> emit("grestore");
}

sub bar
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   ( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
   $self-> fill('', 
     "newpath $x1 $y1 moveto $x1 $y2 lineto $x2 $y2 lineto $x2 $y1 lineto closepath fill", '');
}

sub rectangle
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   ( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
   $self-> stroke( '', 
      "newpath $x1 $y1 moveto $x1 $y2 lineto $x2 $y2 lineto $x2 $y1 lineto closepath stroke", '');
}

sub clear
{
   my ( $self, $x1, $y1, $x2, $y2) = @_; 
   if ( grep { ! defined } $x1, $y1, $x2, $y2) {
      ($x1, $y1, $x2, $y2) = $self-> clipRect;
      unless ( grep { $_ != 0 } $x1, $y1, $x2, $y2) {
         ($x1, $y1, $x2, $y2) = (0,0,@{$self->{size}});
      }
   }
   ( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
   my $c = $self-> cmd_rgb( $self-> backColor);
   $self-> emit(<<CLEAR);
$c
newpath $x1 $y1 moveto $x1 $y2 lineto $x2 $y2 lineto $x2 $y1 lineto closepath fill
CLEAR
   $self-> {changed}-> {fill} = 1;
}

sub line
{
   my ( $self, $x1, $y1, $x2, $y2) = @_;
   ( $x1, $y1, $x2, $y2) = $self-> pixel2point( $x1, $y1, $x2, $y2);
   $self-> stroke('', "newpath $x1 $y1 moveto $x2 $y2 lineto stroke", '');
}

sub lines
{
   my ( $self, $array) = @_;
   my $i; 
   my $c = scalar @$array;
   my @a = $self-> pixel2point( @$array);
   $c = int( $c / 4) * 4;
   my $z = '';
   for ( $i = 0; $i < $c; $i += 4) {
      $z .= "newpath @a[$i,$i+1] moveto @a[$i+2,$i+3] lineto stroke";
   }
   $self-> stroke( '', $z, '');
}

sub polyline
{
   my ( $self, $array) = @_;
   my $i; 
   my $c = scalar @$array;
   my @a = $self-> pixel2point( @$array);
   $c = int( $c / 2) * 2;
   return if $c < 2;
   my $z = "newpath @a[0,1] moveto ";
   for ( $i = 2; $i < $c; $i += 2) {
      $z .= "@a[$i,$i+1] lineto ";
   }
   $z .= "closepath stroke";
   $self-> stroke( '', $z, '');
}

sub fillpoly
{
   my ( $self, $array) = @_;
   my $i; 
   my $c = scalar @$array;
   $c = int( $c / 2) * 2;
   return if $c < 2;
   my @a = $self-> pixel2point( @$array); 
   my $x = "newpath @a[0,1] moveto ";
   for ( $i = 2; $i < $c; $i += 2) {
      $x .= "@a[$i,$i+1] lineto ";
   }
   $x .= "closepath fill";
   $self-> fill( '', $x, '');
}

sub flood_fill { return 0; }

sub pixel
{
   my ( $self, $x, $y, $pix) = @_;
   return cl::Invalid unless defined $pix;
   my $c = $self-> cmd_rgb( $pix);   
   ($x, $y) = $self-> pixel2point( $x, $y);
   $self-> emit(<<PIXEL);
gsave
$c
newpath $x $y moveto 0 0 rlineto fill
grestore
PIXEL
   $self-> {changed}-> {fill} = 1;
}


# methods

sub put_image_indirect
{
   return unless $_[0]-> {canDraw};
   my ( $self, $image, $x, $y, $xFrom, $yFrom, $xDestLen, $yDestLen, $xLen, $yLen) = @_;
   
   my $touch;
   $touch = 1, $image = $image-> image if $image-> isa('Prima::DeviceBitmap');

   
   unless ( $xFrom == 0 && $yFrom == 0 && $xLen == $image-> width && $yLen == $image-> height) {
      $image = $image-> extract( $xFrom, $yFrom, $xLen, $yLen);
      $touch = 1;
   }    

   my $ib = $image-> get_bpp;
   if ( $ib != $self-> get_bpp) {
      $image = $image-> dup unless $touch;     
      if ( $self-> {grayscale} || $image-> type & im::GrayScale) {
         $image-> type( im::Byte);
      } else {
         $image-> type( im::RGB); 
      }
   } elsif ( $self-> {grayscale} || $image-> type & im::GrayScale) {
      $image = $image-> dup unless $touch;     
      $image-> type( im::Byte);
   }
   
   $ib = $image-> get_bpp;
   $image-> type( im::RGB) if $ib != 8 && $ib != 24;
   
   
   my @is = $image-> size;
   ($x, $y, $xDestLen, $yDestLen) = $self-> pixel2point( $x, $y, $xDestLen, $yDestLen);
   my @fullScale = (
      $is[0] / $xLen * $xDestLen,
      $is[1] / $yLen * $yDestLen,
   );
   

   my $g  = $image-> data;
   my $bt = ( $image-> type & im::BPP) * $is[0] / 8;
   my $ls = int(( $is[0] * ( $image-> type & im::BPP) + 31) / 32) * 4; 
   my ( $i, $j);

   $self-> emit("gsave $x $y translate @fullScale scale");
   $self-> emit("/scanline $bt string def");
   $self-> emit("@is 8 [$is[0] 0 0 $is[1] 0 0]");
   $self-> emit('{currentfile scanline readhexstring pop}');
   $self-> emit(( $image-> type & im::GrayScale) ? "image" : "false 3 colorimage");

   for ( $i = 0; $i < $is[1]; $i++) {
      my $w  = substr( $g, $ls * $i, $bt);
      $w =~ s/(.)(.)(.)/$3$2$1/g if $ib == 24;
      $w =~ s/(.)/sprintf("%02x",ord($1))/eg;
      $self-> emit( $w);
   }
   $self-> emit('grestore');
}

sub get_bpp              { return $_[0]-> {grayscale} ? 8 : 24 }
sub get_nearest_color    { return $_[1] }
sub get_physical_palette { return $_[0]-> {grayscale} ? [map { $_, $_, $_ } 0..255] : 0 }
sub get_handle           { return 0 }
sub put_image            { $_[0]-> put_image_indirect( $_[3], $_[1], $_[2], 0, 0, $_[3]-> size, $_[3]-> size, $_[0]-> rop) }
sub stretch_image        { $_[0]-> put_image_indirect( $_[5], $_[1], $_[2], 0, 0, $_[3], $_[4], $_[5]-> size, $_[0]-> rop) }

# fonts
=item fonts

Returns Prima::Application::font plus those that defined into Prima::PS::Fonts module.

=cut
sub fonts
{
   my $f1;
   if ( $_[0]-> {useDeviceFonts}) {
      $f1 = Prima::PS::Fonts::enum_fonts($_[1]);
      return $f1 unless $::application;
   } else {
      $f1 = {};
   }
   
   my $f2 = $::application-> fonts;
   my %f = map { $_-> {name} => $_ } @$f2;
   $f{$_->{name}} = $_ for @$f1;
   my @x = map { $f{$_} } sort keys %f;
   return \@x;
}

sub get_font 
{ 
   my $z = {%{$_[0]-> {font}}};
   delete $z-> {charmap};
   delete $z-> {docname};
   return $z;
}

sub set_font 
{
   my ( $self, $font) = @_;
   my $n = exists($font-> {name}) ? $font-> {name} : $self-> {font}-> {name};
   $n = $self-> {useDeviceFonts} ? $Prima::PS::Fonts::defaultFontName : 'Default'
      unless defined $n;
   if ( $self-> {useDeviceFonts} && 
        ( exists $Prima::PS::Fonts::enum_families{ $n} || 
        exists $Prima::PS::Fonts::files{ $n})) {
      $self-> {font} = Prima::PS::Fonts::font_pick( $font, $self-> {font}); 
      $self-> {docFontMap}-> {$self-> {font}-> {docname}} = 1; 
      $self-> {typeFontMap}-> {$self-> {font}-> {name}} = 1; 
      $self-> {fontWidthDivisor} = $self-> {font}-> {maximalWidth};      
   } else {
      my $wscale = $font-> {width};
      delete $font-> {width};
      $self-> {font} = Prima::Drawable-> font_match( $font, $self-> {font});
      $self-> {typeFontMap}-> {$self-> {font}-> {name}} = 2; 
      $self-> {fontWidthDivisor} = $self-> {font}-> {width};
      $self-> {font}-> {width} = $wscale if $wscale;
   }
   $self-> {changed}-> {font} = 1;
   $self-> {plate}-> destroy, $self-> {plate} = undef if $self-> {plate};
}

my %fontmap = 
  (Prima::Application-> get_system_info->{apc} == apc::Win32) ? (
     'Helvetica' => 'Arial',
     'Times'     => 'Times New Roman',
     'Courier'   => 'Courier New',
  ) : ();

sub plate
{
   my $self = $_[0];
   return $self-> {plate} if $self-> {plate};
   my ( $dimx, $dimy) = ( $self-> {font}-> {maximalWidth}, $self-> {font}-> {height});
   my %f = %{$self-> {font}};
   $f{style} &= ~fs::Underlined;
   if ( $self-> {useDeviceFonts} && exists $Prima::PS::Fonts::files{$f{name}}) {
      $f{name} =~ s/^([^-]+)\-.*$/$1/;
      $f{pitch} = fp::Default unless $f{pitch} == fp::Fixed;
      $f{name} = $fontmap{$f{name}} if exists $fontmap{$f{name}};
   }
   # $f{height} = 72.27 * $self->{font}->{height} / $self-> {resolution}-> [0];
   delete $f{size};
   delete $f{width};
   delete $f{direction};
   $self-> {plate} = Prima::Image-> create(
      type   => im::BW,
      width  => $dimx,
      height => $dimy,
      font      => \%f,
      backColor => cl::Black,
      color     => cl::White,
      textOutBaseline => 1,
      preserveType => 1,
      conversion   => ict::None,
   );
   my ( $f, $l) = ( $self-> {plate}-> font-> {firstChar}, $self-> {plate}-> font-> {lastChar});
   my $x = $self-> {plate}-> {ABC} = $self-> {plate}-> get_font_abc( $f, $l);
   my $j = (230 - $f) * 3;
   return $self-> {plate};
}

sub plate_glyph
{
   my $z = $_[0]-> plate;
   my $x = $_[1];
   my $d  = $z-> font-> descent;
   my ( $dimx, $dimy) = $z-> size;
   my ( $f, $l) = ( $z-> font-> firstChar, $z-> font-> lastChar);
   my $ls = int(( $dimx + 31) / 32) * 4; 
   my $la = int ($dimx / 8) + (( $dimx & 7) ? 1 : 0);
   my $ax = ( $dimx & 7) ? (( 0xff << (7-( $dimx & 7))) & 0xff) : 0xff;
   
   my $xsf = 0;
   $x = $f if $x < $f || $x > $l;

   my $abc = $z-> {ABC};
   my ( $a, $b, $c) = (
      $abc->[ ( $x - $f) * 3],
      $abc->[ ( $x - $f) * 3 + 1],
      $abc->[ ( $x - $f) * 3 + 2],
   );
   return '' if $b <= 0;
   $z-> begin_paint;
   $z-> clear;
   $z-> text_out( chr( $x), ($a < 0) ? -$a : 0, $d - 1);
   $z-> end_paint;
   my $dd = $z-> data;
   my ($j, $k);
   my @emmap = (0) x $dimy;
   my @bbox = ( $a, 0, $b - $a, $dimy - 1);
   for ( $j = $dimy - 1; $j >= 0; $j--) {
      #my @ss  = map { my $x = ord $_; map { ($x & (0x80>>$_))?'X':'.'} 0..7 } split( '', substr( $dd, $ls * $j, $la)); 
      my @xdd = map { ord $_ } split( '', substr( $dd, $ls * $j, $la));
      #print "@ss @xdd\n";
      $xdd[-1] &= $ax;
      $emmap[$j] = 1 unless grep { $_ } @xdd;
   }
   for ( $j = 0; $j < $dimy; $j++) {
      last unless $emmap[$j];
      $bbox[1]++;
   }
   for ( $j = $dimy - 1; $j >= 0; $j--) {
      last unless $emmap[$j];
      $bbox[3]--;
   }
   
   if ( $bbox[3] >= 0) {
      $bbox[1] -= $d;
      $bbox[3] -= $d;
      my $zd = $z-> extract( 
         ( $a < 0) ? 0 : $a,
         $bbox[1] + $d,
         $b,
         $bbox[3] - $bbox[1] + 1,
      );
      # $z-> save("a.gif");
      
      my $bby = $bbox[3] - $bbox[1] + 1;
      my $zls = int(( $b + 31) / 32) * 4; 
      my $zla = int ($b / 8) + (( $b & 7) ? 1 : 0);
      $zd = $zd-> data;
      my $cd = '';
      for ( $j = $bbox[3] - $bbox[1]; $j >= 0; $j--) {
         $cd .= substr( $zd, $j * $zls, $zla);
      }

      my $cdz = '';
      for ( $j = 0; $j < length $cd; $j++) {
         $cdz .= sprintf("%02x", ord substr( $cd, $j, 1));
      }

      $_[0]-> {plate}-> {yd} = $bbox[1] * 72.27 / $_[0]-> {resolution}-> [1];
      my $scalex = 72.27 * $b   / $_[0]-> {resolution}-> [0];
      my $scaley = 72.27 * $bby / $_[0]-> {resolution}-> [1];
      return "$scalex $scaley scale $b $bby true [$b 0 0 -$bby 0 $bby] <$cdz> imagemask";
   } 
   return '';
}

sub get_rmap
{
   my @rmap;
   my $c = $_[0]-> {font}-> {chardata};
   my $le = $_[0]-> {localeEncoding};
   my $nd = $c-> {'.notdef'};
   my $fs = $_[0]-> {font}-> {size} / 1000;
   if ( defined $nd) {
      $$nd[$_] *= $fs for 1..3;
   } else {
      $nd = [0,0,0,0];
   }

#  for ( keys %$c) {
#     next if $c-> {$_}->[0] < 0;
#     $rmap[$c-> {$_}->[0]] = $c->{$_};
#  }
 
   my ( $f, $l) = ( $_[0]-> {font}-> {firstChar}, $_[0]-> {font}-> {lastChar});
   my $i;
   my $abc;
   if ( $_[0]-> {typeFontMap}-> {$_[0]-> {font}-> {name}} == 1) {
      for ( $i = 0; $i < 255; $i++) {
         if (( $le-> [$i] ne '.notdef') && $c-> { $le->[ $i]}) {
            $rmap[$i] = [ $i, map { $_ * $fs } @{$c-> { $le->[ $i]}}[1..3]];
         } elsif ( $i >= $f && $i <= $l) {
            $abc = $_[0]-> plate-> {ABC} unless $abc; 
            my $j = ( $i - $f) * 3; 
            $rmap[$i] = [ $i, @$abc[ $j .. $j + 2]];   
         }
      }
   } else {
      $abc = $_[0]-> plate-> {ABC};
      for ( $i = $f; $i <= $l; $i++) {
         my $j = ( $i - $f) * 3;
         $rmap[$i] = [ $i, @$abc[ $j .. $j + 2]];
      }
   }
#  @rmap = map { $c-> {$_} } @{$_[0]-> {localeEncoding}};
   
   return \@rmap, $nd;
}

sub get_font_abc
{
   my ( $self, $first, $last) = @_;
   my $lim = ( defined ($self-> {font}-> {encoding}) && $self-> {font}-> {encoding} eq 'Specific') ? 255 : 127;
   $first = 0    if !defined $first || $first < 0;
   $first = $lim if $first > $lim;
   $last  = $lim if !defined $last || $last < 0 || $last > $lim;
   my $i;
   my @ret; 
   my ( $rmap, $nd) = $self-> get_rmap;
   my $wmul = $self-> {font}-> {width} / $self-> {fontWidthDivisor};
   for ( $i = $first; $i < $last; $i++) {
      my $cd = $rmap-> [ $i] || $nd;
      push( @ret, map { $_ * $wmul } @$cd[1..3]);
   }
   return \@ret;
}

sub get_text_width
{
   my ( $self, $text, $len, $addOverhang) = @_;
   if ( !defined( $len) || $len < 0) {
      $len = length( $text);
   } else {
      $text = substr( $text, 0, $len);
   }
   return 0 unless $len;
   my $i;
   my ( $rmap, $nd) = $self-> get_rmap;
   my $cd;
   my $w = 0;
   
   for ( $i = 0; $i < $len; $i++) {
      my $cd = $rmap-> [ ord( substr( $text, $i, 1))] || $nd;
      $w += $cd->[1] + $cd->[2] + $cd-> [3];
   }
   
   if ( $addOverhang) {
      $cd = $rmap-> [ ord( substr( $text, 0, 1))] || $nd; 
      $w += ( $cd->[1] < 0) ? -$cd->[1] : 0; 
      $cd = $rmap-> [ ord( substr( $text, $len - 1, 1))] || $nd; 
      $w += ( $cd->[3] < 0) ? -$cd->[3] : 0; 
   }
   return $w * $self-> {font}-> {width} / $self-> {fontWidthDivisor}; 
}

sub get_text_box
{
   my ( $self, $text, $len) = @_;
   if ( !defined( $len) || $len < 0) {
      $len = length( $text);
   } else {
      $text = substr( $text, 0, $len);
   }
   my ( $rmap, $nd) = $self-> get_rmap;
   my $cd;
   my $wmul = $self-> {font}-> {width} / $self-> {fontWidthDivisor};
   $cd = $rmap-> [ ord( substr( $text, 0, 1))] || $nd; 
   my $ovxa = $wmul * (( $cd->[1] < 0) ? -$cd-> [1] : 0);
   $cd = $rmap-> [ ord( substr( $text, $len - 1, 1))] || $nd; 
   my $ovxb = $wmul * (( $cd->[3] < 0) ? -$cd-> [3] : 0);
   
   my $w = $self-> get_text_width( $text, $len);
   my @ret = (
      -$ovxa,      $self-> {font}-> {ascent},
      -$ovxa,     -$self-> {font}-> {descent}, 
      $w - $ovxb,  $self-> {font}-> {ascent},
      $w - $ovxb, -$self-> {font}-> {descent},
      $w, 0
   );
   unless ( $self-> textOutBaseline) {
      $ret[$_] += $self-> {font}-> {descent} for (1,3,5,7,9);
   }
   if ( $self-> {font}-> {direction} != 0) {
      my $s = sin( $self-> {font}-> {direction} / 572.9577951);
      my $c = cos( $self-> {font}-> {direction} / 572.9577951);
      my $i;
      for ( $i = 0; $i < 10; $i+=2) {
         my ( $x, $y) = @ret[$i,$i+1];
         $ret[$i]   = $x * $c - $y * $s;
         $ret[$i+1] = $x * $s + $y * $c;
      }
   }
   return \@ret;
}


1;
