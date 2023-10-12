=pod

=head1 NAME

examples/ps_setup.pl - PostScript/PDF printer setup

=head1 FEATURES

Prints a PS/PDF document after the setup dialog is finished.

Whereas Prima::PS modules can be used on any platform, they serve as an only
printing API on unix systems.  The Prima::PS interface can load user
preferences from $HOME/.prima/printer file. This file is maintained by the
standard printer setup dialog.

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
$x-> font-> size(100);
$x-> text_out( "Hello world!", 100, 100);
$x-> end_doc;

1;
