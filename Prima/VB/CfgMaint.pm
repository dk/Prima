#
#  Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
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
# $Id$
package Prima::VB::CfgMaint;
use strict;
use Prima::Utils;
use vars qw(@pages %classes $backup $userCfg $rootCfg $systemWide);

@pages      = ();
%classes    = ();
$backup     = 1;
$systemWide = 0;
$rootCfg    = 'Prima/VB/Config.pm';
$userCfg    = 'vbconfig';

my $file;
my $pkg;


sub reset_cfg
{
	@pages   = ();
	%classes = ();
}

sub open_cfg
{
	if ( $systemWide) {
		$file = undef;
		for (@INC) {
			$file = "$_/$rootCfg", last if -f "$_/$rootCfg" && -r _;
		}
		return (0, "Cannot find $rootCfg") unless $file;
		$pkg = "Prima::VB::Config";
		$file =~ s[\\][/]g;
		eval "require \"$file\";";
	} else {
		$file = Prima::Utils::path($userCfg);
		$pkg  = "Prima::VB::UserConfig";
		return 1 unless -f $file;
		$file =~ s[\\][/]g;
		eval "require \"$file\";";
	}
	return (0,  "$@") if $@;

	@pages   = eval "$pkg".'::pages';
	my @csa  = eval "$pkg".'::classes';
	return ( 0, "Invalid package \'$pkg\'") if scalar @csa % 2;
	%classes = @csa;
	return 1;
}

sub read_cfg
{
	my $sw = $systemWide;
	my ( $f, $p);
	reset_cfg;
	$systemWide = 0;
	my @r = open_cfg;
	return @r unless $r[0];
	( $f, $p) = ( $file, $pkg) if $sw == 0;
	my %cs = %classes;
	my @pg = @pages;
	$systemWide = 1;
	@r = open_cfg;
	return @r unless $r[0];
	( $f, $p) = ( $file, $pkg) if $sw == 1;
	my %pgref = map { $classes{$_}-> {page} => 1} keys %classes;
	%classes = ( %classes, %cs);
	for ( @pg) {
		next if $pgref{$_};
		$pgref{$_} = 1;
		push( @pages, $_);
	}
	($systemWide, $file, $pkg) = ( $sw, $f, $p);
	return 1;
}

sub write_cfg
{
	return ( 0, "Cannot write to $file") if -f $file && ! -w _;

	unless ( -f $file) {
		my $x = $file;
		$x =~ s/[\\\/]?[^\\\/]+$//;
		unless ( -d $x) {
			eval "use File::Path"; die "$@\n" if $@;
			File::Path::mkpath( $x);
		}
	}

	if ( $backup && -f $file) {
		local $/;
		open F, "$file" or return ( 0, "Cannot read $file");
		my $f = <F>;
		close F;
		open F, ">$file.bak" or return ( 0, "Cannot write backup $file.bak");
		print F $f;
		close F;
	}

	open F, ">$file" or return ( 0, "Cannot write to $file");
	my $c = <<HEAD;
package $pkg;

sub pages
{
	return qw(@pages);
}

sub classes
{
   return (
HEAD
	for ( keys %classes) {
		my %dt = %{$classes{$_}};
		$c .= "\t\'$_\' => {\n";
		my $maxln = 0;
		for ( keys %dt) {
			$maxln = length($_) if length( $_) > $maxln;
		}
		$c .= sprintf( "\t\t%-${maxln}s => \'$dt{$_}\',\n", $_) for keys %dt;
		$c .= "\t},\n";
	}

	$c .= <<HEAD2;
	);
}


1;
HEAD2

	print F $c;
	close F;
	return 1;
}

