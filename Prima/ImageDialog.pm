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

package Prima::ImageDialog;

use strict;
use Prima;
use Prima::ImageViewer;
use Prima::ComboBox;
use Prima::Label;
use Prima::FileDialog;

sub filtered_codecs
{
   my $codecs = defined($_[0]) ? $_[0] : Prima::Image-> codecs;
   return map { 
      my $n = uc join( ';', @{$_->{fileExtensions}});
      my $x = join( ';', map {"*.$_"} @{$_->{fileExtensions}});
      [ "$_->{fileType} ($n)" => $x ]} 
   @$codecs;
}

sub filtered_codecs2all
{
   my $codecs = defined($_[0]) ? $_[0] : Prima::Image-> codecs;
   return join(';', map {"*.$_"} map { @{$_->{fileExtensions}}} @$codecs)
}

package Prima::ImageOpenDialog;
use vars qw( @ISA);
@ISA = qw( Prima::OpenDialog);

sub profile_default {
   my $codecs = [ grep { $_-> {canLoad} } @{Prima::Image-> codecs}];
   return { 
      %{$_[ 0]-> SUPER::profile_default},
      preview  => 1,
      text     => 'Open image',
      filter  => [
         ['Images' => Prima::ImageDialog::filtered_codecs2all( $codecs)],
         ['All files' => '*'],
         Prima::ImageDialog::filtered_codecs($codecs),
      ],
   }
}


sub init
{
   my $self = shift;
   $self-> {preview} = 0;
   my %profile = $self-> SUPER::init(@_);
   
   my $pk = $self-> insert( ImageViewer => 
      origin => [ 524, 120],
      name   => 'PickHole',
      borderWidth => 1,
      alignment => ta::Center,
      valignment => ta::Center,
   );
   $pk-> size(($self-> Cancel-> width) x 2); # force square dimension

   $self-> insert( ScrollBar => 
      origin      => [ $pk-> left, $pk-> top + 2],
      width       => $pk-> width,
      selectable  => 1,
      tabStop     => 1,
      name        => 'FrameSelector',
      designScale => undef,
      visible     => 0,
      value       => 0,
      delegations => ['Change'],
   );

   $self-> {Preview} = $self-> insert( CheckBox => 
      origin => [ 524, 80],
      text   => '~Preview',
      size   => [ 96, 36],
      name   => 'Preview',
      delegations => [qw(Click)]
   );

   $self-> insert( Label => 
      origin => [ 524, 25],
      size   => [ 96, 56],
      name   => 'Info',
      text   => '',
      alignment => ta::Center,
      valignment => ta::Center,
      wordWrap => 1,
   );
   
   $self-> preview( $profile{preview});
   $self-> {frameIndex} = 0;
   return %profile;
}


sub update_preview
{
   my $self = $_[0];
   my $i = $self-> PickHole;
   my $j = $self-> Info;
   my $s = $self-> FrameSelector;
   $i-> image( undef);
   $j-> text('');
   $s-> hide unless $s-> {block};
   $self-> {frameIndex} = 0;
   return unless $self-> preview;
   my $x = $self-> Files;
   $x = $x-> get_items( $x-> focusedItem);
   $i-> image( undef);
   return unless defined $x;
   $x = $self-> directory . $x;
   return unless -f $x;
   $x = Prima::Image-> load( $x, 
      loadExtras => 1, 
      wantFrames => 1, 
      index => $s->{block} ? $s-> value : 0,
   );
   return unless defined $x;
   $i-> image( $x);
   my @szA = $x->size;
   my @szB = $i->get_active_area(2);
   my $xx = $szB[0]/$szA[0];
   my $yy = $szB[1]/$szA[1];
   my $codecs = $x-> codecs;
   $i-> zoom( $xx < $yy ? $xx : $yy);
   my $text = $szA[0].'x'.$szA[1].'x'. ($x-> type & im::BPP) . " bpp ".
       $codecs->[$x->{extras}->{codecID}]->{fileShortType};
   $text .= ',grayscale' if $x-> type & im::GrayScale;
   if ( $x->{extras}->{frames} > 1) {
      unless ( $s-> {block}) {
         $text .= ",$x->{extras}->{frames} frames";
         $s-> {block} = 1;
         $s-> set(
            visible => 1,
            max     => $x-> {extras}-> {frames} - 1,
            value   => 0,
         );
         $s-> {block} = 0;
      } else {
         $text .= ",frame ". ($s-> value + 1) . "of $x->{extras}->{frames}";
         $self-> {frameIndex} = $s-> value;
      }
   }
   $j-> text( $text);
}


