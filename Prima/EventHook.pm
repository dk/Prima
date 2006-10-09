# $Id$
#
#  Copyright (c) 1997-2004 Dmitry Karasik
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

package Prima::EventHook;
use vars qw($hook $auto_hook %hooks %groups);

$auto_hook = 1;

%groups = (
	keyboard   => [qw(KeyDown KeyUp TranslateAccel)],
	mouse      => [qw(MouseDown MouseUp MouseMove MouseClick MouseEnter MouseLeave MouseWheel)],
	geometry   => [qw(Size Move ZOrderChanged)],
	objects    => [qw(ChangeOwner ChildEnter ChildLeave Create Destroy)],
	focus      => [qw(Leave Enter)],
	visibility => [qw(Hide Show)],
	ability    => [qw(Enable Disable)],
	menu       => [qw(Menu Popup)],
);

sub install
{
	my ( $sub, %rules) = @_;
	
	my @params;
	if ( defined($rules{param})) {
		@params = ( ref($rules{param}) eq 'ARRAY') ? @{$rules{param}} : $rules{param};
	}
	
	my @names;
	if ( defined($rules{event})) {
		@names = ( ref($rules{event}) eq 'ARRAY') ? @{$rules{event}} : $rules{event};
	} else {
		@names = ('*');
	}
	@names = map { exists($groups{$_}) ? @{$groups{$_}} : $_} @names;

	my @objects;
	if ( defined($rules{object})) {
		@objects = ( ref($rules{object}) eq 'ARRAY') ? @{$rules{object}} : $rules{object};
	} else {
		@objects = (undef);
	}
	
	for (@names) {
		$hooks{$_} = [] unless $hooks{$_};
		my $array = $hooks{$_};
		for ( @objects) {
			push @$array, [$sub, $_, $rules{children}, @params];
		}
	}

	Prima::Component-> event_hook( $hook = \&hook_proc) 
		if $auto_hook && !$hook;
}

sub deinstall
{
	my $sub = $_[0];
	my $total = 0;
	for ( keys %hooks) {
		@{$hooks{$_}} = grep { $$_[0] != $sub } @{$hooks{$_}};
		$total += @{$hooks{$_}};
	}
	Prima::Component-> event_hook( $hook = undef) 
		if !$total && $hook && $auto_hook;
}

sub hook_proc
{
	my ( $object, $event, @params) = @_;
	for ( '*', $event) {
		next unless exists $hooks{$_};
		for ( @{$hooks{$_}}) {
			my ( $sub, $sub_object, $sub_children, @sub_params) = @$_;
			next if 
				defined $sub_object &&
				(
					(  $sub_children && $sub_object-> is_owner( $object) == 0) ||
					( !$sub_children && $sub_object != $object)
				);
			return 0 unless $sub-> ( @sub_params, $object, $event, @params);
		}
	}
	return 1;
}

1;

__DATA__

=pod

=head1 NAME

Prima::EventHook - event filtering

=head1 SYNOPSIS

	use Prima::EventHook;

	sub hook
	{
		my ( $my_param, $object, $event, @params) = @_;
		...
		print "Object $object received event $event\n";
		...
		return 1;
	}

	Prima::EventHook::install( \&hook, 
		param    => $my_param,
		object   => $my_window, 
		event    => [qw(Size Move Destroy)],
		children => 1
	);

	Prima::EventHook::deinstall(\&hook);

=head1 DESCRIPTION

Prima dispatches events by calling notifications registered
on one or more objects interested in the events. Also, one 
event hook can be installed that would receive all events occurred on
all objects. C<Prima::EventHook> provides multiplex access to
the core event hook and introduces set of dispatching rules so 
the user hook subs receive only a defined subset of events.

The filtering criteria are event names and object hierarchy.

=head1 API

=head2 install SUB, %RULES

Installs SUB into hook list using hash of RULES.

The SUB is called with variable list of parameters, formed so first passed
parameters from C<'param'> key ( see below ), then event source object, then
event name, and finally parameters to the event. SUB must return an integer,
either 0 or 1, to block or pass the event, respectively.  If 1 is returned,
other hook subs are called; if 0 is returned, the event is efficiently blocked
and no hooks are further called.

Rules can contain the following keys:

=over

=item event

Event is either a string, an array of strings, or C<undef> value.  In the latter
case it is equal to C<'*'> string, which selects all events to be passed in the
SUB. A string is either name of an event, or one of pre-defined event groups, 
declared in C<%groups> package hash. The group names are:

	ability
	focus
	geometry
	keyboard
	menu
	mouse  
	objects
	visibility

These contain respective events. See source for detailed description.

In case C<'event'> key is an array of strings, each of the strings is
also name of either an event or a group. In this case, if C<'*'> string
or event duplicate names are present in the list, SUB is called several
times which is obviously inefficient.

=item object

A Prima object, or an array of Prima objects, or undef; the latter case
matches all objects. If an object is defined, the SUB is called
if event source is same as the object.

=item children

If 1, SUB is called using same rules as described in C<'object'>, but also if
the event source is a child of the object. Thus, selecting C<undef> as a filter
object and setting C<'children'> to 0 is almost the same as selecting
C<$::application>, which is the root of Prima object hierarchy, as filter
object with C<'children'> set to 1.

Setting together object to C<undef> and children to 1 is inefficient.

=item param

A scalar or array of scalars passed as first parameters to SUB 
whenever it is called.

=back

=head2 deinstall SUB

Removes the hook sub for the hook list.

=head1 NOTES

C<Prima::EventHook> by default automatically starts and stops Prima event hook
mechanism when appropriate. If it is not desired, for example for your own
event hook management, set C<$auto_hook> to 0.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Object>

=cut
