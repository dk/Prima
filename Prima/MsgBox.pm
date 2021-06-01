package Prima::MsgBox;

use strict;
use warnings;
require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);
@ISA = qw(Exporter);
@EXPORT = qw(message_box message input_box);
@EXPORT_OK = qw(message_box message input_box);

use Prima::Classes;
use Prima::Buttons;
use Prima::StdBitmap;
use Prima::Label;
use Prima::InputLine;
use Prima::Utils;

sub insert_buttons
{
	my ( $dlg, $buttons, $extras) = @_;
	my ( $left, $right) = ( 10, 425);
	my $i;
	my @bConsts =  (
		mb::Help, mb::Cancel, mb::Ignore, mb::Retry, mb::Abort, mb::No, mb::Yes, mb::Ok
	);
	my @bTexts  = qw(
		~Help    ~Cancel     ~Ignore     ~Retry     ~Abort     ~No     ~Yes     ~OK
	);
	my $helpTopic = defined $$extras{helpTopic} ? $$extras{helpTopic} : 'Prima';
	my $defButton = defined $$extras{defButton} ? $$extras{defButton} : 0xFF;
	my $fresh;
	my $freshFirst;

	my $dir = 0; # set to 1 for reverse direction of buttons
	@bConsts = reverse @bConsts unless $dir;
	@bTexts  = reverse @bTexts  unless $dir;

	my $btns = 0;
	for ( $i = 0; $i < scalar @bConsts; $i++) {
		next unless $buttons & $bConsts[$i];
		my %epr;

		%epr = %{$extras-> {buttons}-> {$bConsts[$i]}}
			if $extras-> {buttons} && $extras-> {buttons}-> {$bConsts[$i]};

		my %hpr = (
			text      => $bTexts[$i],
			ownerFont => 0,
			bottom    => 10,
			default   => ( $bConsts[$i] & $defButton) ? 1 : 0,
			current   => ( $bConsts[$i] & $defButton) ? 1 : 0,
			size      => [ 96, 36],
		);

		$defButton = 0 if $bConsts[$i] & $defButton;
		$dir ? ( $hpr{right} = $right, $hpr{tabOrder} = 0) : ( $hpr{left} = $left);

		if ( $bConsts[$i] == mb::Help) {
			$hpr{onClick} = sub {
				$::application-> open_help( $helpTopic);
			};
			unless ( $dir) {
				$hpr{right} = 425;
				delete $hpr{left};
			}
		} else {
			$hpr{modalResult} = $bConsts[$i];
		}
		$fresh = $dlg-> insert( Button => %hpr, %epr);
		$freshFirst = $fresh unless $freshFirst;
		my $w = $fresh-> width + 10;
		$right -= 106;
		$left  += 106;
		last if ++$btns > 3;
	}
	$fresh = $freshFirst unless $dir;
	return $fresh;
}

