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
#  Created by:
#     Dmitry Karasik <dk@plab.ku.dk> 
#
#  $Id$

use strict;
use Prima;
use Prima::IntUtils;
use Prima::ScrollBar;

package tb;
use vars qw(@oplen);

@oplen = ( 4, 2, 3, 4, 3, 2, 4);   # lengths of tb::OP_XXX constants ( see below ) + 1

# basic opcodes
use constant OP_TEXT               =>  0; # (3) text offset, text length, text width
use constant OP_COLOR              =>  1; # (1) 0xRRGGBB or COLOR_INDEX | palette_index
use constant OP_FONT               =>  2; # (2) op_font_mode, font info
use constant OP_TRANSPOSE          =>  3; # (3) move current point to delta X, delta Y
use constant OP_CODE               =>  4; # (2) code pointer and parameters 

# formatting opcodes
use constant OP_WRAP               =>  5; # (1) on / off
use constant OP_MARK               =>  6; # (3) id, x, y

# OP_TEXT 
use constant T_OFS                => 1;
use constant T_LEN                => 2;
use constant T_WID                => 3;

# OP_FONT
use constant F_MODE                => 1;
use constant F_DATA                => 2;

# OP_COLOR
use constant COLOR_INDEX           => 0x01000000; # index in colormap() array
use constant BACKCOLOR_FLAG        => 0x02000000; # OP_COLOR flag for backColor
use constant BACKCOLOR_DEFAULT     => BACKCOLOR_FLAG|COLOR_INDEX|1;
use constant COLOR_MASK            => 0xFCFFFFFF;

# OP_TRANSPOSE - indeces
use constant X_X     => 1;
use constant X_Y     => 2;
use constant X_FLAGS => 3;

# OP_TRANSPOSE - X_FLAGS constants
use constant X_DIMENSION_PIXEL       => 0;
use constant X_DIMENSION_FONT_HEIGHT => 1; # used only in formatting
use constant X_TRANSPOSE             => 0;
use constant X_EXTEND                => 2;

# block header indeces
use constant  BLK_FLAGS            => 0;
use constant  BLK_WIDTH            => 1;
use constant  BLK_HEIGHT           => 2;
use constant  BLK_X                => 3;
use constant  BLK_Y                => 4;
use constant  BLK_APERTURE_X       => 5;
use constant  BLK_APERTURE_Y       => 6;
use constant  BLK_TEXT_OFFSET      => 7;
use constant  BLK_DATA_START       => 8;
use constant  BLK_FONT_ID          => BLK_DATA_START; 
use constant  BLK_FONT_SIZE        => 9; 
use constant  BLK_FONT_STYLE       => 10;
use constant  BLK_COLOR            => 11; 
use constant  BLK_DATA_END         => 12;
use constant  BLK_BACKCOLOR        => BLK_DATA_END;
use constant  BLK_START            => BLK_DATA_END + 1;

# OP_FONT again
use constant  F_ID    => BLK_FONT_ID;
use constant  F_SIZE  => BLK_FONT_SIZE;
use constant  F_STYLE => BLK_FONT_STYLE;

# BLK_FLAGS constants
use constant T_SIZE      => 0x1;
use constant T_WRAPABLE  => 0x2;

# realize_state mode

use constant REALIZE_FONTS   => 0x1;
use constant REALIZE_COLORS  => 0x2;
use constant REALIZE_ALL     => 0x3;


use constant YMAX => 1000;

=pod

  The text information is presented by a set of text blocks,
  each for one line.  A text layout block is an array, logically
  separated in two parts: the first is BLK_START-1 header parameters,
  and second - tb::OP_XXX constants and their parameters. A tb::OP_XXX
  command has $tb::oplen[ tb::OP_XXX] parameters.

=cut

sub block_create
{
   my $ret = [ ( 0 ) x BLK_START ];
   $$ret[ BLK_FLAGS ] |= T_SIZE;
   push @$ret, @_;
   return $ret;
}

sub block_count
{
   my $block = $_[0];
   my $ret = 0;
   my ( $i, $lim) = ( BLK_START, scalar @$block);
   $i += $oplen[$$block[$i]], $ret++ while $i < $lim;
   return $ret;
}

# creates a new opcode for custom use
sub opcode
{
   my $len = $_[0] || 0;
   $len = 0 if $len < 0;
   push @oplen, $len + 1;
   return scalar(@oplen) - 1;
}


sub text           { return OP_TEXT, $_[0], $_[1], 0 }
sub color          { return OP_COLOR, $_[0] } 
sub backColor      { return OP_COLOR, $_[0] | BACKCOLOR_FLAG}
sub colorIndex     { return OP_COLOR, $_[0] | COLOR_INDEX }  
sub backColorIndex { return OP_COLOR, $_[0] | COLOR_INDEX | BACKCOLOR_FLAG}  
sub fontId         { return OP_FONT, F_ID, $_[0] }
sub fontSize       { return OP_FONT, F_SIZE, $_[0] }
sub fontStyle      { return OP_FONT, F_STYLE, $_[0] }
sub moveto         { return OP_TRANSPOSE, $_[0], $_[1],  $_[2] || 0 } 
sub extend         { return OP_TRANSPOSE, $_[0], $_[1], ($_[2] || 0) | X_EXTEND } 
sub code           { return OP_CODE, $_[0], $_[1] } 
sub wrap           { return OP_WRAP, $_[0] } 
sub mark           { return OP_MARK, $_[0], 0, 0 } 

package Prima::TextView::EventContent;

sub on_mousedown {}
sub on_mousemove {}
sub on_mouseup   {}

package Prima::TextView;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::GroupScroller);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
      borderWidth     => 2,
      colorMap        => [ $def-> {color}, $def-> {backColor} ],
      fontPalette     => [ { 
         name     => $def-> {font}-> {name},
         encoding => '',
         pitch    => fp::Default,
      }],
      hScroll         => 1,
      offset          => 0,
      paneWidth       => 0,
      paneHeight      => 0,
      paneSize        => [0,0],
      topLine         => 0,
      scaleChildren   => 0,
      selectable      => 1,
      textOutBaseline => 1,
      textRef         => '',
      vScroll         => 1,
      widgetClass     => wc::Edit,
      pointer         => cr::Text,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub profile_check_in
{
   my ( $self, $p, $default) = @_;
   $self-> SUPER::profile_check_in( $p, $default);
   if ( exists( $p-> { paneSize})) {
      $p-> { paneWidth}  = $p-> { paneSize}-> [ 0];
      $p-> { paneHeight} = $p-> { paneSize}-> [ 1];
   }
   $p-> { text} = '' if exists( $p->{ textRef});
}   

sub init
{
   my $self = shift;
   for ( qw( topLine scrollTransaction hScroll vScroll offset 
             paneWidth paneHeight borderWidth))
      { $self->{$_} = 0; }
   my %profile = $self-> SUPER::init(@_);
   $self-> {paneSize} = [0,0];
   $self-> {colorMap} = [];
   $self-> {fontPalette} = [];
   $self-> {blocks} = [];
   $self-> {defaultFontSize} = $self-> font-> size;
   $self-> {selection}   = [ -1, -1, -1, -1];
   $self-> {selectionPaintMode} = 0;
   $self-> {ymap} = [];
   $self-> setup_indents;
   for ( qw( colorMap fontPalette hScroll vScroll borderWidth paneWidth paneHeight offset topLine textRef))
      { $self->$_( $profile{ $_}); }
   return %profile;
}


