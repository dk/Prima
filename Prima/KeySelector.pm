#  Contains:
#       Prima::KeySelector
#  Provides:
#       Control set for assigning and exporting keys

package Prima::KeySelector;

use strict;
use warnings;
use Prima qw(Buttons Label ComboBox DetailedOutline MsgBox);

use vars qw(@ISA %vkeys);
@ISA = qw(Prima::Widget);

for ( keys %kb::) {
	next if m/^(constant|AUTOLOAD|CharMask|CodeMask|ModMask|[LR]\d+)$/;
	$vkeys{$_} = &{$kb::{$_}}();
}

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		key                => kb::NoKey,
		scaleChildren      => 0,
		autoEnableChildren => 1,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);
	my $fh = $self-> font-> height;


	$self-> insert( [ComboBox =>
		name        => 'Keys',
		delegations => [qw(Change)],
		pack        => { side => 'top', fill => 'x'},
		style    => cs::DropDownList,
		items    => [ sort keys %vkeys, 'A'..'Z', '0'..'9', '+', '-', '*'],
	], [ GroupBox =>
		pack        => { side => 'top', fill => 'x' },
		style    => cs::DropDown,
		text     => '',
		name     => 'GB',
	], [ Label =>
		pack        => { side => 'top', fill => 'x'},
		text       => '~Press shortcut key:',
		focusLink  => 'Hook',
	], [ Widget =>
		name          => 'Hook',
		delegations   => [qw(Paint KeyDown TranslateAccel )],
		pack          => { side => 'top', fill => 'x'},
		height        => $fh + 2,
		selectable    => 1,
		cursorPos     => [ 4, 2],
		cursorSize    => [ $::application-> get_default_cursor_width, $fh - 2],
		cursorVisible => 1,
		tabStop       => 1,
	] );

	$self-> GB-> insert( [ Widget =>
		name        => 'dummy1',
		height      => 0,
		visible     => 0,
		pack        => { side => 'top', fill => 'x', pady => (( $fh < 10) ? 5 : ($fh - 5)) },
	], [ CheckBox =>
		name        => 'Shift',
		delegations => [$self, qw(Click)],
		pack        => { side => 'top', fill => 'x', padx => 15 },
		text        => '~Shift',
	], [ CheckBox =>
		name        => 'Ctrl',
		delegations => [$self, qw(Click)],
		pack        => { side => 'top', fill => 'x', padx => 15 },
		text        => '~Ctrl',
	], [ CheckBox =>
		name        => 'Alt',
		delegations => [$self, qw(Click)],
		pack        => { side => 'top', fill => 'x', padx => 15},
		text        => '~Alt',
 	], [ Widget =>
		name        => 'dummy2',
		height      => 0,
		visible     => 0,
		pack        => { side => 'top', fill => 'x', pad => 5 },
	] );

	$self-> key( $profile{key});
	return %profile;
}

sub Keys_Change  { $_[0]-> _gather; }
sub Shift_Click  { $_[0]-> _gather; }
sub Ctrl_Click   { $_[0]-> _gather; }
sub Alt_Click    { $_[0]-> _gather; }

sub _gather
{
	my $self = $_[0];
	return if $self-> {blockChange};

	my $mod = ( $self-> GB-> Alt-> checked ? km::Alt : 0) |
			( $self-> GB-> Ctrl-> checked ? km::Ctrl : 0) |
			( $self-> GB-> Shift-> checked ? km::Shift : 0);
	my $tx = $self-> Keys-> text;
	my $vk = exists $vkeys{$tx} ? $vkeys{$tx} : kb::NoKey;
	my $ck;
	if ( exists $vkeys{$tx}) {
		$ck = 0;
	} elsif (( $mod & km::Ctrl) && ( ord($tx) >= ord('A')) && ( ord($tx) <= ord('z'))) {
		$ck = ord( uc $tx) - ord('@');
	} else {
		$ck = ord( $tx);
	}
	$self-> {key} = Prima::AbstractMenu-> translate_key( $ck, $vk, $mod);
	$self-> notify(q(Change));
}

sub Hook_KeyDown
{
	my ( $self, $hook, $code, $key, $mod) = @_;
	$self-> key( Prima::AbstractMenu-> translate_key( $code, $key, $mod));
}

