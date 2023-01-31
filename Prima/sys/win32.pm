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
	my $r = Prima::Application->sys_action("win32.ReadConsoleInputEx $flags");
	return unless defined $r;
	return map { split '=' } split ' ', $r;
}

1;
