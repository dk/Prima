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
use Prima::MsgBox;

$::application = Prima::Application-> create( name => "Chess puzzle");

my %figs = (
  'K' => [0,0],
  'B1' => [0,1],
  'B2' => [0,2],
  'Q'  => [0,3],
  'T1' => [0,4],
  'T2' => [0,5],
  'R1' => [0,6],
  'R2' => [0,7],
);

my @colors = (
   0x808080,
   0x707070,
   0x606060,
   0x505050,
   0x404040,
   0x303030,
   0x202020,
   0x101010,
   0x000000,
);

my $w = Prima::Window-> create(
   name => 'Chess puzzle',
   size => [ 300, 300],
   font => { style => fs::Bold, size => 11,},
   buffered => 1,
   onDestroy => sub { $::application-> close},
   menuItems => [
      ["~Help" => sub{
          my $txt = 'Chess puzzle. Objective is to put figures so they could reach every cell upon the board';
          Prima::MsgBox::message( $txt, mb::OK | mb::Cancel, {
             buttons => { mb::Cancel => {
                text => '~Solution',
                onClick => sub {
                   Prima::MsgBox::message('Use Ctrl + mouse doubleclick on the board ', mb::OK);
                }
             }}
          });
      }],
   ],
   onPaint => sub {
      my $self = $_[0];
      my $i;
      $self-> color( cl::Back);
      $self-> bar ( 0, 0, $self-> size);
      $self-> color( cl::Black);
      for ( $i = 0; $i < 9; $i++) {
         $self-> line( $i * 32, 0, $i * 32, 32 * 8);
         $self-> line( 0, $i * 32, 32 * 8, $i * 32);
      }
      my @boy = (0) x 64;
      my @busy = (0) x 64;
      for ( keys %figs) {
         my ($x,$y) = @{$figs{$_}};
         $busy[$y*8+$x] = 1;
      }
      my $add = sub {
         my ($x,$y) = @_;
         if ( $x >= 0 && $x < 8 && $y >= 0 && $y < 8) {
            $boy[$y*8+$x] += 1;
            return !$busy[$y*8+$x];
         } else {
            return 0;
         }
      };
      for ( keys %figs) {
         my ( $x, $y) = @{$figs{$_}};
         next unless $x >= 0 && $x < 8 && $y >= 0 && $y < 8;
         if ($_ eq 'K') {
            $add->($x-1,$y-1);
            $add->($x-1,$y);
            $add->($x-1,$y+1);
            $add->($x,$y-1);
            $add->($x,$y+1);
            $add->($x+1,$y-1);
            $add->($x+1,$y);
            $add->($x+1,$y+1);
         } elsif ($_ eq 'Q') {
            for (1..7) { last unless $add->($x-$_,$y); }
            for (1..7) { last unless $add->($x+$_,$y); }
            for (1..7) { last unless $add->($x,$y-$_); }
            for (1..7) { last unless $add->($x,$y+$_); }
            for (1..7) { last unless $add->($x-$_,$y-$_); }
            for (1..7) { last unless $add->($x-$_,$y+$_); }
            for (1..7) { last unless $add->($x+$_,$y-$_); }
            for (1..7) { last unless $add->($x+$_,$y+$_); }
         } elsif (/^T\d$/) {
            for (1..7) { last unless $add->($x-$_,$y); }
            for (1..7) { last unless $add->($x+$_,$y); }
            for (1..7) { last unless $add->($x,$y-$_); }
            for (1..7) { last unless $add->($x,$y+$_); }
         } elsif (/^B\d$/) {
            for (1..7) { last unless $add->($x-$_,$y-$_); }
            for (1..7) { last unless $add->($x-$_,$y+$_); }
            for (1..7) { last unless $add->($x+$_,$y-$_); }
            for (1..7) { last unless $add->($x+$_,$y+$_); }
         } elsif (/^R\d$/) {
            $add->($x-1,$y-2);
            $add->($x-1,$y+2);
            $add->($x-2,$y-1);
            $add->($x-2,$y+1);
            $add->($x+1,$y-2);
            $add->($x+1,$y+2);
            $add->($x+2,$y-1);
            $add->($x+2,$y+1);
         }
      }
      for ( grep $boy[$_], 0..63) {
         my ( $x, $y) = ($_ % 8, int($_/8));
         $self-> color( $colors[$boy[$_]] );
         $self-> bar( $x * 32+1, $y * 32+1, $x * 32+31, $y * 32+31);
      }
      for ( keys %figs) {
         my ( $x, $y) = @{$figs{$_}};
         $self-> color( $boy[ $y * 8 + $x] ? cl::LightGreen : cl::Green);
         $self-> text_out( $_, $x * 32 + 6, $y * 32 + 6);
      }
   },
   onMouseDown => sub {
      my ( $self, $mod, $btn, $x, $y) = @_;
      return if $self->{cap};
      $x = int( $x / 32);
      $y = int( $y / 32);
      return if $x < 0 || $x > 8 || $y < 0 || $y > 8;
      my $i = '';
      for ( keys %figs) {
         my ( $ax, $ay) = @{$figs{$_}};
         $i = $_, last if $ax == $x and $ay == $y;
      }
      return unless $i;
      $self-> {cap} = $i;
      $self-> capture(1);
      $self-> pointer( cr::Size);
      my $xx = $self-> pointerIcon;
      my ( $xor, $and) = $xx-> split;

      $and-> begin_paint;
      $and-> font-> style(fs::Bold);
      $and-> font-> size(11);
      $and-> color( cl::Black);
      $and-> text_out( $i, 0, 0);
      $and-> end_paint;

      $xor-> begin_paint;
      $xor-> font-> style(fs::Bold);
      $xor-> font-> size(11);
      $xor-> color( cl::Green);
      $xor-> text_out( $i, 0, 0);
      $xor-> end_paint;

      $xx-> combine( $xor, $and);
      $self-> pointer( $xx);
   },
   onMouseUp => sub {
      my ( $self, $mod, $btn, $x, $y) = @_;
      return unless $self->{cap};
      $x = int( $x / 32);
      $y = int( $y / 32);
      $self-> capture(0);
      $self-> pointer( cr::Default);
      my $fg = $self-> {cap};
      delete $self-> {cap};
      return if $x < 0 || $x > 8 || $y < 0 || $y > 8;
      my $i = '';
      for ( keys %figs) {
         my ( $ax, $ay) = @{$figs{$_}};
         $i = $_, last if $ax == $x and $ay == $y;
      }
      return if $i;
      $figs{$fg} = [ $x, $y];
      $self-> repaint;
   },
   onMouseClick => sub {
      my ( $self, $btn, $mod, $x, $y, $dbl) = @_;
      if ( $dbl and ( $mod & km::Ctrl)) {
        %figs = (
          'K'  => [2,5],
          'B1' => [5,5],
          'B2' => [2,2],
          'Q'  => [5,2],
          'T1' => [7,7],
          'T2' => [0,0],
          'R1' => [4,4],
          'R2' => [3,3],
        );
        $self-> repaint;
      }
   },
);

run Prima;