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
package Prima::PS::Fonts;

=head1 NAME

Prima::PS::Fonts - PostScript device fonts metrics

=head1 SYNOPSIS

use Prima;
use Prima::PS::Fonts;

=head1 DESCRIPTION

This module primary use is to be invoked from Prima::PS::Drawable module.
Assumed that some common fonts like Times and Couries are supported by PS
interpreter, and it is assumed that typeface is preserved more-less the
same, so typesetting based on font's a-b-c metrics can be valid. 
35 font files are supplied with 11 font families. Font files with metrics
located into 'fonts' subdirectory. 

=cut

use strict;
use Prima;
use vars qw(%files %enum_families $defaultFontName $variablePitchName $fixedPitchName);

my %cache;
$defaultFontName   = 'Helvetica';
$variablePitchName = 'Helvetica';
$fixedPitchName    = 'Courier';

=item query_metrics( $fontName)

Returns font metric hash with requested font data, uses $defaultFontName
if give name is not found. Metric hash is the same as Prima::Types::Font
record, plus 2 extra fields: 'docname' containing font name ( equals 
always to 'name') and 'chardata' - hash of named glyphs. Every hash
entry in 'chardata' record contains four numbers - suggested character 
index and a, b and c glyph dimensions. 

=cut

sub query_metrics
{
   my $f = $_[0];
   my ( $file, $name, $family);
   if ( exists $enum_families{$f}) {
      $family = $f;
      $f = $enum_families{$f};
   } 
   $name = $f;
   if ( exists $files{$f}) {
      $file = $files{ $f};
      unless ( defined $family) {
      # pick up family
         for ( keys %enum_families) {
            $family = $_, last if $f =~ m[^$_];
         }
      }
      $family = $defaultFontName unless defined $family;
   } else {
      $family = $defaultFontName;
      $name   = $enum_families{$family};
      $file   = $files{ $name};
   }
   
   return $cache{$file} if exists $cache{$file};

   my $defFN = $files{ $defaultFontName};
   my $fx = Prima-> find_image( $file);

   unless ( $fx) {
      if ( $name eq $defaultFontName) {
         warn("Prima::PS::Fontmap: can't load default font\n");
         return undef;
      } else {
         warn("Broken Prima::PS::Fontmap installation: $name not found\n"), 
            return query_metrics( $defaultFontName) 
      }   
   }   
   unless ( open F, $fx) {
      warn( "Prima::PS::Fontmap: cannot open file: $fx\n") ;
      return undef if $f eq $defaultFontName;
      return query_metrics( $defaultFontName);
   }
   {
      local $/;
      my @ra;
      my $z = '@ra = ' . <F>;
      close F;
      eval($z);
      
      if ( $@ || scalar(@ra) < 2) {
         warn( "Prima::PS::Fontmap: invalid file: $fx\n");
         return undef if $name eq $defaultFontName;
         return query_metrics( $defaultFontName);
      }
      $ra[1]-> {docname}  = $name;
      $ra[1]-> {name}     = $name;
      $ra[1]-> {family}   = $family;
      $cache{$file} = $ra[1];
   }
   return $cache{$file};
}

=item enum_fonts( $fontFamily)

Returns font records for given family, or all families
perpesented by one member, if no family name given.
Compliant to Prima::Application::font interface.

=cut

sub enum_fonts
{
   my $family = $_[0];
   my @names;
   if ( defined $family) {
      return [] unless defined $enum_families{$family};
      for ( keys %enum_families) {
         push @names, $_ if $_ eq $family;
      }
   } else {
      @names = keys %enum_families;
   }
   for ( @names) {
      my %x = %{query_metrics( $_)};
      $x{name} = $x{family};
      delete $x{docname};
      delete $x{chardata};
      $_ = \%x;
   }
   return \@names;
}

=item enum_family( $fontFamily)

Returns font names that are presented in given family

=cut

sub enum_family
{
   my $family = $_[0];
   my @names;
   return unless defined $enum_families{$family};
   for ( keys %files) {
      push @names, $_ if m/^$family/;
   }
   return @names;
}

=item font_pick( $src, $dest)

Merges two font records using Prima::Drawable::font_match, picks
the result and returns new record.  $variablePitchName and
$fixedPitchName used on this stage.

=cut


