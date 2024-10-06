package Prima::Label;
use strict;
use warnings;
use Prima;
use base qw(Prima::Widget);
use Prima::Widget::Link;

{
my %RNT = (
	%{Prima::Widget-> notification_types()},
	%{Prima::Widget::Link-> notification_types()},
);

sub notification_types { return \%RNT; }
}

sub profile_default
{
	my $font = $_[ 0]-> get_default_font;
	my $rtl  = $::application-> textDirection;
	return {
		%{$_[ 0]-> SUPER::profile_default},
		alignment      => $rtl ? ta::Right : ta::Left,
		autoHeight     => 0,
		autoWidth      => 1,
		focusLink      => undef,
		height         => 4 + $font-> { height},
		hotKey         => undef,
		linkColor      => Prima::Widget::Link->profile_default->{color},
		ownerBackColor => 1,
		selectable     => 0,
		showAccelChar  => 0,
		showPartial    => 1,
		tabStop        => 0,
		textDirection  => $rtl,
		textJustify    => 0,
		valignment     => ta::Top,
		widgetClass    => wc::Label,
		wordWrap       => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> { autoWidth} = 0 if
		! exists $p->{autoWidth} and (
		exists $p-> {width} ||
		exists $p-> {size} ||
		exists $p-> {rect} ||
		( exists $p-> {left} && exists $p-> {right})
	);
	$p-> {autoHeight} = 0 if
		! exists $p-> {autoHeight} and (
		exists $p-> {height} ||
		exists $p-> {size} ||
		exists $p-> {rect} ||
		( exists $p-> {top} && exists $p-> {bottom})
	);
	$p-> {alignment} = ( $p->{textDirection} // $default->{textDirection} ) ?
		ta::Right : ta::Left unless exists $p->{alignment};
	$self-> SUPER::profile_check_in( $p, $default);
	my $vertical = exists $p-> {vertical} ?
		$p-> {vertical} :
		$default-> { vertical};
}


sub init
{
	my $self = shift;
	$self->{lock} = 1;
	$self->{textLines} = 0;
	my %profile = $self-> SUPER::init(@_);
	$self->textJustify($profile{textJustify});
	$self-> {$_} = $profile{$_} for qw(
		textDirection alignment valignment autoHeight autoWidth
		wordWrap focusLink showAccelChar showPartial hotKey 
	);
	$self->$_($profile{$_}) for qw(linkColor);
	$self-> check_auto_size;
	delete $self->{lock};
	$self->reset_lines;
	return %profile;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my @size = $canvas-> size;
	my @clr;
	if ( $self-> enabled) {
		if ( $self-> focused) {
			@clr = ($self-> hiliteColor, $self-> hiliteBackColor);
		} else {
			@clr = ($self-> color, $self-> backColor);
		}
	} else {
		@clr = ($self-> disabledColor, $self-> disabledBackColor);
	}

	unless ( $self-> transparent) {
		$canvas-> color( $clr[1]);
		$canvas-> bar(0,0,@size);
	}

	my $fh  = $canvas-> font-> height;
	my $ws  = $self->{words};
	my $wx  = $self->{start_x};
	my $y   = $self->{start_y};
	my $tl  = $self-> {tildeLine};
	my $i;
	my $paintLine = !$self-> {showAccelChar} && defined($tl) && $tl < scalar @{$ws};

	unless ( $self-> enabled) {
		$canvas-> color( $self-> light3DColor);
		for ( $i = 0; $i < @$ws; $i++) {
			$canvas-> text_out( $$ws[$i], $$wx[$i] + 1, $y - 1);
			$y -= $fh;
		}
		$y = $self->{start_y};

		if ( $paintLine) {
			my $x = $$wx[$tl] + 1;
			$canvas-> line(
				$x + $self-> {tildeStart}, $self->{start_y} - $fh * $tl - 1,
				$x + $self-> {tildeEnd},   $self->{start_y} - $fh * $tl - 1
			);
		}
	}

	$canvas-> color( $clr[0]);
	for ( $i = 0; $i < @$ws; $i++) {
		$canvas-> text_out( $$ws[$i], $$wx[$i], $y);
		$y -= $fh;
	}
	if ( $paintLine) {
		my $x = $$wx[$tl];
		$canvas-> line(
			$x + $self-> {tildeStart}, $self->{start_y} - $fh * $tl,
			$x + $self-> {tildeEnd},   $self->{start_y} - $fh * $tl
		);
	}
	$self->{link_handler}->on_paint( $self, $canvas )
		if $self->{link_handler};
}

sub set_text
{
	my $self = $_[0];
	undef $self->{link_handler};
	$self-> SUPER::set_text( $_[1]);

	if ( ref($_[1]) ) {
		my $text = $self->SUPER::text;
		$self->{link_handler} = Prima::Widget::Link->new(
			markup => $text,
			color  => $self->linkColor,
		) if UNIVERSAL::isa($text, 'Prima::Drawable::Markup');
	}

	$self-> check_auto_size;
	$self-> reset_lines;
	$self-> repaint;
}


sub on_translateaccel
{
	my ( $self, $code, $key, $mod) = @_;
	if (
		!$self-> {showAccelChar} &&
		defined $self-> {accel} &&
		( $key == kb::NoKey) &&
		lc chr $code eq $self-> { accel}
	) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
	if (
		defined $self-> {hotKey} &&
		( $key == kb::NoKey) &&
		lc chr $code eq $self-> {hotKey}
	) {
		$self-> clear_event;
		$self-> notify( 'Click');
	}
}

sub on_click
{
	my ( $self, $f) = ( $_[0], $_[0]-> {focusLink});
	$f = $self->owner->bring($f) if defined $f && ! ref $f;
	$f-> select if defined $f && $f-> alive && $f-> enabled;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	if (
		defined $self-> {accel} &&
		( $key == kb::NoKey) &&
		lc chr $code eq $self-> { accel}
	) {
		$self-> notify( 'Click');
		$self-> clear_event;
	}
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	$self-> clear_event;
	return if $self->{link_handler} && $self->{link_handler}->on_mousedown($self, $btn, $mod, $x, $y);
	$self-> notify( 'Click');
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self->{link_handler}->on_mousemove($self, $mod, $x, $y)
		if $self->{link_handler};
}

sub on_mouseleave
{
	my ( $self) = @_;
	$self->{link_handler}->on_mousemove($self, 0, -1, -1)
		if $self->{link_handler};
}

sub on_fontchanged
{
	$_[0]-> check_auto_size;
}

sub on_size
{
	my $self = shift;
	return $self-> reset_lines unless $self->{autoHeight};

	if ($self->{auto_height_adjustment}) {
		$self->reposition_lines;
		return;
	}

	local $self->{auto_height_adjustment} = 1;
	$self-> check_auto_size;
}

sub on_enable { $_[0]-> repaint } sub on_disable { $_[0]-> repaint }

sub set_alignment
{
	$_[0]-> {alignment} = $_[1];
	$_[0]-> repaint;
}

sub set_valignment
{
	$_[0]-> {valignment} = $_[1];
	$_[0]-> repaint;
}


sub reset_lines
{
	my ($self, $nomaxlines) = @_;
	return if $self->{lock};

	my @res;
	my $maxLines = int($self-> height / $self-> font-> height);
	$maxLines++ if $self-> {showPartial} and (($self-> height % $self-> font-> height) > 0);

	my $opt   = tw::NewLineBreak|tw::ReturnLines|tw::WordBreak|tw::CalcMnemonic|tw::ExpandTabs|tw::CalcTabs;
	my $width;
	$opt |= tw::CollapseTilde unless $self-> {showAccelChar};
	$width = $self-> width if $self-> {wordWrap};

	$self-> begin_paint_info;

	my $lines = $self-> text_wrap( $self->text, $width // -1, $opt, 1);
	my $lastRef = pop @{$lines};
	$self-> {textLines} = scalar @$lines;
	for( qw( tildeStart tildeEnd tildeLine)) {$self-> {$_} = $lastRef-> {$_}}

	$self-> {accel} = defined($self-> {tildeStart}) ? lc( $lastRef-> {tildeChar}) : undef;
	splice( @{$lines}, $maxLines) if scalar @{$lines} > $maxLines && !$nomaxlines;

	$self->{words} = [$self->shape_and_justify_text($self, $lines)];
	$self-> reposition_lines;

	$self-> end_paint_info;
}

sub reposition_lines
{
	my $self = shift;

	my $ps = $self->get_paint_state;
	$self-> begin_paint_info if $ps == ps::Disabled;

	my $ws = $self->{words};
	my $fh = $self->font->height;
	my ($starty,$ycommon) = (0, scalar @$ws * $fh);
	my @size = $self->size;

	if ( $self-> {valignment} == ta::Top)  {
		$starty = $size[1] - $fh;
	} elsif ( $self-> {valignment} == ta::Bottom) {
		$starty = $ycommon - $fh;
	} else {
		$starty = ( $size[1] + $ycommon)/2 - $fh;
	}
	$self->{start_y} = $starty;

	my @wx;
	if ( $self->{alignment} == ta::Left ) {
		@wx = map { 0 } @$ws;
	} else {
		my $ta = $self->{alignment};
		my @ww = map { $self->get_text_width($_) } @$ws;
		for ( my $i = 0; $i < @$ws; $i++) {
			my $x = 0;
			if ( $ta == ta::Center) {
				$x = ( $size[0] - $ww[$i]) / 2;
			} elsif ( $ta == ta::Right) {
				$x = $size[0] - $ww[$i];
			}
			push @wx, $x;
		}
	}
	$self->{start_x} = \@wx;

	if ($self->{link_handler}) {
		my $ta = $self->{alignment};
		my (@sx, @sy);
		for ( my $i = 0; $i < @$ws; $i++) {
			my $b = $$ws[$i]->block;
			push @sx, $$b[tb::BLK_X];
			push @sy, $$b[tb::BLK_Y];
			$$b[tb::BLK_X] = $wx[$i];
			$$b[tb::BLK_Y] = $starty;
			$starty -= $fh;
		}
		$self->{link_handler}->reset_positions_markup($ws);
		for ( my $i = 0; $i < @$ws; $i++) {
			my $b = $$ws[$i]->block;
			$$b[tb::BLK_X] = $sx[$i];
			$$b[tb::BLK_Y] = $sy[$i];
		}
	}

	$self-> end_paint_info if $ps == ps::Disabled;
}

sub check_auto_size
{
	my $self = $_[0];
	my $cap = $self-> text;
	$cap =~ s/~//s unless $self-> {showAccelChar};
	my %sets;

	if ( $self-> {wordWrap}) {
		$self-> reset_lines(1);
		if ( $self-> {autoHeight}) {
			$self-> geomHeight( $self-> {textLines} * $self-> font-> height + 2);
		}
	} else {
		my @lines = $self-> text_split_lines($cap);
		if ( $self-> {autoWidth}) {
			$self-> begin_paint_info;
			$sets{geomWidth} = 0;
			for my $line ( @lines) {
				my $width = $self-> get_text_width( $line);
				$sets{geomWidth} = $width if
					$sets{geomWidth} < $width;
			}
			$sets{geomWidth} += 6;
			$self-> end_paint_info;
		}
		$sets{ geomHeight} = scalar(@lines) * $self-> font-> height  + 2
			if $self-> {autoHeight};
		$self-> set( %sets);
		$self-> reset_lines;
	}
}

sub set_auto_width
{
	my ( $self, $aw) = @_;
	return if $self-> {autoWidth} == $aw;
	$self-> {autoWidth} = $aw;
	$self-> check_auto_size;
}

sub set_auto_height
{
	my ( $self, $ah) = @_;
	return if $self-> {autoHeight} == $ah;
	$self-> {autoHeight} = $ah;
	$self-> check_auto_size;
}

sub set_word_wrap
{
	my ( $self, $ww) = @_;
	return if $self-> {wordWrap} == $ww;
	$self-> {wordWrap} = $ww;
	$self-> check_auto_size;
}

sub set_show_accel_char
{
	my ( $self, $sac) = @_;
	return if $self-> {showAccelChar} == $sac;
	$self-> {showAccelChar} = $sac;
	$self-> check_auto_size;
}

sub set_show_partial
{
	my ( $self, $sp) = @_;
	return if $self-> {showPartial} == $sp;
	$self-> {showPartial} = $sp;
	$self-> check_auto_size;
}

sub get_lines
{
	return @{$_[0]-> {words}};
}

sub showAccelChar {($#_)?($_[0]-> set_show_accel_char($_[1])) :return $_[0]-> {showAccelChar}}
sub showPartial   {($#_)?($_[0]-> set_show_partial($_[1]))    :return $_[0]-> {showPartial}}
sub focusLink     {($#_)?($_[0]-> {focusLink}     = $_[1])    :return $_[0]-> {focusLink}    }
sub alignment     {($#_)?($_[0]-> set_alignment(    $_[1]))   :return $_[0]-> {alignment}    }
sub valignment    {($#_)?($_[0]-> set_valignment(    $_[1]))  :return $_[0]-> {valignment}   }
sub autoWidth     {($#_)?($_[0]-> set_auto_width(   $_[1]))   :return $_[0]-> {autoWidth}    }
sub autoHeight    {($#_)?($_[0]-> set_auto_height(  $_[1]))   :return $_[0]-> {autoHeight}   }
sub wordWrap      {($#_)?($_[0]-> set_word_wrap(    $_[1]))   :return $_[0]-> {wordWrap}     }
sub hotKey        { $#_ ? $_[0]->{hotKey} = $_[1] : $_[0]->{hotKey} }

sub linkColor     {
	return $_[0]->{linkColor} unless $#_;
	my ( $self, $lc ) = @_;
	$self->{linkColor} = $lc;
	$self->{link_handler}->color($lc) if $self->{link_handler};
	$self->repaint;
}

sub textDirection
{
	return $_[0]-> {textDirection} unless $#_;
	my ( $self, $td ) = @_;
	$self-> {textDirection} = $td;
	$self-> text( $self-> text );
	$self-> alignment( $td ? ta::Right : ta::Left );
}

sub textJustify
{
	return {%{$_[0]-> {textJustify}}} unless $#_;
	my $self = shift;
	if ( @_ % 2 ) {
		my $tj = shift;
		my %tj = ref($tj) ? %$tj : map { ($_, !!$tj) } qw(letter word kashida);
		$tj{$_} //= 0 for qw(letter word kashida min_kashida);
		if ( ref($tj) ) {
			$tj{$_} = $$tj{$_} for qw(min_text_to_space_ratio);
		}
		$self->{textJustify} = \%tj;
	} else {
		my %h = @_;
		$self->{textJustify}->{$_} = $h{$_} for keys %h;
	}
	$self-> text( $self-> text );
}

sub shape_and_justify_text
{
	my ($self, $canvas, $ws) = @_;

	my @size = $canvas-> size;
	my %justify = ( %{$self->textJustify}, rtl => $self->textDirection);
	my @wss;

	for ( my $i = 0; $i < @$ws; $i++) {
		my $s = $canvas->text_shape($ws->[$i], %justify );
		if ( $s && ref($s)) {
			$s->justify(
				$canvas, $ws->[$i], $size[0],
				%justify,
				($i == $#$ws) ? (letter => 0, word => 0) : ()
			);
			push @wss, $s;
		} else {
			push @wss, $ws->[$i];
		}
	}

	return @wss;
}

1;

=pod

=head1 NAME

Prima::Label - static text widget

=head1 DESCRIPTION

The class is designed for the display of text and provides no user interaction. The
text output capabilities include wrapping, horizontal and vertical alignment,
and automatic widget resizing to match text extensions. If the text contains a
tilde-escaped ( hot ) character, the label can explicitly focus the specified
widget upon pressing of the character key; this feature is useful for the design of
dialogs.

Labels can display rich text with links. See L<Prima::Drawable::Markup> for
more.

=head1 SYNOPSIS

	use Prima qw(Label InputLine Application);
	my $w = Prima::MainWindow->new;
	$w->insert( 'Prima::Label',
		text      => 'Enter ~name:',
		focusLink => 'InputLine1',
		alignment => ta::Center,
		pack => { fill => 'x', side => 'top', pad => 10 },
	);
	$w->insert(
		'Prima::InputLine',
		text => '',
		pack => { fill => 'x', side => 'top', pad => 10 },
	);
	run Prima;

=for podview <img src="label.gif">

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/label.gif">

=head1 API

=head2 Properties

=over

=item alignment INTEGER

One of the following C<ta::XXX> constants:

	ta::Left
	ta::Center
	ta::Right

Selects the horizontal text alignment.

Default value: C<ta::Left>

=item autoHeight BOOLEAN

If 1, the widget height is automatically changed as text extensions
change.

Default value: 0

=item autoWidth BOOLEAN

If 1, the widget width is automatically changed as text extensions
change.

Default value: 1

=item focusLink WIDGET

Points to a widget or a widget name (has to be a sibling widget), which is
explicitly focused when the user presses the combination of the hotkey with the
C<Alt> key.

Prima::Label does not provide a separate property to access the hotkey value
from the tilde-escaped string, however, it can be read from the C<{accel}>
variable.

Default value: C<undef>.

=item hotKey CHAR

A key (defined by CHAR) that the label will react to if pressed if the label has the focus
The combination ALT + Key works always whether the label has the focus or not

=item linkColor COLOR

The color of text in links.

The default value is taken from L<Prima::Widget::Link> and is currently hardcoded as green.
So far there is no support for the system link color.

=item showAccelChar BOOLEAN

If 0, the tilde ( ~ ) character is collapsed from the text, and the hot
character is underlined. When the user presses the combination of the escaped
character with the C<Alt> key, the C<focusLink> widget is explicitly focused.

If 1, the text is shown as is, and no hot character is underlined.
Key combinations with the C<Alt> key are not recognized. See also: C<hotKey>.

Default value: 0

=item showPartial BOOLEAN

Used to determine if the last line of text should be drawn if
it can not be vertically fit in the widget interior. If 1, the
last line is shown even if not visible in full. If 0, only full
lines are drawn.

Default value: 1

=item textJustify $BOOL | { letter => 0, word => 0, kashida => 0, min_kashida => 0 } | %VALUES

If set, justifies wrapped text according to the option passed in the hash
( see L<Prima::Drawable::Glyphs/arabic_justify> and L<Prima::Drawable::Glyphs/interspace_justify> ).
Can accept three forms:

If an anonymous hash is used, overwrites all the currently defined options.

If C<$BOOL> is used, treated as a shortcut for C<< { letter => $BOOL, word => $BOOL, kashida => $BOOL } >>;
consequent get-calls return a full hash, not the C<$BOOL> value.

If the C<%VALUES> form is used, overwrites only values found in C<%VALUES>.

Only actual when C<wordWrap> is set.

=item textDirection BOOLEAN

If set, indicates RTL text direction.

=item wordWrap BOOLEAN

If 0, the text is not wrapped unless new line characters are present
in the text.

If 1, the text is wrapped if it can not be fit horizontally in the
widget interior. The text is also wrapped over new lines.

Default value: 0

=item valignment INTEGER

One of the following C<ta::XXX> constants:

	ta::Top
	ta::Middle or ta::Center
	ta::Bottom

Selects the vertical text alignment.

Note: the C<ta::Middle> value is not equal to C<ta::Center>'s, however,
both constants produce an equal effect here.

Default value: C<ta::Top>

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, L<Prima::Drawable::Markup>, L<Prima::Widget::Link>, F<examples/label.pl>

=cut