sub reset_scrolls
{
   my $self = shift;
   my @sz = $self-> get_active_area( 2, @_);
   if ( $self-> {scrollTransaction} != 1 && $self->{vScroll})
   {
      $self-> {vScrollBar}-> set(
         max      => $self-> {paneHeight} - $sz[1],
         pageStep => int($sz[1] * 0.9),
         step     => $self-> font-> height,
         whole    => $self-> {paneHeight},
         partial  => $sz[1],
         value    => $self-> {topLine},
      );
   }
   if ( $self->{scrollTransaction} != 2 && $self->{hScroll})
   {
       $self-> {hScrollBar}-> set(
          max      => $self-> {paneWidth} - $sz[0],
          whole    => $self-> {paneWidth},
          value    => $self-> {offset},
          partial  => $sz[0],
          pageStep => int($sz[0] * 0.75),
       );
   }
}

sub on_size
{
   my ( $self, $oldx, $oldy, $x, $y) = @_;
   $self-> reset_scrolls( $x, $y);
}

sub on_fontchanged
{
   my $f = $_[0]-> font;
   $_[0]-> {defaultFontSize}           = $f-> size;
   $_[0]-> {fontPalette}->[0]-> {name} = $f-> name;
}

sub set
{
   my ( $self, %set) = @_;
   if ( exists $set{paneSize}) {
      $self-> paneSize( @{$set{paneSize}});
      delete $set{paneSize};
   }
   $self-> SUPER::set( %set);
}

sub text
{
   unless ($#_) {
      my $hugeScalarRef = $_[0]-> textRef;
      return $$hugeScalarRef;
   } else {
      my $s = $_[1];
      $_[0]-> textRef( \$s);
   }
}

sub textRef 
{
   return $_[0]-> {text} unless $#_;
   $_[0]-> {text} = $_[1] if $_[1];
}