sub font_pick
{
   my ( $src, $dest) = @_;
   my $bySize = exists( $src->{size}) && !exists( $src-> {height});
   $dest = Prima::Drawable-> font_match( $src, $dest, 0);

   # find name
   my $m1   = query_metrics( $dest-> {name});
   
   # find pitch
   if ( $dest-> {pitch} != fp::Default && $dest-> {pitch} != $m1-> {pitch}) {
      if ( $dest-> {pitch} == fp::Variable) {
         $m1 = query_metrics( $variablePitchName);
      } else {
         $m1 = query_metrics( $fixedPitchName);
      }
   }
   
   # get all family members
   my @famx = map { query_metrics( $_) } enum_family( $m1-> {family});
   
   # find style
   my $m2; 
   for ( @famx) { # exact match
      $m2 = $_, last if $_->{style} == $dest->{style};
   }
   unless ( $m2) { # second pass
      my $maxDiff = 1000;
      my ( $italic, $bold) = (( $dest-> {style} & fs::Italic) ? 1 : 0, ($dest-> {style} & fs::Bold) ? 1 :0);
      for ( @famx) {
          my ( $i, $b) = (( $_-> {style} & fs::Italic) ? 1 : 0, ( $_-> {style} & fs::Bold) ? 1 : 0);   
          my $diff = (( $i == $italic) ? 0 : 2) + (( $b == $bold) ? 0 : 1);
          $m2 = $_, $maxDiff = $diff if $diff < $maxDiff;
      }
   }
   $m2 = $m1 unless defined $m2;

   # scale dimensions
   my $a = $bySize ? ( $dest-> {size} / $m2-> {size}) : ( $dest-> {height} / $m2-> {height});
   my %muls = %$m2;
   my $du   = $dest-> {style} & fs::Underlined;
   my $dw   = $dest-> {width};
   $muls{$_} *= $a for
     qw( height ascent descent width maximalWidth internalLeading externalLeading size);
   $dest-> {$_}     = $muls{$_} for keys %muls;
   $dest-> {style} |= fs::Underlined if $du;  
   $dest-> {width} = $dw if $dw != 0;
   return $dest;
}

=item files & enum_families

Hash with paths to font metric files. File names not necessarily
should be as font names, and it is possible to override font name
contained in the file just by specifying different font key - this
case will be recognized on loading stage and loaded font structure
patched correspondingly. 

Example:

  $Prima::PS::Fonts::files{Standard Symbols} = $Prima::PS::Fonts::files{Symbol};
  
  $Prima::PS::Fonts::files{'Device-specific symbols, set 1'} = 'my/devspec/data.1';
  $Prima::PS::Fonts::files{'Device-specific symbols, set 2'} = 'my/devspec/data.2';
  $Prima::PS::Fonts::enum_families{DevSpec} = 'Device-specific symbols, set 1';

=cut

%files = map { $_ => "PS/fonts/$_" } (
    'Bookman-Demi'                ,
    'Bookman-DemiItalic'          ,
    'Bookman-Light'               ,
    'Bookman-LightItalic'         ,
    'Courier'                     ,
    'Courier-Oblique'             ,
    'Courier-Bold'                ,
    'Courier-BoldOblique'         ,
    'AvantGarde-Book'             ,
    'AvantGarde-BookOblique'      ,
    'AvantGarde-Demi'             ,
    'AvantGarde-DemiOblique'      ,
    'Helvetica'                   ,
    'Helvetica-Oblique'           ,
    'Helvetica-Bold'              ,
    'Helvetica-BoldOblique'       ,
    'Helvetica-Narrow'            ,
    'Helvetica-Narrow-Oblique'    ,
    'Helvetica-Narrow-Bold'       ,
    'Helvetica-Narrow-BoldOblique',
    'Palatino-Roman'              ,
    'Palatino-Italic'             ,
    'Palatino-Bold'               ,
    'Palatino-BoldItalic'         ,
    'NewCenturySchlbk-Roman'      ,
    'NewCenturySchlbk-Italic'     ,
    'NewCenturySchlbk-Bold'       ,
    'NewCenturySchlbk-BoldItalic' ,
    'Times-Roman'                 ,
    'Times-Italic'                ,
    'Times-Bold'                  ,
    'Times-BoldItalic'            ,
    'Symbol'                      ,
    'ZapfChancery-MediumItalic'   , 
    'ZapfDingbats'                ,
);


# The keys of %enum_families are only font names, - in Prima terms.
# The only problem that the family field is always the same as the 
# font name

%enum_families = (
   'Bookman'                => 'Bookman-Light',
   'Courier'                => 'Courier',
   'AvantGarde'             => 'AvantGarde-Book',
   'Helvetica'              => 'Helvetica',
   'Helvetica-Narrow'       => 'Helvetica-Narrow',
   'Palatino'               => 'Palatino-Roman',
   'NewCenturySchlbk'       => 'NewCenturySchlbk-Roman',
   'Times'                  => 'Times-Roman',
   'Symbol'                 => 'Symbol', 
   'ZapfChancery'           => 'ZapfChancery-MediumItalic',
   'ZapfDingbats'           => 'ZapfDingbats',
);

1;