sub message_box
{
	my ( $title, $text, $options, @extras) = @_;

	if ( (ref($text) // '') eq 'SCALAR') {
		require Prima::Drawable::Markup;
		$text = Prima::Drawable::Markup::M( $$text );
	}

	my $extras =
		( 1 == @extras and (ref($extras[0])||'') eq 'HASH') ?
	   	$extras[0] : # old style
		{ @extras }; # new style

	$options = mb::Ok | mb::Error unless defined $options;
	$options |= mb::Ok unless $options & 0x0FF;
	my $buttons = $options & 0xFF;
	$options &= 0xFF00;
	my @mbs   = qw( Error Warning Information Question);
	my $icon;
	my $nosound = $options & mb::NoSound;

	for ( @mbs)
	{
		my $const = $mb::{$_}-> ();
		next unless $options & $const;
		$options &= $const;
		$icon = $sbmp::{$_}-> ();
		last;
	}

	my $dlg = Prima::Dialog-> create(
		centered      => 1,
		width         => 435,
		height        => 125,
		designScale   => [7, 18],
		scaleChildren => 1,
		visible       => 0,
		text          => $title,
		font          => $::application-> get_message_font,
		owner         => $extras-> {owner} || $::main_window || $::application,
		onExecute     => sub {
			Prima::Utils::beep( $options) if $options && !$nosound;
		},
	);

	my $fresh = insert_buttons( $dlg, $buttons, $extras);

	$dlg-> scaleChildren(0);

	my $iconRight = 0;
	my $iconView;
	if ( $icon)
	{
		$icon = Prima::StdBitmap::icon( $icon);
		if ( defined $icon) {
			$iconView = $dlg-> insert( Widget =>
				origin         => [
					20,
					($dlg-> height + $fresh-> height - $icon-> height)/2
				],
				size           => [ $icon-> width, $icon-> height],
				onPaint        => sub {
					my $self = $_[1];
					$self-> color( $dlg-> backColor);
					$self-> bar( 0, 0, $self-> size);
					$self-> put_image( 0, 0, $icon);
				},
			);

			$iconRight = $iconView-> right;
		}
	}

	my $label = $dlg-> insert( Label =>
		left          => $iconRight + 15,
		right         => $dlg-> width  - 10,
		top           => $dlg-> height - 10,
		bottom        => $fresh-> height + 20,
		text          => $text,
		autoWidth     => 0,
		wordWrap      => 1,
		showAccelChar => 1,
		valignment    => ta::Middle,
		growMode      => gm::Client,
	);

	my $gl = int( $label-> height / $label-> font-> height);
	my $lc = scalar @{ $label-> text_wrap(
		$text,
		$label-> width,
		tw::NewLineBreak|tw::ReturnLines|tw::WordBreak|tw::ExpandTabs|tw::CalcTabs
	)};

	if ( $lc > $gl) {
		my $delta = ( $lc - $gl) * $label-> font-> height;
		my $dh = $dlg-> height;
		$delta = $::application-> height - $dh
			if $dh + $delta > $::application-> height;
		$dlg-> height( $dh + $delta);
		$dlg-> centered(1);
		$iconView-> bottom( $iconView-> bottom + $delta / 2) if $iconView;
	}

	my $ret = $dlg-> execute;
	$dlg-> destroy;
	return $ret;
}


sub message
{
	return message_box( $::application-> name, @_);
}

sub input_box
{
	my ( $title, $text, $string, $buttons, @extras) = @_;
	$buttons = mb::Ok|mb::Cancel unless defined $buttons;

	my $extras =
		( 1 == @extras and (ref($extras[0])||'') eq 'HASH') ?
	   	$extras[0] : # old style
		{ @extras }; # new style

	my $dlg = Prima::Dialog-> create(
		centered      => 1,
		width         => 435,
		height        => 110,
		designScale   => [7, 16],
		scaleChildren => 1,
		visible       => 0,
		text          => $title,
	);

	my $fresh = insert_buttons( $dlg, $buttons, $extras);

	$dlg-> insert( Label =>
		origin        => [ 10, 82],
		size          => [ 415, 16],
		text          => $text,
		showAccelChar => 1,
	);

	my $input = $dlg-> insert( InputLine =>
		size          => [ 415, 20],
		origin        => [ 10, 56],
		text          => $string,
		current       => 1,
		defined($extras-> {inputLine}) ? %{$extras-> {inputLine}} : (),
	);

	my @ret = ( $dlg-> execute, $input-> text);
	$dlg-> destroy;
	return wantarray ?
		@ret :
		(( $ret[0] == mb::OK || $ret[0] == mb::Yes) ? $ret[1] : undef);
}


sub import
{
	no strict 'refs';
	my $callpkg = $Prima::__import || caller;
	for my $sym (qw(message_box message input_box)) {
		next if $callpkg eq 'Prima' && $sym eq 'message'; # leave Prima::message intact
		*{"${callpkg}::$sym"} = \&{"Prima::MsgBox::$sym"}
	}
	use strict 'refs';
}

1;


=pod

=head1 NAME

Prima::MsgBox - standard message and input dialog boxes

=head1 DESCRIPTION

The module contains two methods, C<message_box> and C<input_box>,
that invoke correspondingly the standard message and one line text input
dialog boxes.

=head1 SYNOPSIS

	use Prima qw(Application);
	use Prima::MsgBox qq(input_box message);

	my $text = input_box( 'Sample input box', 'Enter text:', '') // '(none)';
	message( \ "You have entered: 'B<Q<< $text >>>'", mb::Ok);

=for podview <img src="msgbox.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/msgbox.gif">

=head1 API

=over

=item input_box TITLE, LABEL, INPUT_STRING, [ BUTTONS = mb::OkCancel, %PROFILES ]

Invokes standard dialog box, that contains an input line, a text label, and
buttons used for ending dialog session. The dialog box uses TITLE string to
display as the window title, LABEL text to draw next to the input line, and
INPUT_STRING, which is the text present in the input box. Depending on the
value of BUTTONS integer parameter, which can be a combination of the button
C<mb::XXX> constants, different combinations of push buttons can be displayed
in the dialog.

PROFILE parameter is a hash, that contains customization parameters for the
buttons and the input line. To access input line C<inputLine> hash key is used.
See L<Buttons and profiles> for more information on BUTTONS and PROFILES.

Returns different results depending on the caller context.  In array context,
returns two values: the result of C<Prima::Dialog::execute>, which is either
C<mb::Cancel> or one of C<mb::XXX> constants of the dialog buttons; and the
text entered. The input text is not restored to its original value if the
dialog was cancelled. In scalar context returns the text entered, if the dialog
was ended with C<mb::OK> or C<mb::Yes> result, or C<undef> otherwise.

=item message TEXT, [ OPTIONS = mb::Ok | mb::Error, %PROFILES ]

Same as C<message_box> call, with application name passed as the title string.

=item message_box TITLE, TEXT, [ OPTIONS = mb::Ok | mb::Error, %PROFILES ]

Invokes standard dialog box, that contains a text label, a predefined icon, and
buttons used for ending dialog session. The dialog box uses TITLE string to
display as the window title, TEXT to display as a main message. Value of
OPTIONS integer parameter is combined from two different sets of C<mb::XXX>
constants. The first set is the buttons constants, - C<mb::OK>, C<mb::Yes> etc.
See L<Buttons and profiles> for the explanations. The second set consists of
the following message type constants:

	mb::Error
	mb::Warning
	mb::Information
	mb::Question

While there can be several constants of the first set, only one constant from
the second set can be selected.  Depending on the message type constant, one of
the predefined icons is displayed and one of the system sounds is played; if no
message type constant is selected, no icon is displayed and no sound is
emitted.  In case if no sound is desired, a special constant C<mb::NoSound> can
be used.

PROFILE parameter is a hash, that contains customization parameters for the
buttons.  See L<Buttons and profiles> for the explanations.

Returns the result of C<Prima::Dialog::execute>, which is either C<mb::Cancel>
or one of C<mb::XXX> constants of the specified dialog buttons.

=back

=head2 Buttons and profiles

The message and input boxes provide several predefined buttons that correspond
to the following C<mb::XXX> constants:

	mb::OK
	mb::Cancel
	mb::Yes
	mb::No
	mb::Abort
	mb::Retry
	mb::Ignore
	mb::Help

To provide more flexibility, PROFILES hash parameter can be used.  In this
hash, predefined keys can be used to tell the dialog methods about certain
customizations:

=over

=item defButton INTEGER

Selects the default button in the dialog, i.e. the button that reacts on the
return key. Its value must be equal to the C<mb::> constant of the desired
button. If this option is not set, the leftmost button is selected as the
default.

=item helpTopic TOPIC

Used to select the help TOPIC, invoked in the help viewer window if C<mb::Help>
button is pressed by the user.  If this option is not set, L<Prima> is
displayed.

=item inputLine HASH

Only for C<input_box>.

Contains a profile hash, passed to the input line as creation parameters.

=item buttons HASH

To modify a button, an integer key with the corresponding C<mb::XXX> constant
can be set with the hash reference under C<buttons> key.  The hash is a
profile, passed to the button as creation parameters. For example, to change
the text and behavior of a button, the following construct can be used:

	Prima::MsgBox::message( 'Hello', mb::OkCancel,
		buttons => {
			mb::Ok, {
				text     => '~Hello',
				onClick  => sub { Prima::message('Hello indeed!'); }
			}
		}
	);

If it is not desired that the dialog must be closed when the user presses a
button, its C<::modalResult> property ( see L<Prima::Buttons> ) must be reset
to 0.

=item owner WINDOW

If set, the dialog owner is set to WINDOW, otherwise to C<$::main_window>.
Necessary to maintain window stack order under some window managers, to disallow
windows to be brought over the message box.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Buttons>, L<Prima::InputLine>, L<Prima::Dialog>.

=cut
