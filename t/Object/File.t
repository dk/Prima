use strict;
use warnings;
use Socket;
use Test::More;
use Prima::sys::Test;
use Prima::Application;
use Prima::Utils qw(post);
use IO::Socket::INET;

plan tests => 9;
alarm(60);

my $serv_sock = IO::Socket::INET-> new(
	Proto     => 'tcp',
	Listen    => 1,
	ReuseAddr => 1,
);
my ($serv_port) = sockaddr_in(getsockname($serv_sock));
ok( $serv_sock, "Listening on port $serv_port");
my ($handle_sock, $handle_file, $serv_file);

eval { $serv_file = Prima::File->new(
	file   => $serv_sock,
	mask   => fe::Read,
	onRead => sub {
		my $self = shift;
		my $ok = accept( $handle_sock, $serv_sock );
		ok( $ok, "Accepted connection" . ( $ok ? '' : "(error:$!)"));
		$handle_file->file( $handle_sock );
		$handle_file->mask( fe::Read );
		$handle_sock->autoflush(1);
		undef $serv_file;
		undef $serv_sock;
	}
) };
ok( $serv_file, "Attached Prima::File object on server_socket" . ( $@ ? "(error:$@)" : ''));

my $handle_data;
$handle_sock = IO::Handle->new;
$handle_file = Prima::File->new(
	onRead  => sub {
		my $self = shift;
		$self-> mask(fe::Write);
		$handle_data = <$handle_sock>;
		ok( length($handle_data), "Read from client" . ( length($handle_data) ? "" : $!));
		chomp $handle_data;
		$handle_data = reverse($handle_data);
	},
	onWrite => sub {
		my $self = shift;
		$self-> mask(0);
		my $ok = print $handle_sock "$handle_data\n";
		ok( $ok, "Write back to client" . ( $ok ? "" : $!));
	},
);

my ($client_sock, $client_file);
post( sub {
	$client_sock = IO::Socket::INET->new(
		Proto     => 'tcp',
		PeerPort  => $serv_port,
		PeerAddr  => '127.0.0.1',
	);
	$client_sock->autoflush(1);
	ok( $client_sock, "Connected to $serv_port" . ( $client_sock ? "" : $!));
	$client_file = Prima::File->new(
		file      => $client_sock,
		mask      => fe::Write,
		onRead    => sub {
			my $self = shift;
			$self->mask(0);
			my $data = <$client_sock>;
			chomp $data;
			is( $data, reverse('hello'), "Read back from server");
			post( sub { $::application->close } );
		},
		onWrite    => sub {
			my $self = shift;
			$self->mask(fe::Read);
			my $ok = print $client_sock "hello\n";
			ok( $ok, "Write to server" . ( $ok ? "" : $!));
		},
	);
} );

run Prima;
ok( 1, "Terminated properly");
