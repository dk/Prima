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
package main;
use vars qw( @ISA $noX11 $w $dong);
use strict;
# startup
use Prima::Config;
use Prima::noX11;
use Prima;

# testing if is running over a dumb terminal
$noX11 = 1 if defined Prima::XOpenDisplay();

# should be ok now
my $verbose = 0;
my $tie     = 1;
my $testing = 0;
my @results = ();
my @extras  = ();
my @extraTestNames  = ();
my $ok_count;
my $testsRan = 0;
my ( $skipped, $passed, $failed) = (0,0,0);
my ( $eskipped, $epassed, $efailed) = (0,0,0);
my @filters;
#
my $tick;

init();
run();
exit;

package ExTiedStdOut;

sub TIEHANDLE
{
	my $class = shift;
	my $isError = shift;
	bless \$isError, $class;
}

sub analyze
{
	unless ( $testing) {
		print STDERR $_[0];
		return;
	}
	if ( scalar @results == 0 && $_[0] =~ /\d+\s*\.\.\s*(\d+)\s*(.*)/) {
		if ( $1 > 0) {
			@results = ((0) x ( $1));
			@extraTestNames = ();
			my @a = split( ',', $2);
			my $i;
			for ( $i = 0; $i < @a; $i++) { $extraTestNames[$i] = $a[$i]; }
		} else {
			push( @extras, $_[0]);
		}
	} else {
		if ( $_[0] =~ /(not\s+)?ok(.*)\b/) {
			my $notOK = defined $1;
			my $id;
			my $ex = $2;
			if ( $_[0] =~ /ok\s+(\d+)/) {
				$id = $1;
			} else {
				$id = $ok_count;
			}
			if ( defined $ex) {
				$ex = ( $ex =~ /\s+\#\s+skip/i) ? 1 : undef;
			}
			if (( $id > 0 && $id <= scalar @results) || ( scalar @results == ( $id - 1))) {
				$results[ $id - 1] = $ex ? -1 : ( $notOK ? 0 : 1);
			} else {
				push( @extras, "! test $id of ".@results." run");
			}
		} else {
			push( @extras, $_[0]);
		}
	}
}

sub PRINT {
	my $r = shift;
	$$r++;
	analyze( join( '', @_));
}

sub PRINTF {
	shift;
	my $fmt = shift;
	analyze( sprintf($fmt, @_));
}

package main;

sub __end_wait
{
	$tick = 1;
}

sub __wait
{
	return 0 if $noX11;
	$tick = 0;
	my $t = Prima::Timer-> create( timeout => 500, 
		onTick => sub { $tick = 1 });
	$dong = 0;
	$t-> start;
	while ( 1) {
		last if $dong || $tick;
		$::application-> yield;
	}
	$t-> destroy;
	return $dong;
}

sub __dong
{
	$dong = 1;
}

sub ok
{
	print $_[0] ? "ok $ok_count\n" : "not ok $ok_count\n";
	$ok_count++;
}

sub skip
{
	print "ok # skip\n";
	$ok_count++;
}

sub runfile
{
	my $d = $_[0];
	$d =~ s/.t$//;
	my $content;
	{
		open F, $_[0] or die "Error: cannot open $_[0]:$!\n";
		local $/;
		$content = <F>;
		close F;
	}
	if ( $noX11) {
		# check if can run
		if ( $content =~ /Widget|Window|Timer|Drawable|DeviceBitmap|pplication|begin_paint|Menu|Popup|__wait/) {
			print "Testing $d...skipped\n" if $verbose;
			$eskipped++;
			return;
		}
	}
	print "Testing $d...";
	$ok_count = 1;
	$testing = 1;
	@results = ();
	@extras  = ();
	my $c = eval { eval $content; };
	$testing = 0;
	if ( $@) {
		print "test error: $@\n"
	} else {
		my $res = 3;
		for ( @results) {
			if ( $_ < 0) { $skipped++} elsif ( $_ > 0) { $passed++} else { $failed++};   
			next if $_ < 0;
			$res &= $_ ? 1 : 0;
		}   
		if ( $res == 3) { $eskipped++} elsif ( $res) { $epassed++} else { $efailed++};
		
		if ( $verbose) {
			my $i = 0;
			print "\n";
			for ( @results) {
				my $exName = defined $extraTestNames[ $i] ? "  ($extraTestNames[$i])" : '';
				my $j = $i + $testsRan + 1;
				print "test $j:".(( $_ < 0 ) ? "skipped" : (( $_ > 0) ? "passed" : "failed")).$exName."\n";
				$i++;
			}
			$testsRan += $i;
			print "$_\n" for @extras;
		} else {
			my $res = 1;
			my ( $nf, $nr) = ( 0, scalar @results);
			for ( @results) {
				next if $_ < 0;
				$res &= $_;
				$nf++ unless $_;
			}
			$testsRan++;
			print (( $res ? "passed" : "failed ($nf/$nr)")."\n");
		}
	}
}

sub rundir
{
	my $dir = $_[0];
	opendir( FDIR, $dir);
	my @f = readdir( FDIR);
	closedir FDIR;
	my @files = ();
	my @index = ();

	for ( @f) {
		next if ( $_ eq ".") || ( $_ eq "..");
		if ( $_ eq 'order') {
			if ( open F, "$dir/$_") {
				@index = <F>;
				close F;
				chomp for @index;
			}
			next;
		}
		push @files, $_;
	}

	my %h = map { $_ => 1 } @files;
	@f = ();
	for ( @index) {
		next unless length( $_);
		if ( exists $h{$_}) {
			push( @f, $_);
			$h{$_} = 0;
		}
	}

	for ( keys %h) {
		push( @f, $_) if $h{$_};
	}

	for ( @f)
	{
		my $ff = "$dir/$_";
		if ( -d $ff) {
			rundir( $ff);
		} elsif ( -f $ff) {
			next unless $_ =~ /\.t$/;
			if ( scalar @filters) {
				my $match;
				for ( @filters) {
					$match = 1, last if $ff =~ /$_$/;
				}
				next unless $match;
			}
			runfile( $ff);
		}
	}

}

sub init
{
	unless ($^O !~ /MSWin32/) {
		untie *STDOUT;
	}
	for ( @ARGV) {
		if ( /^-(.*)/) {
			for ( split(' *', lc $1)) {
				if ( $_ eq 'v') {
					$verbose = 1;
				} elsif ( $_ eq 'x') {
					$noX11 = 1; 
				} elsif ( $_ eq 'd') {
					$tie = 0;
				}
			}
		} else {
			push( @filters, $_);
		}
	}
	tie *STDOUT, 'ExTiedStdOut', 0 if $tie;

	unless ( $noX11) {
		eval "use Prima::Application name => 'failtester';"; die $@ if $@;
		$w = Prima::Window-> create(
			onDestroy => sub { $::application-> close},
			size => [ 200,200],
		);
	} else {
	print <<NOX11;
** Warning: skipping X11 tests
NOX11
	}
}

sub run
{
	rundir('.');
	print("Atomic tests passed:$passed, skipped:$skipped, failed:$failed\n") if $verbose;
	print("Total tests passed:$epassed, skipped:$eskipped, failed:$efailed\n");
	print("All tests successful\n") unless $efailed; # fake Test::Harness output for CPAN::reporter
}


