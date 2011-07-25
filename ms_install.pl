#! /usr/bin/perl -w
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

BEGIN {
die <<UX if $^O !~ /os2|win32|cygwin/i;
  This program is only intented to be run under MSWin32 and OS/2,
  for the binary installations. Please install the toolkit from the 
  source distribution, by typing 
    
      perl Makefile.PL
      make install
  
UX
}


use strict;
use Config;
use File::Path;
use File::Find;
use File::Copy;

my $install = 1;

for ( @ARGV) {
	$_ = lc $_;
	if ( $_ eq '-uninstall') {
		$install = 0;
		last;
	}
	if (
		$_ eq 'help' || $_ eq 'h' || $_ eq '?' ||
		$_ eq '-help' || $_ eq '-h' || $_ eq '-?' ||
		$_ eq '--help' || $_ eq '--h' || $_ eq '--?'
	) {
		print <<SD;
Prima binary distribution installer for MS systems ( WinNT, Win9X, OS/2).

Format: perl ms_install.pl [ -uninstall]
SD
		exit(0);
	}
}

my $os2 = $^O eq 'os2';
my $mswin32 = ($^O =~ /MSWin32/);
my $cygwin = ($^O =~ /cygwin/);

my $iarc = $Config{ installsitearch};
my $ibin = $Config{ installbin};
my $perlpath = $Config{ perlpath};
$perlpath =~ s/(perl)(\.exe)?$/$1__$2/i if $os2 && $perlpath =~ /perl(\.exe)?$/i;

unless ( $cygwin) {
	$iarc =~ s/\//\\/g;
	$ibin =~ s/\//\\/g;
}

die "Broken config: cannot find directory $iarc\n" unless -d $iarc;
die "Broken config: cannot find directory $ibin\n" unless -d $ibin;
$iarc =~ s/(\\|\/)$//;

my $binlib = $^O;
$binlib =~ s/\s/_/g;

die "No distribution found. The install script must be put into the toolkit root directory\n" 
	unless -f 'Prima.pm';


my (@instfiles, @instdir);

sub abort
{
	warn $_[0];
	unlink $_ for @instfiles;
	rmdir $_ for @instdir;
	rmdir $_ for @instdir;
	rmdir $_ for @instdir;
	exit;
}


if ( $install) {
	my (@cp, @cpbin);

	finddepth( sub {
		my $destdir = $File::Find::dir;
		$destdir =~ s/^\.//;
		if ( $File::Find::dir =~ /(utils|pod)/i) {
			if ( m/^(.*)\.pl$/i) {
				$destdir = $ibin;
			} else {
				$destdir =~ s/[\\\/]*(utils|pod)//i;
				$destdir = $iarc . $destdir;
			}
		} else {
			$destdir = $iarc . $destdir;
		}

		return if -d $_ && m/(utils|pod|test|os2|unix|img|CVS|include|scripts)$/i;
		return if $File::Find::dir =~ /test|CVS|include|os2|unix|bsd|scripts/i;
		return if m/ms_install|Makefile|\.(pdb|opt|pal|obj|log|dsp|dsw|ncb|c|cls|h|inc|def|tml|o)/;

		if ( -d $_) {
			print "Creating $destdir/$_\n";
			File::Path::mkpath( "$destdir/$_");
			push @instdir,"$destdir/$_";
		} elsif ( m/^(.*)\.pl$/i) {
			my $un = $1;
			$destdir = $ibin unless $File::Find::dir =~ /examples/i;
			push @cpbin, [ "$File::Find::dir/$_", "$destdir/$un"];
		} elsif ( /(\S*\.dll)$/i && !(/Prima[^\\\/]*\.dll$/i)) {
			push @cp, [ "$File::Find::dir/$_", "$ibin/$_" ]; 
		} else {
			push @cp, [ "$File::Find::dir/$_", "$destdir/$_"];
		}
	}, ".");

	print "Copying files...\n";
	for ( @cp) {
		my ( $from, $to) = @$_;
		print "Installing $to ...\n";
		push @instfiles, $to;
		next if copy $from, $to;
		abort "Error:$!\n";
	}
	print "Copying executables...\n";
	for ( @cpbin) {
		my ( $src, $dst) = @$_;
		$dst .= ( $os2 ? '.cmd' : ( $mswin32 ? '.bat' : ''));
		push @instfiles, $dst;
		print "Installing $src ...\n";
		if ( $os2) {
			open SRCPL, "<$src" or abort "Cannot open $src: $!";
			open DSTPL, ">$dst" or abort "Cannot create $dst: $!";
			print DSTPL <<ENDP;
extproc $perlpath -wS
ENDP
			my $filestart = 1;
			while ( <SRCPL>) {
				next if $filestart && /^\#\!/;
				$filestart = 0;
				print DSTPL;
			}
			close SRCPL;
			close DSTPL;
		} elsif ( $mswin32) {
			print "Installing $dst ...\n";
			$dst =~ s/bat$/pl/;
			abort "Error:$!\n" unless copy $src, $dst;
			my $i = system("pl2bat $dst");
			$src = $dst;
			$dst =~ s/pl$/bat/;
			abort "Error: pl2bat $dst failed\n" unless -f $dst;
			unlink $src;
		} else {
			open SRCPL, "<$src" or abort "Cannot open $src: $!";
			open DSTPL, ">$dst" or abort "Cannot create $dst: $!";
			print DSTPL <<ENDP;
#!$Config{perlpath} -w
ENDP
			my $filestart = 1;
			while ( <SRCPL>) {
				next if $filestart && /^\#\!/;
				$filestart = 0;
				print DSTPL;
			}
			close SRCPL;
			close DSTPL;
		}
	}

	if ( open F, "> install.log") {
		print F "f:$_\n" for @instfiles;
		print F "d:$_\n" for @instdir;
		close F;
	} else {
		print "(!) Unable to write 'install.log' ($!), uninstall will be unavailable\n";
	}

	print <<D;

  Installation of the Prima tookit is finished. Congratulations!

  * To try out examples, cd to $iarc/examples and run the batch files there.
   
  * To run Visual Builder, type 'VB'. 
  
  * To read the documentation, type 'podview'.

  * To uninstall the toolkit, run 'perl ms_install.pl -uninstall'.

  Visit http://www.prima.eu.org/ for the newest version.

D

	my $found;
	$ibin = lc $ibin;
	$ibin =~ s/\\/\//g;
	$ibin =~ s/[\/]*$//;
	for ( split ( $Config{ path_sep}, $ENV{PATH})) {
		my $path = lc;
		$path =~ s/\\/\//g;
		$path =~ s/[\/]*$//;
		next if $path ne $ibin;
		$found = 1;
	}

	warn <<NODLL unless $found;

** Warning: the executable and DLL files were installed into $ibin.
   However, this directory does not seem to be included into
   your PATH environment variable. You have to add $ibin 
   into your PATH, otherwise the toolkit will not work.
   
NODLL
	
} else {
	die "Cannot uninstall - install.log not found\n" unless open F, "install.log";
	my @dirs;
	while ( <F>) {
		chomp;
		next unless m/^(.)\:(.*)$/;
		print "Deleting $2...\n";
		if ( $1 eq 'f') {
			unlink $2; 
		} elsif ( $1 eq 'd') {
			rmdir $2;
			push @dirs, $2;
		}
	}
	close F;

	rmdir $_ for @dirs;
	rmdir $_ for @dirs;
	print "Done.\n";
}