sub preview
{
   return $_[0]-> {Preview}-> checked unless $#_;
   $_[0]-> {Preview}-> checked( $_[1]);
   $_[0]-> update_preview;
}

sub Preview_Click
{
   $_[0]-> update_preview;
}   

sub Files_SelectItem
{
   my ( $self, $lst) = @_;
   $self-> SUPER::Files_SelectItem( $lst);
   $self-> update_preview if $self-> preview;
}

sub Dir_Change
{
   my ( $self, $lst) = @_;
   $self-> SUPER::Dir_Change( $lst);
   $self-> update_preview if $self-> preview;
}

sub FrameSelector_Change
{
   my ( $self, $fs) = @_;
   return if $fs-> {block};
   my $v = $fs-> value;
   $fs-> {block} = 1;
   $self-> update_preview;
   $fs-> {block} = 0;
}

sub load
{
   my ( $self, %profile) = @_;
   return undef unless defined $self-> execute;
   $profile{loadExtras} = 1 unless exists $profile{loadExtras};
   $profile{index} = $self-> {frameIndex};
   my @r = Prima::Image-> load( $self-> fileName, %profile);
   unless ( defined $r[-1]) {
      Prima::MsgBox::message("Error loading " . $self-> fileName . ":$@");
      pop @r;
      return undef unless scalar @r;
   }
   return undef, Prima::MsgBox::message( "Empty image") unless scalar @r;
   return $r[0] if !wantarray && ( 1 == scalar @r);
   return @r;
}

package Prima::ImageSaveDialog;
use vars qw( @ISA);
@ISA = qw( Prima::SaveDialog);

sub profile_default  {
   my $codecs = [ grep { $_-> {canSave} } @{Prima::Image-> codecs}];
   return { 
      %{$_[ 0]-> SUPER::profile_default},
      text     => 'Save image',
      filter   => [ Prima::ImageDialog::filtered_codecs($codecs) ],
      image    => undef,
      filterDialog => 1,
   }
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> {ConvertTo} = $self-> insert( ComboBox => 
      origin => [ 524, 80],
      items  => [],
      enabled => 0,
      name => 'ConvertTo',
      style => cs::DropDownList,
      size  => [ 96, 25],
   );   
   $self-> insert( Label => 
      origin => [ 524, 110],
      text   => '~Convert to:',
      size  => [ 96, 20],
      name => 'ConvertToLabel',
      focusLink => $self-> {ConvertTo},
   );
   $self-> {UseFilter} = $self-> insert( CheckBox => 
      origin => [ 524, 20],
      text   => '~Use filter',
      size   => [ 96, 36],
      name   => 'UseFilter',
      delegations => [qw(Click)],
   );
   $self-> {codecFilters} = [];
   $self-> {allCodecs} = Prima::Image-> codecs;
   $self-> {codecs} = [ grep { $_-> {canSave}} @{$self-> {allCodecs}}];
   my $codec = $self-> {codecs}-> [$self-> filterIndex];
   $self-> image( $profile{image});
   $self-> filterDialog( $profile{filterDialog});
   $self-> ExtensionsLabel-> text("Sav~e as type ($codec->{fileShortType})");
   return %profile;
}

sub on_destroy
{
   for ( @{$_[0]-> {codecFilters}}) {
      next unless $_;
      $_-> destroy;
   }
}

