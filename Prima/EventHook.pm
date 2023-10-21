use strict;
use warnings;

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

	Prima::Component-> event_hook( $hook = \&_hook_proc)
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

sub _hook_proc
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

The toolkit dispatches notifications by calling subroutines registered on one
or more objects. Also, the core part of the toolkit allows a single event hook
callback to be installed that would receive all events occurring on all objects.
C<Prima::EventHook> provides multiplexed access to the core event hook and
introduces a set of dispatching rules so that the user hooks can receive only a
subset of events.

=head1 API

=head2 install SUB, %RULES

Installs SUB using a hash of RULES.

The SUB is called with a variable list of parameters, formed so that first come
parameters from the C<'param'> key ( see below ), then the event source object,
then the event name, and finally the parameters to the event. The SUB must return
an integer, either 0 or 1, to block or pass the event, respectively.  If 1 is
returned, other hook subs are called; if 0 is returned, the event is
efficiently blocked and no hooks are called further.

Rules can contain the following keys:

=over

=item event

An event is either a string, an array of strings, or an C<undef> value.  In the latter
case, it is equal to a C<'*'> string which selects all events to be passed to the
SUB. A string is either the name of an event or one of the pre-defined event groups,
declared in the C<%groups> package hash. The group names are:

	ability
	focus
	geometry
	keyboard
	menu
	mouse
	objects
	visibility

These contain the respective events. See the source for a detailed description.

In case the C<'event'> key is an array of strings, each of the strings is
also the name of either an event or a group. In this case, if the C<'*'> string
or event duplicate names are present in the list, SUB is called several
times.

=item object

A Prima object, or an array of Prima objects, or undef; in the latter case
matches all objects. If an object is defined, the SUB is called
if the event source is the same as the object.

=item children

If 1, SUB is called using the same rules as described in C<'object'>, but also if
the event source is a child of the object. Thus, selecting C<undef> as a filter
object and setting C<'children'> to 0 is almost the same as selecting
C<$::application>, which is the root of the Prima object hierarchy, as a filter
object with C<'children'> set to 1.

Setting C<object> to C<undef> and children to 1 is inefficient.

=item param

A scalar or array of scalars passed as first parameters to SUB

=back

=head2 deinstall SUB

Removes the hook sub

=head1 NOTES

C<Prima::EventHook> by default automatically starts and stops the Prima event
hook mechanism when appropriate. If it is not desired, for example for your own
event hook management, set C<$auto_hook> to 0.

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Object>

=cut