sub Hook_TranslateAccel
{
	my ( $self, $hook, $code, $key, $mod) = @_;
	return unless $hook-> focused;
	$hook-> clear_event unless $key == kb::Tab || $key == kb::BackTab;
}

sub Hook_Paint
{
	my ( $self, $hook, $canvas) = @_;
	$canvas-> rect3d( 0, 0, $canvas-> width - 1, $canvas-> height - 1, 1,
		$hook-> dark3DColor, $hook-> light3DColor, $hook-> backColor);
	$canvas-> text_out( describe($self->key), 2, 2);
}

sub translate_codes
{
	my ( $data, $useCTRL) = @_;
	my ( $code, $key, $mod);
	if ((( $data & 0xFF) >= ord('A')) && (( $data & 0xFF) <= ord('z'))) {
		$code = $data & 0xFF;
		$key  = kb::NoKey;
	} elsif ((( $data & 0xFF) >= 1) && (( $data & 0xFF) <= 26)) {
		$code  = $useCTRL ? ( $data & 0xFF) : ord( lc chr(ord( '@') + $data & 0xFF));
		$key   = kb::NoKey;
		$data |= km::Ctrl;
	} elsif ( $data & 0xFF) {
		$code = $data & 0xFF;
		$key  = kb::NoKey;
	} else {
		$code = 0;
		$key  = $data & kb::CodeMask;
	}
	$mod = $data & kb::ModMask;
	return $code, $key, $mod;
}

sub key
{
	return $_[0]-> {key} unless $#_;
	my ( $self, $data) = @_;
	my ( $code, $key, $mod) = translate_codes( $data, 0);

	if ( $code) {
		$self-> Keys-> text( chr($code));
	} else {
		my $x = 'NoKey';
		for ( keys %vkeys) {
			next if $_ eq 'constant';
			$x = $_, last if $key == $vkeys{$_};
		}
		$self-> Keys-> text( $x);
	}
	$self-> {key} = $data;
	$self-> {blockChange} = 1;
	$self-> GB-> Alt-> checked( $mod & km::Alt);
	$self-> GB-> Ctrl-> checked( $mod & km::Ctrl);
	$self-> GB-> Shift-> checked( $mod & km::Shift);
	$self-> Hook-> repaint;
	delete $self-> {blockChange};
	$self-> notify(q(Change));
}

# static functions

# exports binary value to a reasonable and perl-evaluable expression

sub export
{
	my $data = $_[0];
	my ( $code, $key, $mod) = translate_codes( $data, 1);
	my $txt = '';
	if ( $code) {
		if (( $code >= 1) && ($code <= 26)) {
			$code += ord('@');
			$txt = '(ord(\''.uc chr($code).'\')-64)';
		} else {
			$txt = 'ord(\''.lc chr($code).'\')';
		}
	} else {
		my $x = 'NoKey';
		for ( keys %vkeys) {
			next if $_ eq 'constant';
			$x = $_, last if $vkeys{$_} == $key;
		}
		$txt = 'kb::'.$x;
	}
	$txt .= '|km::Alt' if $mod & km::Alt;
	$txt .= '|km::Ctrl' if $mod & km::Ctrl;
	$txt .= '|km::Shift' if $mod & km::Shift;
	return $txt;
}

# creates a key description, suitable for a menu accelerator text

sub describe
{
	my $data = $_[0];
	my ( $code, $key, $mod) = translate_codes( $data, 0);
	my $txt = '';
	my $lonekey;
	if ( $code) {
		$txt = uc chr $code;
	} elsif ( $key == kb::NoKey) {
		$lonekey = 1;
	} else {
		for ( keys %vkeys) {
			next if $_ eq 'constant';
			$txt = $_, last if $vkeys{$_} == $key;
		}
	}
	$txt = 'Shift+' . $txt if $mod & km::Shift;
	$txt = 'Alt+'   . $txt if $mod & km::Alt;
	$txt = 'Ctrl+'  . $txt if $mod & km::Ctrl;
	$txt =~ s/\+$// if $lonekey;
	return $txt;
}

# exports binary value to AbstractMenu-> translate_shortcut input

