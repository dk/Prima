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
#  Created by:
#     Vadim Belman   <voland@plab.ku.dk>
#     Anton Berezin  <tobez@plab.ku.dk>
#
#  $Id$

package Prima::Utils;
use strict;
use Prima::Const;
require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT = ();
@EXPORT_OK = qw(query_drives_map query_drive_type
                getdir get_os get_gui
                beep sound
                username
                xcolor
               );

sub xcolor {
# input: '#rgb' or '#rrggbb' or '#rrrgggbbb'
# output: internal color used by Prima
   my ($r,$g,$b,$d);
   $_ = $_[0];
   $d=1/16, ($r,$g,$b) = /^#([\da-fA-F]{3})([\da-fA-F]{3})([\da-fA-F]{3})/
   or
   $d=1, ($r,$g,$b) = /^#([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})/
   or
   $d=16, ($r,$g,$b) = /^#([\da-fA-F])([\da-fA-F])([\da-fA-F])/
   or return 0;
   ($r,$g,$b) = (hex($r)*$d,hex($g)*$d,hex($b)*$d);
   return ($r<<16)|($g<<8)|($b);
}

1;
