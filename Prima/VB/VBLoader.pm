package Prima::VB::VBLoader;
use strict;
use warnings;
use Prima::Utils;
use vars qw($builderActive $fileVersion @eventContext $form);

$fileVersion   = '1.2';
@eventContext  = ('', '');

my %fileVerCompat = (
	'1'   => '1.0',
	'1.0' => '1.1',
	'1.1' => '1.2',
);

sub check_version
{
	my $header = $_[0];
	return (0, 'unknown') unless $header =~ /file=(\d+\.*\d*)/;
	my $fv = $1;
	while( $fv ne $fileVersion) {
		$fv = $fileVerCompat{$fv}, next if exists $fileVerCompat{$fv};
		return (0, $1);
	}
	return (1, $fv);
}

sub GO_SUB
{
	if ( $builderActive) {
		my $x = $_[0];
		$x =~ s/\n$//s;
		return $x;
	}
	my $x = eval "sub { $_[0] }";
	if ( $@) {
		@eventContext = ( $_[1], $_[2]);
		die $@;
	}
	return $x;
}

sub AUTOFORM_REALIZE
{
	my ( $seq, $parms) = @_;
	my %ret = ();
	my %modules = ();
	my $main;
	my $i;

	my %dep;
	for ( $i = 0; $i < scalar @$seq; $i+= 2) {
		$dep{$$seq[$i]} = $$seq[$i + 1];
	}

	for ( keys %dep) {
		$modules{$dep{$_}-> {module}} = 1 if $dep{$_}-> {module};
		$main = $_ if $dep{$_}-> {parent};
	}
	$form = $main;
	for ( keys %modules) {
		my $c = $_;
		eval("use $c;");
		die "$@" if $@;
	}

	my %owners = ( $main => 0);
	my %siblings;
	for ( keys %dep) {
		next if $_ eq $main;
		$owners{$_} = exists $parms-> {$_}-> {owner} ? $parms-> {$_}-> {owner} :
			( exists $dep{$_}-> {profile}-> {owner} ? $dep{$_}-> {profile}-> {owner} : $main);
		delete $dep{$_}-> {profile}-> {owner};
	}

	my @actNames  = qw( onBegin onFormCreate onCreate onChild onChildCreate onEnd);
	my %actions   = map { $_ => {}} @actNames;
	my %instances = ();
	for ( keys %dep) {
		my $key = $_;
		my $act = $dep{$_}-> {actions};
		$instances{$_} = {};
		$instances{$_}-> {extras} = $dep{$_}-> {extras} if $dep{$_}-> {extras};

		for ( @actNames) {
			next unless exists $act-> {$_};
			$actions{$_}-> {$key} = $act-> {$_};
		}
	}

	$actions{onBegin}-> {$_}-> ($_, $instances{$_})
		for keys %{$actions{onBegin}};

	for ( @{$dep{$main}-> {siblings}}) {
		if ( exists $dep{$main}-> {profile}-> {$_}) {
			$siblings{$main}-> {$_} = $dep{$main}-> {profile}-> {$_};
			delete $dep{$main}-> {profile}-> {$_};
		}
		if ( exists $parms-> {$main}-> {$_}) {
			$siblings{$main}-> {$_} = $parms-> {$main}-> {$_};
			delete $parms-> {$main}-> {$_};
		}
	}

	delete $dep{$main}-> {profile}-> {owner};

	$dep{$main}-> {code}-> () if defined $dep{$main}-> {code} ;

	$ret{$main} = $dep{$main}-> {class}-> create(
		%{$dep{$main}-> {profile}},
		%{$parms-> {$main}},
	);
	$ret{$main}-> lock;
	$actions{onFormCreate}-> {$_}-> ($_, $instances{$_}, $ret{$main})
		for keys %{$actions{onFormCreate}};
	$actions{onCreate}-> {$main}-> ($main, $instances{$_}, $ret{$main})
		if $actions{onCreate}-> {$main};

	my $do_layer;
	$do_layer = sub
	{
		my $id = $_[0];
		my $i;
		for ( $i = 0; $i < scalar @$seq; $i += 2) {
			my $name = $$seq[$i];
			next unless $owners{$name} eq $id;
			# validating owner entry
			$owners{$name} = $main unless exists $ret{$owners{$name}};

			my $o = $owners{$name};
			$actions{onChild}-> {$o}-> ($o, $instances{$o}, $ret{$o}, $name)
				if $actions{onChild}-> {$o};

			for ( @{$dep{$name}-> {siblings}}) {
				if ( exists $dep{$name}-> {profile}-> {$_}) {
					if ( exists $ret{$dep{$name}-> {profile}-> {$_}}) {
						$dep{$name}-> {profile}-> {$_} =
							$ret{$dep{$name}-> {profile}-> {$_}}
					} else {
						$siblings{$name}-> {$_} = $dep{$name}-> {profile}-> {$_};
						delete $dep{$name}-> {profile}-> {$_};
					}
				}
				if ( exists $parms-> {$name}-> {$_}) {
					if ( exists $ret{$parms-> {$name}-> {$_}}) {
						$parms-> {$name}-> {$_} = $ret{$parms-> {$name}-> {$_}}
					} else {
						$siblings{$name}-> {$_} = $parms-> {$name}-> {$_};
						delete $parms-> {$name}-> {$_};
					}
				}
			}
			$ret{$name} = $ret{$o}-> insert(
				$dep{$name}-> {class},
				%{$dep{$name}-> {profile}},
				%{$parms-> {$name}},
			);

			$actions{onCreate}-> {$name}-> ($name, $instances{$name}, $ret{$name})
				if $actions{onCreate}-> {$name};
			$actions{onChildCreate}-> {$o}-> ($o, $instances{$o}, $ret{$o}, $ret{$name})
				if $actions{onChildCreate}-> {$o};

			$do_layer-> ( $name);
		}
	};
	$do_layer-> ( $main, \%owners);

	for ( keys %siblings) {
		my $data = $siblings{$_};
		$ret{$_}-> set( map {
			exists($ret{$data-> {$_}}) ? ( $_ => $ret{$data-> {$_}}) : ()
		} keys %$data );
	}

	$actions{onEnd}-> {$_}-> ($_, $instances{$_}, $ret{$_})
		for keys %{$actions{onEnd}};
	$ret{$main}-> unlock;

	return %ret;
}

