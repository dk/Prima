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
#  $Id$
#

=pod 
=item NAME

Prima notebook widget

=item FEATURES

Demonstrates the basic Prima toolkit usage and
Prima::TabbedNotebook standard class.

=cut

use Prima;
use Prima::Buttons;
use Prima::Notebooks;

package Bla;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub init
{
   my $self = shift;
   my %profile = $self-> SUPER::init( @_);

   my $n = $self-> insert( TabbedNotebook =>
      origin => [10, 10],
      size => [ $self-> width - 20, $self-> height - 20],
      growMode => gm::Client,
#     pageCount => 11,
      tabs => [0..5,5,5..10],
   );

   $n-> insert_to_page( 0 => 'Button');
   my $j = $n-> insert_to_page( 1 => 'CheckBox' => left => 200);
   $n-> insert_to_page( 2,
      [ Button => origin => [ 0, 0], ],
      [ Button => origin => [ 10, 40], ],
      [ Button => origin => [ 10, 70], ],
      [ Button => origin => [ 10,100], ],
      [ Button => origin => [ 110, 10], ],
      [ Button => origin => [ 110, 40], ],
      [ Button => origin => [ 110, 70], ],
      [ Button => origin => [ 110,100], ],
   );
   return %profile;
}


package Generic;

$::application = Prima::Application-> create( name => "Generic.pm");

my $l;
my $w = Bla-> create(
  size => [ 600, 300],
  y_centered  => 1,
 # current  => 1,
  onDestroy => sub { $::application-> close},
);


run Prima;
