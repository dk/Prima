package Prima::sys::win32;

use strict;
use warnings;
use Prima;

sub ReadConsoleInput
{
	my %opt = @_;

	my $flags = 0;
	$flags |= 1 if $opt{peek};        # CONSOLE_READ_NOREMOVE
	$flags |= 2 if $opt{nonblocking}; # CONSOLE_READ_NOWAIT

	my $fd = $opt{fileno} // 0;
	my $r = Prima::Application->sys_action("win32.ReadConsoleInputEx $fd $flags");
	return unless defined $r;
	return map { split '=' } split ' ', $r;
}

1;

=pod

=head1 NAME

Prima::sys::win32 - Windows facilities

=head1 DESCRIPTION

Miscellaneous semi-hackish calls to win32 API

=head1 API

=over

=item ReadConsoleInput ( peek => 0, nonblocking => 0, fileno => 0 )

See https://learn.microsoft.com/en-us/windows/console/readconsoleinputex .

Returns a hash with information about a single input event if there are 1 or
more, or an empty list otherwise.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima::Dialog::FileDialog>

=cut

