# $ Id $
package main;
use vars qw($clipboard);

package Application;
use strict;
use Classes;
use vars qw($uses);

sub import {
   shift;
   my %profile = ( name => q(Prima), @_);
   $::application ||= Application-> create( %profile);
   $::application and $::clipboard = $::application-> get_clipboard;
   $uses++;
}

1;