sub paneWidth
{
   return $_[0]-> {paneWidth} unless $#_;
   my ( $self, $pw) = @_;
   $pw = 0 if $pw < 0;
   return if $pw == $self-> {paneWidth};
   $self-> {paneWidth} = $pw;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub paneHeight
{
   return $_[0]-> {paneHeight} unless $#_;
   my ( $self, $ph) = @_;
   $ph = 0 if $ph < 0;
   return if $ph == $self-> {paneHeight};
   $self-> {paneHeight} = $ph;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub paneSize
{
   return $_[0]-> {paneWidth}, $_[0]-> {paneHeight} if $#_ < 2;
   my ( $self, $pw, $ph) = @_;
   $ph = 0 if $ph < 0;
   $pw = 0 if $pw < 0;
   return if $ph == $self-> {paneHeight} && $pw == $self->{paneWidth};
   $self-> {paneWidth}  = $pw;
   $self-> {paneHeight} = $ph;
   $self-> reset_scrolls;
   $self-> repaint;
}

sub offset
{
   return $_[0]-> {offset} unless $#_;
   my ( $self, $offset) = @_;
   $offset = int($offset);
   my @sz = $self-> size;
   my @aa = $self-> get_active_area(2, @sz);
   my $pw = $self-> {paneWidth};
   $offset = $pw - $aa[0] if $offset > $pw - $aa[0];
   $offset = 0 if $offset < 0;
   return if $self->{offset} == $offset;
   my $dt = $offset - $self->{offset};
   $self-> {offset} = $offset;
   if ( $self->{hScroll} && $self->{scrollTransaction} != 2) {
      $self->{scrollTransaction} = 2;
      $self-> {hScrollBar}-> value( $offset);
      $self->{scrollTransaction} = 0;
   }
   $self-> scroll( -$dt, 0, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub topLine
{
   return $_[0]-> {topLine} unless $#_;
   my ( $self, $top) = @_;
   $top = int($top);
   my @sz = $self-> size;
   my @aa = $self-> get_active_area(2, @sz);
   my $ph = $self-> {paneHeight};
   $top = $ph - $aa[1] if $top > $ph - $aa[1];
   $top = 0 if $top < 0;
   return if $self->{topLine} == $top;
   my $dt = $top - $self->{topLine};
   $self-> {topLine} = $top;
   if ( $self->{vScroll} && $self->{scrollTransaction} != 1) {
      $self->{scrollTransaction} = 1;
      $self-> {vScrollBar}-> value( $top);
      $self->{scrollTransaction} = 0;
   }
   $self-> scroll( 0, $dt, clipRect => [ $self-> get_active_area(0, @sz)]);
}

sub VScroll_Change
{
   my ( $self, $scr) = @_;
   return if $self-> {scrollTransaction};
   $self-> {scrollTransaction} = 1;
   $self-> topLine( $scr-> value);
   $self-> {scrollTransaction} = 0;
}

sub HScroll_Change
{
   my ( $self, $scr) = @_;
   return if $self-> {scrollTransaction};
   $self-> {scrollTransaction} = 2;
   $self-> offset( $scr-> value);
   $self-> {scrollTransaction} = 0;
}

sub colorMap
{
   return [ @{$_[0]->{colorMap}}] unless $#_;
   my ( $self, $cm) = @_;
   $self-> {colorMap} = [@$cm];
   $self-> {colorMap}->[1] = $self-> backColor if scalar @$cm < 2;
   $self-> {colorMap}->[0] = $self-> color if scalar @$cm < 1;
   $self-> repaint;
}

sub fontPalette
{
   return [ @{$_[0]->{fontPalette}}] unless $#_;
   my ( $self, $fm) = @_;
   $self-> {fontPalette} = [@$fm];
   $self-> {fontPalette}->[0] = {
     name     => $self-> font-> name,
     encoding => '',
     pitch    => fp::Default,
   } if scalar @$fm < 1;
   $self-> repaint;
}

sub create_state
{
   my $self = $_[0];
   my $g = tb::block_create();
   $$g[ tb::BLK_FONT_SIZE] = $self-> {defaultFontSize};
   $$g[ tb::BLK_COLOR]     = tb::COLOR_INDEX;
   $$g[ tb::BLK_BACKCOLOR] = tb::BACKCOLOR_DEFAULT;
   return $g;
}

sub realize_state
{
   my ( $self, $canvas, $state, $mode) = @_;
   my %f = %{$self-> {fontPalette}->[ $$state[ tb::BLK_FONT_ID]]};
   $f{size} = $$state[ tb::BLK_FONT_SIZE];
   $f{style} = $$state[ tb::BLK_FONT_STYLE];
   $canvas-> set_font( \%f) if $mode & tb::REALIZE_FONTS;

   return unless $mode & tb::REALIZE_COLORS;
   if ( $self-> {selectionPaintMode}) {
      $self-> selection_state( $canvas);
   } else {
      $canvas-> set(
         color     =>  (( $$state[ tb::BLK_COLOR] & tb::COLOR_INDEX) ? 
                          ( $self-> {colorMap}-> [$$state[ tb::BLK_COLOR] & tb::COLOR_MASK]) :
                          ( $$state[ tb::BLK_COLOR] & tb::COLOR_MASK)),
         backColor =>  (( $$state[ tb::BLK_BACKCOLOR] & tb::COLOR_INDEX) ? 
                          ( $self-> {colorMap}-> [$$state[ tb::BLK_BACKCOLOR] & tb::COLOR_MASK]) : 
                          ( $$state[ tb::BLK_BACKCOLOR] & tb::COLOR_MASK)),
         textOpaque => (( $$state[ tb::BLK_BACKCOLOR] == tb::BACKCOLOR_DEFAULT) ? 0 : 1),
      );
   }
}


sub recalc_ymap
{
   my ( $self, $from) = @_;
   $self-> {ymap} = [] unless $from; # ok if $from == 0
   my $ymap = $self-> {ymap};
   my ( $i, $lim) = ( defined($from) ? $from : 0, scalar(@{$self->{blocks}}));
   my $b = $self->{blocks};
   for ( ; $i < $lim; $i++) {
      $_ = $$b[$i];
      my $y1 = $$_[ tb::BLK_Y];
      my $y2 = $$_[ tb::BLK_HEIGHT] + $y1;
      for ( int( $y1 / tb::YMAX) .. int ( $y2 / tb::YMAX)) {
         push @{$ymap-> [$_]}, $i; 
      }
   }
}

sub block_wrap
{
   my ( $self, $canvas, $b, $state, $width) = @_;
   $width = 0 if $width < 0;
   my ( $i, $lim) = ( tb::BLK_START, scalar @$b);

   my $cmd;
   my ( $t, $o) = ( $self-> {text}, $$b[ tb::BLK_TEXT_OFFSET]);
   my ( $x, $y) = (0, 0);
   my $f_taint;
   my $wrapmode = 1;
   my $stsave = $state;
   $state = [ @$state ];
   my ( $haswrapinfo, @wrapret);
   my ( @ret, $z);
   my $lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET];

   my $newblock = sub 
   {
      push @ret, $z = tb::block_create();
      @$z[ tb::BLK_DATA_START .. tb::BLK_DATA_END ] = @$state[ tb::BLK_DATA_START .. tb::BLK_DATA_END];
      $$z[ tb::BLK_X] = $$b[ tb::BLK_X];
      $$z[ tb::BLK_FLAGS] &= ~ tb::T_SIZE;
      $$z[ tb::BLK_TEXT_OFFSET] = -1;
      $x = 0;
   };

   my $retrace = sub 
   {
      $haswrapinfo = 0;
      splice( @{$ret[-1]}, $wrapret[0]); 
      @$state = @{$wrapret[2]};
      $newblock->();
      $$z[ tb::BLK_TEXT_OFFSET] = $wrapret[1];
      $i = $wrapret[3];
   };

   $newblock-> ();
   $$z[tb::BLK_TEXT_OFFSET] = $$b[tb::BLK_TEXT_OFFSET];

   my %state_hash;

# print "start - $$b[tb::BLK_TEXT_OFFSET] \n";

   # first state - wrap the block
# print "new wrap for $width\n";
   for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]]) {
      $cmd = $$b[$i];
      if ( $cmd == tb::OP_TEXT) {
# print "OP_TEXT @$b[$i+1..$i+3], x = $x\n";
         unless ( $f_taint) {
            $self-> realize_state( $canvas, $state, tb::REALIZE_FONTS);
            $f_taint = $canvas-> get_font;
            my $state_key = join('.', @$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]);
            $state_hash{$state_key} = $f_taint unless $state_hash{$state_key};
         }
         my $ofs  = $$b[ $i + 1];
         my $tlen = $$b[ $i + 2];
         $lastTextOffset = $ofs + $tlen unless $wrapmode;
      REWRAP: 
         my $tw = $canvas-> get_text_width( substr( $$t, $o + $ofs, $tlen), $tlen);
         my $apx = $f_taint-> {width};
# print "$x+$apx: new text $tw :|",substr( $$t, $o + $ofs, $tlen),"|\n";
         if ( $x + $tw + $apx <= $width) {
            push @$z, tb::OP_TEXT, $o + $ofs - $$z[ tb::BLK_TEXT_OFFSET], $tlen, $tw;
            $x += $tw;
# print "copied as is, advanced to $x, width $tw\n";
         } elsif ( $wrapmode) {
            next if $tlen <= 0;
            my $str = substr( $$t, $o + $ofs, $tlen);
            my $leadingSpaces = '';
            if ( $str =~ /^(\s+)/) {
               $leadingSpaces = $1;
               $str =~ s/^\s+//;
            }
            my $l = $canvas-> text_wrap( $str, $width - $apx - $x,
               tw::ReturnFirstLineLength | tw::WordBreak | tw::BreakSingle);
# print "repo $l bytes wrapped in $width - $apx - $x\n";
            if ( $l > 0) {
               push @$z, tb::OP_TEXT, $o + $ofs - $$z[ tb::BLK_TEXT_OFFSET], 
                  $l + length $leadingSpaces, 
                  $tw = $canvas-> get_text_width( $leadingSpaces . substr( $str, 0, $l));
# print "$x + advance $$z[-1] |", $canvas-> get_text_width( $leadingSpaces . substr( $str, 0, $l)), "|\n";
               $str = substr( $str, $l);
               $l += length $leadingSpaces;
               $newblock-> ();
               $$z[ tb::BLK_TEXT_OFFSET] = $o + $ofs + $l;
               $ofs += $l;
               $tlen -= $l;
# print "tx shift $l, str=|$str|, x=$x\n";
               if ( $str =~ /^(\s+)/) {
                  $$z[ tb::BLK_TEXT_OFFSET] += length( $1 );
                  $ofs  += length $1;
                  $tlen -= length $1;
                  $x    += $canvas-> get_text_width( $1);
                  $str =~ s/^\s+//;
               }
               goto REWRAP if length $str;
            } else { # does not fit into $width
# print "new block: x = $x |$str|\n";
               my $ox = $x;
               $newblock-> ();
               $$z[ tb::BLK_TEXT_OFFSET] = $o + $ofs + length $leadingSpaces;
               $ofs  += length $leadingSpaces;
               $tlen -= length $leadingSpaces; 
               if ( length $str) { # well, it cannot be fit into width,
                                   # but may be some words can be stripped?
                  goto REWRAP if $ox > 0;
                  if ( $str =~ m/^(\S+)(\s*)/) {
                     $tw = $canvas-> get_text_width( $1);
                     push @$z, tb::OP_TEXT, 0, length $1, $tw;
                     $x += $tw;
                     $ofs  += length($1) + length($2);
                     $tlen -= length($1) + length($2);
                     goto REWRAP;
                  }
               }
               push @$z, tb::OP_TEXT, 0, length($str), $x += $canvas-> get_text_width( $str);
            }
         } elsif ( $haswrapinfo) { # unwrappable, and cannot be fit - retrace
            $retrace->();
# print "retrace\n";
            next;
         } else { # unwrappable, cannot be fit, no wrap info! - whole new block
# print "new empty block - |", substr( $$t, $o + $ofs, $tlen), "|\n";
            push @$z, tb::OP_TEXT, $o + $ofs - $$z[ tb::BLK_TEXT_OFFSET], $tlen, $tw;
            $newblock-> ();
         }
      } elsif ( $cmd == tb::OP_WRAP) {
         if ( $wrapmode == 1 && $$b[ $i + 1] == 0) {
            @wrapret = ( scalar @$z, $lastTextOffset, [ @$state ], $i);
            $haswrapinfo = 1;
# print "wrap start record x = $x\n";
         }
         $wrapmode = $$b[ $i + 1];
# print "wrap: $wrapmode\n";
      } elsif ( $cmd == tb::OP_FONT) {
         if ( $$b[$i + 1] == tb::F_SIZE) {
            $$state[ $$b[$i + 1]] = $self-> {defaultFontSize} + $$b[$i + 2];
         } else {
            $$state[ $$b[$i + 1]] = $$b[$i + 2];
         }
         $f_taint = undef;
         push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
      } elsif ( $cmd == tb::OP_COLOR) {
         $$state[ tb::BLK_COLOR + (($$b[ $i + 1] & tb::BACKCOLOR_FLAG) ? 1 : 0)] = $$b[$i + 1];
         push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
      } elsif ( $cmd == tb::OP_TRANSPOSE) {
         my @r = @$b[ $i .. $i + 3];
         if ( $$b[ $i + tb::X_FLAGS] & tb::X_DIMENSION_FONT_HEIGHT) {
            unless ( $f_taint) {
               $self-> realize_state( $canvas, $state, tb::REALIZE_FONTS); 
               $f_taint = $canvas-> get_font;
               my $state_key = join('.', @$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]);
               $state_hash{$state_key} = $f_taint unless $state_hash{$state_key};
            }
            $r[ tb::X_X] *= $f_taint-> {height};
            $r[ tb::X_Y] *= $f_taint-> {height};
         } 
# print "advance block $x $r[tb::X_X]\n";
         if ( $x + $r[tb::X_X] >= $width) {
            if ( $wrapmode) {
               $newblock-> (); 
            } elsif ( $haswrapinfo) {
               $retrace->();
               next;
            } 
         } else {
            $x += $r[ tb::X_X];
         }
         push @$z, @r;
      } else {
         push @$z, @$b[ $i .. ( $i + $tb::oplen[ $cmd] - 1)];
      }
   }

   # remove eventual empty trailing blocks
   pop @ret while scalar ( @ret) && ( tb::BLK_START == scalar @{$ret[-1]});

   # second stage - position the blocks
   $state = $stsave;
   $f_taint = undef;
   my $start;
   if ( !defined $$b[ tb::BLK_Y]) { 
      # auto position the block if the creator didn't care
      $start = $$state[ tb::BLK_Y] + $$state[ tb::BLK_HEIGHT];
   } else {
      $start = $$b[ tb::BLK_Y];
   }

   $lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET];

   for ( @ret) {
      $b = $_;
      $$b[ tb::BLK_Y] = $start;
      $$b[ tb::BLK_TEXT_OFFSET] = $lastTextOffset if $$b[ tb::BLK_TEXT_OFFSET] < 0;

      ( $x, $y, $i, $lim) = ( 0, 0, tb::BLK_START, scalar @$b);
      for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]]) {
         $cmd = $$b[$i];
         if ( $cmd == tb::OP_TEXT) {
            $f_taint = $state_hash{ join('.', @$state[tb::BLK_FONT_ID .. tb::BLK_FONT_STYLE]) };
            $x += $$b[ $i + 3];
            $$b[ tb::BLK_WIDTH] = $x if $$b[ tb::BLK_WIDTH ] < $x;
            $$b[ tb::BLK_APERTURE_Y] = $f_taint-> {descent} - $y if $$b[ tb::BLK_APERTURE_Y] < $f_taint-> {descent} - $y;
            $$b[ tb::BLK_APERTURE_X] = $f_taint-> {width}   - $x if $$b[ tb::BLK_APERTURE_X] < $f_taint-> {width}   - $x;
            my $newY = $y + $f_taint-> {ascent} + $f_taint-> {externalLeading};
            $$b[ tb::BLK_HEIGHT] = $newY if $$b[ tb::BLK_HEIGHT] < $newY; 
            $lastTextOffset = $$b[ tb::BLK_TEXT_OFFSET] + $$b[ $i + 1] + $$b[ $i + 2];
         } elsif ( $cmd == tb::OP_FONT) {
            if ( $$b[$i + 1] == tb::F_SIZE) {
               $$state[ $$b[$i + 1]] = $self-> {defaultFontSize} + $$b[$i + 2];
            } else {
               $$state[ $$b[$i + 1]] = $$b[$i + 2];
            }
         } elsif ( $cmd == tb::OP_TRANSPOSE) {
            my ( $newX, $newY) = ( $x + $$b[ $i + tb::X_X], $y + $$b[ $i + tb::X_Y]);
            $$b[ tb::BLK_WIDTH]  = $newX if $$b[ tb::BLK_WIDTH ] < $newX;
            $$b[ tb::BLK_HEIGHT] = $newY if $$b[ tb::BLK_HEIGHT] < $newY;
            $$b[ tb::BLK_APERTURE_X] = $newX if $$b[ tb::BLK_APERTURE_X] > $newX;
            $$b[ tb::BLK_APERTURE_Y] = $newY if $$b[ tb::BLK_APERTURE_Y] > $newY;
            unless ( $$b[ $i + tb::X_FLAGS] & tb::X_EXTEND) {
               ( $x, $y) = ( $newX, $newY);
            }
         } elsif ( $cmd == tb::OP_MARK) {
            $$b[ $i + 2] = $x;
            $$b[ $i + 3] = $y;
         }
      }
      $$b[ tb::BLK_HEIGHT] += $$b[ tb::BLK_APERTURE_Y];
      $$b[ tb::BLK_WIDTH]  += $$b[ tb::BLK_APERTURE_X];
      $start += $$b[ tb::BLK_HEIGHT];
   }

   if ( $ret[-1]) {
      $b = $ret[-1];
      $$state[$_] = $$b[$_] for tb::BLK_X, tb::BLK_Y, tb::BLK_HEIGHT, tb::BLK_WIDTH;
   }

   return @ret;
}

