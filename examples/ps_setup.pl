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

$::application-> icon( Prima::StdBitmap::icon(0));

my $x = Prima::PS::Printer-> create;
my %z = %{$x-> {data}};
my %p = %{$x-> {data}-> {devParms}};
$x-> setup_dialog;

for ( keys %z) {
	next if $_ eq 'devParms';
#	print "$_:$z{$_} => $x->{data}->{$_}\n";
}
for ( keys %p) {
#	print "$_:$p{$_} => $x->{data}->{devParms}->{$_}\n";
}

# uncomment this to print document with the applied changes
$x-> begin_doc;
$x-> text_out( "hello!", 100, 100);
$x-> end_doc;

1;
