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
use strict;
use Prima qw( StdBitmap Buttons);

package UserInit;
$::application = Prima::Application-> create( name => "Generic.pm");

my $w = Prima::Window-> create(
   size    => [ 190, 460],
   left    => 200,
   onDestroy => sub {$::application-> close},
   text    => 'Pointers',
);

my %grep_out = (
   'BEGIN' => 1,
   'END' => 1,
   'AUTOLOAD' => 1,
   'constant' => 1
);

my @a = sort { &{$cr::{$a}}() <=> &{$cr::{$b}}()} grep { !exists $grep_out{$_}} keys %cr::;
shift @a;
$w-> pointerVisible(0);
my $i;

for ( $i = cr::Arrow; $i <= cr::Invalid; $i++)
{
   $w-> pointer( $i);
   my $b = $w-> insert( Button =>
      flat => 1,
      left    => 10,
      width   => 160,
      bottom  => $w-> height - ($i+1) * 40 - 10,
      pointer => $i,
      name    => shift @a,
      image   => $w-> pointerIcon,
      onClick => sub { $::application-> pointer( $_[0]-> pointer); },
   );
};

my $ptr = Prima::StdBitmap::icon( sbmp::DriveCDROM);

my $b = $w-> insert( SpeedButton =>
   left    => 10,
   width   => 160,
   bottom  => $w-> height - (cr::User+1) * 40 - 10,
   pointer => $ptr,
   image   => $ptr,
   text => $a[-1],
   flat => 1,
   onClick => sub {
       $::application-> pointer( $_[0]-> pointer);
   },
);

$w-> pointer( cr::Default);
$w-> pointerVisible(1);

run Prima;
