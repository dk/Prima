=pod

=head1 NAME

examples/socket_anyevent_poe.pl - AnyEvent version of examples/socket.pl with POE::Loop::Prima

=head1 AUTHOR

Vikas N Kumar

=cut

use strict;
use warnings;
use POE 'Loop::Prima';
use Prima qw(InputLine Edit Application);
use AnyEvent;
use AnyEvent::Socket;

my $impl = AnyEvent::detect;
print "AnyEvent implementation is $impl\n";

my $w = Prima::MainWindow-> new(
	size      => [ $::application-> width * 0.6, $::application-> height * 0.6],
	text      => 'AnyEvent demo',
    onDestroy => sub {
        # when the window is closed we want AnyEvent to also exit
    },
);

my $e = $w-> insert( Edit =>
	pack     => { side => 'top', expand => 1, fill => 'both'},
	text     => '',
	wordWrap => 1,
);

my $il = $w-> insert( InputLine =>
	text      => 'Enter URL:',
	current   => 1,
	pack      => { side => 'bottom', fill => 'x'},
	onKeyDown => sub {
	    my ( $me, $code, $key, $mod) = @_;
	    return unless $key == kb::Enter;
	    $me-> clear_event;
	    my $t = $me-> text;

	    return $e-> text( "Invalid URL") unless $t =~ m/^(?:http:\/\/)?([^\/]*)((?:\/.*$)|$)/;
    	my ($remote, $uri, $port) = ($1,$2,80);
	    $uri = '/' unless length $uri;

        $e-> text("");

        tcp_connect $remote, $port, sub {
            my ($fh, $host, $port) = @_;

			return $e-> text("error:$!") unless $fh;

            # make a GET request on the socket
            syswrite $fh, "GET $uri HTTP/1.1 \r\n\r\n";
            shutdown $fh, 1;

            # create an event watcher on the socket $fh for reading
            my $ev;
            $ev = AnyEvent->io(fh => $fh, poll => 'r', cb => sub {
                my $response = '';
                my $len = sysread $fh, $response, 10240;
                if (not defined $len or $len <= 0) {
                    undef $ev; # remove the watcher
		        } else {
			        $e-> text( $e->text . $response );
                }
            });
       };
       0; # keep void context of tcp_connect
    },
);
$il-> select_all;

run Prima;
