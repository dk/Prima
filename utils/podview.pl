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

my $htx = ( @ARGV ? $ARGV[0] : $0 );
if ( -f $htx) {
	$htx = "file://$htx";
} else {
	$htx .= '/' unless $htx =~ /\//;
}
$::application-> open_help( $htx);

run Prima;

=pod

=head1 NAME

podview - graphical pod viewer

=head2 DESCRIPTION

The small GUI browser for POD format. Given the argument either
as a file path, or perl module name (f.ex. I<File::Copy>), displays
the documentation found.

=head2 SEE ALSO

L<perlpod> - the Plain Old Documentation format

L<Prima> - perl graphic toolkit the viewer is based on

L<Prima::HelpViewer> - menu commands explained

=head1 AUTHORS

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>
