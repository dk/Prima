#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
#  $Id$
#

=pod 
=item NAME

The mouse tale from Alice In Wonderland

=item FEATURES

Demonstrates Prima::TextView capabilities

=cut

use strict;
use Prima;
use Prima::TextView;
use Prima::Application;


my $tale = <<TALE;
Fury said to a mouse, 
That he met in the house,
"Let us both go to law:
I will prosecute YOU.
--Come, I'll take no denial;
We must have a trial:
For really this morning 
I've nothing to do."
Said the mouse to the cur,
"Such a trial, dear Sir,
With no jury or judge,
would be wasting our breath."
"I'll be judge, I'll be jury,"
Said cunning old Fury:
"I'll try the whole cause,
and condemn you to death".
TALE

$tale =~ s/\n//g;

my $rcenter = 25;
my @indents = (
19,20,18,16,13,15,17,19,21,24,25,22,18,15,13,12,13,14,16,18,20,
22,25,17,15,14,15,20,18,20,21,23,27,23,18,20,11,15,17,26,11,15,
17,16,25
);

my @offsets = (
15,15,10,7,8,10,12,9,13,12,10,11,11,12,13,8,8,8,13,
11,8,10,5,8,9,9,7,4,8,9,11,11,5,7,10,5,8,6,6,4,8,4,3,6,2,
);


my $w = Prima::Window-> create(
   name => 'Mouse tale',
   onDestroy => sub { $::application-> close },
);

my $t = Prima::TextView-> create(
   owner => $w,
   growMode => gm::Client,
   origin   => [0,0],
   size     => [$w-> size],
   text     => $tale,
);

my ($i, $tb, $pos, $y, $fh, $fd, $fs, $state, $indent, $maxw);

$y = 0;
$maxw = 0;
$pos = 0;

$t-> begin_paint_info; # need to calculate text widths, this way it goes faster
my $fontstate = $t-> create_state;

# select initial font size
$$fontstate[ tb::BLK_FONT_SIZE] = 40;
$t-> realize_state( $t, $fontstate, tb::REALIZE_FONTS);
$fs = $t-> font-> height;

for ( $i = 0, $pos = 0; $i < scalar @indents; $i++, $y += $fh) {
   my $len = $offsets[$i];
   my $str = substr( $tale, $pos, $len);
   $len--, $pos++ while $str =~ s/^\s//;

   $tb = tb::block_create();
   $fs = 12 + ( $fs - 12) * 0.9;
   if ( int($fs) != $t-> font-> size) {
      $$fontstate[ tb::BLK_FONT_SIZE] = int($fs) + tb::F_HEIGHT;
      $t-> realize_state( $t, $fontstate, tb::REALIZE_FONTS);
      $fh = $t-> font-> height;
      $fd = $t-> font-> descent;
   }
   
   $indent = $fh / 4 unless $i;
   my $di = ($indents[$i] - $rcenter) * ( $fs / 36 ) ;
  
   # set block position and attributes - each block contains single line and single text op
   $$tb[ tb::BLK_TEXT_OFFSET] = $pos;
   $$tb[ tb::BLK_WIDTH]  = $t-> get_text_width( $str);
   $$tb[ tb::BLK_HEIGHT] = $fh;
   $$tb[ tb::BLK_Y] = $y;
   $$tb[ tb::BLK_APERTURE_Y] = $fd;
   $$tb[ tb::BLK_X] = ( $rcenter + $di) * $indent + ( $len * $indent - $$tb[ tb::BLK_WIDTH]) / 2;
   $$tb[ tb::BLK_COLOR]     = cl::Fore;
   $$tb[ tb::BLK_BACKCOLOR] = cl::Back;
   $$tb[ tb::BLK_FONT_SIZE]  = int($fs) + tb::F_HEIGHT;

   # add text op and its width, store block
   push @$tb, tb::text( 0, $len, $$tb[tb::BLK_WIDTH]);
   push @{$t-> {blocks}}, $tb;

   $maxw = $$tb[ tb::BLK_WIDTH] + $$tb[ tb::BLK_X] 
      if $maxw < $$tb[ tb::BLK_WIDTH] + $$tb[ tb::BLK_X]; 
   $pos += $len;
}

$t-> end_paint_info;

$t-> recalc_ymap; # need this as a finalization act
$t-> paneSize( $maxw, $y);

run Prima;
