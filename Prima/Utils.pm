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

__DATA__

=head1 NAME

Prima::Utils - miscellanneous routines

=head1 DESCRIPTION

The module contains several routines, implemented in C. These
routines can be accessed via C<Prima::Utils::> prefix.

=head1 API

=over

=item beep [ FLAGS = mb::Error ] 

Invokes the system-depended sound and/or visual bell, 
corresponding to one of following constants:

  mb::Error
  mb::Warning
  mb::Information
  mb::Question

=item get_gui

Returns one of C<gui::XXX> constants, reflecting the graphic
user interface used in the system:

   gui::Default
   gui::PM  
   gui::Windows
   gui::XLib 
   gui::OpenLook
   gui::Motif

The meaning of the return value is somewhat vague, and
might be deprecated in future releases.

=item get_os

Returns one of C<apc::XXX> constants, reflecting the platfrom.
Currently, the list of the supported platforms is:

   apc::Os2    
   apc::Win32  
   apc::Unix

=item ceil DOUBLE

Obsolete function.

Returns stdlib's ceil() of DOUBLE

=item floor DOUBLE

Obsolete function.

Returns stdlib's floor() of DOUBLE

=item getdir PATH 

Reads content of PATH directory and 
returns array of string pairs, where the first item is a file
name, and the second is a file type.

The file type is a string, one of the following:

  "fifo" - named pipe
  "chr"  - character special file
  "dir"  - directory
  "blk"  - block special file
  "reg"  - regular file
  "lnk"  - symbolic link
  "sock" - socket
  "wht"  - whiteout

This function was implemented for faster directory reading, 
to avoid successive call of C<stat> for every file.

=item query_drives_map [ FIRST_DRIVE = "A:" ]

Returns anonymous array to drive letters, used by the system.
FIRST_DRIVE can be set to other value to start enumeration from.
Some OSes can probe eventual diskette drives inside the drive enumeration
routines, so there is a chance to increase responsiveness of the function
it might be reasonable to set FIRST_DRIVE to C<C:> string.

If the system supports no drive letters, empty array reference is returned ( unix ).

=item query_drive_type DRIVE

Returns one of C<dt::XXX> constants, describing the type of drive,
where DRIVE is a 1-character string. If there is no such drive, or
the system supports no drive letters ( unix ), C<dt::None> is returned.

   dt::None
   dt::Unknown
   dt::Floppy
   dt::HDD
   dt::Network
   dt::CDROM
   dt::Memory

=item sound [ FREQUENCY = 2000, DURATION = 100 ]

Issues a tone of FREQUENCY in Hz with DURATION in milliseconds.

=item username

Returns the login name of the user. 
Sometimes is preferred to the perl-provided C<getlogin> ( see L<perlfunc/getlogin> ) .

=item xcolor COLOR

Accepts COLOR string on one of the three formats:

  #rgb
  #rrggbb
  #rrrgggbbb

and returns 24-bit RGB integer value.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>

