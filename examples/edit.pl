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
use Prima::Widgets;
use Prima::InputLine;

$::application = Prima::Application-> create( name => "Generic.pm");

my $l;
my $w = Prima::Window-> create(
  size => [ 700, 300],
  onDestroy => sub { $::application-> destroy},
  accelItems => [
    [a=>a=> kb::F3=>sub{$l-> repaint}],
  ]

);

$l = $w-> insert( InputLine =>
   text  => '0:::1234 5678 90ab cdef ghij klmn oprq stuv:1::1234 5678 90ab cdef ghij klmn oprq stuv:2::1234 5678 90ab cdef ghij klmn oprq stuv:3::1234 5678 90ab cdef ghij klmn oprq stuv:4::1234 5678 90ab cdef ghij klmn oprq stuv::End',
   y_centered  => 1,
   left   => 200,
   width => 300,
   #selection => [1,12],
   height => 40,
#  maxLen => 15,
   firstChar => 2,
#  offset => 3,
   alignment => ta::Center,
   font   => { size => 18, name => 'Tms Rmn'},
   growMode => gm::GrowHiX,
   buffered => 1,
   xonPaint => sub {},
   borderWidth => 3,
   autoSelect => 0,
);

run Prima;
