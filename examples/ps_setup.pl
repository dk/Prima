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
# $Id$
=pod 
=item NAME

A prima PostScript printer output setup program.
Prints a PS document after setup dialog is finished.

=item FEATURES

Whereas Prima::PS modules can be used on any platform,
they serve as an only remedy on *nix systems when printing
via Prima is desired. The Prima::PS interface can load user
preferences from $HOME/.prima/printer file. This file is
maintained by the PostScript output setup dialog.

=cut

use strict;
use Prima;
use Prima::PS::Printer;
use Prima::Application;
use Prima::StdBitmap;

$::application-> icon( Prima::StdBitmap::icon(0));

my $x = Prima::PS::Printer-> create;
my %z = %{$x-> {data}};
my %p = %{$x-> {data}-> {devParms}};
$x-> setup_dialog;

for ( keys %z) {
	next if $_ eq 'devParms';
#	print "$_:$z{$_} => $x->{data}->{$_}\n";
}
for ( keys %p) {
#	print "$_:$p{$_} => $x->{data}->{devParms}->{$_}\n";
}

# uncomment this to print document with the applied changes
#$x-> begin_doc;
#$x-> text_out( "hello!", 100, 100);
#$x-> end_doc;

1;
