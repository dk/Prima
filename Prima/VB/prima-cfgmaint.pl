use strict;
use warnings;
use Prima::VB::CfgMaint;

die <<ABOUT unless @ARGV;
format: cfgmaint [options] command object [parameters]
options:
   -r    use root config to write
   -b    use both user and root config to read
   -x    do not write backups
   -o    read-only mode
   -p    execute 'use Prima;' code
commands:
   a  - add     p|m    name
   l  - list    w|p|m  ( w - [page])
   d  - remove  w|p|m  name
   r  - rename  w|p    name new_name
   m  - move    w|p    name new_page or none to end
objects:
   w  - widgets
   p  - pages
   m  - modules

examples:
   cfgmaint -r a m CPAN/Prima/VB/New/MyCtrls.pm
   cfgmaint -b l w
ABOUT

my @cmd  = ();
my $both = 0;
my $ro   = 0;
$Prima::VB::CfgMaint::systemWide = 0;
$Prima::VB::CfgMaint::backup     = 1;

for ( @ARGV) {
	push( @cmd, $_), next unless /^-/;
	$_ = lc $_;
	s/^-//;
	for ( split( '', $_)) {
		if ( $_ eq 'b') {
			$both = 1;
		} elsif ( $_ eq 'r') {
			$Prima::VB::CfgMaint::systemWide = 1;
		} elsif ( $_ eq 'x') {
			$Prima::VB::CfgMaint::backup = 0;
		} elsif ( $_ eq 'o') {
			$ro = 1;
		} elsif ( $_ eq 'p') {
			eval "use Prima;";
			die "$@" if $@;
		} else {
			die "Unknown option: $_\n";
		}
	}
}

sub check
{
	die "format: $cmd[0] [$_[0]]\n" if scalar @cmd < $_[1];
	my %h = map { $_ => 1 } split( '', $_[0]);
	return if $h{$cmd[1]};
	die "Invalid sub-option: $cmd[1]. Use one of '$_[0]'\n";
}

sub assert
{
	die "$_[1]\n" unless $_[0];
}


die "Insufficient number of parameters\n" if @cmd < 2;

$cmd[$_] = lc $cmd[$_] for 0..1;
if ( $cmd[0] eq 'a') {
	check('pm', 3);
} elsif ( $cmd[0] eq 'l') {
	check('wpm', 2);
} elsif( $cmd[0] eq 'd') {
	check('wpm', 3);
} elsif( $cmd[0] eq 'r') {
	check('wp', 4);
} elsif( $cmd[0] eq 'm') {
	check('wp', 3);
	die "Insufficient number of parameters\n" if @cmd < 4 && $cmd[1] eq 'w';
} else {
	die "Unknown action: $cmd[0]\n";
}

my @r;
if ( $both) {
	@r = Prima::VB::CfgMaint::read_cfg();
} else {
	@r = Prima::VB::CfgMaint::open_cfg();
}
die "$r[1]\n" unless $r[0];