sub selection_state
{
   my ( $self, $canvas) = @_;
   $canvas-> color( $self-> hiliteColor);
   $canvas-> backColor( $self-> hiliteBackColor);
   $canvas-> textOpaque(0);
}

sub on_paint
{
   my ( $self, $canvas) = @_;
   my @size = $canvas-> size;
   unless ( $self-> enabled) {
      $self-> color( $self-> disabledColor);
      $self-> backColor( $self-> disabledBackColor);
   }
   my ( $bw, $t, $offset, @aa) = (
     $self-> {borderWidth}, 
     $self-> { topLine}, $self-> { offset},
     $self-> get_active_area(1,@size));

   my @clipRect = $canvas-> clipRect;
   $canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $bw, 
      $self-> dark3DColor, $self-> light3DColor, $self-> backColor);

   my $bx = $self-> {blocks};
   my $lim = scalar @$bx;
   return unless $lim;

   my @cy = ( $aa[3] - $clipRect[3], $aa[3] - $clipRect[1]);
   $cy[0] = 0 if $cy[0] < 0;
   $cy[1] = $aa[3] - $aa[1] if $cy[1] > $aa[3] - $aa[1];
   $cy[$_] += $t for 0,1;

   $self-> clipRect( $self-> get_active_area( 1, @size));
   @clipRect = $self-> clipRect; 
   my $i = 0;
   my $b;

   my ( $sx1, $sy1, $sx2, $sy2) = @{$self->{selection}};

   for ( int( $cy[0] / tb::YMAX) .. int( $cy[1] / tb::YMAX)) {
      next unless $self-> {ymap}->[$_];
      for ( @{$self-> {ymap}->[$_]}) {
         my $j = $_;
         $b = $$bx[$j];
         my ( $x, $y) = ( 
            $aa[0] - $offset + $$b[ tb::BLK_X], 
            $aa[3] + $t - $$b[ tb::BLK_Y] - $$b[ tb::BLK_HEIGHT] 
         );
         next if $x + $$b[ tb::BLK_WIDTH]  < $clipRect[0] || $x > $clipRect[2] ||
                 $y + $$b[ tb::BLK_HEIGHT] < $clipRect[1] || $y > $clipRect[3] ||
                 $$b[ tb::BLK_WIDTH] == 0 || $$b[ tb::BLK_HEIGHT] == 0;
                 

         if ( $j == $sy1 || $j == $sy2) { # complex selection case
            my @cr = @clipRect;
            my $x1 = $x + $self-> text2xoffset(( $j == $sy1) ? $sx1 : $sx2, $j);
            my $eq = ( $j == $sy1 ) && ( $j == $sy2 );
            $self-> {selectionPaintMode} = ( $eq || $j == $sy1 ) ? 0 : 1;
            if ( $cr[0] <= $x1 ) { # left upper part
              $cr[2] = $x1 - 1 if $cr[2] > $x1 - 1;
              $cr[2] = $aa[2] if $cr[2] > $aa[2];
              $cr[2] = $aa[0] if $cr[2] < $aa[0];
              if ( $cr[0] <= $cr[2]) {
                  $self-> selection_state( $canvas) if $self-> {selectionPaintMode}; 
                  $self-> clipRect( @cr);
                  $self-> block_draw( $canvas, $b, $x, $y);
              }
               @cr = @clipRect;
            }
            $self-> {selectionPaintMode} = (( $eq || $j == $sy1 ) ? 1 : 0); 
            if ( $cr[2] >= $x1) { # right part
               $cr[0] = $x1 if $cr[0] < $x1;
               $cr[0] = $aa[0] if $cr[0] < $aa[0];
               $cr[0] = $aa[2] if $cr[0] > $aa[2];
               my $x2 = $x + $self-> text2xoffset( $sx2, $j);
               if ( $eq) { # selection is one block - center part
                  if ( $cr[0] <= $x2) {
                     my $cr2 = $cr[2];
                     $cr[2] = $x2 - 1 if $cr[2] > $x2 - 1;
                     $cr[2] = $aa[0] if $cr[2] < $aa[0];
                     $cr[2] = $aa[2] if $cr[2] > $aa[2];
                    if ( $cr[0] <= $cr[2]) {
                        $self-> selection_state( $canvas) if $self-> {selectionPaintMode}; 
                        $self-> clipRect( @cr);
                        $self-> block_draw( $canvas, $b, $x, $y);
                    }
                     @cr = @clipRect;
                  }
                  $cr[0] = $x2 if $cr[0] < $x2;
                  $cr[0] = $aa[0] if $cr[0] < $aa[0];
                  $cr[0] = $aa[2] if $cr[0] > $aa[2];
               }
               $self-> {selectionPaintMode} = ( $eq || $j == $sy2 ) ? 0 : 1;
               if ( $cr[0] <= $cr[2]) {
                   $self-> selection_state( $canvas) if $self-> {selectionPaintMode}; 
                   $self-> clipRect( @cr);
                   $self-> block_draw( $canvas, $b, $x, $y);
              }
            } 
            $self-> {selectionPaintMode} = 0;
            $self-> clipRect( @clipRect);
         } elsif ( $j > $sy1 && $j < $sy2) { # simple selection case
            $self-> {selectionPaintMode} = 1;
            $self-> selection_state( $canvas);
            $self-> block_draw( $canvas, $b, $x, $y);
            $self-> {selectionPaintMode} = 0;
         } else {
            $self-> block_draw( $canvas, $b, $x, $y);
         }
      }
   }

   $self-> {selectionPaintMode} = 0;
}


