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
#  Created by Anton Berezin  <tobez@plab.ku.dk>
#
#  $Id$

package Prima;

use strict;
require DynaLoader;
use vars qw($VERSION @ISA $__import);
@ISA = qw(DynaLoader);

BEGIN {
    if ( $^O =~ /freebsd/i) {
	( my $ver = `/usr/bin/uname -r`) =~ s/^(\d+\.\d+).*$/$1/;
	if ( $ver >= 3.4) {
	    eval "sub dl_load_flags { 0x01 }";
	}
    }
}

$VERSION = '1.03';
bootstrap Prima $VERSION;
unless ( UNIVERSAL::can('Prima', 'init')) {
   $::application = 0;
   return 0;
}
$::application = undef;
require Prima::Const;
require Prima::Classes;
init Prima $VERSION;

sub END
{
   &Prima::cleanup() if UNIVERSAL::can('Prima', 'cleanup');
}

sub run
{
   die "Prima was not properly initialized\n" unless $::application;
   $::application-> go if $::application-> alive;
   $::application = undef if $::application and not $::application->alive;
}

sub find_image
{
   my $mod = @_ > 1 ? shift : 'Prima';
   my $name = shift;
   $name =~ s!::!/!g;
   $mod =~ s!::!/!g;
   for (@INC) {
      return "$_/$mod/$name" if -f "$_/$mod/$name" && -r _;
   }
   return undef;
}

sub import
{
   my @module = @_;
   while (@module) {
      my $module = shift @module;
      my %parameters = ();
      %parameters = %{shift @module} if @module && ref($module[0]) eq 'HASH';
      next if $module eq 'Prima' || $module eq '';
      $module = "Prima::$module" unless $module =~ /^Prima::/;
      $__import = caller;
      eval "use $module \%parameters;" if $module;
      $__import = 0;
   }
}

1;
