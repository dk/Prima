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
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#

package Prima::PS::Encodings;
use vars qw(%files %cache);

use strict;
use Prima;


=head1 NAME

Prima::PS::Encodings -  manage latin-based encodings

=head1 SYNOPSIS

use Prima::PS::Encodings;

=head1 DESCRIPTION

This module provides code tables for some latin-based encodings, for 
the glyphs that usually provided for every PS-based printer or interpreter.
Prima::PS::Drawable uses these encodings when it decides whether document
have to be supplied with a bitmap character glyph or a character index, 
thus relying on PS interpreter capabilities. Latter is obviously preferable,
but as it's not possible to know beforehand what glyphs are supported by
PS interpreter, latin glyph set was selected as a ground level.

=item files

It's unlikely that users will need to supply their own encodings, however
this can be accomplished by:
  
  use Prima::PS::Encodings;
  $Prima::PS::Encodings::files{iso8859-5} = 'PS/locale/greek-iso';

=cut

%files = (
  'default'    => 'PS/locale/ascii',
  '1276'       => 'PS/locale/ps-standart',
  'ps-standart'=> 'PS/locale/ps-standart',
  '1277'       => 'PS/locale/ps-iso-latin1',
  'ps-latin1'  => 'PS/locale/ps-iso-latin1',
  'iso8859-1'  => 'PS/locale/iso8859-1',
  'iso8859-2'  => 'PS/locale/iso8859-2',
  'iso8859-3'  => 'PS/locale/iso8859-3',
  'iso8859-4'  => 'PS/locale/iso8859-4',
  'iso8859-7'  => 'PS/locale/iso8859-7',
  'iso8859-9'  => 'PS/locale/iso8859-9',
  'iso8859-10' => 'PS/locale/iso8859-10',
  'iso8859-11' => 'PS/locale/iso8859-11',
  'iso8859-13' => 'PS/locale/iso8859-13',
  'iso8859-14' => 'PS/locale/iso8859-14',
  'iso8859-15' => 'PS/locale/iso8859-15',
  '437'        => 'PS/locale/ibm-cp437',
  '850'        => 'PS/locale/ibm-cp850',
  '1250'       => 'PS/locale/win-cp1250',
  '1252'       => 'PS/locale/win-cp1252',
);

sub exists
{
   my $cp = defined( $_[0]) ? ( lc $_[0]) : 'default';
   $cp = $1 if $cp =~ /\.([^\.]*)$/;
   $cp =~ s/_//g;
   return exists($files{$cp});
}

=item load

Loads encoding file by given string. Tries to be smart to guess
actual file from identifier string returned from setlocale(NULL).
If fails, loads default encoding, which defines only glyphs from
32 to 126. Special case is 'null' encoding, returns array of
256 .notdef's.

=cut

sub load
{
   my $cp = defined( $_[0]) ? ( lc $_[0]) : 'default';
   $cp = $1 if $cp =~ /\.([^\.]*)$/;
   $cp =~ s/_//g;

   if ( $cp eq 'null') {
      return $cache{null} if exists $cache{null};
      return $cache{null} = [('.notdef') x 256];
   }

   my $fx = exists($files{$cp}) ? $files{$cp} : $files{default};

   return $cache{$fx} if exists $cache{$fx};
   
   my $f = Prima-> find_image( $fx); 
   unless ( $f) {
      warn("Prima::PS::Encodings: cannot find encoding file for $cp\n");
      return load('default') unless $cp eq 'default';
      return;
   }

   unless ( open F, $f) {
      warn("Prima::PS::Encodings: cannot load $f\n");
      return load('default') unless $cp eq 'default';
      return;
   }
   my @f = map { chomp; length($_) ? $_ : ()} <F>;
   close F;
   splice( @f, 256) if 256 < @f;
   push( @f, '.notdef') while 256 > @f;
   return $cache{$fx} = \@f;
}

