#
#  Created by:
#     Vadim Belman   <voland@plab.ku.dk>
#     Anton Berezin  <tobez@plab.ku.dk>
#
package Prima::Utils;
use strict;
use warnings;
require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT = ();
@EXPORT_OK = qw(
	query_drives_map query_drive_type
	get_os get_gui
	beep sound
	username
	xcolor
	find_image path
	sleep alarm post wait
	last_error

	chdir chmod closedir getcwd link mkdir open_file open_dir
	read_dir rename rmdir unlink utime
	seekdir telldir rewinddir
	getenv setenv stat access getdir sv2local local2sv

	nearest_d nearest_i
);

sub xcolor {
# input: '#rgb' or '#rrggbb' or '#rrrgggbbb'
# output: internal color used by Prima
	my ($r,$g,$b,$d);
	$_ = $_[0];
	$d=1/16, ($r,$g,$b) = /^#([\da-fA-F]{3})([\da-fA-F]{3})([\da-fA-F]{3})/
	or
	$d=1, ($r,$g,$b) = /^#([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})/
	or
	$d=16, ($r,$g,$b) = /^#([\da-fA-F])([\da-fA-F])([\da-fA-F])/
	or return 0;
	($r,$g,$b) = (hex($r)*$d,hex($g)*$d,hex($b)*$d);
	return ($r<<16)|($g<<8)|($b);
}

sub find_image
{
	my $mod = @_ > 1 ? shift : 'Prima';
	my $name = shift;
	$name =~ s!::!/!g;
	$mod =~ s!::!/!g;
	for (@INC) {
		return "$_/$mod/$name" if -f "$_/$mod/$name" && -r _;
	}
	return undef;
}

# returns a preferred path for the toolkit configuration files,
# or, if a filename given, returns the name appended to the path
# and proofs that the path exists
sub path
{
	my $path;
	if ( exists $ENV{HOME}) {
		$path = "$ENV{HOME}/.prima";
	} elsif ( $^O =~ /win/i && exists $ENV{USERPROFILE}) {
		$path = "$ENV{USERPROFILE}/.prima";
	} elsif ( $^O =~ /win/i && exists $ENV{WINDIR}) {
		$path = "$ENV{WINDIR}/.prima";
	} else {
		$path = "/.prima";
	}

	if ( $_[0]) {
		unless ( -d $path) {
			eval "use File::Path"; die "$@\n" if $@;
			File::Path::mkpath( $path);
		}
		$path .= "/$_[0]";
	}

	return $path;
}

sub alarm
{
	my ( $timeout, $sub, @params) = @_;
	return undef unless $::application;
	my $timer = Prima::Timer-> create(
		name    => $sub,
		timeout => $timeout,
		owner   => $::application,
		onTick  => sub {
			$_[0]-> destroy;
			$sub-> (@params);
		}
	);
	$timer-> start;
	return $timer if $timer-> get_active;
	$timer-> destroy;
	return undef;
}

sub sleep
{
	my $stop;
	Prima::Utils::alarm( 1000 * shift, sub { $stop++ } );
	$::application->yield(1) while !$stop;
}

sub wait
{
	my ( $cond, $timeout ) = @_;
	Carp::carp("bad condition"), return undef unless ref $cond;
	return undef unless $::application;

	my $sub = (ref($cond) eq 'CODE') ? $cond : sub { !!$$cond };
	return 1 if $sub->();

	unless ( defined $timeout ) {
		$::application->yield(1) while !$sub->();
		return 1;
	}

	my $alarm;
	my $timer = Prima::Utils::alarm( $timeout, sub { $alarm = 1 }) or return undef;
	$::application->yield(1) while !$alarm && !$sub->();
	return 0 if $alarm;
	$timer->destroy if $timer->alive;
	return 1;
}

sub post
{
	my ( $sub, @params) = @_;
	return 0 unless $::application;
	my $id;
	$id = $::application-> add_notification( 'PostMessage', sub {
		my ( $me, $parm1, $parm2) = @_;
		if ( defined($parm1) && $parm1 eq 'Prima::Utils::post' && $parm2 == $id) {
			$::application-> remove_notification( $id);
			$sub-> ( @params);
			$me-> clear_event;
		}
	});
	return 0 unless $id;
	$::application-> post_message( 'Prima::Utils::post', $id);
	return 1;
}

1;

=pod

=head1 NAME

Prima::Utils - miscellaneous routines