sub AUTOFORM_CREATE
{
	my ( $filename, %parms) = @_;
	my $contents;
	my @preload_modules;
	{
		open F, $filename or die "Cannot open $filename:$!\n";
		my $first = <F>;
		die "Corrupted file $filename\n" unless $first =~ /^# VBForm/;
		my @fvc = check_version( $first);
		die "Incompatible version ($fvc[1]) of file $filename\n" unless $fvc[0];
		while (<F>) {
			$contents = $_, last unless /^#/;
			next unless /^#\s*\[([^\]]+)\](.*)$/;
			if ( $1 eq 'preload') {
				push( @preload_modules, split( ' ', $2));
			}
		}
		local $/;
		$contents .= <F>;
		close F;
	}

	for ( @preload_modules) {
		eval "use $_;";
		die "$@\n" if $@;
	}

	my $sub = eval( $contents);
	die "$@\n" if $@;
	my @dep = $sub-> ();
	return AUTOFORM_REALIZE( \@dep, \%parms);
}

package Prima;
use strict;

sub VBLoad
{
	my ( $filename, %parms) = @_;

	if ( $filename =~ /.+\:\:([^\:]+)$/ && $filename !~ /^</ ) { # seemingly a module syntax
		my @path = split( '::', $filename);
		my $file = pop @path;
		my $ret = Prima::Utils::find_image( join('::', @path), $file);
		$@ = "Cannot find resource: $filename", return undef unless $ret;
		$filename = $ret;
	} else {
		$filename =~ s/^<//;
	}

	my %ret;
	eval { %ret = Prima::VB::VBLoader::AUTOFORM_CREATE( $filename, %parms); };
	if ( $@ ) {
		$@ = "Error in setup resource: $@";
		return undef;
	}
	unless ( $ret{$Prima::VB::VBLoader::form} ) {
		$@ = "Error in setup resource: form not found";
		return undef;
	}
	return $ret{$Prima::VB::VBLoader::form};
}