sub add_module
{
	my $file = $_[0];
	my $pkg;

	return ( 0, "Cannot open module $file") unless open F, $file;
	while ( <F>) {
		next if /^#/;
		next unless /package\s+([^\s;].*)/m;
		$pkg = $1;
		$pkg =~ s[[\s;]*$][];
		last;
	}
	close F;
	return ( 0, "Cannot locate 'package' section in $file") unless defined $pkg;

	eval "use $pkg;";
	if ( $@) {
		my $err = "$@";
		if ( $err =~ /Can\'t locate\s([^\s]+)\sin/) {
			$err = "Corrupted module $file - internal and file names doesn't match";
		}
		return ( 0, $err);
	}
	my @clsa = eval "$pkg".'::classes';
	return ( 0, "$@") if $@;
	return ( 0, "Invalid package \'$pkg\'") if scalar @clsa % 2;
	my %cls = @clsa;
	$cls{$_}-> {module} = $pkg for keys %cls;
	my %pgref = map { $classes{$_}-> {page} => 1} keys %classes;
	%classes = ( %classes, %cls);
	for ( keys %cls) {
		my $pg = $cls{$_}-> {page};
		next if $pgref{$pg};
		$pgref{$pg} = 1;
		push @pages, $pg;
	}
	return 1;
}


1;

__DATA__

=pod

=head1 NAME

Prima::VB::CfgMaint - maintains visual builder widget palette configuration.

=head1 DESCRIPTION

The module is used by the Visual Builder and C<cfgmaint> programs, to maintain
the Visual Builder widget palette. The installed widgets are displayed
in main panel of the Visual Builder, and can be maintained by C<cfgmaint>.

=head1 USAGE

The Visual Builder widget palette configuration is contained in two files - the system-wide
C<Prima::VB::Config> and the user C<~/.prima/vbconfig>. The user config file take the precedence
when loaded by the Visual Builder. The module can select either configuration
by assigning C<$systemWide> boolean property.

The widgets are grouped in pages, which are accessible by names. 

New widgets can be added to the palette by calling C<add_module> method,
which accepts a perl module file as its first parameter. The module must
conform to the VB-loadable format.

=head1 FORMAT

This section describes format of a module with VB-loadable widgets.

The module must define a package with same name as the module.
In the package, C<class> sub must be declared, that returns an array
or paired scalars, where each first item in a pair corresponds to the 
widget class and the second to a hash, that contains the class loading information,
and must contain the following keys:

=over

=item class STRING

Name of the VB-representation class, which represents the original
widget class in the Visual Builder. This is usually a lightweight
class, which does not contain all functionality of the original
class, but is capable of visually reflecting changes to the class properties.

=item icon PATH

Sets an image file, where the class icon is contained. 
PATH provides an extended syntax for indicating a frame index, if the image file 
is multiframed: the frame index is appended to the path name
with C<:> character prefix, for example: C<"NewWidget::icons.gif:2">.

=item module STRING

Sets the module name, that contains C<class>.

=item page STRING

Sets the default palette page where the widget is to be put.
The current implementation of the Visual Builder provides four
pages: C<General,Additional,Sliders,Abstract>. If the page
is not present, new page is automatically created when the
widget class is registered.

=item RTModule STRING

Sets the module name, that contains the original class.

=back

The reader is urged to explore F<Prima::VB::examples::Widgety> file,
which contains an example class C<Prima::SampleWidget>, its 
VB-representation, and a property C<lineRoundStyle> definition example.

=head1 API

=head2 Methods

=over

=item add_module FILE

Reads FILE module and loads all VB-loadable widgets from it.

=item classes

Returns string declaration of all registered classes in format
of C<classes> registration procedure ( see L</FORMAT> ).

=item open_cfg

Loads class and pages information from either a system-wide or a user configuration file.
If succeeds, the information is stored in C<@pages> and C<%classes> variables (
the old information is lost ) and returns 1. If fails, returns 0 and string with the error
explanation; C<@pages> and C<%classes> content is undefined.

=item pages

Returns array of page names

=item read_cfg

Reads information from both system-wide and user configuration files,
and merges the information. If succeeds, returns 1. If fails, returns 0
and string with the error explanation.

=item reset_cfg 

Erases all information about pages and classes.

=item write_cfg

Writes either the system-wide or the user configuration file.
If C<$backup> flag is set to 1, the old file renamed with C<.bak> extension.
If succeeds, returns 1. If fails, returns 0 and string with the error explanation.

=back

=head1 FILES

F<Prima::VB::Config.pm>, C<~/.prima/vbconfig>.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<VB>, L<cfgmaint>, F<Prima::VB::examples::Widgety>.

=cut