sub block_draw
{
   my ( $self, $canvas, $b, $x, $y) = @_;
   my ( $i, $lim) = ( tb::BLK_START, scalar @$b);

   my $cmd;
   my ( $t, $o) = ( $self-> {text}, $$b[ tb::BLK_TEXT_OFFSET]);
   my @state = @$b[ 0 .. tb::BLK_DATA_END ];
   my ( $f_taint, $c_taint); 

   $canvas-> clear( $x, $y, $x + $$b[ tb::BLK_WIDTH] - 1, $y + $$b[ tb::BLK_HEIGHT] - 1)
     if $self-> {selectionPaintMode};

   $x += $$b[ tb::BLK_APERTURE_X];
   $y += $$b[ tb::BLK_APERTURE_Y];

   for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
      $cmd = $$b[$i];
      if ( $cmd == tb::OP_TEXT) {
         if ( $$b[$i + 2] > 0) {
            unless ( $f_taint) {
               $self-> realize_state( $canvas, \@state, tb::REALIZE_FONTS); 
               $f_taint = $canvas-> get_font;
            }
            unless ( $c_taint) {
               $self-> realize_state( $canvas, \@state, tb::REALIZE_COLORS); 
               $c_taint = 1;
            }
            $canvas-> text_out( substr( $$t, $o + $$b[$i + 1], $$b[$i + 2]), 
                              $x, $y, $$b[ $i + 2]);
         }
         $x += $$b[ $i + 3];
      } elsif ( $cmd == tb::OP_FONT) {
         if ( $$b[$i + 1] == tb::F_SIZE) {
            $state[ $$b[$i + 1]] = $self-> {defaultFontSize} + $$b[$i + 2];
         } else {
            $state[ $$b[$i + 1]] = $$b[$i + 2];
         }
         $f_taint = undef;
      } elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
         ( $x, $y) = ( $x + $$b[ $i + tb::X_X], $y + $$b[ $i + tb::X_Y]);
      } elsif ( $cmd == tb::OP_CODE) {
         unless ( $f_taint) {
            $self-> realize_state( $canvas, \@state, tb::REALIZE_FONTS); 
            $f_taint = $canvas-> get_font;
         }
         unless ( $c_taint) {
            $self-> realize_state( $canvas, \@state, tb::REALIZE_FONTS); 
            $c_taint = 1;
         }
         $$b[ $i + 1]-> ( $self, $canvas, $b, \@state, $x, $y, $$b[ $i + 2]);
      } elsif ( $cmd == tb::OP_COLOR) {
         my $c = $$b[ $i + 1];
         $state[ tb::BLK_COLOR + (($$b[ $i + 1] & tb::BACKCOLOR_FLAG) ? 1 : 0)] = $$b[$i + 1];
         $c_taint = undef;
      }
   }
}

