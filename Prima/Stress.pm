#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
#  Module for simulated extreme situations, useful for testing.
#
#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  $Id$
#
use strict;
no warnings;
use Prima;

my $rtfs = (  5, 7, 14, 18) [ int(rand(4)) ] ;

Prima::Application::add_startup_notification( sub {
	$::application-> font-> size( $rtfs);
});

package Prima::Widget;

sub get_default_font
{
	return Prima::Drawable-> font_match( { size => $rtfs }, {});
}

package Prima::Application;

sub get_default_font
{
	return Prima::Drawable-> font_match( { size => $rtfs }, {});
}


1;

__DATA__

=pod

=head1 NAME

Prima::Stress - stress test module

=head1 DESCRIPTION

The module is intended for use in test purposes, to check the functionality of
a program or a module under particular conditions that
might be overlooked during the design. Currently, the only stress factor implemented
is change of the default font size, which is set to different value every time 
the module is invoked.

To use the module functionality it is enough to include a typical

	use Prima::Stress;

code, or, if the program is invoked by calling perl, by using

	perl -MPrima::Stress program

syntax. The module does not provide any methods.

=head1 AUTHOR

Dmitry Karasik E<lt>dmitry@karasik.eu.orgkE<gt>,

=cut