# onBegin       ( name, instance)
# onFormCreate  ( name, instance, formObject)
# onCreate      ( name, instance, object)
# onChild       ( name, instance, object, childName)
# onChildCreate ( name, instance, object, childObject)
# onEnd         ( name, instance, object)

1;

=pod

=head1 NAME

Prima::VB::VBLoader - Visual Builder file loader

=head1 DESCRIPTION

The module provides functionality for loading resource files
created by Visual Builder. After a successful load, the newly created
window with all children is returned.

=head1 SYNOPSIS

A simple way to use the loader is as follows:

	use Prima qw(Application VB::VBLoader);
	my $form = Prima::VBLoad( './your_resource.fm',
		Form1 => { centered => 1 },
	);
	die $@ unless $form;
	$form-> execute;

All form widgets can be supplied with custom parameters, combined
in a hash of hashes, and passed as the second parameter to the C<VBLoad()> function.
The example above supplies values for the C<::centered> property to
the C<Form1> widget, which is the default name for a form window created by Visual
Builder. All other widgets are accessible by their names in a similar fashion;
after the creation, the widget hierarchy can be accessed in the standard way:

	$form = Prima::VBLoad( $fi,
		....
		StartButton => {
			onMouseOver => sub { die "No start buttons here\n" },
		}
	);
	...
	$form-> StartButton-> hide;

