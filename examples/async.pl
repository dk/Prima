# $Id$

=pod

=head1 NAME

async.pl - example of using asynchonous communication with a process

=cut

BEGIN {
	die "Win32 doesn't support non-blocking pipe IO, aborting\n" if $^O =~ /win32/i;
};

use strict;
use Fcntl qw(O_NONBLOCK F_GETFL F_SETFL);
use Prima qw(Application Label);

open F, "$^X -e '\$|++;for(1..10){sleep(1);print qq(\$_\\n)}' |";

my $fc;
fcntl( F, F_GETFL, $fc) or die "can't fcntl(F_GETFL):$!\n";
fcntl( F, F_SETFL, O_NONBLOCK|$fc) or die "can't fcntl(F_SETFL):$!\n";

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

run Prima;
