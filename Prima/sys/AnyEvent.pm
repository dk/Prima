package Prima::sys::AnyEvent;

use strict;
use warnings;
use AnyEvent;
use Prima qw(Application);

sub io
{
	my (undef, %r) = @_;

	my $f = Prima::File->new(
		mask        => (("w" eq $r{poll}) ? fe::Write : fe::Read),
		onRead	    => $r{cb},
		onWrite     => $r{cb},
		onException => $r{cb},
	);

	if (ref $r{fh}) {
		$f->file( $r{fh} )
	} else {
		$f->fd( $r{fh} )
	}

	return $f;
}

sub timer
{
	my ( undef, %r ) = @_;

	my $cb     = $r{cb};
	my $next   = $r{ after } || $r{ interval };
	my $repeat = $r{ interval };

	# Convert to miliseconds for Prima
	$next   *= 1000;
	$repeat *= 1000 if $repeat;

	my $timer = Prima::Timer->new(
		timeout => $next,
		onTick	=> sub {
			if ( $repeat ) {
				$_[0]->timeout( $repeat );
			} else {
				$_[0]->stop;
			};
			&$cb()
		}
	);
	$timer->start;
	return $timer;
}

sub poll { $::application->yield }

{
	no warnings 'redefine';
	sub AnyEvent::CondVar::Base::_wait { $::application->yield until exists $_[0]{_ae_sent} }
}

push @AnyEvent::REGISTRY, ["Prima", __PACKAGE__ ];

=head1 NAME

Prima::sys::AnyEvent - AnyEvent bridge

=head1 SYNOPSIS

   use Prima qw(sys::AnyEvent);
   use AnyEvent;

   my $ev = AnyEvent->timer(after => 1, cb => sub { print "waited 1 second!\n" });
   run Prima;

=head1 DESCRIPTION

This is an I<experiment> to bring in L<AnyEvent::Impl::Prima> into the toolkit's core.
The code is almost 100% from there.

=head1 AUTHORS

Zsban Ambrus

Max Maischein

Dmitry Karasik

=head1 SEE ALSO

F<examples/socket_anyevent.pl>

=cut

1;
