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
package StdBitmap;
use strict;
use Const;
require Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '1.00';
@EXPORT = qw();
@EXPORT_OK = qw(icon image);
%EXPORT_TAGS = ();

my %bmCache = ();
my $bmImageFile = undef;

{
   my $scriptPath = (grep { m/Script$/ } @INC)[-1];
   ( my $imagePath = $scriptPath) =~ s/Script$/Images/;
   $bmImageFile = "$imagePath/sysimage.gif";
}

sub load_std_bmp
{
   my ( $index, $asIcon, $copy) = @_;
   my $class = ( $asIcon ? q(Icon) : q(Image));
   return undef if $index < 0 || $index > sbmp::Last;
   $asIcon = ( $asIcon ? 1 : 0);
   if ( $copy)
   {
      my $i = $class-> create(name => $index);
      undef $i unless $i-> load( $bmImageFile, index => $index);
      return $i;
   }
   return $bmCache{$index}->[$asIcon] if exists $bmCache{$index} && defined $bmCache{$index}->[$asIcon];
   $bmCache{$index} = [ undef, undef] unless exists $bmCache{$index};
   my $i = $class-> create(name => $index);
   undef $i unless $i-> load( $bmImageFile, index => $index);
   $bmCache{$index}->[$asIcon] = $i;
   return $i;
}

sub icon { return load_std_bmp( $_[0], 1, 0); }
sub image{ return load_std_bmp( $_[0], 0, 0); }

1;
