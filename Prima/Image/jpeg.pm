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
use Prima::Label;
use Prima::Sliders;

package Prima::Image::jpeg;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);


sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = (
       text   => 'JPEG filter',
       width  => 241,
       height => 92,
       designScale => [ 7, 16],
       centered => 1,
   );
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   my $se = $self-> insert( qq(Prima::SpinEdit) => 
       origin => [ 5, 45],
       name => 'Quality',
       size => [ 74, 20],
       min => 1,
       max => 100,
   );
   $self-> insert( qq(Prima::Label) => 
       origin => [ 5, 69],
       size => [ 131, 20],
       text => '~Quality (1-100)',
       focusLink => $se,
   );
   $self-> insert( qq(Prima::CheckBox) => 
       origin => [ 5, 5],
       name => 'Progressive',
       size => [ 131, 36],
       text => '~Progressive',
   );
   $self-> insert( qq(Prima::Button) => 
       origin => [ 141, 50],
       name => 'OK',
       size => [ 96, 36],
       text => '~OK',
       default => 1,
       modalResult => cm::OK,
       delegations => ['Click'],
   );
   $self-> insert( qq(Prima::Button) => 
       origin => [ 141, 5],
       name => 'Cancel',
       size => [ 96, 36],
       text => 'Cancel',
       modalResult => cm::Cancel,
   );
   return %profile;
}

sub on_change
{
   my ( $self, $codec, $image) = @_;
   $self-> {image} = $image;
   $self-> Quality-> value( exists( $image-> {extras}-> {quality}) ? 
      $image-> {extras}-> {quality} : $codec-> {saveInput}-> {quality});
   $self-> Progressive-> checked( exists( $image-> {extras}-> {progressive}) ? 
      $image-> {extras}-> {progressive} : $codec-> {saveInput}-> {progressive});
}

sub OK_Click
{
   my $self = $_[0];
   $self-> {image}-> {extras}-> {quality} = $self-> Quality-> value;
   $self-> {image}-> {extras}-> {progressive} = $self-> Progressive-> checked;
   delete $self-> {image};
}

sub save_dialog
{
   return Prima::Image::jpeg-> create;
}
