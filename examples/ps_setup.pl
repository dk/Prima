=pod

=head1 NAME

examples/ps_setup.pl - A prima PostScript printer output setup program.

=head1 FEATURES

Prints a PS document after setup dialog is finished.

Whereas Prima::PS modules can be used on any platform,
they serve as an only remedy on *nix systems when printing
via Prima is desired. The Prima::PS interface can load user
preferences from $HOME/.prima/printer file. This file is
maintained by the PostScript output setup dialog.

=cut

use strict;
use warnings;
use Prima;
use Prima::PS::Printer;
use Prima::Application;
use Prima::StdBitmap;

if (!@ARGV || $ARGV[0] !~ /^\-(ps|pdf)$/) {
	print "Please run with either -ps or -pdf\n";
	exit(1);
} 

$::application-> icon( Prima::StdBitmap::icon(0));

my $class = ($ARGV[0] eq '-ps') ?
	'Prima::PS::Printer' :
	'Prima::PS::PDF::Printer';

my $x = $class-> new;
$x-> setup_dialog;

$x-> begin_doc;
$x-> color(cl::Green);
$x-> text_out( "hello!", 100, 100);
$x-> end_doc;

1;
