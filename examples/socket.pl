=pod

=head1 NAME

examples/socket.pl - Downloads a html file from a given URL and strips it to text.

=head1 FEATURES

Tests the functionality of Prima::File and its
events - onRead, onWrite and onException. ( The latter is not exemplified )

Note that the toolkit interface is alive during the download.

=cut

use strict;
use warnings;
use Prima qw(InputLine Edit Application);
use Socket;


my $w = Prima::MainWindow-> create(
	size => [ $::application-> width * 0.8, $::application-> height * 0.8],
	text => 'Socket demo',
);

my $e = $w-> insert( Edit =>
	pack   => { side => 'top', expand => 1, fill => 'both'},
	text => '',
	wordWrap => 1,
);

my %amps = (
	amp => '&',
	oslash => 'o',
	lt => '<',
	gt => '>',
	nbsp => '',
	raquo => '>>',
);


sub parse
{
	my $t = $_[0];
	$$t =~ s/(\r|\n)//g;
	$$t =~ s/<br>/\n/gi;
	$$t =~ s/<p>/\n\n/gi;
	$$t =~ s/<[^<]*>//g;
	$$t =~ s/&([^&;]*);/exists($amps{lc($1)})?$amps{lc($1)}:'?'/ge;
}

my $data;
my $watcher = Prima::File-> create(
	onRead => sub {
		my ($me,$fh) = @_;
		my ($d,$l);
		$l = sysread $fh, $d, 10240;
		if ($l) {
			$data .= $d;
			$e-> text( ( length $data) . " bytes loaded");
		} else {
			$data =~ s/^(.*)\x0D\x0A\x0D\x0A(.*)$/$2/s;
			my $headers = $1;
			$headers =~ s/\r//g;
			parse( \$data);
			$data = "$headers\n\n$data";
			$e-> textRef( \$data);
			close $fh;
			$me-> file( undef);
		}
	},
	onWrite => sub {
		my ($me,$fh) = @_;
		my $r = "GET $me->{remote_file} HTTP/1.1\r\nHost: $me->{remote_host}\r\nConnection: close\r\n\r\n";
		syswrite $fh, $r, length($r);
		$me-> mask( fe::Read); # want no write notifications
	},
);

my $il = $w-> insert( InputLine =>
	text => '',
	current => 1,
	pack   => { side => 'bottom', fill => 'x'},
	onKeyDown => sub {
	my ( $me, $code, $key, $mod) = @_;
	return unless $key == kb::Enter;
	$me-> clear_event;
	my $t = $me-> text;

	unless ($t =~ m/^(?:http:\/\/)?([^\/]*)((?:\/.*$)|$)/) {
		$e-> text( "Invalid URL");
		return;
	}

	my $remote = $1;
	my $remote_file = $2;
	$remote_file = '/' unless length $remote_file;
	$watcher-> {remote_host} = $remote;
	$watcher-> {remote_file} = $remote_file;
	$watcher-> mask( 0);

	my $port = 80;
	my $proto = getprotobyname('tcp');
	my $iaddr;
	my $tx = $w-> text;
	$w-> text( "Resolving $remote...");
	if ( $remote =~ m/^(.*):(\d+)$/) {
		$port = $2;
		$remote = $1;
	}
	unless ( $iaddr = inet_aton($remote)) {
		$w-> text( $tx);
		$e-> text( "Cannot resolve $remote");
		return;
	}
	my $paddr = sockaddr_in($port, $iaddr);
	unless (socket(SOCK, PF_INET, SOCK_STREAM, $proto)) {
		$w-> text( $tx);
		$e-> text( "Error creating socket: $!");
		return;
	}
	$w-> text( "Connecting to $remote...");
	unless( connect(SOCK, $paddr)) {
		$w-> text( $tx);
		$e-> text( "Error connecting to $remote: $!");
		return;
	}
	$w-> text( $tx);
	$e-> text( $data = '');
	$watcher-> file( *SOCK);
	$watcher-> mask( fe::Write);
}
);

$il-> text("Enter URL:");
$il-> select_all;

run Prima;
