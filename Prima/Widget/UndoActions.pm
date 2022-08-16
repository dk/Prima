package Prima::Widget::UndoActions;

use strict;
use warnings;

sub init_undo
{
	my ($self, $profile) = @_;
	$self-> {undo} = [];
	$self-> {redo} = [];
	$self-> {undoLimit} = $profile->{undoLimit};
}

sub begin_undo_group
{
	my $self = $_[0];
	return if !$self-> {undoLimit};
	if ( $self-> {undo_in_action}) {
		push @{$self-> {redo}}, [] unless $self-> {grouped_undo}++;
	} else {
		push @{$self-> {undo}}, [] unless $self-> {grouped_undo}++;
		$self-> {redo} = [] if !$self-> {redo_in_action};
	}
}

sub end_undo_group
{
	my $self = $_[0];
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	$self-> {grouped_undo}-- if $self-> {grouped_undo} > 0;
	# skip last record if empty
	pop @{$self-> {$ref}}
		if !$self-> {grouped_undo} &&
			@{$self-> {$ref}} &&
			0 == @{$self-> {$ref}-> [-1]};
	shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
}

sub push_undo_action
{
	my $self = shift;
	return if !$self-> {undoLimit};

	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	my $action = [ @_ ];
	if ( $self-> {grouped_undo}) {
		push @{$self-> {$ref}}, [] unless @{$self-> {$ref} // []};
		push @{$self-> {$ref}-> [-1]}, $action;
	} else {
		push @{$self-> {$ref}}, [ $action ];
		shift @{$self-> {$ref}} if @{$self-> {$ref}} > $self-> {undoLimit};
		$self-> {redo} = []
			if !$self-> {redo_in_action} && !$self-> {undo_in_action};
	}
}

sub has_undo_action
{
	my ($self, $method) = @_;
	my $has = 0;
	if ( !$self-> {undo_in_action} && @{$self-> {undo} // []} && @{$self-> {undo}-> [-1]}) {
		my $ok = 1;
		for ( @{$self-> {undo}-> [-1]}) {
			$ok = 0, last if $$_[0] ne $method;
		}
		$has = 1 if $ok;
	}
	return $has;
}

sub push_group_undo_action
{
	my $self = shift;
	return if !$self-> {undoLimit};
	my $ref = $self-> {undo_in_action} ? 'redo' : 'undo';
	return $self-> push_undo_action(@_) if $self-> {grouped_undo};

	push @{$self-> {$ref}}, [] unless @{$self-> {$ref}};
	$self-> {grouped_undo} = 1;
	$self-> push_undo_action(@_);
	$self-> {grouped_undo} = 0;
}

sub can_undo { scalar shift @{ shift->{undo} } }
sub can_redo { scalar shift @{ shift->{redo} } }

sub undo
{
	my $self = $_[0];
	return if $self-> {undo_in_action} || !$self-> {undoLimit};
	return unless @{$self-> {undo}};
	my $group = pop @{$self-> {undo}};
	return unless $group && @$group;

	$self-> {undo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {undo_in_action} = 0;
}

sub redo
{
	my $self = $_[0];
	return if !$self-> {undoLimit};
	return unless @{$self-> {redo}};
	my $group = pop @{$self-> {redo}};
	return unless $group && @$group;

	$self-> {redo_in_action} = 1;
	$self-> begin_undo_group;
	for ( reverse @$group) {
		my ( $method, @params) = @$_;
		next unless $self-> can($method);
		$self-> $method( @params);
	}
	$self-> end_undo_group;
	$self-> {redo_in_action} = 0;
}

sub undoLimit
{
	return $_[0]-> {undoLimit} unless $#_;

	my ( $self, $ul) = @_;
	$self-> {undoLimit} = $ul if $ul >= 0;
	splice @{$self-> {undo}}, 0, $ul - @{$self-> {undo}} if @{$self-> {undo}} > $ul;
}

1;

=head1 NAME

Prima::Widget::UndoActions - undo and redo the content of editable widgets

=head1 Properties

=over

=item undoLimit INTEGER

Sets limit on number of stored atomic undo operations. If 0,
undo is disabled.

=back

=head2 Methods

=over

=item begin_undo_group

Opens bracket for group of actions, undone as single operation.
The bracket is closed by calling C<end_undo_group>.

=item end_undo_group

Closes bracket for group of actions, opened by C<begin_undo_group>.

=item redo

Re-applies changes, formerly rolled back by C<undo>.

=item undo

Rolls back changes into internal array, which size cannot extend C<undoLimit>
value. In case C<undoLimit> is 0, no undo actions can be made.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Edit>,

=cut