if ( $cmd[0] eq 'a') {
	if ( $cmd[1] eq 'm') {
		my %cs = %Prima::VB::CfgMaint::classes;
		my %pg = map { $_ => 1} @Prima::VB::CfgMaint::pages;
		assert( Prima::VB::CfgMaint::add_module( $cmd[2]));
		for ( @Prima::VB::CfgMaint::pages) {
			next if $pg{$_};
			print "page '$_' added\n";
		}
		for ( keys %Prima::VB::CfgMaint::classes) {
			next if $cs{$_};
			print "widget '$_' added\n";
		}
	} elsif ( $cmd[1] eq 'p') {
		my %pg = map { $_ => 1} @Prima::VB::CfgMaint::pages;
		die "Page '$cmd[2]' already exists\n" if $pg{$cmd[2]};
		push @Prima::VB::CfgMaint::pages, $cmd[2];
	}
} elsif ( $cmd[0] eq 'l') {
	if ( $cmd[1] eq 'w') {
		my $ok = defined $cmd[2] ? 0 : 1;
		for ( keys %Prima::VB::CfgMaint::classes) {
			next if defined $cmd[2] && $Prima::VB::CfgMaint::classes{$_}-> {page} ne $cmd[2];
			print "$_\n";
			$ok = 1
		}
		die "Page '$cmd[2]' doesn't exist\n" unless $ok;
	} elsif ( $cmd[1] eq 'p') {
		print join( "\n", @Prima::VB::CfgMaint::pages);
	} elsif ( $cmd[1] eq 'm') {
		my %pk = ();
		$pk{$Prima::VB::CfgMaint::classes{$_}-> {module}} = 1
		for keys %Prima::VB::CfgMaint::classes;
		for ( keys %pk) { print "$_\n"; }
	}
	exit;
} elsif( $cmd[0] eq 'd') {
	if ( $cmd[1] eq 'w') {
		die "Widget '$cmd[2]' doesn't exist\n" unless
			$Prima::VB::CfgMaint::classes{$cmd[2]};
		delete $Prima::VB::CfgMaint::classes{$cmd[2]};
	} elsif ( $cmd[1] eq 'p') {
		my @p;
		for ( @Prima::VB::CfgMaint::pages) {
			push ( @p, $_) unless $cmd[2] eq $_;
		}
		die "Page '$cmd[2]' doesn't exist\n"
			if scalar @Prima::VB::CfgMaint::pages == scalar @p;
		@Prima::VB::CfgMaint::pages = @p;
		for ( keys %Prima::VB::CfgMaint::classes) {
			next unless $Prima::VB::CfgMaint::classes{$_}-> {page} eq $cmd[2];
			delete $Prima::VB::CfgMaint::classes{$_};
			print "Widget '$_' deleted\n";
		}
	} elsif ( $cmd[1] eq 'm') {
		my %dep;
		my $ok = 0;
		for ( keys %Prima::VB::CfgMaint::classes) {
			unless ( $Prima::VB::CfgMaint::classes{$_}-> {module} eq $cmd[2]) {
				$dep{$Prima::VB::CfgMaint::classes{$_}-> {page}} = 1;
				next;
			}
			delete $Prima::VB::CfgMaint::classes{$_};
			$ok = 1;
			print "widget '$_' removed\n";
		}
		my @newpages;
		for ( @Prima::VB::CfgMaint::pages) {
			push ( @newpages, $_) , next if $dep{$_};
			print "page '$_' removed\n";
		}
		@Prima::VB::CfgMaint::pages = @newpages;
		die "Package '$cmd[2]' not found\n" unless $ok;
	}
} elsif( $cmd[0] eq 'r') {
	if ( $cmd[1] eq 'w') {
		die "Widget '$cmd[2]' doesn't exist\n" unless
			$Prima::VB::CfgMaint::classes{$cmd[2]};
		die "Widget '$cmd[3]' already exist\n" if
			$Prima::VB::CfgMaint::classes{$cmd[3]};
		$Prima::VB::CfgMaint::classes{$cmd[3]} = $Prima::VB::CfgMaint::classes{$cmd[2]};
		delete $Prima::VB::CfgMaint::classes{$cmd[2]};
	} elsif ( $cmd[1] eq 'p') {
		my %pg = map { $_ => 1} @Prima::VB::CfgMaint::pages;
		die "Page '$cmd[2]' doesn't exist\n" unless $pg{$cmd[2]};
		die "Page '$cmd[3]' already exist\n" if $pg{$cmd[3]};
		for ( @Prima::VB::CfgMaint::pages) {
			$_ = $cmd[3], last if $_ eq $cmd[2];
		}
		for ( keys %Prima::VB::CfgMaint::classes) {
			$Prima::VB::CfgMaint::classes{$_}-> {page} = $cmd[3] if
				$Prima::VB::CfgMaint::classes{$_}-> {page} eq $cmd[2];
		}
	}
} elsif( $cmd[0] eq 'm') {
	if ( $cmd[1] eq 'w') {
		my %pg = map { $_ => 1} @Prima::VB::CfgMaint::pages;
		die "Page '$cmd[3]' doesn't exist\n" unless $pg{$cmd[3]};
		die "Widget '$cmd[2]' doesn't exist\n" unless
			$Prima::VB::CfgMaint::classes{$cmd[2]};
		$Prima::VB::CfgMaint::classes{$cmd[2]}-> {page} = $cmd[3];
	} elsif ( $cmd[1] eq 'p') {
		my %pg = map { $_ => 1} @Prima::VB::CfgMaint::pages;
		die "Page '$cmd[2]' doesn't exist\n" unless $pg{$cmd[2]};
		die "Page '$cmd[3]' doesn't exist\n" if ! exists $pg{$cmd[3]} && defined $cmd[3];
		my @p;
		for ( @Prima::VB::CfgMaint::pages) {
			push ( @p, $_) unless $cmd[2] eq $_;
		}
		@Prima::VB::CfgMaint::pages = @p;
		@p = ();
		if ( defined $cmd[3]) {
			for ( @Prima::VB::CfgMaint::pages) {
				push ( @p, $cmd[2]) if $_ eq $cmd[3];
				push ( @p, $_);
			}
			@Prima::VB::CfgMaint::pages = @p;
		} else {
			push @Prima::VB::CfgMaint::pages, $cmd[2];
		}
		print join( "\n", @Prima::VB::CfgMaint::pages);
	}
}