sub shortcut
{
	my $data = $_[0];
	my ( $code, $key, $mod) = translate_codes( $data, 0);
	my $txt = '';

	if ( $code || (( $key >= kb::F1) && ( $key <= kb::F30))) {
		$txt = $code ?
			( uc chr $code) :
			( 'F' . (( $key - kb::F1) / ( kb::F2 - kb::F1) + 1));
		$txt = '^' . $txt if $mod & km::Ctrl;
		$txt = '@' . $txt if $mod & km::Alt;
		$txt = '#' . $txt if $mod & km::Shift;
	} else {
		return export( $data);
	}
	return "'" . $txt . "'";
}

# safe eval for key description produced by shortcut

sub eval_shortcut
{
	my $text = shift;

	return undef unless defined $text;
	return $text if $text =~ /^(\d+)$/;

	$text =~ s/^'(.*)'$/$1/;
	my $mod = 0;
	while ( $text =~ s/\|km::(\w+)// ) {
		$mod |= km::Alt   if $1 eq 'Alt';
		$mod |= km::Ctrl  if $1 eq 'Ctrl';
		$mod |= km::Shift if $1 eq 'Shift';
	}

	if ( $text =~ s/^kb::(\w+)// ) {
		my $vk = $vkeys{$1};
		return $vk | $mod;
	} elsif ($text =~ m/^ord\(\'(.)\'\)/) {
		return ord($1) | $mod;
	} elsif ($text =~ m/^\(ord\(\'(.)\'\)\s*\-(\d+)\)/) {
		return (ord($1) - $2) | $mod;
	} else {
		my $v = Prima::AbstractMenu-> translate_shortcut( $text );
		return ($v == kb::NoKey) ? undef : $v;
	}
}

sub apply_to_menu
{
	my ($menu, $vkeys) = @_;
	while ( my ( $id, $value ) = each %$vkeys) {
		$menu-> key( $id, $value);
		$menu-> accel( $id, ($value == kb::NoKey) ?  '' : describe( $value));
	}
}

package Prima::KeySelector::MenuEditor;
use vars qw(@ISA);
@ISA = qw(Prima::Widget);

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},
		menu               => undef,
		applyButton        => 1,
		scaleChildren      => 0,
		autoEnableChildren => 1,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init( @_);

	$self->{$_} = $profile{$_} for qw(menu);
	$self-> {vkeys} = {};
	my $items = $self-> {menu} ? $self-> menu_to_items( $self-> {menu} ) : [];

	$self-> insert( DetailedOutline  =>
		pack        => { side => 'left', fill => 'both', expand => 1, pad => 10 },
		name        => 'KeyList',
		items       => $items,
		delegations => [ qw(SelectItem) ],
		columns     => 2,
		headers     => ['Name', 'Shortcut'],
		autoRecalc  => 1,
		width       => 300,
	);

	$self-> insert( [ Button  =>
		pack    => { side => 'top', pad => 15 },
		text    => 'Appl~y',
		hint    => 'Apply changes',
		name    => 'Apply',
		delegations => [ qw(Click) ],
	] ) if $profile{applyButton};

	$self-> insert( [ Button  =>
		pack    => { side => 'top', pad => 15 },
		text    => '~Restore all',
		hint    => 'Restore all shortcuts to defaults',
		name    => 'Restore',
		delegations => [ qw(Click) ],
	] );


	$self-> insert( [ KeySelector =>
		pack    => { side => 'top', pad => 10 },
		name   => 'KeySelector',
		visible => 0,
		delegations => [ qw(Change) ],
	], [ Button =>
		pack    => { side => 'top', pad => 15 },
		text    => '~Clear',
		hint    => 'Clears the key',
		name    => 'Clear',
		delegations => [ qw(Click) ],
	] , [ Button =>
		pack    => { side => 'top', pad => 15 },
		text    => '~Default',
		hint    => 'Set default value for a key',
		name    => 'Default',
		delegations => [ qw(Click) ],
	] );
	$self-> KeyList-> focusedItem(0);
	return %profile;
}

sub menu_to_items
{
	my ( $self, $menu ) = @_;
	my ($i, $ptr, $tree, @stack) = (0, $menu-> get_items(''), []);

	while ( 1) {
		for ( ; $i < @$ptr; $i++) {
			my ( $id, $text, $accel, $vkey, $ref_or_sub) = @{ $ptr->[$i] };
			$id =~ s/^[\*\-\@\(\)\?]*//;
			if ( ref($ref_or_sub // '') eq 'ARRAY') {
				push @stack, [ $i + 1, $ptr, $tree ];
				$ptr = $ref_or_sub;
				$i = -1;
				$text =~ s/~//;
				my $subtree = [];
				push @$tree, [[ $text, '', '', $id ], $subtree, 1];
				$tree = $subtree;
			} elsif (defined $text) {
				$text =~ s/~//;
				push @$tree, [[ $text, Prima::KeySelector::describe($vkey), $id, $id ]]
					unless $id =~ /^\#\d+$/; # don't do autogenerated items
			}
		}
		@stack ? ( $i, $ptr, $tree ) = @{ pop @stack } : last;
	}

	return $tree;
}

sub get_focused_id
{
	my $self = shift;
	my $kl = $self-> KeyList;
	my ( $item ) = $kl-> get_item( $kl-> focusedItem);
	return unless $item;
	my ( undef, undef, undef, $id ) = @{ $item->[0] };
	return $id;
}

sub update_focused_item
{
	my ($self, $mark_changed) = @_;
	my $kl = $self-> KeyList;
	my ( $item ) = $kl-> get_item( $kl-> focusedItem);
	return unless $item;
	my ( undef, undef, undef, $id ) = @{ $item->[0] };
	return unless defined $id;

	my $vkey = $self-> get_vkey( $id );
	return unless defined $vkey;

	$self-> KeySelector-> key( $vkey );
	$item-> [0]-> [1] = ($mark_changed ? '*' : '') . Prima::KeySelector::describe( $vkey );
	$kl-> redraw_items( $kl-> focusedItem );
	$self-> notify(q(Change));
}

sub menu
{
	return shift-> {menu} unless $#_;
	my ( $self, $menu ) = @_;
	$self-> {menu} = $menu;
	$self-> reset;
}

sub vkeys { shift-> {vkeys} }

sub reset
{
	my $self = shift;
	%{ $self->{vkeys} } = ();
	$self-> KeyList->items( $self-> menu_to_items( $self-> menu ) );
	$self-> update_focused_item;
}

sub reset_to_defaults
{
	my $self = shift;
	%{ $self->{vkeys} } = %{ $self-> menu-> keys_defaults };
	$self-> KeyList-> iterate( sub {
		my $item = shift;
		$item->[0]->[1] = Prima::KeySelector::describe( $self->{vkeys}-> {$item->[0]->[3]})
			unless $item->[1];
		return 0;
	} );
	$self-> KeyList->repaint;
	$self-> update_focused_item;
}

sub get_vkey
{
	my ( $self, $id ) = @_;
	return $self->{vkeys}->{$id} // $self->menu->key($id);
}

sub KeyList_SelectItem
{
	my ( $self, $me, $foc ) = @_;
	my ( $item, $lev) = $me-> get_item( $foc->[0]->[0]);
	return unless $item;
	my ( $text, undef, undef, $id ) = @{ $item->[0] };
	$self-> {keyMappings_change} = 1;
	unless ( ref($item-> [1])) {
		$self-> KeySelector-> enabled(1);
		$self-> KeySelector-> key( $self-> get_vkey($id));
		$self-> KeySelector-> show;
		$self-> Clear-> show;
		$self-> Default-> show;
	} else {
		$self-> Clear-> hide;
		$self-> Default-> hide;
		$self-> KeySelector-> hide;
		$self-> KeySelector-> enabled(0);
	}
	delete $self-> {keyMappings_change};
}

sub KeySelector_Change
{
	my ( $self, $me ) = @_;
	return if $self-> {keyMappings_change};
	my $kl = $self-> KeyList;
	my ( $item, $lev) = $kl-> get_item( $kl-> focusedItem);
	return unless $item;

	my ( $text, undef, undef, $id ) = @{ $item->[0] };
	my $value = $me-> key;
	if ( $value != kb::NoKey) {
		my $d = $self-> menu-> keys_defaults;
		for my $k ( keys %$d) {
			next if $k eq $id;
			next unless $value == $self-> get_vkey($k);
			my $menutext = $self-> menu-> text( $k ) // '';
			$menutext =~ s/~//;
			if ( Prima::MsgBox::message_box(
				$::application-> name,
				"This key combination is already occupied by $k. Apply anyway?",
				mb::YesNo) == mb::Yes) {
				$self->{vkeys}->{$k} = kb::NoKey;
				$self-> KeyList-> iterate( sub {
					my ($node, undef, undef, $index) = @_;
					return unless $node->[0]->[3] eq $k;
					$node->[0]->[1] = '';
					$self-> KeyList-> redraw_items($index);
					return 1;
				} );
				last;
			} else {
				local $self-> {keyMappings_change} = 1;
				$me-> key( $self-> get_vkey( $id ));
				return;
			}
		}
	}
	$self-> {vkeys}-> {$id} = $value;
	local $self-> {keyMappings_change} = 1;
	$self-> update_focused_item(1);
}

sub Clear_Click
{
	my $self = shift;
	my $id = $self-> get_focused_id;
	return unless defined $id;
	$self-> {vkeys}->{$id} = kb::NoKey;
	$self-> update_focused_item(1);
}

sub Default_Click
{
	my $self = shift;
	my $id = $self-> get_focused_id;
	return unless defined $id;
	$self-> {vkeys}->{$id} = $self-> menu-> keys_defaults-> {$id};
	$self-> update_focused_item;
}

sub Restore_Click
{
	my $self = shift;
	$self-> reset_to_defaults;
}

sub apply
{
	my $self = shift;
	Prima::KeySelector::apply_to_menu( $self-> menu, $self-> vkeys );
	$self-> KeyList-> iterate( sub { shift-> [0]-> [1] =~ s/^\*//; 0 } );
	$self-> KeyList-> repaint;
}

sub Apply_Click { shift-> apply }

package
	Prima::AbstractMenu;

sub _parse_menu_items
{
	my $items = shift;
	my %vkeys;
	my ($i, $ptr, $tree, @stack) = (0, $items, []);
	while ( 1) {
		for ( ; $i < @$ptr; $i++) {
			my ( $id, $text, $accel, $vkey, $ref_or_sub) = @{ $ptr->[$i] };
			$id =~ s/^[\*\-\@\(\)\?]*//;
			if ( ref($ref_or_sub // '') eq 'ARRAY') {
				push @stack, [ $i + 1, $ptr ];
				$ptr = $ref_or_sub;
				$i = -1;
			} elsif ( defined $text && $id !~ /^#/) { # kindly saves you from hell when your menu layout will change,
			                                          # but config file stays the same
				$text =~ s/~//;
				$vkeys{$id} = $vkey;
			}
		}
		@stack ? ( $i, $ptr ) = @{ pop @stack } : last;
	}

	return \%vkeys;
}

sub _init_keys
{
	my $self = shift;
	return $self-> {_key_loader} //= {
		defaults => _parse_menu_items( $self-> get_items('') ),
	};
}

sub keys_load
{
	my ($self, $ini, $all_items) = @_;
	my $k = _init_keys($self);

	my %v;
	if ( $all_items ) {
		for my $id ( keys %{ $k-> {defaults} } ) {
			my $value = Prima::KeySelector::eval_shortcut( $ini->{ $id } // $k-> {defaults}->{$id} );
			$v{$id} = $value if defined $value;
		}
	} else {
		for my $id ( keys %$ini ) {
			next unless exists $k-> {defaults}-> {$id};
			my $value = Prima::KeySelector::eval_shortcut( $ini->{ $id } );
			$v{$id} = $value if defined($value) && $value != $k-> {defaults}->{$id};
		}
	}
	Prima::KeySelector::apply_to_menu( $self, \%v);
}

sub keys_save
{
	my ($self, $ini, $all_items) = @_;
	my $k = _init_keys($self);

	my $vkeys = _parse_menu_items( $self-> get_items('') );
	for my $id ( keys %{ $k->{ defaults } } ) {
		my $value = $vkeys->{ $id } // $k-> {defaults}-> {$id};
		if (!$all_items && $value == $k-> {defaults}-> {$id}) {
			delete $ini->{$id};
		} else {
			$ini->{$id} = Prima::KeySelector::shortcut( $value );
		}
	}
}

sub keys_reset
{
	my ($self) = @_;
	my $k = _init_keys($self);
	Prima::KeySelector::apply_to_menu( $self, $k->{defaults});
}

sub keys_defaults
{
	my ($self) = @_;
	my $k = _init_keys($self);
	return $k-> {defaults};
}

package Prima::KeySelector::Dialog;
use vars qw( @ISA );
@ISA = qw( Prima::Dialog );

sub profile_default
{
	return {
		%{$_[ 0]-> SUPER::profile_default},

		borderStyle   => bs::Sizeable,
		centered      => 1,
		visible       => 0,
		text          => 'Edit shortcuts',

		menuTree      => undef,
	}
}

sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);

	my $me = $self-> insert( 'Prima::KeySelector::MenuEditor' =>
		pack        => { expand => 1, fill => 'both' },
		menu        => $profile{menuTree},
		name        => 'Editor',
		applyButton => 0,
	);
	$me-> insert( [ Button =>
		text        => 'Cancel',
		modalResult => mb::Cancel,
		pack        => { side => 'bottom', pad => 20 },
	], [ Button =>
		text        => '~Ok',
		modalResult => mb::OK,
		default     => 1,
		pack        => { side => 'bottom', pad => 20 },
	] );

	$self-> set_centered(
		$profile{x_centered} || $profile{centered},
		$profile{y_centered} || $profile{centered});

	return $self;
}

sub menuTree { shift-> Editor-> menu(@_) }

sub execute
{
	my $self = shift;
	my $ret  = $self-> SUPER::execute;
	$self-> Editor-> apply if $ret == mb::Ok;
	return $ret;
}

1;

=pod

=head1 NAME

Prima::KeySelector - key combination widget and routines

=head1 DESCRIPTION

The module provides a standard widget for selecting user-defined key
combinations. The widget class allows import, export, and modification of key
combinations.  The module also provides a set of routines useful for the conversion
of key combinations between various representations.

=head1 SYNOPSIS

	my $ks = Prima::KeySelector-> new( );
	$ks-> key( km::Alt | ord('X'));
	print Prima::KeySelector::describe( $ks-> key );

=head1 API

=head2 Properties

=over

=item key INTEGER

Selects a key combination in integer format. The format is described in
L<Prima::Menu/Hotkey>, and is a combination of the C<km::XXX> key modifiers and
is either a C<kb::XXX> virtual key or a character code value.

The property allows almost, but not all possible combinations of key constants.
Only the C<km::Ctrl>, C<km::Alt>, and C<km::Shift> modifiers are allowed.

=back

=head2 Methods

All methods must be called without the object as a first parameter.

=over

=item describe KEY

Accepts KEY in integer format and returns a string
description of the key combination in a human-readable
format. Useful for supplying an accelerator text to
a menu.

	print Prima::KeySelector::describe( km::Shift|km::Ctrl|km::F10);
	Ctrl+Shift+F10

=item export KEY

Accepts KEY in integer format and returns a string
with a perl-evaluable expression, which after the
evaluation resolves to the original KEY value. Useful for storing
a key into text config files, where the value must be both
human-readable and easily passed to a program.

	print Prima::KeySelector::export( km::Shift|km::Ctrl|km::F10);
	km::Shift|km::Ctrl|km::F10

=item shortcut KEY

Converts KEY from integer format to a string,
acceptable by C<Prima::AbstractMenu> input methods.

	print Prima::KeySelector::shortcut( km::Ctrl|ord('X'));
	^X

=item translate_codes KEY, [ USE_CTRL = 0 ]

Converts KEY in integer format to three integers in the format accepted by the
L<Prima::Widget/KeyDown> event: code, key, and modifier. USE_CTRL is only
relevant when the KEY first byte ( C<KEY & 0xFF> ) is between 1 and 26, which means
that the key is a combination of an alpha key with the control key.  If
USE_CTRL is 1, the code result is unaltered and is in range 1 - 26.  Otherwise,
the code result is converted to the character code ( 1 to ord('A'), 2 to ord('B'),
etc ).

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Menu>.

=cut