In case a form is to be included not from another data source, the
L<AUTOFORM_REALIZE> method can be used to transform the array of hashes that
describes the form widget hierarchy into a set of widgets:

	$form = AUTOFORM_REALIZE( [ Form1 => {
		class   => 'Prima::Window',
		parent  => 1,
		profile => {
			name => 'Form1',
			size => [ 330, 421],
		}], {});

There are several examples of how the form files are used in the toolkit; for
instance, the F<Prima/PS/setup.fm> dialog is used by the C<Prima::PS::Setup>
package.

=head1 API

=head2 Methods

=over

=item check_version HEADER

Scans HEADER that the first line of the .fm file for the version info.
Returns two scalars - the first is a boolean flag, which is set
to 1 if the file can be used and loaded, 0 otherwise. The second
scalar is the version string.

=item GO_SUB SUB [ @EXTRA_DATA ]

Depending on the value of the boolean flag
C<Prima::VB::VBLoader::builderActive> performs the following: if it is 1, the
SUB text is returned as is.  If it is 0, evaluates it in the C<sub{}> context
and returns the code reference.  If the evaluation fails, EXTRA_DATA is stored
in the C<Prima::VB::VBLoader::eventContext> array and the exception is
re-thrown.  C<Prima::VB::VBLoader::builderActive> is an internal flag that
helps the Visual Builder to use the module interface without actual evaluation
of the SUB code.

=item AUTOFORM_REALIZE WIDGETS, PARAMETERS

WIDGETS is an array reference which contains evaluated data of the content of
a .fm file, assuming its format is preserved.  PARAMETERS is a hash reference
with custom parameters passed to widgets during the creation. The widgets are
distinguished by their names.  Visual Builder ensures that no widgets have
equal names.

C<AUTOFORM_REALIZE> creates a tree of widgets and returns the root
window which is usually named C<Form1>. It automatically resolves
parent-child relations, so the order in which WIDGETS are specified does not matter.
Moreover, if a parent widget is passed as a parameter to
a child widget, the parameter is deferred and passed only after
the creation using the C<::set> call.

During the parsing and the creation process, some internal notifications may be invoked.
These notifications (events) are stored in the .fm file and usually provide
class-specific loading instructions. See L<Events> for details.

=item AUTOFORM_CREATE FILENAME, %PARAMETERS

Reads FILENAME in the .fm file format, checks its version, loads,
and creates a widget tree. Upon successful load, the root widget
is returned. The parsing and creation are performed by calling
C<AUTOFORM_REALIZE>. If loading fails, C<die()> is called.

=item Prima::VBLoad FILENAME, %PARAMETERS

A wrapper around C<AUTOFORM_CREATE> is exported in the C<Prima>
namespace. FILENAME can be specified either as a file system
path name or as a relative module name. In a way,

	Prima::VBLoad( 'Module::form.fm' )

and

	Prima::VBLoad(
		Prima::Utils::find_image( 'Module', 'form.fm'))

are identical. If the procedure finds that FILENAME is a relative
module name it calls C<Prima::Utils::find_image> automatically. To
tell explicitly that FILENAME is a file system path name, FILENAME
must be prefixed with the C<E<lt>> symbol ( the syntax is influenced by C<CORE::open> ).

%PARAMETERS is a hash with custom parameters passed to
the widgets during the creation. The widgets are distinguished by their names.
Visual Builder ensures that no widgets have equal names.

If the form file loaded successfully returns the form object reference.
Otherwise C<undef> is returned and the error string is stored in the C<$@>
variable.

=back

=head2 Events

The events stored in the .fm file are executed during the loading process.  The
module provides no functionality for supplying extra events during the load. This
interface is useful only for developers of new widget classes for Visual Builder.

The events section is located in the C<actions> section of the widget entry.  There can
be more than one event of each type, registered to different widgets.  NAME
parameter is a string with the name of the widget. INSTANCE is a hash created
during load for every widget, and is provided to keep internal event-specific or
class-specific data there. The C<extras> section of the widget entry is present there
as the place to store local data.

=over

=item Begin NAME, INSTANCE

Called before the creation of a widget tree.

=item FormCreate NAME, INSTANCE, ROOT_WIDGET

Called after the creation of the form, which reference is
contained in ROOT_WIDGET.

=item Create NAME, INSTANCE, WIDGET.

Called after the creation of a widget. The newly created
widget is passed in WIDGET

=item Child NAME, INSTANCE, WIDGET, CHILD_NAME

Called before a child of WIDGET is created with CHILD_NAME as a name.

=item ChildCreate NAME, INSTANCE, WIDGET, CHILD_WIDGET.

Called after a child of WIDGET is created; the newly created
widget is passed in CHILD_WIDGET.

=item End NAME, INSTANCE, WIDGET

Called after the creation of all widgets is finished.

=back

=head1 FILE FORMAT

The idea of the format of the .fm file is that it should be evaluated by the
perl C<eval()> call without special manipulations and kept as plain text.  The
file starts with a header which is a #-prefixed string, and contains a
signature, the version of file format, and the version of the creator of the file:

	# VBForm version file=1 builder=0.1

The header can also contain additional headers, also prefixed with #.  These
can be used to tell the loader that another perl module is needed to be loaded
before the parsing; this is useful, for example, if a constant is declared in
the module.

	# [preload] Prima::ComboBox

The main part of a file is enclosed in a C<sub{}> statement.  After evaluation,
this sub returns an array of paired scalars where each first item is a widget
name and the second item is a hash of its parameters and other associated data:

	sub
	{
		return (
			'Form1' => {
				class   => 'Prima::Window',
				module  => 'Prima::Classes',
				parent => 1,
				code   => GO_SUB('init()'),
				profile => {
					width => 144,
					name => 'Form1',
					origin => [ 490, 412],
					size => [ 144, 100],
			}},
		);
	}

The hash has several predefined keys:

=over

=item actions HASH

Contains a hash of events. The events are evaluated via the C<GO_SUB> mechanism
and executed during the creation of the widget tree. See L<Events> for details.

=item code STRING

Contains the code executed before the form is created.
This key is present only on the root widget record.

=item class STRING

Contains the name of the class to be instantiated.

=item extras HASH

Contains class-specific parameters used by the events.

=item module STRING

Contains the name of the perl module that contains the class. The module
will be C<use>'d by the loader.

=item parent BOOLEAN

A boolean flag; is set to 1 for the root widget only.

=item profile HASH

Contains the profile hash passed as a set of parameters to the widget
during its creation. If custom parameters are passed to
C<AUTOFORM_CREATE>, these are merged with C<profile>
( the custom parameters take precedence ) before passing
the result to the C<new()> call.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<VB>

=cut
