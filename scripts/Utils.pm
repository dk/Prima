package Utils;
use strict;
use Const;
require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT = ();
@EXPORT_OK = qw(query_drives_map query_drive_type
                get_os get_gui
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