=head1 DESCRIPTION

The module contains miscellaneous helper routines

=head1 API

=over

=item alarm $TIMEOUT, $SUB, @PARAMS

Calls SUB with PARAMS after TIMEOUT milliseconds.
Returns 0 on failure, and the active timer on success.
The timer can be stopped to disarm the alarm.

=item beep [ FLAGS = mb::Error ]

Invokes the system-depended sound and/or visual bell,
corresponding to one of the following constants:

	mb::Error
	mb::Warning
	mb::Information
	mb::Question

=item get_gui

Returns one of the C<gui::XXX> constants that report the graphic
user interface used in the system:

	gui::Default
	gui::Windows
	gui::XLib
	gui::GTK

=item get_os

Returns one of the C<apc::XXX> constants that report the system platform.
Currently, the list of the supported platforms is:

	apc::Win32
	apc::Unix

=item find_image PATH

Converts PATH from a perl module notation into a file path and
searches for the file in the C<@INC> paths set. If the file is
found, its full filename is returned; otherwise C<undef> is
returned.

=item last_error

Returns last system error, if any

=item nearest_i NUMBERS

Performs C< floor($_ + .5) > operation over NUMBERS which can be an array or an
arrayref.  Returns converted integers in either an array or an arrayref form,
depending on the calling syntax.

=item nearest_d NUMBERS

Performs C< floor($_ * 1e15 + .5) / 1e15 > operation over NUMBERS which can be
an array or an arrayref.  Returns converted NVs in either an array or an
arrayref form, depending on the calling syntax.  Used to protect against perl
configurations that calculate C<sin>, C<cos> etc with only 15 significant
digits in the mantissa. This function prevents the accumulation of error in these
configurations.

=item path [ FILE ]

If called with no parameters, returns the path to a directory, usually F<~/.prima>,
that can be used to store the user settings of a toolkit module or a program.
If FILE is specified, appends it to the path and returns the full file name. In
the latter case, the path is automatically created by C<File::Path::mkpath>
unless it already exists.

=item post $SUB, @PARAMS

Postpones a call to SUB with PARAMS until the next event loop tick.

=item query_drives_map [ FIRST_DRIVE = "A:" ]

Returns an anonymous array to drive letters used by the system.  FIRST_DRIVE can
be set to another value to start enumeration from.  Win32 can probe removable
drives there, so to increase the responsiveness of the function it might be
reasonable to call it with FIRST_DRIVE set to C<C:> .

If the system supports no drive letters, an empty array reference is returned ( unix ).

=item query_drive_type DRIVE

Returns one of the C<dt::XXX> constants that describe the type of a drive,
where DRIVE is a 1-character string. If there is no such drive, or
the system supports no drive letters ( unix ), C<dt::None> is returned.

	dt::None
	dt::Unknown
	dt::Floppy
	dt::HDD
	dt::Network
	dt::CDROM
	dt::Memory

=item sleep SECONDS

Same as perl's native C<sleep> (i.e. C<CORE::sleep>) but with the event loop
running.  Note that the argument it takes is seconds, for the sake of
compatibility, while the rest of the toolkit operates in milliseconds.

=item sound [ FREQUENCY = 2000, DURATION = 100 ]

Issues a tone of FREQUENCY in Hz with DURATION in milliseconds.

Obsoleted as modern systems stopped supporting APIs for primitive sound generation.

=item username

Returns the login name of the user.
Sometimes is preferred to the perl-provided C<getlogin> ( see L<perlfunc/getlogin> ) .

=item xcolor COLOR

Accepts COLOR string in one of the three formats:

	#rgb
	#rrggbb
	#rrrgggbbb

and returns a 24-bit RGB integer value

=item wait CONDITION [, TIMEOUT ]

Waits for a condition for max TIMEOUT milliseconds, or forever if TIMEOUT is undefined.

Returns undef on failure, 0 on TIMEOUT, 1 on a successful CONDITION.

CONDITION is either a scalar reference, or a sub to be polled, where their values
are treated as 0 as a signal to continue the waiting, and 1 as a stop signal.

=back

=head1 Unicode-aware filesystem functions

Since perl's win32 unicode support for files is unexistent, Prima has its own
parallel set of functions mimicking native functions, ie C<open>, C<chdir> etc.
This means that files with names that cannot be converted to ANSI (ie
user-preferred) codepage are not visible in perl, but the functions below
mitigate that problem.