sub xy2info
{
   my ( $self, $x, $y) = @_;

   my $bx = $self-> {blocks};
   my ( $pw, $ph) = $self-> paneSize;
   $x = 0 if $x < 0;
   $x = $pw if $x > $pw;
   return (0,0) if $y < 0 || !scalar(@$bx) ;
   $x = $pw, $y = $ph if $y > $ph;
   
   my ( $b, $bid);

   my $xhint = 0;

   # find if there's a block that has $y in its inferior
   my $ymapix = int( $y / tb::YMAX);
   if ( $self-> {ymap}-> [ $ymapix]) {
      my ( $minxdist, $bdist, $bdistid) = ( $self-> {paneWidth} * 2, undef, undef);
      for ( @{$self-> {ymap}-> [ $ymapix]}) {
         my $z = $$bx[$_];
         if ( $y >= $$z[ tb::BLK_Y] && $y < $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT]) {
            if ( $x >= $$z[ tb::BLK_X] && $x < $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH]) {
               $b = $z;
               $bid = $_;
               last;
            } elsif ( abs($$z[ tb::BLK_X] - $x) < $minxdist || abs($$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x) < $minxdist) {
               $minxdist = ( abs( $$z[ tb::BLK_X] - $x) < $minxdist) ? 
                  abs( $$z[ tb::BLK_X] - $x) :
                  abs( $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH] - $x);
               $bdist = $z;
               $bdistid = $_;
            }
         }
      }
      if ( !$b && $bdist) {
         $b   = $bdist;
         $bid = $bdistid;
         $xhint = (( $$b[ tb::BLK_X] > $x) ? -1 : 1);
      }
   }

   # if still no block found, find the closest block down 
   unless ( $b) {
      my $minydist = $self-> {paneHeight} * 2;
      my $ymax = scalar @{$self->{ymap}};
      while ( $ymapix < $ymax) {
         if ( $self-> {ymap}-> [ $ymapix]) {
            for ( @{$self-> {ymap}-> [ $ymapix]}) {
               my $z = $$bx[$_];
               if ( $minydist > $$z[ tb::BLK_Y] - $y && $$z[ tb::BLK_Y] >= $y) {
                  $minydist = $$z[ tb::BLK_Y] - $y;
                  $b = $z;
                  $bid = $_;
               }
            }
         }
         last if $b;
         $ymapix++;
      }
      $ymapix = int( $y / tb::YMAX);
      $xhint = -1;
   }

   # if still no block found, assume EOT
   unless ( $b) {
      $b = $$bx[-1];
      $bid = scalar @{$bx} - 1;
      $xhint = 1;
   }

   if ( $xhint < 0) { # start of line
      return ( 0, $bid);
   } elsif ( $xhint > 0) { # end of line
      if ( $bid < ( scalar @{$bx} - 1)) {
         return ( $$bx[ $bid + 1]-> [ tb::BLK_TEXT_OFFSET] - $$b[ tb::BLK_TEXT_OFFSET], $bid);
      } else {
         return ( length( ${$self->{text}}) - $$b[ tb::BLK_TEXT_OFFSET], $bid);
      }
   }

   # find text offset
   my $bofs = $$b[ tb::BLK_TEXT_OFFSET];
   my ( $ofs, $unofs) = (0,0);
   my $pm = $self-> get_paint_state;
   $self-> begin_paint_info unless $pm;
   my $savefont  = $self-> get_font;
   my @state = @$b[ 0 .. tb::BLK_DATA_END ];
   my $f_taint;

   {
      my ( $i, $lim) = ( tb::BLK_START, scalar @$b);
      my $px = $$b[ tb::BLK_X];
      for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
         my $cmd = $$b[$i];
         if ( $cmd == tb::OP_TEXT) {
            my $npx = $px + $$b[$i+3];
            if ( $px > $x) {
               $ofs = $$b[ $i + 1]; 
               undef $unofs;
               last;
            } elsif ( $px <= $x && $npx > $x) {
               unless ( $f_taint) {
                  $self-> realize_state( $self, \@state, tb::REALIZE_FONTS); 
                  $f_taint = $self-> get_font;
               }
               $ofs = $$b[ $i + 1] + $self-> text_wrap( 
                   substr( ${$self-> {text}}, $bofs + $$b[ $i + 1], $$b[ $i + 2]),
                   $x - $px, tw::ReturnFirstLineLength | tw::BreakSingle);
               undef $unofs;
               last;
            } 
            $unofs = $$b[ $i + 1] + $$b[ $i + 2];
            $px = $npx;
         } elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
            $px += $$b[ $i + tb::X_X];
         } elsif ( $cmd == tb::OP_FONT) {
            if ( $$b[$i + 1] == tb::F_SIZE) {
               $state[ $$b[$i + 1]] = $self-> {defaultFontSize} + $$b[$i + 2];
            } else {
               $state[ $$b[$i + 1]] = $$b[$i + 2];
            }
            $f_taint = undef;
         }
      }
   }

   $pm ? $self-> set_font( $savefont) : $self-> end_paint_info;

   return defined( $unofs) ? $unofs : $ofs, $bid;
}

sub screen2point
{
   my ( $self, $x, $y, @size) = @_;

   @size = $self-> size unless @size;
   my @aa = $self-> get_active_area( 0, @size);

   $x -= $aa[0];
   $y  = $aa[3] - $y;
   $y += $self-> {topLine};
   $x += $self-> {offset};

   return $x, $y;
}

sub text2xoffset
{
   my ( $self, $x, $bid) = @_;

   my $b = $self-> {blocks}-> [$bid];
   return 0 unless $b;
   return 0 if $x <= 0; # XXX

   my $pm = $self-> get_paint_state;
   $self-> begin_paint_info unless $pm;
   my $savefont = $self-> get_font;
   my @state = @$b[ 0 .. tb::BLK_DATA_END ];
   my $f_taint;

   my ( $i, $lim) = ( tb::BLK_START, scalar @$b);
   my $px   = $$b[tb::BLK_APERTURE_X];
   my $bofs = $$b[tb::BLK_TEXT_OFFSET];
   for ( ; $i < $lim; $i += $tb::oplen[ $$b[ $i]] ) {
       my $cmd = $$b[$i];
       if ( $cmd == tb::OP_TEXT) {
          if ( $x >= $$b[$i+1]) {
             if ( $x < $$b[$i+1] + $$b[$i+2]) {
                unless ( $f_taint) {
                   $self-> realize_state( $self, \@state, tb::REALIZE_FONTS); 
                   $f_taint = $self-> get_font;
                }
                $px += $self-> get_text_width( 
                   substr( ${$self->{text}}, $bofs + $$b[$i+1], $x - $$b[$i+1] ),
                   $x - $$b[$i+1] );
                last;
             } elsif ( $x == $$b[$i+1] + $$b[$i+2]) {
                $px += $$b[$i+3];
                last;
             }
          }
          $px += $$b[$i+3];
       } elsif (( $cmd == tb::OP_TRANSPOSE) && !($$b[ $i + tb::X_FLAGS] & tb::X_EXTEND)) {
          $px += $$b[ $i + tb::X_X];
       } elsif ( $cmd == tb::OP_FONT) {
          if ( $$b[$i + 1] == tb::F_SIZE) {
             $state[ $$b[$i + 1]] = $self-> {defaultFontSize} + $$b[$i + 2];
          } else {
             $state[ $$b[$i + 1]] = $$b[$i + 2];
          }
          $f_taint = undef;
       }
   }
   $pm ? $self-> set_font( $savefont) : $self-> end_paint_info;
   return $px;
}

sub info2text_offset
{
   my ( $self, $ofs, $blk) = @_;
   if ( $blk >= 0 && $ofs >= 0) {
      return $self-> {blocks}->[$blk]->[tb::BLK_TEXT_OFFSET] + $ofs;
   } else {
      return length ${$self->{text}};
   }
}

sub text_offset2info
{
   my ( $self, $ofs) = @_;
   my $blk = $self-> text_offset2block( $ofs);
   return undef unless defined $blk;
   return $ofs - $self-> {blocks}-> [$blk]-> [ tb::BLK_TEXT_OFFSET], $blk;
}

