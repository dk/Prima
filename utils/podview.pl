use strict;
use warnings;
use Getopt::Long;
use Prima;
use Prima::HelpViewer;
use Prima::Application name => 'POD viewer';
use Prima::Image::base64;
use Prima::sys::GUIException;

package SoleHelpViewer;
use vars qw(@ISA %profile_default);
@ISA = qw(Prima::PodViewWindow);

sub profile_default
{
	my $def  = $_[ 0]-> SUPER::profile_default;
	@$def{keys %profile_default} = values %profile_default;
	$def->{icon} = Prima::Image::base64->load_image(<<POD);
R0lGODdhQABAANUAADRlpDlppj1sqEBuqUVyq0p2rk14r055sFN8sleAtFqCtWCGuGSJuWqOvG6Q
vXCSv3SVwHeYwnqaw4GfxoSiyImmyo6pzJKszpav0Juz0qC31aK51am+2K3B2rLE3LbI37vL4MDP
48TS5MnW5s7a6dLc6tbg7Nrj7t3l8OTq8unu9ezx9u7y+PL1+fb4+/7+/v///wAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACwAAAAAQABAAIU0ZaQ5aaY9bKhAbqlF
cqtKdq5NeK9OebBTfLJXgLRagrVghrhkiblqjrxukL1wkr90lcB3mMJ6msOBn8aEosiJpsqOqcyS
rM6Wr9Cbs9Kgt9WiudWpvtitwdqyxNy2yN+7y+DAz+PE0uTJ1ubO2unS3OrW4Oza4+7d5fDk6vLp
7vXs8fbu8vjy9fn2+Pv+/v7///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAG/8CXcEgsGo/IpFKoEomW0KhU2RQJptgs9GQFaL/goUkA8IbPWEIZwEK7oYV1601H
xsvzun5oWKf2gAp+gHsIg4R1gmV/iHSKACeNdAxrkZJulGWWl2eZACScaBBroKFhDqSmYaNlI1Ir
IBsYGRwlSSkgGRcZHZtHuLoZHr4vrABPSx2PawACE5suGobMZQQVjEItGdPUBRYqQhJryEgid9TU
FC0eaujMAhZCHe3uZQIYL+Jl5EYX9e5k/lFD8EAgNQUR1nhAUgHdAQoZLEwIWA8BxAoNKNZTUCFD
BQYam635cGQDtQorjJA4t0ZDHiJd0G04AqIeySIoAqwRYKsctf8GSVwsEwAOSYsG6BYWQXUICbdU
dqhhQ4KUGYciJpi5SlITHdAjHtBVgELtKhF/ZRgsMWaMzRGmzAZA6WC1yCN+RyieOAcir7ueSpiN
JcLs5a+dLzCsuWAkqz19ADpA4SaBiApmS0qsKfAibJkJRkSsUYAWQAYonr4K0bxmiegyCF58WBMh
9OgMi1GvcUDkBGYlrAEYeGGyDAUjI9ZYXHN6CeSZQ1Iwm2qEBWZjzYlIV76m75ID47Qzy46kT5mu
rY7Qm75kBbOUQ1owI7CkYRnzAOQemYAu9hIKoxUhHzOMJeGbWEgEtwZ0SSgoGREDMvMgEm0BEAB1
RHiSX3sUcWb2hDuDHUECNRAYiI5ZRygIQAhH1CPABi70llB/E65WFToKKDUECRoCUOCH9tRTwDIC
HcCSQAIUIEBIAFSGBDMrGbQGSFKWcYEHTNZDnhEUvdACBVkq94QKkG3UUwoVosMAYEgM0BoTGSyg
EQES4JUCBk8BUAAFpfR2QZ4GUMBmEm6W8UsJJ8CnBAoloGAYEi6cgOijS9DThipfnEMpplFoyqkW
5xT16RSPYDiqEgk0dSoUqS6yqhSPEPMqEp7IOqsRtd66hCd96nqEMb36uhRUwhphjKnFsoJXsUOI
syyz4WwFrREoTGsEDNhmq+223Hbr7bfgfhsEADs=
POD
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

A small GUI browser for POD-formatted files. Accepts either a file path or a
perl module name (f.ex. I<File::Copy>) as a command line agrument, displays the
documentation found.

=head2 SEE ALSO

L<perlpod> - the Plain Old Documentation format

L<Prima> - perl graphic toolkit the viewer is based on

L<Prima::HelpViewer> - menu commands explained

L<Prima::tutorial/Adding help to your program> - how to add help content

=head1 AUTHORS

Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>

=cut
