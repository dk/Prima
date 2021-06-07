#! /usr/bin/perl
use strict;
use warnings;
use Prima::sys::Gencls;

# Main
unless ( $ARGV[ 0]) {
print <<TEXT;
Perl+C interface parser for Prima
format  : prima-gencls.pl [ options] filename.cls [ out_directory]
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
		push @{ $args->{ incpath}}, split ';', $ii;
		next ARGUMENT;
	};
	last ARGUMENT;
} continue { shift @ARGV; }
die "insufficient number of parameters" unless $ARGV [0];

$ARGV[ 0] =~ m{^(.*[\\/])[^\\/]*$};
$args->{ dirPrefix} = $1 || "";
$args->{ dirOut} = "$ARGV[ 1]/" if $ARGV[ 1];
my @ancestors = gencls( $ARGV[ 0], $args);
if ( @ancestors) {
	print ( map { "ancestor: $_\n"} @ancestors);
}
