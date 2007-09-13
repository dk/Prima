#
#  Copyright (c) 1997-2003 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  Created by Dmitry Karasik <dmitry@karasik.eu.org>
#
#  $Id$

use strict;
use Prima;
package Prima::Themes;
use vars qw(%themes %data $load_rc_file);

use constant INSTALLED => 0;
use constant CALLBACK  => 1;
use constant PROFILE   => 2;
use constant MODULE    => 3;
use constant INSTALL   => 4;

# install implicit property 'theme' selector
push @Prima::Object::hooks, \&hook;
# load and execute theme from rc file
load_rc(1) if $load_rc_file || !defined $load_rc_file;

sub hook
{
	my ( $object, $profile, $default) = @_;
	if ( exists $default-> {theme} || exists $profile-> {theme}) {
		my $theme = exists($profile-> {theme}) ? $profile-> {theme} : $default-> {theme};
		# execute explicitly selected theme
		execute( $themes{$theme}, $object, $profile, $default) 
			if exists $themes{$theme};
	} else {
		# execute for all installed themes
		execute( $themes{$_}, $object, $profile, $default) 
			for grep { $themes{$_}-> [INSTALLED] } keys %themes;
	}
};

sub load_rc
{
	my $install = defined( $_[0]) ? $_[0] : 1;
	eval "use Prima::Utils;"; die $@ if $@;
	my $f = Prima::Utils::path('themes');
	if ( $f && -f $f && open F, '<'.$f) {
		while ( <F>) {
			next if m/^\s*#/;
			chomp;
			my @r = split(',', $_, 3);
			next unless defined $r[0] && defined $r[1];
			$data{$r[1]} = $r[2];
			eval "use $r[0];";
			warn( "** warning: error loading module `$r[0]': $@\n"), next if $@;
			warn( "** warning: theme `$r[1]' is not defined\n"), next 
				unless loaded($r[1]);
			install($r[1]) if $install;
		}
		close F;
	}
}

# saves currently installed modules
sub save_rc
{
	eval "use Prima::Utils;"; die $@ if $@;
	my $f = Prima::Utils::path('themes');
	return 0 unless open F, '>'.$f;
	for ( keys %themes) {
		next unless $themes{$_}-> [INSTALLED] && $themes{$_}-> [MODULE];
		my $data = defined($data{$_}) ? $data{$_} : '';
		print F "$themes{$_}->[MODULE],$_,$data\n";
	}
	return close F;
}

# register theme
sub register
{
	my ( $file, $theme, $profile, $merger, $installer) = @_;
	deregister($_) if $themes{$theme};
	$themes{$theme} = [ 
		0,         # activity flag
		$merger,   # merger routine, our own if undef
		$profile,  # theme profile
		$file,     # theme file
		$installer,# installer/uninstaller routine
	];
}

# kill theme
sub deregister 
{
	uninstall($_[0]);
	delete $themes{$_[0]};
}

# list registered themes
sub list { keys %themes }
# list active themes
sub list_active { grep { $themes{$_}-> [INSTALLED] } keys %themes }

# checks if theme is loaded
sub loaded { defined($_[0]) ? exists $themes{$_[0]} : undef }
# checks if theme is active
sub active { (defined($_[0]) && exists $themes{$_[0]}) ? $themes{$_[0]}-> [INSTALLED] : undef }


# unistall all themes and select new
sub select
{
	my @themes = @_;
	uninstall (keys %themes);
	install (@themes);
}

# load themes from files
sub load { for ( @_) { eval "use Prima::themes::$_"; die $@ if $@ }}
# makes 'use Prima::Themes qw(mytheme theme1);' possible
sub import { 
	shift; 
	my $install;
	for ( @_ ) {
		if ( $_ eq ':install') {
			$install = 1;
		} else {
			$install ? install($_) : load($_);
		}
	}
}

# install themes
sub install
{
	for ( @_) {
		my $theme = $_;
		next if !exists $themes{$theme} || $themes{$theme}-> [INSTALLED];
		if ( $themes{$theme}-> [INSTALL]) {
			$themes{$theme}-> [INSTALLED] = $themes{$theme}-> [INSTALL]-> ($theme, 1);
		} else {
			$themes{$theme}-> [INSTALLED] = 1;
		}
	}
}

# uninstall themes
sub uninstall
{
	for ( @_) {
		my $theme = $_;
		next if !exists $themes{$theme} || !$themes{$theme}-> [INSTALLED];
		$themes{$theme}-> [INSTALL]-> ($theme, 0) if $themes{$theme}-> [INSTALL];
		$themes{$theme}-> [INSTALLED] = 0;
	}
}

# theme data property
sub data
{
	return $data{$_[0]} unless $#_;
	$data{$_[0]} = $_[1];
}

# default merger procedure
sub merger
{
	my ( $object, $profile, $default, $new) = @_;
	$profile-> {$_} = $new-> {$_} for keys %$new;
}

# applies theme during Object::profile_add
sub execute
{
	my ( $instance, $object, $profile, $default) = @_;
	my $merger = $instance-> [CALLBACK] || \&merger; 
	my $profiles = $instance-> [PROFILE];
	return unless $profiles;
	my $i;
	for ( $i = 0; $i < @$profiles; $i += 2) {
		$merger-> ( $object, $profile, $default, $$profiles[$i+1]) if $object-> isa($$profiles[$i]);
	}
}

