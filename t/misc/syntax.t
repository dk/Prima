use strict;
use warnings qw(FATAL);
use Test::More;
use Prima::sys::Test qw(noX11);
use FindBin qw($Bin);
use File::Find qw(find);

my @packages;

find ( sub {
	return unless /\.pm$/;
	return if $File::Find::dir =~ /blib/;
	my $f = "$File::Find::dir/$_";
	$f =~ s/^.*(Prima.*).pm/$1/;
	$f =~ s[/][::]g;
	push @packages, $f;
}, "$Bin/../../");

plan tests => scalar @packages;
require_ok($_) for @packages;
