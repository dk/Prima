=pod

=head1 NAME

examples/async.pl - example of using asynchonous communication with another process

=head1 RANT

Windows non-blocking I/O API is hell

=cut

use strict;
use warnings;
use Socket;
use IO::Handle;
use Prima qw(Application Label);

my $q   =  ($^O =~ /win32/i) ? '"' : "'";
my $cmd = "$^X -e $q \$|++; for(1..10){sleep(1);print qq(\$_\\n)} $q";

if ( $^O =~ /win32/i) {
	# use socketpair, works everywhere
	my $r = IO::Handle-> new;
	my $w = IO::Handle-> new;
	socketpair( $r, $w, AF_UNIX, SOCK_STREAM, PF_UNSPEC);

	if (fork) {
		close($w);
		$r-> blocking(0);
		open F, ">&", $r or die $!;
	} else {
		close($r);
		open STDOUT, ">&", $w or die $!;
		exec $cmd;
		exit;
	}
} else {
	# use pipes, works only on unix
	require Fcntl;
	open F, "-|", $cmd or die $!;
	my $fc = 0;
	fcntl( F, Fcntl::F_GETFL(), $fc) or die "can't fcntl(F_GETFL):$!\n";
	fcntl( F, Fcntl::F_SETFL(), Fcntl::O_NONBLOCK() | $fc) or die "can't fcntl(F_SETFL):$!\n";
}

my ( $file, $label, $window);

$file = Prima::File-> new(
	file	=> \*F,
	onRead	=> sub {
		my ( $what, $nbytes);
		$nbytes = sysread( F, $what, 1024);
		if ( !defined $nbytes) {
			close F;
			$what = "Error reading:$!\n";
		} elsif ( 0 == $nbytes) {
			close F;
			$file-> destroy;
			$what = "\nDone";
		}
		$label-> text( $label-> text . $what);
	},
);

$window = Prima::MainWindow-> new( text => 'async' );
$label = $window-> insert( 'Prima::Label' =>
	pack		=> { fill => 'both' },
	autoHeight	=> 1,
	wordWrap	=> 1,
	text		=> "Reading from a subprocess..\n\n",
);

Prima->run;