sub info2xy
{
   my ( $self, $ofs, $blk) = @_;
   $blk = $self-> {blocks}->[$blk];
   return undef unless defined $blk;
   return @$blk[ tb::BLK_X, tb::BLK_Y];
}

sub text_offset2block
{
   my ( $self, $ofs) = @_;
   
   my $bx = $self-> {blocks};
   my $end = length ${$self-> {text}};
   my $ret = 0;
   return undef if $ofs < 0 || $ofs >= $end;

   my ( $l, $r) = ( 0, scalar @$bx);
   while ( 1) {
      my $i = int(( $l + $r) / 2);
      last if $i == $ret;
      $ret = $i;
      my ( $b1, $b2) = ( $$bx[$i], $$bx[$i+1]);

      last if $ofs == $$b1[ tb::BLK_TEXT_OFFSET];

      if ( $ofs > $$b1[ tb::BLK_TEXT_OFFSET]) { 
         if ( $b2) {
            last if $ofs < $$b2[ tb::BLK_TEXT_OFFSET];
            $l = $i;
         } else {
            last;
         }
      } else {
        $r = $i;
      }
   }
   return $ret;
}


sub on_mousedown
{
   my ( $self, $btn, $mod, $x, $y) = @_;
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;

   my @size = $self-> size;
   my @aa = $self-> get_active_area( 0, @size);
   return if $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3];

   ( $x, $y) = $self-> screen2point( $x, $y, @size);

   for my $obj ( @{$self-> {contents}}) {
      unless ( $obj-> on_mousedown( $self, $btn, $mod, $x, $y)) {
         $self-> clear_event;
         return;
      }
   }
   
   my ( $text_offset, $bid) = $self-> xy2info( $x, $y);

   $self->{mouseTransaction} = 1;
   $self-> {mouseAnchor} = [ $text_offset, $bid ]; 
   $self-> selection( -1, -1, -1, -1);

   $self-> capture(1);
   $self-> clear_event;
}

sub on_mouseclick
{
   my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
   return unless $dbl;
   return if $self->{mouseTransaction};
   return if $btn != mb::Left;
   
   my @size = $self-> size;
   my @aa = $self-> get_active_area( 0, @size);
   if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
      if ( $self-> has_selection) {
         $self-> selection( -1, -1, -1, -1);
         my $cp = $::application-> bring('Primary');
         $cp-> text( '') if $cp;
      }
      return;
   }

   ( $x, $y) = $self-> screen2point( $x, $y, @size);
   my ( $text_offset, $bid) = $self-> xy2info( $x, $y);
   my $ln = ( $bid + 1 == scalar @{$self->{blocks}}) ? 
      length ${$self->{text}} : $self-> {blocks}-> [$bid+1]-> [tb::BLK_TEXT_OFFSET];
   $self-> selection( 0, $bid, $ln - $self-> {blocks}-> [$bid]-> [tb::BLK_TEXT_OFFSET], $bid);
   $self-> clear_event;
}

sub on_mouseup
{
   my ( $self, $btn, $mod, $x, $y) = @_;

   unless ( $self->{mouseTransaction}) {
      ( $x, $y) = $self-> screen2point( $x, $y);
      for my $obj ( @{$self-> {contents}}) {
         unless ( $obj-> on_mouseup( $self, $btn, $mod, $x, $y)) {
            $self-> clear_event;
            return;
         }
      }
      return;
   }

   return if $btn != mb::Left;
   
   $self-> capture(0);
   $self->{mouseTransaction} = undef;
   $self-> clear_event;

   my $cp = $::application-> bring('Primary');
   $cp-> text( $self-> get_selected_text) if $cp;
}

sub on_mousemove
{
   my ( $self, $mod, $x, $y) = @_;

   unless ( $self->{mouseTransaction}) {
      ( $x, $y) = $self-> screen2point( $x, $y);
      for my $obj ( @{$self-> {contents}}) {
         unless ( $obj-> on_mousemove( $self, $mod, $x, $y)) {
            $self-> clear_event;
            return;
         }
      }
      return;
   }


   my @size = $self-> size;
   my @aa = $self-> get_active_area( 0, @size);
   if ( $x < $aa[0] || $x >= $aa[2] || $y < $aa[1] || $y >= $aa[3]) {
      $self-> scroll_timer_start unless $self-> scroll_timer_active;
      return unless $self->scroll_timer_semaphore;
      $self->scroll_timer_semaphore(0);
   } else {
      $self-> scroll_timer_stop;
   }

   my ( $nx, $ny) = $self-> screen2point( $x, $y, @size);
   my ( $text_offset, $bid) = $self-> xy2info( $nx, $ny);

   $self-> selection( @{$self-> {mouseAnchor}}, $text_offset, $bid);

   if ( $x < $aa[0] || $x >= $aa[2]) {
      my $px = $self->{paneWidth} / 8;
      $px = 5 if $px < 5;
      $px *= -1 if $x < $aa[0];
      $self-> offset( $self-> {offset} + $px);
   }
   if ( $y < $aa[1] || $y >= $aa[3]) {
      my $py = $self-> font-> height;
      $py = 5 if $py < 5;
      $py *= -1 if $y >= $aa[3];
      $self-> topLine( $self-> {topLine} + $py);
   }
}


sub on_mousewheel
{
   my ( $self, $mod, $x, $y, $z) = @_;
   $z = int( $z/120);
   $z *= $self-> font-> height + $self-> font-> externalLeading unless $mod & km::Ctrl;
   my $newTop = $self-> {topLine} - $z;
   $self-> topLine( $newTop > $self-> {paneHeight} ? $self-> {paneHeight} : $newTop);
   $self-> clear_event;
}

sub on_keydown
{
   my ( $self, $code, $key, $mod, $repeat) = @_;

   $mod &= km::Alt|km::Ctrl|km::Shift;
   return if $mod & km::Alt;

   if ( grep { $key == $_ } 
          ( kb::Up, kb::Down, kb::Left, kb::Right, kb::Space, kb::PgDn, kb::PgUp, kb::Home, kb::End)) {
      my ( $dx, $dy) = (0,0);
      if ( $key == kb::Up || $key == kb::Down) {
         $dy = $self-> font-> height;
         $dy = 5 if $dy < 5;
         $dy *= $repeat;
         $dy = -$dy if $key == kb::Up;
      } elsif ( $key == kb::Left || $key == kb::Right) {
         $dx = $self->{paneWidth} / 8;
         $dx = 5 if $dx < 5;
         $dx *= $repeat;
         $dx = -$dx if $key == kb::Left;
     } elsif ( $key == kb::PgUp || $key == kb::PgDn || $key == kb::Space) {
         my @aa = $self-> get_active_area(0);
         $dy = ( $aa[3] - $aa[1]) * 0.9;
         $dy = 5 if $dy < 5;
         $dy *= $repeat;
         $dy = -$dy if $key == kb::PgUp;
      } 

      $dx += $self-> {offset};
      $dy += $self-> {topLine};
      
      if ( $key == kb::Home) {
         $dy = 0;
      } elsif ( $key == kb::End) {
         $dy = $self-> {paneHeight};
      }
      $self-> offset( $dx);
      $self-> topLine( $dy);
      $self-> clear_event; 
   }

   if (( $key == kb::Insert) && ( $mod & km::Ctrl) && $self-> has_selection) {
      $self-> copy;
      $self-> clear_event;
   }
}

sub has_selection
{
  return ( grep { $_ != -1 } @{$_[0]-> {selection}} ) ? 1 : 0;
}

