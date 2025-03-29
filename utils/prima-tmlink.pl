#
# TMLINK
#
# .tml -> .inc
# tml files linker
#

use strict;
use warnings;

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
	open FILE, $_[0] or die "Cannot open $_[0]\n";
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
			die "Unmatched bracket in $_[0] ( in $title)\n" if $brackets;
			$funcs{$title} = "$res$title$parms$body";
		} else {
			s|[\s\n\t]*||;
			last if length( $_) == 0;
			my $title ||= 'start of file';
			die "Error parsing $_[0] ( in $title)\n";
		}
	}
}

# Main
unless ( $ARGV[ 0]) {
print "Tml file linker for Prima\n";
print "format: prima-tmlink.pl [ options] filename.tml [filename2.tml...]\n";
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
die "insufficient number of parameters" unless $ARGV [0];


@files = @ARGV;
for ( @files) {
	s/\\/\//g;
	my $fName = find_file( $_);
	die "Cannot find file: $_\n" if !defined $fName;
	load_file( $fName);
}

open FILE, ">$incFile" or die "Cannot open $incFile\n" if defined $incFile;
my $f = ( defined $incFile) ? \*FILE : \*STDOUT;
my $fName = defined $incFile ? $incFile : '.Untitled.tml';

print $f <<HEAD;
/* This file was automatically generated.
   Do not edit, you'll lose your changes anyway.
   file: $fName   */
HEAD

for ( sort keys %funcs) {
	print $f $funcs{$_};
}
print $f "\n";

close FILE if defined $incFile;

