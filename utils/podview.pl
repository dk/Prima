use strict;
use warnings;
use Prima;
use Prima::HelpViewer;
use Prima::Application;

package SoleHelpViewer;
use vars qw(@ISA);
@ISA = qw(Prima::PodViewWindow);

sub on_destroy
{
	$_[0]-> SUPER::on_destroy;
	$::application-> close unless @Prima::HelpViewer::helpWindows;
}

package main;

$Prima::HelpViewer::windowClass = 'SoleHelpViewer';

my $htx = ( @ARGV ? $ARGV[0] : 'Prima' );
if ( -f $htx) {
	$htx = "file://$htx";
} else {
	$htx .= '/' unless $htx =~ /\//;
}
$::application-> open_help( $htx);

run Prima;
