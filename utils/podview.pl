use strict;
use warnings;
use Getopt::Long;
use Prima;
use Prima::HelpViewer;
use Prima::Application;

package SoleHelpViewer;
use vars qw(@ISA %profile_default);
@ISA = qw(Prima::PodViewWindow);

sub profile_default
{
	my $def  = $_[ 0]-> SUPER::profile_default;
	@$def{keys %profile_default} = values %profile_default;
	return $def;
}

sub on_destroy
{
	$_[0]-> SUPER::on_destroy;
	$::application-> close unless @Prima::HelpViewer::helpWindows;
}

package main;

my %opt = (
	help     => 0,
	geometry => undef,
);

sub usage
{
	print <<USAGE;
$0 - POD viewer

options:
   help  - this information
   geometry=WIDTHxHEIGHT[+XOFF+YOFF] - set window size and optionally position in pixels (see man x(7))

USAGE
	exit;
}

GetOptions(\%opt,
	"help|h",
	"geometry|g=s"
) or usage;

usage if $opt{help};

if ( defined $opt{geometry} ) {
	my ( $w, $h, $sx, $x, $sy, $y );
	if ( $opt{geometry} =~ /^(\d+)x(\d+)$/) {
		( $w, $h ) = ( $1, $2 );
	} elsif ( $opt{geometry} =~ /^(\d+)x(\d+)([\+\-])(-?\d+)([\+\-])(-?\d+)$/ ) {
		( $w, $h, $sx, $x, $sy, $y ) = ( $1, $2, $3, $4, $5, $6 );
	} else {
		usage;
	}
	my @desktop = $::application-> size;
	my $p = \ %SoleHelpViewer::profile_default;
	$p->{width}  = $w;
	$p->{height} = $h;
	$p->{sizeDontCare}   = 0;

	if ( defined $x ) {
		$p->{left}   = $x;
		$p->{bottom} = $y;
		$p->{left}   = $desktop[0] - $w - $x if $sx eq '-';
		$p->{bottom} = $desktop[1] - $h - $y - $::application-> get_system_value(sv::YTitleBar) - $::application-> get_system_value(sv::YMenu)
			if $sy eq '+';
		$p->{originDontCare} = 0;
	}
}

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

=cut
