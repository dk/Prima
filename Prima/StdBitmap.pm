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
package Prima::StdBitmap;
use strict;
require Prima;

my %bmCache;

sub load_std_bmp
{
   my ( $index, $asIcon, $copy, $imageFile) = @_;
   my $class = ( $asIcon ? q(Prima::Icon) : q(Prima::Image));
   return undef if !defined $index || $index < 0 || $index > sbmp::Last;
   $asIcon = ( $asIcon ? 1 : 0);
   if ( $copy)
   {
      my $i = $class-> create(name => $index);
      undef $i unless $i-> load( $imageFile, index => $index);
      return $i;
   }
   $bmCache{$imageFile} = {} unless exists $bmCache{$imageFile};
   my $x = $bmCache{$imageFile};
   return $x-> {$index}->[$asIcon] if exists $x-> {$index} && defined $x-> {$index}->[$asIcon];
   $x-> {$index} = [ undef, undef] unless exists $x-> {$index};
   my $i = $class-> create(name => $index);
   undef $i unless $i-> load( $imageFile, index => $index);
   $x-> {$index}->[$asIcon] = $i;
   return $i;
}

my $bmImageFile = Prima-> find_image( "sysimage.gif");
sub icon { return load_std_bmp( $_[0], 1, 0, $bmImageFile); }
sub image{ return load_std_bmp( $_[0], 0, 0, $bmImageFile); }

1;
