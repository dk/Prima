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
use Prima::Image::TransparencyControl;

package Prima::Image::GBM::lbm;
use vars qw(@ISA);
@ISA = qw(Prima::Image::BasicTransparencyDialog);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = ( text     => 'LBM/IFF filter',);
   @$def{keys %prf} = values %prf;
   return $def;
}

sub save_dialog
{
   return Prima::Image::GBM::lbm-> create;
}

package Prima::Image::GBM::gif;
use vars qw(@ISA);
@ISA = qw(Prima::Image::BasicTransparencyDialog);

sub profile_default
{
   my $def = $_[ 0]-> SUPER::profile_default;
   my %prf = ( text     => 'Gif89a filter',);
   @$def{keys %prf} = values %prf;
   return $def;
}

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init(@_);
   $self-> insert( qq(Prima::CheckBox) => 
       origin => [ 143, 167],
       name => 'Interlaced',
       size => [ 100, 36],
       text => '~Interlaced',
   );
   return %profile;
}

sub on_change
{
   my ( $self, $codec, $image) = @_;
   $self-> SUPER::on_change( $codec, $image);
   return unless $image;
   $self-> Interlaced-> checked( exists( $image-> {extras}-> {interlaced}) ? 
      $image-> {extras}-> {interlaced} : $codec-> {saveInput}-> {interlaced});
}

sub OK_Click
{
   my $self = shift;
   $self-> {image}-> {extras}-> {interlaced} = $self-> Interlaced-> checked;
   $self-> SUPER::OK_Click(@_);
}

sub save_dialog
{
   return Prima::Image::GBM::gif-> create;
}

1;
