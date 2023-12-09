package Prima::sys::Test;

use strict;
use warnings;
use Prima::Config;
use Prima::noX11;
use Prima;
use Test::More;

our @ISA     = qw(Exporter);
our @EXPORT  = qw(create_window set_flag get_flag reset_flag wait_flag);
our $noX11   = defined Prima::XOpenDisplay();
our $flag;
our $timeout = 500;

sub import
{
	my @args    = grep { $_ ne 'noX11' } @_;
	my $needX11 = @args == @_;
	__PACKAGE__->export_to_level(1,@args);
	plan skip_all => "no X11 connection" if $needX11 and $noX11;
}

sub create_window
{
	return if $noX11;
	eval "use Prima::Application name => 'failtester';"; die $@ if $@;
	return Prima::Window-> create(
		onDestroy => sub { $::application-> close},
		origin => [ 200,200], # for twm
		size => [ 200,200],
		@_,
	);
}

sub __wait
{
	return 0 if $noX11;

	my $tick = 0;
	my $t = Prima::Timer-> create( timeout => $timeout, onTick => sub { $tick = 1 });
	$flag = 0;
	$t-> start;
	$::application-> yield while not $flag and not $tick;
	$t-> destroy;
	return $flag;
}

sub set_flag   { $flag = 1 }
sub get_flag   { $flag }
sub reset_flag { $flag = 0 }
sub wait_flag  { get_flag || &__wait }

1;

=pod

=head1 NAME

Prima::sys::Test - GUI test tools

=head1 DESCRIPTION

The module contains a small set of tools used for testing of
Prima-related code together with the standard perl C<Test::> suite.

=head1 SYNOPSIS

	use Test::More;
	use Prima::Test;
	plan tests => 1;
	ok( create_window, "can create window");

=head1 USAGE

=head2 Methods

=over

=item create_window %args

Creates and returns a standard simple Prima window

=item set_flag,get_flag,reset_flag

These manipulate the state of the internal C<$flag> that stops the event loop when set.

=item wait_flag

Waits for the flag to be raised in 500 msec, or returns false.

=back

=head2 no-X11 environment

By default fires skip_all condition if running without an X11 connection. If the test
can be ran without X11, use as:

	use Prima::Test qw(noX11);

which signals the module not to do any GUI initialization.

=head1 AUTHORS

Upasana Shukla, E<lt>me@upasana.meE<gt>,
Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, F<t/*/*.t>

=cut