package Prima::Themes::Proxy;

sub new
{
	return bless { object => $_[1] }, $_[0];
}

sub AUTOLOAD
{
	no strict;
	my ($method) = $AUTOLOAD =~ /::([^:]+)$/;
	return shift-> {object}-> $method( @_);
}

# do not fordward DESTROY
sub DESTROY {}


1;

=pod 

=head1 NAME

Prima::Themes - object themes management

=head1 DESCRIPTION

Provides layer for theme registration in Prima. Themes are loosely grouped
alternations of default class properties and behavior, by default stored in 
C<Prima/themes> subdirectory. The theme realization is implemented as interception
of object profile during its creation, inside C<::profile_add>. Various themes
apply various alterations, one way only - once an object is applied a theme,
it cannot be neither changed nor revoked thereafter.

Theme configuration can be stored in an rc file, F<~/.prima/themes>, and is
loaded automatically, unless C<$Prima::Themes::load_rc_file> explicitly set to C<0>
before loading the C<Prima::Themes> module. In effect, any Prima application 
not aware of themes can be coupled with themes in the rc file by the following:

	perl -MPrima::Themes program

C<Prima::Themes> namespace provides registration and execution functionality.
C<Prima::Themes::Proxy> is a class for overriding certain methods, for internal
realization of a theme.

For interactive theme selection use F<examples/theme.pl> sample program.

=head1 SYNOPSIS

	# register a theme file
	use Prima::Themes qw(color);
	# or
	use Prima::Themes; load('color');
	# list registered themes
	print Prima::Themes::list;

	# install a theme
	Prima::Themes::install('cyan');
	# list installed themes
	print Prima::Themes::list_active;
	# create object with another theme while 'cyan' is active
	Class->create( theme => 'yellow');
	# remove a theme
	Prima::Themes::uninstall('cyan');

=head1 Prima::Themes

=over 4 

=item load @THEME_MODULES

Load THEME_MODULES from files via C<use> clause, dies on error.
Can be used instead of explicit C<use>.

A loaded theme file may register one or more themes.

=item register $FILE, $THEME, $MATCH, $CALLBACK, $INSTALLER

Registers a previously loaded theme. $THEME is a unique string identifier.
$MATCH is an array of pairs, where the first item is a class name,
and the second is an arbitrary scalar parameter. When a new object is created,
its class is matched via C<isa> to each given class name,
and if matched, the $CALLBACK routine is called with the following parameters:
object, default profile, user profile, second item of the matched pair.

If $CALLBACK is C<undef>, the default L<merger> routine is called,
which treats the second items of the pairs as hashes of the same format as
the default and user profiles.

The theme is inactive until C<install> is called. If $INSTALLER subroutine is
passed, it is called during install and uninstall, with two parameters, the
name of the theme and boolean install/uninstall flag. When install flag is 1,
the theme is about to be installed; the subroutine is expected to return a
boolean success flag. Otherwise, subroutine return value is not used.

$FILE is used to indicate the file in which the theme is stored.

=item deregister $THEME

Un-registers $THEME.

=item install @THEMES

Installs previosuly loaded and registered loaded THEMES; the installed themes
are now used to match new objects.

=item uninstall @THEMES

Uninstalls loaded THEMES.

=item list

Returns the list of registered themes.

=item list_active

Returns the list of installed themes.

=item loaded $THEME

Return 1 if $THEME is registered, 0 otherwise.

=item active $THEME

Return 1 if $THEME is installed, 0 otherwise.

=item select @THEMES

Uninstalls all currently installed themes, and installs THEMES instead.

=item merger $OBJECT, $PROFILE_DEFAULT, $PROFILE_USER, $PROFILE_THEME

Default profile merging routine, merges $PROFILE_THEME into $PROFILE_USER
by keys from $PROFILE_DEFAULT.

=item load_rc [ $INSTALL = 1 ]

Reads data F<~/.prima/themes> and loads listed modules.
If $INSTALL = 1, installs the themes from the rc file.

=item save_rc

Writes configuration of currently installed themes into rc file,
returns success flag. If success flag is 0, C<$!> contains the error.

=back

=head1 Prima::Themes::Proxy

An instance of C<Prima::Themes::Proxy>, created as

Prima::Themes::Proxy-> new( $OBJECT)

is a non-functional wrapper for any Perl object $OBJECT. All methods of $OBJECT,
except C<AUTOLOAD>, C<DESTROY>, and C<new>, are forwarded to $OBJECT
itself transparently. The class can be used, for example, to deny all
changes to C<lineWidth> inside object's painting routine:

	package ConstLineWidth;
	use vars qw(@ISA);
	@ISA = qw(Prima::Themes::Proxy);

	sub lineWidth { 1 } # line width is always 1 now!

	Prima::Themes::register( '~/lib/constlinewidth.pm', 'constlinewidth', 
		[ 'Prima::Widget' => {
			onPaint => sub {
				my ( $object, $canvas) = @_;
				$object-> on_paint( ConstLineWidth-> new( $canvas));
			},
		} ]
	);

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 FILES

F<~/.prima/themes>

=head1 SEE ALSO

L<Prima>, L<Prima::Object>, F<examples/themes.pl>


=cut
