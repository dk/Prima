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

use strict;
use Prima;
use Prima::Buttons;
use Prima::ComboBox;
use Prima::Edit;
use Prima::ImageViewer;
use Prima::Label;
use Prima::Sliders;
use Prima::Image::TransparencyControl;

package Prima::Image::gif;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
       width    => 480,
       text     => 'Gif89a filter',
       height   => 330,
       centered => 1,
       designScale => [ 7, 16],
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   my $a = $self-> insert( qq(Prima::CheckBox) => 
       origin => [ 4, 289],
       name => 'Transparent',
       size => [ 133, 36],
       text => '~Transparent',
       delegations => ['Check'],
   );
   my $b = $self-> insert( qq(Prima::CheckBox) => 
       origin => [ 144, 289],
       name => 'Interlaced',
       size => [ 100, 36],
       text => '~Interlaced',
   );
   $self-> insert( qq(Prima::CheckBox) => 
       origin => [ 249, 289],
       name => 'UserInput',
       size => [ 101, 36],
       text => '~User input',
   );
   $self-> insert( qq(Prima::Image::TransparencyControl) => 
       origin => [ 4, 128],
       size => [ 364, 158],
       text => '',
       name => 'TC',
   );
   my $se = $self-> insert( qq(Prima::Edit) => 
       origin => [ 10, 10],
       name => 'Comment',
       size => [ 272, 91],
       vScroll => 1,
       text => '',
   );
   $self-> insert( qq(Prima::Label) => 
       origin => [ 10, 105],
       size => [ 100, 20],
       text => '~Comment',
       focusLink => $se,
   );
   $se = $self-> insert( qq(Prima::SpinEdit) => 
       origin => [ 290, 75],
       name => 'Delay',
       size => [ 180, 20],
       step => 250,
       min => 0,
       max => 32768,
   );
   $self-> insert( qq(Prima::Label) => 
       origin => [ 290, 105],
       size => [ 180, 20],
       text => 'Frame ~delay, ms',
       focusLink => $se,
   );
   my $cb = $self-> insert( qq(Prima::ComboBox) => 
       origin => [ 290, 10],
       name  => 'Disposal',
       style => cs::DropDownList,
       items => ['None', 'Do not dispose', 'Restore to background color', 'Restore to previous frame', ],
       size  => [ 180, 20],
   );
   $self-> insert( qq(Prima::Label) => 
       origin => [ 290, 40],
       size => [ 180, 20],
       text => 'Disposal ~method',
       focusLink => $cb,
   );
   $self-> insert( qq(Prima::Button) => 
       origin => [ 380, 287],
       name => 'OK',
       size => [ 96, 36],
       text => '~OK',
       default => 1,
       modalResult => mb::OK,
       delegations => ['Click'],
   );
   $self-> insert( qq(Prima::Button) => 
       origin => [ 380, 242],
       size => [ 96, 36],
       text => 'Cancel',
       modalResult => mb::Cancel,
   );
   return %profile;
}


sub transparent
{
   my $self = $_[0];
   $self-> Transparent-> checked( $_[1]);
   $self-> TC-> enabled( $_[1]);
}

sub Transparent_Check
{
   my ( $self, $tr) = @_;
   $self-> transparent( $tr-> checked);
}

sub on_change
{
   my ( $self, $codec, $image) = @_;
   $self-> {image} = $image;
   return unless $image;
   $self-> Interlaced-> checked( exists( $image-> {extras}-> {interlaced}) ? 
      $image-> {extras}-> {interlaced} : $codec-> {saveInput}-> {interlaced});
   $self-> transparent( $image-> {extras}-> {transparentColorIndex} ? 1 : 0);
   $self-> TC-> image( $image);
   $self-> TC-> index( exists( $image-> {extras}-> {transparentColorIndex}) ? 
      $image-> {extras}-> {transparentColorIndex} : 0);
   $self-> UserInput-> checked( exists( $image-> {extras}-> {userInput}) ? 
      $image-> {extras}-> {userInput} : ($codec-> {saveInput}-> {userInput} || 0));
   $self-> Disposal-> focusedItem( exists( $image-> {extras}-> {disposalMethod}) ? 
      $image-> {extras}-> {disposalMethod} : ($codec-> {saveInput}-> {disposalMethod} || 0));
   $self-> Delay-> value( exists( $image-> {extras}-> {delayTime}) ? 
      $image-> {extras}-> {delayTime} : ($codec-> {saveInput}-> {delayTime} || 0));
   $self-> Comment-> text( exists( $image-> {extras}-> {comment}) ? 
      $image-> {extras}-> {comment} : ($codec-> {saveInput}-> {comment}));
}

sub OK_Click
{
   my $self = $_[0];
   if ( $self-> Transparent-> checked) {
      $self-> {image}-> {extras}-> {transparentColorIndex} = $self-> TC-> index;
   } else {
      delete $self-> {image}-> {extras}-> {transparentColorIndex};
   }
   $self-> {image}-> {extras}-> {interlaced} = $self-> Interlaced-> checked;
   my $x = $self-> Comment-> text;
   if ( length $x) {
      $self-> {image}-> {extras}-> {comment} = $x;
   } else {
      delete $self-> {image}-> {extras}-> {comment};
   }
   $x = $self-> Disposal-> focusedItem;
   if ( $x) {
      $self-> {image}-> {extras}-> {disposalMethod} = $x;
   } else {
      delete $self-> {image}-> {extras}-> {disposalMethod};
   }
   $x = $self-> UserInput-> checked;
   if ( $x) {
      $self-> {image}-> {extras}-> {userInput} = $x;
   } else {
      delete $self-> {image}-> {extras}-> {UserInput};
   }
   $x = $self-> Delay-> value;
   if ( $x) {
      $self-> {image}-> {extras}-> {delayTime} = $x;
   } else {
      delete $self-> {image}-> {extras}-> {delayTime};
   }
   delete $self-> {image};
   $self-> TC-> image( undef);
}

sub save_dialog
{
   my $d = Prima::Image::gif-> create;
   $d-> Delay-> enabled(0);
   $d-> Disposal-> enabled(0);
   return $d;
}

1;