assert( Prima::VB::CfgMaint::write_cfg) unless $ro;

=pod

=head1 NAME

cfgmaint - configuration tool for Visual Builder

=head1 SYNTAX

cfgmaint [ -rbxop ] command object [ parameters ]

=head1 DESCRIPTION

Maintains configuration of the widget palette in the Visual Builder.
The widget palette can be stored in the system-wide and local user config files.
C<cfgmaint> allows adding, renaming, moving, and deleting the
classes and pages in the Visual Builder widget palette.

=head1 USAGE

C<cfgmaint> is invoked with the C<command> and C<object> arguments
where the C<command> defines the action to be taken and C<object>
the object to be handled.

=head2 Options

=over

=item -r

Write configuration to the system-wide config file

=item -b

Read configuration from both the system-wide and user config files

=item -x

Do not write backups

=item -o

Read-only mode

=item -p

Execute C<use Prima;> code before start. This option might be necessary when
adding a module that relies on the toolkit but does not invoke the code itself.

=back

=head2 Objects

=over

=item m

Selects a module. Valid for the add, list, and remove commands.

=item p

Selects a page. Valid for all commands.

=item w

Selects a widget. Valid for the list, remove, rename, and move commands.

=back

=head2 Commands

=over

=item a

Adds a new object to the configuration. Can be either a page or
a module.

=item d

Removes an object.

=item l

Prints the object name. In case the object is a widget, prints all
registered widgets. If the string is specified as an additional
parameter, it is treated as a page name, and only widgets from
the page are printed.

=item r

Renames the object to a new name, which is passed as an additional parameter.
Can be either a widget or a page.

=item m

If the C<object> is a widget, relocates one or more widgets to a new page.
If the C<object> is a page, moves the page before the page specified as an additional parameter,
or to the end if no additional page is specified.

=back

=head1 EXAMPLE

Add a new module to the system-wide configuration:

	cfgmaint -r a m CPAN/Prima/VB/New/MyCtrls.pm

List widgets that are present in both config files:

	cfgmaint -b l w

Rename a page:

	cfgmaint r p General Basic

=head1 FILES

F<Prima/VB/Config.pm>, F<~/.prima/vbconfig>

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<VB>, L<Prima::VB::CfgMaint>

=cut

