package Prima::Dialog::FindDialog;
use strict;
use warnings;
use Prima qw(Buttons Label ComboBox);
use vars qw(@ISA);
@ISA = qw(Prima::Dialog);

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		centered    => 1,
		designScale => [ 4, 6],
		width       => 227,
		height      => 137,
		visible     => 0,

		findText    => '',
		replaceText => '',
		scope       => fds::Cursor,
		options     => 0,
		findStyle   => 1,
	}
}


sub init
{
	my $self = shift;
	my %profile = $self-> SUPER::init(@_);
	my $j;
	$self-> insert( ComboBox =>
		name    => 'Find',
		origin  => [ 51, 120],
		size    => [ 163, 8],
		style   => cs::DropDown,
		text => $profile{findText},
	);
	$self-> {findStyle} = $profile{findStyle};

	$self-> insert( Label =>
		origin    => [ 5, 120],
		size      => [ 40, 8],
		text  => '~Find:',
		focusLink => $self-> Find,
	);

	$self-> insert( ComboBox =>
		name    => 'Replace',
		origin  => [ 51, 103],
		size    => [ 163, 8],
		style   => cs::DropDown,
		enabled => !$profile{findStyle},
		text => $profile{replaceText},
	);

	$self-> insert( Label =>
		origin    => [ 5, 103],
		size      => [ 43, 8],
		text   => 'R~eplace:',
		focusLink => $self-> Replace,
		enabled   => !$profile{findStyle},
		name      => 'ReplaceLabel',
	);

	my $o = $self-> insert( GroupBox =>
		name    => 'Options',
		origin  => [ 10, 26],
		size    => [ 100, 68],
		text => 'Options',
	);

	$o-> insert( CheckBox =>
		name    => 'Case',
		origin  => [ 5, 49],
		size    => [ 90, 10],
		text => 'Match ~case',
	);

	$o-> insert( CheckBox =>
		name    => 'Words',
		origin  => [ 5, 38],
		size    => [ 90, 10],
		text => '~Whole words only',
	);

	$o-> insert( CheckBox =>
		name    => 'RE',
		origin  => [ 5, 27],
		size    => [ 90, 10],
		text => '~Regular expression',
	);

	$o-> insert( CheckBox =>
		name    => 'Back',
		origin  => [ 5,  16],
		size    => [ 90, 10],
		text => 'Search bac~kward',
	);

	$o-> insert( CheckBox =>
		name    => 'Prompt',
		origin  => [ 5,  5],
		size    => [ 90, 10],
		enabled => !$profile{findStyle},
		text => 'Replace ~prompt',
	);

	$o-> value( $profile{options});

	$o = $self-> insert( GroupBox =>
		name    => 'Scope',
		origin  => [ 117, 48],
		size    => [ 100, 46],
		text => 'Scope from',
	);

	$o-> insert( Radio =>
		name    => 'Cursor',
		origin  => [ 5,  27],
		size    => [ 90, 10],
		text     => 'C~ursor',
	);

	$o-> insert( Radio =>
		name    => 'Top',
		origin  => [ 5,  16],
		size    => [ 90, 10],
		text => '~Top',
	);

	$o-> insert( Radio =>
		name    => 'Bottom',
		origin  => [ 5,  5],
		size    => [ 90, 10],
		text => '~Bottom',
	);
	$o-> index( $profile{scope});

	$self-> insert( Button =>
		name    => 'OK',
		origin  => [ 8,  5],
		size    => [ 40, 14],
		default => 1,
		text => '~OK',
		delegations => [qw(Click)],
	);

	$self-> insert( Button =>
		name    => 'ChangeAll',
		origin  => [ 56,  5],
		size    => [ 76, 14],
		text => 'Change ~all',
		enabled => !$profile{findStyle},
		delegations => [qw(Click)],
	);

	$self-> insert( Button =>
		origin  => [ 139, 5],
		size    => [ 40, 14],
		text => 'Cancel',
		modalResult => mb::Cancel,
	);
	$self-> Find-> focus;
	return %profile;
}

