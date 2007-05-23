#! /usr/bin/perl
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
#  $Id$
#

use Prima::Gencls;

# Main
unless ( $ARGV[ 0]) {
print <<TEXT;
Apricot project. Pseudoobject Perl+C Bondage interface parser.
format  : gencls.pl [ options] filename.cls [ out_directory]
options :
   --h          generates .h file
   --inc        generates .inc file
   --tml        generates .tml file ( and turns -O flag on)
   -O           turns optimized .inc generation on
   -Idirlist    specifies include directories list
   --depend     produces output of dependences of given object only
   --sayparent  produces parent dependency of object only
TEXT
die "\n";
}

my $args;

ARGUMENT: while( 1)
{
	$_ = $ARGV[0];
	last unless defined $_;
	/^--depend$/    && do { $args->{ depend} = 1; next ARGUMENT; };
	/^--sayparent$/ && do { $args->{ sayparent} = 1; next ARGUMENT; };
	/^--h$/         && do { $args->{ genH} = 1; next ARGUMENT; };
	/^--inc$/       && do { $args->{ genInc} = 1; next ARGUMENT; };
	/^--tml$/       && do { $args->{ genTml} = 1; next ARGUMENT; };
	/^-O$/          && do { $args->{ optimize} = 1; next ARGUMENT; };
	/^-I(.*)$/      && do {
		my $ii = $1;
		push @{ $args->{ incpath}}, map { m{[\\/]$} ? $_ : "$_/" } split ';', $ii;
		next ARGUMENT;
	};
	last ARGUMENT;
} continue { shift @ARGV; }
die "APC000: insufficient number of parameters" unless $ARGV [0];

$ARGV[ 0] =~ m{^(.*[\\/])[^\\/]*$};
$args->{ dirPrefix} = $1 || "";
$args->{ dirOut} = "$ARGV[ 1]/" if $ARGV[ 1];
my @ancestors = gencls( $ARGV[ 0], $args);
if ( @ancestors) {
	print ( map { "ancestor: $_\n"} @ancestors);
}
