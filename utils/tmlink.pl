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
# TMLINK
#
# .tml -> .inc
# tml files linker
#

use strict;

my $warnings       = 1;
my $incFile;
my @files          = ();
my @includeDirs    = qw(./);
my %funcs          = ();

sub find_file
{
	my $fName = $_[0];
	return $fName if -f $fName;
	for (@includeDirs) {
		return "$_$fName" if -f "$_$fName";
	}
	return undef;
}

sub load_file
{
	open FILE, $_[0] or die "APC001: Cannot open $_[0]\n";
	{
		local $/;
		$_ = <FILE>;
		close FILE;
	}

	s#/\*.*?\*/##gs;
	s#//.*?\n##g;

	my ( $res, $title, $parms, $body);
	while (1) {
		if ( s|([\s\n\t]*\w+\s*\*?[\s\n\t]*)(\w+)([\s\n\t]*\([^\)]*\)[\s\n\t]*)(\{)||s) {
			( $res, $title, $parms, $body) = ( $1, $2, $3, '{');
			my $brackets = 1;
			while ( s|(.*?)([{}])||s) {
				$body .= "$1$2";
				last if ( $brackets += (( $2 eq '{') ? 1 : -1)) == 0;
			}
			die "APC003: Unmatched bracket in $_[0] ( in $title)\n" if $brackets;
			$funcs{$title} = "$res$title$parms$body";
		} else {
			s|[\s\n\t]*||;
			last if length( $_) == 0;
			my $title ||= 'start of file';
			die "APC004: Error parsing $_[0] ( in $title)\n";
		}
	}
}

# Main
unless ( $ARGV[ 0]) {
print "Apricot project. Tml file linker.\n";
print "format: tmlink.pl [ options] filename.tml [filename2.tml...]\n";
print "options: -Iinclude_path\n";
print "options: -Ofilename.inc\n";
die "\n";
}

ARGUMENT: while( 1)
{
	$_ = $ARGV[0];
	/^-I(.*)$/      && do {
		my $ii = $1;
		@includeDirs = map { m{[\\/]$} ? $_ : "$_/" } (@includeDirs, split ';', $ii);
		next ARGUMENT;
	};
	/^-o(.*)$/      && do {
		$incFile = $1;
		next ARGUMENT;
	};
	last ARGUMENT;
} continue { shift @ARGV; }
die "APC000: insufficient number of parameters" unless $ARGV [0];


@files = @ARGV;
for ( @files) {
	s/\\/\//g;
	my $fName = find_file( $_);
	die "APC005: Cannot find file: $_\n" if !defined $fName;
	load_file( $fName);
}

open FILE, ">$incFile" or die "APC002: Cannot open $incFile\n" if defined $incFile;
my $f = ( defined $incFile) ? \*FILE : \*STDOUT;
my $fName = defined $incFile ? $incFile : '.Untitled.tml';

print $f <<HEAD;
/* This file was automatically generated.
   Do not edit, you'll loose your changes anyway.
   file: $fName   */
HEAD

for ( sort keys %funcs) {
	print $f $funcs{$_};
}
print $f "\n";

close FILE if defined $incFile;