The following fine points need to be understood before using these functions:

=over

=item *

Prima makes a distinction between whether scalars have their utf8 bit set or not
throughout the whole toolkit. For example, text output in both unix and
windows is different depending on the bit, treating non-utf8-bit text as
locale-specific, and utf8-bit text as unicode. The same model is applied to
file names.

=item *

Perl implementation for native Win32 creates virtual environments for each
thread and may keep more than one instance of the current directory, environment
variables, etc. This means that under Win32, calling C<Prima::Utils::chdir>
will NOT automatically make C<CORE::chdir> assume that value, even if the path
is convertible to ANSI. Keep that in mind when mixing Prima and core functions.
To add more confusion, under the unix, these two chdir calls are identical when the
path is fully convertible.

=item *

Under unix, reading entries from the environment or the file system is
opportunistic: if a text string (file name, environment entry) is a valid utf8
string, then it is treated and reported as one. Mostly because the .UTF-8
locales are default and found everywhere. Note that Prima ignores C< $ENV{LANG}
> here. This is a bit problematic on Perls under 5.22 as these don't provide
> means to check for the utf8 string validity, so every string will be slapped
> a utf8 bit on here -- beware.

=item *

Setting environment variables may or may not sync with C< %ENV >, depending on
how perl is built. Also, C< %ENV > will warn when trying to set scalars with
utf-8 bit there.

=back

=over

=item access PATH, MODE

Same as C<POSIX::access>.

=item chdir DIR

Same as C<CORE::chdir> but disregards the thread-local environment on Win32.

=item chmod PATH, MODE

Same as C<CORE::chmod>

=item closedir, readdir, rewinddir, seekdir, telldir DIRHANDLE

Mimic homonymous perl functions

=item getcwd

Same as C<Cwd::getcwd>

=item getdir PATH

Reads the content of the PATH directory and and returns array of string pairs where
the first item is a file name and the second is a file type.

The file type is a string, one of the following:

	"fifo" - named pipe
	"chr"  - character special file
	"dir"  - directory
	"blk"  - block special file
	"reg"  - regular file
	"lnk"  - symbolic link
	"sock" - socket
	"wht"  - whiteout

This function was implemented for faster directory reading,
to avoid successive calls of C<stat> for every file.

Also, getdir is consistently inclined to treat filenames in utf8,
disregarding both perl unicode settings and the locale.

=item getenv NAME

Reads directly from environment, possibly bypassing C< %ENV >, and disregarding
thread-local environment on Win32.

=item link OLDNAME, NEWNAME

Same as C<CORE::link>.

=item local2sv TEXT

Converts 8-bit text into either 8-bit non-utf8-bit or unicode utf8-bit string.
May return undef on memory allocation failure.

=item mkdir DIR, [ MODE = 0666 ] 

Same as C<CORE::mkdir>.

=item open_file PATH, FLAGS

Same as C<POSIX::open>

=item open_dir PATH

Returns directory handle to be used on C<readdir>, C<closedir>, C<rewinddir>, C<telldir>, C<seekdir>.

=item rename OLDNAME, NEWNAME

Same as C<CORE::rename>

=item rmdir PATH

Same as C<CORE::rmdir>

=item setenv NAME, VAL

Directly sets environment variable, possibly bypassing C< %ENV >, depending on
how perl is built. Also disregards the thread-local environment on Win32.

Note that effective synchronization between this call and C< %ENV > is not
always possible, since Win32 perl implementation simply does not allow that.
One is advised to assign to C< %ENV > manually, but only if both NAME and VAL
don't have their utf8 bit set, otherwise perl will warn about wide characters.

=item stat PATH

Same as C<CORE::stat>, except on systems that provide sub-second time
resolution, in which case returns the atime/mtime/ctime entries as floats, the
same as C<Time::HiRes::stat> does.

=item sv2local TEXT, FAIL_IF_CANNOT = 1

Converts either 8-bit non-utf8-bit or unicode utf8-bit string into a local encoding.
May return undef on memory allocation failure, or if TEXT contains unconvertible
characters when FAIL_IF_CANNOT = 1

=item unlink PATH

Same as C<CORE::unlink>.

=item utime ATIME, MTIME, PATH

Same as C<CORE::utime>, except on systems that provide sub-second time
resolution, in which case returns the atime/mtime/ctime entries as floats, the
same as C<Time::HiRes::utime> does.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::sys::FS>.