sub update_conversion
{
   my ( $self, $codec) = @_;
   my $i = $self-> {image};
   my $x = $self-> {ConvertTo};
   my $xl = $self-> ConvertToLabel;
   $x-> items([]);
   $x-> text('');
   $x-> enabled(0);
   $xl-> enabled(0);
   return unless $i;
   my $t = $i-> type;
   my @st = @{$codec-> {types}};
   return unless @st;
   my $max = 0;
   my $j = 0;
   for ( @st) { 
      return if $_ == $t; 
      $max = $j if ( $st[$max] & im::BPP) < ( $_ & im::BPP);
      $j++;
   }
   for ( @st) {
      my $x = ( 1 << ( $_ & im::BPP));
      $x = '24-bit' if $x == 16777216;
      $x .= ' gray' if $_ & im::GrayScale;
      $x .= ' colors';
      $_ = $x;
   }
   $x-> items( \@st);
   $x-> focusedItem( $max);
   $x-> enabled(1);
   $xl-> enabled(1);
}

sub Ext_Change
{
   my ( $self, $ext) = @_;
   $self-> SUPER::Ext_Change( $ext);
   my $codec = $self-> {codecs}-> [ $self-> filterIndex];
   $self-> ExtensionsLabel-> text("Sav~e as type ($codec->{fileShortType})");
   $self-> update_conversion( $codec);
}

sub filterDialog
{
   return $_[0]-> {UseFilter}-> checked unless $#_;
   $_[0]-> {UseFilter}-> checked( $_[1]);
}

sub image
{
   return $_[0]-> {image} unless $#_;
   my ( $self, $image) = @_;
   $self-> {image} = $image;
   if ( $image && exists($image-> {extras}) && 
        (ref($image-> {extras}) eq 'HASH') && defined($image-> {extras}-> {codecID})) {
      my $c = $self-> {allCodecs}-> [$image-> {extras}-> {codecID}];
      my $i = 0;
      for ( @{$self-> {codecs}}) {
         $self-> filterIndex( $i) if $_ == $c;
         $i++;
      }
      $self-> update_conversion( $c);
   }
}

sub on_endmodal
{
   $_[0]-> image( undef); # just freeing the reference
}

sub save
{
   my ( $self, $image) = @_;
   my $ret;
   my $dup;
   return 0 unless $image;
   $dup = $image;
   my $extras = $image-> {extras};
   $self-> image( $image);

   goto EXIT unless defined $self-> execute;

   $image-> {extras} = { map { $_ => $extras->{$_} } keys %$extras } ;

   my $fi = $self-> filterIndex;
   my $codec = $self-> {codecs}-> [ $fi];
   

# loading filter dialog, if any
   if ( $self-> filterDialog) {
      unless ( $self-> {codecFilters}-> [ $fi]) {
         if ( $image && $codec && length($codec-> {module}) && length( $codec-> {package})) {
            my $x = "use $codec->{module};";
            eval $x;
            if ($@) {
               Prima::MsgBox::message("Error invoking $codec->{fileShortType} filter dialog");
            } else {
               $self-> {codecFilters}-> [$fi] = $codec-> {package}-> save_dialog;
            }
         }
      }    
   }

   if ( $self-> ConvertTo-> enabled) {
      $dup = $image-> dup;
      $dup-> type( $codec-> {types}-> [ $self-> ConvertTo-> focusedItem]);
   }

# invoking filter dialog
   if ( $self-> {codecFilters}-> [ $fi]) {
      my $dlg = $self-> {codecFilters}-> [ $fi];
      $dlg-> notify( q(Change), $codec, $dup);
      unless ( $dlg-> execute == cm::OK) {
         $self-> cancel;
         goto EXIT;
      }
   }

# selecting codec
   my $j = 0;
   for ( @{$self-> {allCodecs}}) {
      $dup-> {extras}-> {codecID} = $j, last if $_ == $codec;
      $j++;
   }

   if ( $dup-> save( $self-> fileName)) {
      $ret = 1;
   } else {
      Prima::MsgBox::message("Error saving " . $self-> fileName . ":$@");
   }
   
EXIT:   
   $self-> image( undef);
   $image-> {extras} = $extras;
   return $ret;
}

1;