sub selection
{
   return @{$_[0]-> {selection}} unless $#_;
   my ( $self, $sx1, $sy1, $sx2, $sy2) = @_;

   $sy1 = 0 if $sy1 < 0;
   $sy2 = 0 if $sy2 < 0;
   my $lim = scalar @{$self->{blocks}} - 1;
   $sy1 = $lim if $sy1 > $lim;
   $sy2 = $lim if $sy2 > $lim;

   my $empty = ! $self-> has_selection;
   my ( $osx1, $osy1, $osx2, $osy2) = @{$self->{selection}};
   my ( $x1, $y1, $x2, $y2) = (0,0,0,0);
   
   unless ( grep { $_ != -1 } $sx1, $sy1, $sx2, $sy2 ) { # new empty selection
   EMPTY:
      return if $empty;     
      $y1 = $osy1;
      $y2 = $osy2;
      if ( $y1 == $y2) {
         $x1 = $osx1;
         $x2 = $osx2;
      }
   } else {
      ( $sy1, $sy2, $sx1, $sx2) = ( $sy2, $sy1, $sx2, $sx1) if $sy2 < $sy1;
      ( $sx1, $sx2) = ( $sx2, $sx1) if $sy2 == $sy1 && $sx2 < $sx1;
      ( $sx1, $sx2, $sy1, $sy2) = ( -1, -1, -1, -1), goto EMPTY if $sy1 == $sy2 && $sx1 == $sx2;
      if ( $empty) {
         $y1 = $sy1;
         $y2 = $sy2;
         if ( $y1 == $y2) {
            $x1 = $sx1;
            $x2 = $sx2;
         }
      } else {
          if ( $sy1 == $osy1 && $sx1 == $osx1) {
             return if $sy2 == $osy2 && $sx2 == $osx2;
             $y1 = $sy2;
             $y2 = $osy2;
             if ( $sy2 == $osy2) {
                $x1 = $sx2;
                $x2 = $osx2;
             }
          } elsif ( $sy2 == $osy2 && $sx2 == $osx2) {
             $y1 = $sy1;
             $y2 = $osy1;
             if ( $sy1 == $osy1) {
                $x1 = $sx1;
                $x2 = $osx1;
             }
          } else {
             $y1 = ( $sy1 < $osy1) ? $sy1 : $osy1;
             $y2 = ( $sy2 > $osy2) ? $sy2 : $osy2;
             if ( $sy1 == $sy2 && $osy1 == $osy2 && $sy2 == $osy1) {
                $x1 = ( $sx1 < $osx1) ? $sx1 : $osx1;
                $x2 = ( $sx2 > $osx2) ? $sx2 : $osx2;
             }
          }
          ( $y1, $y2, $x1, $x2) = ( $y2, $y1, $x2, $x1) if $y2 < $y1;
      }
   }

   my $bx = $self-> {blocks};
   my @clipRect;
   my @size = $self-> size;
   my @aa   = $self-> get_active_area( 0, @size);

   if ( $y2 != $y1) {
      my $b = $$bx[ $y1];
      my @a = ( $$b[ tb::BLK_X], $$b[tb::BLK_Y], $$b[ tb::BLK_X], $$b[ tb::BLK_Y]);
      for ( $y1 .. $y2) {
         my $z = $$bx[ $_];
         my @b = ( $$z[ tb::BLK_X], $$z[tb::BLK_Y], $$z[ tb::BLK_X] + $$z[ tb::BLK_WIDTH], $$z[ tb::BLK_Y] + $$z[ tb::BLK_HEIGHT]);
         for ( 0, 1) { $a[$_] = $b[$_] if $a[$_] > $b[$_] }
         for ( 2, 3) { $a[$_] = $b[$_] if $a[$_] < $b[$_] }
      }
      $clipRect[0] = $aa[0] - $self-> {offset}  + $a[0];
      $clipRect[1] = $aa[3] + $self-> {topLine} - $a[1] - 1;
      $clipRect[2] = $aa[0] - $self-> {offset}  + $a[2];
      $clipRect[3] = $aa[3] + $self-> {topLine} - $a[3] - 1;
   } else {
      my $b = $$bx[ $y1]; 
      ( $x2, $x1) = ( $x1, $x2) if $x1 > $x2;
      $clipRect[0] = $aa[0] - $self-> {offset} + $$b[ tb::BLK_X] + $self-> text2xoffset( $x1, $y1); 
      $clipRect[1] = $aa[3] - $$b[ tb::BLK_Y] - $$b[ tb::BLK_HEIGHT] + $self-> {topLine} - 1;
      $clipRect[2] = $aa[0] - $self-> {offset} + $$b[ tb::BLK_X] + $self-> text2xoffset( $x2, $y1);
      $clipRect[3] = $aa[3] - $$b[ tb::BLK_Y] + $self-> {topLine} - 1;
   }

   for ( 0, 1) {
      $clipRect[$_] = $aa[$_] if $clipRect[$_] < $aa[$_]; 
      $clipRect[$_+2] = $aa[$_+2] if $clipRect[$_+2] > $aa[$_+2];
   }

   $self-> {selection} = [ $sx1, $sy1, $sx2, $sy2 ];
   my @cpr = $self-> get_invalid_rect;
   if ( $cpr[0] != $cpr[2] || $cpr[1] != $cpr[3]) {
      for ( 0,1) {
         $clipRect[$_] = $cpr[$_] if $clipRect[$_] > $cpr[$_];
         $clipRect[$_+2] = $cpr[$_+2] if $clipRect[$_+2] < $cpr[$_+2];
      }
   }
   $self-> invalidate_rect( @clipRect);
   
}

sub get_selected_text
{
   my $self = $_[0];
   return unless $self-> has_selection;
   my ( $sx1, $sy1, $sx2, $sy2) = $self-> selection;
   my ( $a1, $a2) = (
      $self-> {blocks}->[$sy1]-> [tb::BLK_TEXT_OFFSET] + $sx1,
      $self-> {blocks}->[$sy2]-> [tb::BLK_TEXT_OFFSET] + $sx2,
   );
   return substr( ${$self->{text}}, $a1, $a2 - $a1);
}

sub copy
{
   my $self = $_[0];
   my $text = $self-> get_selected_text;
   $::application-> Clipboard-> store( 'Text', $text) if defined $text;
}

sub clear_all
{
   my $self = $_[0];
   $self-> selection(-1,-1,-1,-1);
   $self-> {blocks} = [];
   $self-> paneSize( 0, 0);
   $self-> text('');
}


package Prima::TextView::EventRectangles;

sub new
{
   my $class = shift;
   my %profile = @_;
   my $self = {};
   bless( $self, $class);
   $self-> {$_} = $profile{$_} ? $profile{$_} : [] 
     for qw( rectangles references);
   return $self;
}

sub contains 
{ 
   my ( $self, $x, $y) = @_;
   my $rec = 0;
   for ( @{$self->{rectangles}}) {
      return $rec if $x >= $$_[0] && $y >= $$_[1] && $x < $$_[2] && $y < $$_[3];
      $rec++;
   }
   return -1;
}

sub rectangles
{
   return $_[0]-> {rectangles} unless $#_;
   $_[0]-> {rectangles} = $_[1];
}

sub references
{
   return $_[0]-> {references} unless $#_;
   $_[0]-> {references} = $_[1];
}

1;