sub valid
{
	my $self = $_[0];
	my $re = $self-> Find-> text;
	return 0 if length( $re) == 0;
	if ( $self-> options & fdo::RegularExpression) {
		if ( $self-> {findStyle}) {
			eval { $_ = ''; m/$re/; };
		} else {
			my $re2 = $self-> Replace-> text;
			eval { $_ = ''; s/$re/$re2/; };
		}
		if ( $@) {
			MsgBox::message_box( $self-> text, $@, mb::OK|mb::Error);
			$self-> Find-> focus;
			return 0;
		}
	}
	my $list = $self-> Find-> List;
	$list-> insert_items( 0, $re), $self-> findText( $re)
		if $list-> count == 0 or $list-> get_items(0) ne $re;
	$list = $self-> Replace-> List;
	$re = $self-> Replace-> text;
	$list-> insert_items( 0, $re), $self-> replaceText( $re)
		if !$self-> {findStyle} && ( $list-> count == 0 or $list-> get_items(0) ne $re);
	return 1;
}

sub OK_Click
{
	my $self = $_[0];
	return unless $self-> valid;
	$self-> ok;
}

sub ChangeAll_Click
{
	my $self = $_[0];
	return unless $self-> valid;
	$self-> modalResult( mb::ChangeAll );
	$self-> end_modal;
}


sub scope       {($#_)?$_[0]-> Scope-> index($_[1])    :return $_[0]-> Scope-> index}
sub options     {($#_)?$_[0]-> Options-> value($_[1])  :return $_[0]-> Options-> value}
sub findText    {($#_)?$_[0]-> Find-> text($_[1])   :return $_[0]-> Find-> text}
sub replaceText {($#_)?$_[0]-> Replace-> text($_[1]):return $_[0]-> Replace-> text}

sub findStyle
{
	return $_[0]-> {findStyle} unless $#_;
	return if $_[0]-> {findStyle} == $_[1];
	my ( $self, $style) = @_;
	$self-> {findStyle} = ( $style ? 1 : 0 );
	$self-> bring($_)-> enabled( ! $style ) for qw( Replace ReplaceLabel ChangeAll);
	$self-> Options-> Prompt-> enabled( !$style);
}

package Prima::Dialog::ReplaceDialog;
use vars qw(@ISA);
@ISA = qw(Prima::Dialog::FindDialog);

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	return {
		%def,
		findStyle  => 0,
	}
}

1;

=head1 NAME

Prima::Dialog::FindDialog - the standard dialogs FindDialog and ReplaceDialog
to find and replace text

=head1 SYNOPSIS

	use Prima qw(Dialog::FindDialog Application);

	my $dlg = Prima::Dialog::FindDialog-> new( findStyle => 0);
	my $res = $dlg-> execute;
	if ( $res == mb::Ok) {
		print $dlg-> findText, " is to be found\n";
	} elsif ( $res == mb::ChangeAll) {
		print "all occurrences of ", $dlg-> findText,
			" are to be replaced by ", $dlg-> replaceText;
	}

=for podview <img src="editdlg.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/Dialog/editdlg.gif">

=head1 DESCRIPTION

The module provides two classes Prima::Dialog::FindDialog and
Prima::Dialog::ReplaceDialog.  The Prima::Dialog::ReplaceDialog class is the same
as Prima::Dialog::FindDialog except that its default L<findStyle> property
value is set to 0. One can cache and reuse the dialog object, changing its
L<findStyle> value to 0 and 1, so that only one instance of
Prima::Dialog::FindDialog is used in the program.

The module does not provide the actual search algorithm; this must be
implemented by the programmer. The toolkit includes some help - the part of
the algorithm for the C<Prima::Edit> class is implemented in the C<Prima::Edit/find>
method, and another part in the F<examples/editor.pl> example program. The
L<Prima::HelpWindow> class also uses the module but implements its own
searching algorithm.

=head1 API

=head2 Properties

All the properties reflect values that the user can change interactively, -
except L<findStyle>.

=over

=item findText STRING

Selects the text string to be found.

Default value: ''

=item findStyle BOOLEAN

If 1, the dialog provides only the 'find text' interface. If 0, the dialog provides
also the'replace text' interface.

Default value: 1 for C<Prima::Dialog::FindDialog>, 0 for C<Prima::Dialog::ReplaceDialog>.

=item options INTEGER

A combination of the C<fdo::> constants. For the detailed description see L<Prima::Edit/find>.

	fdo::MatchCase
	fdo::WordsOnly
	fdo::RegularExpression
	fdo::BackwardSearch
	fdo::ReplacePrompt

Default value: 0

=item replaceText STRING

Selects the text string to replace the found text.

Default value: ''

=item scope

One of the C<fds::> constants. Represents the scope of the search: it can be started
from the cursor position, from the top of the text, or from the bottom.

	fds::Cursor
	fds::Top
	fds::Bottom

Default value: C<fds::Cursor>

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Window>, L<Prima::Edit>,
L<Prima::HelpWindow>, F<examples/editor.pl>

=cut
