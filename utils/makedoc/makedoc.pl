# $Id$
use strict;
use Config;

my $path;

my $build;

for ( @ARGV) {
	if ( m/^--build$/) {
		$build = 1;
	} elsif ( m/^--path=(.+)$/) {
		$path = $1;
	}
}

unless ( $path) {
	for ( '../..', @INC) {
		next unless -f "$_/Prima.pm";
		$path = $_;
		last;
	}
}

print "Using $path as root\n";

$build = 1 unless -f 'Prima.cache.tex';

my @tex;
my @bs;

if ( $build) {
	open F, "$path/Prima.pm" or die "Cannot open $path/Prima.pm:$!\n";
	my $begin;
	for ( <F>) {
		$begin = 1 if !$begin && m/Tutorials/;
		next unless $begin;
		if (  m/L\<([^<]*)\>/) {
			push @bs, [ 0, $1];
		} elsif ( m/^=item\s*(.*)/) {
			if ( $1 eq '*') {
				$_ = <F>;
				$_ = <F>;
				chomp;
				push @bs, [ 1, $_];
			} else {
				push @bs, [ 1, $1];
			}
			push @bs, [0,'Prima'] 
				if $#bs && $bs[-1][0] == 1 && $bs[-1][1] =~ /Core toolkit classes/;
		}
	}
	close F;
} else {
	open F, "Prima.cache.tex" or die "Cannot open Prima.cache.tex:$!\n";
	push @tex, '';
	for ( <F>) {
		push @tex, '' if m/^\\documentclass{article}/;
		$tex[-1] .= $_;
	}
	close F;
}


sub Link
{
	my $x = $_[0];
	if ( $x =~ /^perl/) {
		return "L<$x>";
	} else {
		$x =~ s/"//g;
		if ( $x =~ /^([^\/]+)\/([^\/]+)$/) {
			return "the B<$2> entry in the I<$1> section";
		} else {
			$x =~ s/\///g;
			if ( $x =~ /^Prima|VB|cfgmaint|gencls/) {
				return "the I<$x> section";
			} else {
				return "the I<$x> entry";
			}
		}
	}
}

if ( $build) {
	my $chapter;
	for ( @bs) {
		my @ch = @$_;
		if ( $ch[0]) {
			$chapter = $ch[1];
			next;
		}
		my $xfn = $ch[1];
		my $fn = $ch[1];
		$fn =~ s/::/\//g;
		for ( qw( .pod .pm .pl), '') {
			my $ext = $_;
			for ( $path, "$path/pod", "$path/utils", "$path/Prima/VB", $Config{installsitebin}) {
				next unless -f "$_/$fn$ext";
				$fn = "$_/$fn$ext";
				goto FOUND;
			}
		}
		die "`$fn' document is not found\n";
	FOUND:   

		open W, $fn or die "Cannot open $fn:$!\n";
		my @ctx;
		my $cut;
		my $cow = 1;
		for ( <W>) {
			if ( m/^=for\s*podview\s*<\s*img\s*src=\"?([^\"\s]+)\"?\s*(cut\s*=\s*1)?\s*>/) {
				my ( $gif, $eps, $do_cut) = ( $1, $1, $2);
				$eps =~ s/\//_/g;
				$eps =~ s/\.[^\.]+$/.eps/;
				unless ( -f $eps) {
					for ( "$path/Prima", "$path/Prima/pod", "$path/pod/Prima") {
						next unless -f "$_/$gif";
						$gif = "$_/$gif";
						goto FOUND;
					}
					warn "** $gif is not found\n";
					undef $gif; 
				FOUND:
					if ( defined $gif) {
						print "convert $gif $eps\n";
						system "convert $gif $eps\n";
					}
				}
				if ( -f $eps) {
					$cow = 1;
					$cut = 1 if $do_cut;
					push @ctx, "=for latex \n\\includegraphics[keepaspectratio]{$eps}\n\n";
				} else {
					warn "** error creating $eps\n";
				}
			} elsif ( m/^=for\s*podview.*\/cut/) {
				$cut = 0;
			}

			s/L<([^\>]+)>/Link($1)/ge;
			s/\b(DESCRIPTION|USAGE|BUGS|SYNOPSIS|EXAMPLE|FORMAT|ARGUMENTS|SYNTAX|FILES|FILE FORMAT|METHODS|BASIC PROGRAM)\b/ucfirst(lc $1)/ge;
			
			push @ctx, $_ unless $cut;
		}
		close W;

		my $ffn = $fn;
		if ( $cow) {
			open W, "> tmp.pm" or die "Cannot write tmp.pm:$!\n";
			print W @ctx;
			close W;
			$ffn = 'tmp.pm';
		}
	
		unlink 'out.tex';
		my $q = ($^O =~ /win32/i) ? '"' : "'";
		system "pod2latex -full -modify -sections $q!SEE ALSO|AUTHOR|AUTHORS|COPYRIGHT$q -out out.tex $ffn";

		unlink 'tmp.pm' if $cow;

		{
			open W, "out.tex" or die "Cannot open out.tex:$!\n";
			local $/;
			push @tex, <W>;
			$tex[-1] =~ s/(\n\\section)/\\chapter{$chapter}$1/ if $chapter;
			print $fn, "\n";
			undef $chapter;
			close W;
		}
	}

	open F, "> Prima.cache.tex" or die "Cannot write Prima.cache.tex:$!\n";
	print F $_ for @tex;
	close F;
}

my $i;
local $/;
open F, "intro.tex" or die "Cannot open intro.tex:$!\n";
my $intro = <F>;
close F;

open W, "> Prima.tex" or die "Cannot open Prima.tex:$!\n";
print W $intro;
for ( $i = 0; $i < @tex; $i++) {
	$tex[$i] =~ s/^.*\\begin{document}//s;
	$tex[$i] =~ s/\\tableofcontents//s;
	$tex[$i] =~ s/\\end{document}.*//s if $i < $#tex;
	$tex[$i] =~ s/\\item \d/\\item/gs;
	
	# $tex[$i] =~ s/ elsewhere in this document//gs;
	# $tex[$i] =~ s/the (\\emph{[^}]+}) manpage/$1/gs;
	print W $tex[$i];
}
close W;
