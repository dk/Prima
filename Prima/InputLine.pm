#  Created by Dmitry Karasik <dk@plab.ku.dk>
#  Modifications by Anton Berezin <tobez@tobez.org>
package Prima::InputLine;
use vars qw(@ISA);
@ISA = qw(Prima::Widget Prima::MouseScroller Prima::UndoActions Prima::BidiInput);

use strict;
use warnings;

use Prima::Classes;
use Prima::IntUtils;
use Prima::Drawable::Glyphs;

sub profile_default
{
	my %def = %{$_[ 0]-> SUPER::profile_default};
	my $font = $_[ 0]-> get_default_font;
	my $rtl  = $::application-> textDirection;
	return {
		%def,
		alignment      => $rtl ? ta::Right : ta::Left,
		autoHeight     => 1,
		autoSelect     => 1,
		autoTab        => 0,
		borderWidth    => 1,
		charOffset     => 0,
		cursorVisible  => 1,
		cursorSize     => [ Prima::Application-> get_default_cursor_width, $font-> { height}],
		firstChar      => 0,
		dndAware       => 'Text',
		height         => 2 + $font-> { height} + 2,
		insertMode     => 0,
		maxLen         => 256,  # length $def{ text},
		passwordChar   => '*',
		pointerType    => cr::Text,
		popupItems     => [
			[ cut        => 'Cu~t'        => 'cut'       ],
			[ copy       => '~Copy'       => 'copy'      ],
			[ paste      => '~Paste'      => 'paste'     ],
			[ delete     => '~Delete'     => 'delete'    ],
			[],
			[select_all  => 'Select ~All' => 'select_all'],
			[undo        => '~Undo', 'Ctrl+Z', '^Z', 'undo'],
			[redo        => 'R~edo', 'Ctrl+Y', '^Y', 'redo'],
			['@rtl'      => '~RTL input', 'Ctrl+Shift+D', '^#D', 'toggle_rtl'],
		],
		readOnly       => 0,
		selection      => [0, 0],
		selStart       => 0,
		selEnd         => 0,
		selectable     => 1,
		textDirection  => $rtl,
		undoLimit      => 10,
		widgetClass    => wc::InputLine,
		width          => 96,
		wordDelimiters => ".()\"',#$@!%^&*{}[]?/|;:<>-= \t",
		writeOnly      => 0,
	}
}

sub profile_check_in
{
	my ( $self, $p, $default) = @_;
	$p-> {autoHeight} = 0
		if exists $p-> {height} || exists $p-> {size} || exists $p-> {rect} || ( exists $p-> {top} && exists $p-> {bottom});
	$p-> {alignment} = ( $p->{textDirection} // $default->{textDirection} ) ?
		ta::Right : ta::Left unless exists $p->{alignment};
	$self-> SUPER::profile_check_in( $p, $default);
	@{$p}{qw(selStart selEnd)} = @{$p-> {selection}} if exists( $p-> { selection});
}

sub init
{
	my $self = shift;

	for ( qw(
		borderWidth passwordChar maxLen alignment autoTab autoSelect
		firstChar charOffset readOnly))
		{ $self-> {$_} = 1; }
	for ( qw( selStart selEnd atDrawX autoHeight undoLimit n_clusters))
		{ $self-> {$_} = 0;}
	$self-> { insertMode}   = $::application-> insertMode;
	$self-> { maxLen}   = -1;
	$self-> {writeOnly} = 0;
	$self-> {defcw} = $::application-> get_default_cursor_width;
	$self-> {resetDisabled} = 1;

	my %profile = $self-> SUPER::init(@_);
	$self->init_undo(\%profile);

	for ( qw(
		textDirection
		writeOnly borderWidth passwordChar maxLen alignment
		autoTab autoSelect readOnly selEnd selStart charOffset
		firstChar wordDelimiters ))
		{ $self-> $_( $profile{ $_}); }
	$self-> {resetDisabled} = 0;
	$self-> {resetLevel}    = 0;

	my $font = $self-> font;
	$self-> {font_height} = $font-> height;
	$self-> {font_width} = $font-> width;

	$self-> reset;
	$self-> autoHeight( $profile{autoHeight});

	return %profile;
}

sub on_paint
{
	my ($self,$canvas) = @_;
	my @size = $canvas-> size;
	my @clr;
	my @selClr;
	@clr = $self-> enabled ?
		($self-> color, $self-> backColor) :
		($self-> disabledColor, $self-> disabledBackColor);
	@selClr = ($self-> hiliteColor, $self-> hiliteBackColor);

	my $border = $self-> {borderWidth};
	if ( $self-> {borderWidth} == 0) {
		$canvas-> color( $clr[1]);
		$canvas-> bar(0,0,@size);
	} else {
		$canvas-> rect3d( 0, 0, $size[0]-1, $size[1]-1, $border, $self-> dark3DColor, $self-> light3DColor, $clr[1]);
	}

	return if $size[0] <= $border * 2 + 2;
	my $cap = $self-> {glyphs} or return;

	$canvas-> clipRect  (
		$border + 1, $border + 1,
		$size[0] - $border - 2, $size[1] - $border - 2
	);
	$canvas-> translate ( $border + 1, $border + 1);
	$size[0] -= ( $border + 1) * 2;
	$size[1] -= ( $border + 1) * 2;

	my ( $fh, $useSel) =
	(
		$self-> {font_height},
		( $self-> {selStart} < $self-> {selEnd}) && $self-> focused && $self-> enabled
	);

	$useSel = 0 if $self-> {selEnd} <= $self-> {firstChar};

	my ( $x, $y) = ( $self-> {atDrawX}, $self-> {atDrawY});
	if ( $useSel && @{ $self->{selChunks} // [] }) {
		$self->{glyphs}->selection_walk(
			$self->{selChunks}, $self->{firstChar}, $self->{n_clusters}, sub {
			my ( $offset, $length, $selected ) = @_;
			my $text = substr( $cap, $offset, $length );
			my $dx = $self->{glyphs}->get_sub_width( $self, $self->{firstChar} + $offset, $length);
			if ( $selected ) {
				$canvas-> color( $self-> hiliteBackColor);
				$canvas-> bar( $x, 0, $x + $dx - 1, $size[1] - 1);
				$canvas-> color( $self-> hiliteColor);
			} else {
				$canvas-> color( $clr[0]);
			}
			$canvas->text_out($self->{glyphs}, $x, $y, $self->{firstChar} + $offset, $length );
			$x += $dx;
		});
	} else {
		$canvas-> color( $clr[0]);
		$cap->sub_text_out( $canvas, $self->{firstChar}, undef, $x, $y);
	}
}

sub reset_cursor
{
	my $self = $_[0];
	$self-> {resetLevel} = 1;
	$self-> reset;
	$self-> {resetLevel} = 0;
}

sub reset
{
	my $self  = $_[0];
	return if $self-> {resetDisabled};
	my @size   = $self-> size;
	my $border = $self-> {borderWidth};
	my $width  = $size[0] - ( $border + 1) * 2;
	my $fc     = $self->{firstChar};
	my $glyphs = $self->{glyphs};

	if ( $self-> {resetLevel} == 0) {
		$self->{atDrawY} = ( $size[1] - ( $border + 1) * 2 - $self-> {font_height}) / 2;
		$self->{lineWidth} = $glyphs->get_sub_width($self, $fc);
		$self->{atDrawX}   = 
			(
				($self->{lineWidth} < $width) && 
				($self->{alignment} != ta::Left)
			) ?
				($width - $self->{lineWidth}) / (
					($self->{alignment} == ta::Center ) ? 2 : 1
				)
			: 0;
	}

	my $ofs = $self->{charOffset} - $fc;
	my $curWidth;
	my $x;
	if ( $self-> {insertMode} ) {
		$curWidth = $self->{defcw};
	} 
	if ( $self-> textDirection) {
		if ( $ofs < 0 ) {
			$x = 0;
			$curWidth //= $self->{defcw};
		} else {
			$x = $glyphs->get_sub_width($self,$fc,$ofs - ($self->{insertMode} ? 0 : 1))
				+ $self->{atDrawX} + $border + 1;
			$curWidth //= $glyphs->get_sub_width($self, $fc + $ofs - 1, 1);
		}
	} else {
		if ( $ofs < 0 ) {
			$x = 0;
			$curWidth //= 0;
		} else {
			$x = $glyphs->get_sub_width($self,$fc,$ofs)
				+ $self->{atDrawX} + $border + 1;
			$curWidth //= $glyphs->get_sub_width($self, $fc + $ofs, 1);
		}
	}
	$curWidth ||= 1;
	$curWidth = $size[0] - $x - $border if $curWidth + $x > $size[0] - $border;
	$self-> cursorSize( $curWidth, $size[1] - $border * 2 - 2);
	$self-> cursorPos( $x, $border + 1);
}

sub text
{
	return $_[0]-> SUPER::text unless $#_;
	my ( $self, $cap) = @_;
	$cap = '' unless defined $cap;
	$cap = substr( $cap, 0, $self-> {maxLen})
		if $self-> {maxLen} >= 0 and length($cap) > $self-> {maxLen};

	$self-> SUPER::text($cap);

	$cap = $self-> {passwordChar} x length $cap if $self-> {writeOnly};
	if ( length($cap)) {
		$self->{glyphs}     = $self-> text_shape( $cap, rtl => $self->textDirection );
		$self->{n_clusters} = $self->{glyphs}->n_clusters;
	} else {
		$self->{glyphs}     = Prima::Drawable::Glyphs->new_empty;
		$self->{n_clusters} = 0;
	}
	$self-> charOffset( $self->{n_clusters} )
		if $self-> {charOffset} > $self->{n_clusters};
	$self-> set_selection(0,0);
	$self-> reset;
	$self-> repaint;
	$self-> notify(q(Change));
}

sub find_word_offset
{
	my ( $self, $offset, $right, $caplen, $delta) = @_;

	my $orgd = $delta;
	if ( $offset + $delta > 0 && $offset + $delta < $caplen)
	{
		my $w = $self-> {wordDelimiters};
		if ( $right )
		{
			if ($w !~ quotemeta($self->char_at($offset)))
			{
				$delta++ while (($w !~ quotemeta( $self->char_at( $offset + $delta) // '')) &&
					( $offset + $delta < $caplen));
			}
			if ( $offset + $delta < $caplen)
			{
				$delta++ while (( $w =~ quotemeta( $self->char_at( $offset + $delta) // '')) &&
					( $offset + $delta < $caplen));
			}
		} else {
			if ( $w =~ quotemeta( $self->char_at( $offset - 1)))
			{
				$delta-- while (( $w =~ quotemeta( $self->char_at( $offset + $delta - 1) // '')) &&
					( $offset + $delta > 0));
			}
			if ( $offset + $delta > 0)
			{
				$delta-- while (( $w !~ quotemeta( $self->char_at( $offset + $delta - 1) // '')) &&
					( $offset + $delta > 0));
			}
		}
	}

	return $delta;
}

sub handle_input
{
	my ( $self, $what ) = @_;
	my %opt = (
		at         => $self->charOffset,
		text       => $self->text,
		glyphs     => $self->{glyphs},
		n_clusters => $self->{n_clusters},
		rtl        => $self->{textDirection}
	);
	if ( $what eq 'backspace') {
		$opt{action} = $what;
	} elsif ( $what eq 'delete') {
		$opt{action} = $self->insertMode ? 'delete' : 'cut';
	} elsif ( $self->insertMode ) {
		@opt{qw(action input)} = (q(insert), $what);
	} else {
		@opt{qw(action input)} = (q(overtype), $what);
	}
	my ($new_text, $new_offset) = $self-> handle_bidi_input(%opt);
	if (defined $new_text) {
		if ( $self-> maxLen >= 0 and length($new_text) > $self-> maxLen) {
			$self-> event_error;
			return;
		}
		$self-> edit_text( $new_text)
	}
	$self-> charOffset(
		$self->{glyphs}->index2cluster(
			$new_offset,
			$opt{action} =~ /^(insert|overtype)$/
		)
	) if defined $new_offset;
}

sub on_keydown
{
	my ( $self, $code, $key, $mod) = @_;
	return if $mod & km::DeadKey;

	my $is_unicode = $mod & km::Unicode;
	$mod &= ( km::Shift|km::Ctrl|km::Alt);
	$self-> notify(q(MouseUp),0,0,0) if defined $self-> {mouseTransaction};
	my $offset = $self-> charOffset;
	my $cap    = $self-> text;
	my $caplen = $self-> {n_clusters};

	# navigation part
	if ( scalar grep { $key == $_ } (kb::Left,kb::Right,kb::Home,kb::End))
	{
		return if $mod & km::Alt;
		my $delta = 0;
		if    ( $key == kb::Left)  { $delta = -1;}
		elsif ( $key == kb::Right) { $delta = 1;}
		elsif ( $key == kb::Home)  { $delta = -$offset;}
		elsif ( $key == kb::End)   { $delta = $caplen - $offset;}

		if (( $mod & km::Ctrl) && ( $key == kb::Left || $key == kb::Right)) {
			$delta = $self->find_word_offset($offset, $key == kb::Right, $caplen, $delta);
		}

		if (( $offset + $delta >= 0) && ( $offset + $delta <= $caplen))
		{
			if ( $mod & km::Shift)
			{
				my ( $start, $end) = $self-> selection;
				($start, $end) = ( $offset, $offset) if $start == $end;
				if ( $start == $offset)
				{
					$start += $delta;
				} else {
					$end += $delta;
				}
				$self-> {autoAdjustDisabled} = 1;
				$self-> selection( $start, $end);
				delete $self-> {autoAdjustDisabled};
			} else {
				$self-> selection(0,0) if $self-> {selStart} != $self-> {selEnd};
			}
			$self-> charOffset( $offset + $delta);
			$self-> clear_event;
		} else {
			# boundary exceeding:
			$self-> clear_event unless $self-> {autoTab};
		}
		return;
	}

	if ( $key == kb::Insert && $mod == 0)
	{
		$self-> insertMode( !$self-> insertMode);
		$self-> clear_event;
		return;
	}

# edit part
	my ($start, $end) = $self->selection;
	($start, $end) = ($offset, $offset) if $start == $end;
	my ($p_start, $p_end) = $self-> selection_strpos;

	if ( $key == kb::Backspace)
	{
		return if $self-> {readOnly};

		$self-> begin_undo_group;
		if ( $p_start != $p_end) {
			substr( $cap, $p_start, $p_end - $p_start) = '';
			$self-> set_selection(0,0);
			$self-> edit_text( $cap);
			$self-> charOffset( $start);
		} else {
			$self-> handle_input(q(backspace));
		}
		$self-> end_undo_group;
		$self-> clear_event;
		return;
	}

	if ( $key == kb::Delete)
	{
		return if $self-> {readOnly};

		my $del;
		$self-> begin_undo_group;
		if ( $p_start != $p_end)
		{
			$del = substr( $cap, $p_start, $p_end - $p_start, '');
			$self-> set_selection(0,0);
			$self-> edit_text( $cap);
			$self-> charOffset( $start);
		} else {
			$self-> handle_input(q(delete));
		}
		$self-> end_undo_group;
		$::application-> Clipboard-> text( $del )
			if $mod & ( km::Ctrl|km::Shift) && defined($del);
		$self-> clear_event;
		return;
	}

	if ( $key == kb::Insert && ( $mod & ( km::Ctrl|km::Shift)))
	{
		if ( $mod & km::Ctrl)
		{
			$self-> copy if $p_start != $p_end;
		} else {
			$self-> push_group_undo_action('text', $self->text);
			$self-> paste;
		}
		$self-> clear_event;
		return;
	}

	if ($code == ord("\cC")) {
		$self-> copy if $p_start != $p_end;
		$self-> clear_event;
		return;
	} elsif ($code == ord("\cA")) {
		$self-> select_all;
		$self-> clear_event;
		return;
	} elsif ($code == ord("\cV")) {
		$self-> push_group_undo_action('text', $self->text);
		$self-> paste;
		$self-> clear_event;
		return;
	} elsif ($code == ord("\cX")) {
		if ( !$self-> {readOnly} && $p_start != $p_end) {
			my $del;
			$del = substr( $cap, $p_start, $p_end - $p_start);
			substr( $cap, $p_start, $p_end - $p_start) = '';
			$self-> begin_undo_group;
			$self-> set_selection(0,0);
			$self-> edit_text( $cap);
			$self-> charOffset( $start);
			$self-> end_undo_group;
			$::application-> Clipboard-> text( $del);
		}
		$self-> clear_event;
		return;
	}

# typing part
	if  (
		!$self-> {readOnly} &&
		( $code >= ord(' ')) &&
		(( $mod  & (km::Alt | km::Ctrl)) == 0) &&
		(( $key == kb::NoKey) || ( $key == kb::Space))
	) {
		my $chr = chr $code;
		$self-> begin_undo_group;
		utf8::upgrade($chr) if $is_unicode;
		my ($curpos, $advance);
		if ( $p_start != $p_end) {
			substr( $cap, $p_start, $p_end - $p_start) = $chr;
			$self-> edit_text( $cap);
			$self-> charOffset($self->{glyphs}->index2cluster($p_start));
		} else {
			$self-> handle_input($chr);
		}
		$self-> selection(0,0);
		$self-> clear_event;
		$self-> end_undo_group;
		return;
	}
}

sub on_popup
{
	my $self = $_[0];
	my $p    = $self-> popup;

	my $sel = $self-> {selStart} != $self-> {selEnd};

	my $c    = $::application-> Clipboard;
	$c-> open;
	my $clip = $c-> format_exists('Text');
	$c-> close;

	$p-> enabled( 'copy',         $sel && not($self-> {writeOnly}));
	$p-> enabled( 'cut',          $sel && not($self-> {writeOnly}));
	$p-> enabled( 'delete',       $sel);
	$p-> enabled( 'paste',        $clip);
	$p-> enabled( 'select_all',   $self->{n_clusters} > 0);
	$p-> enabled( 'undo',         $self->can_undo );
	$p-> enabled( 'redo',         $self->can_redo );
	$p-> checked( 'rtl',          $self-> textDirection );
}

sub default_geom_height
{
	my $self = $_[0];
	return $self-> font-> height + 2 + $self-> {borderWidth} * 2;
}

sub check_auto_size
{
	my $self = $_[0];
	$self-> geomHeight( $self-> default_geom_height )
		if $self-> {autoHeight};
}

sub copy
{
	my $self = $_[0];
	my ( $start, $end) = $self-> selection_strpos;
	return if $start == $end;
	return if $self-> {writeOnly};

	my $cap = $self-> text;
	$::application-> Clipboard-> text( substr( $cap, $start, $end - $start));
}

sub toggle_rtl
{	
	my ( $self, $menu, $value ) = @_;
	$self-> textDirection($value);
}

sub paste
{
	my $self = $_[0];
	return if $self-> {readOnly};
	my $cap = $self-> text;
	my ( $start, $end) = $self-> selection;
	($start, $end) = ( $self-> charOffset, $self-> charOffset) if $start == $end;
	my $s = $::application-> Clipboard-> text;
	return if !defined($s) or length( $s) == 0;

	my ($p_start, $p_end) = ($start == $end) ? 
		(($self->{glyphs}->cursor2offset($self->charOffset, $self->textDirection)) x 2) :
		$self->selection_strpos;
	substr( $cap, $p_start, $p_end - $p_start) = $s;
	$self-> selection(0,0);
	$self-> text( $cap);
	$self-> charOffset( $self->{glyphs}-> index2cluster($p_start + length( $s)));
}

sub delete
{
	my $self = $_[0];
	my ( $start, $end) = $self-> selection_strpos;
	return if $start == $end;

	my $cap = $self-> text;
	substr( $cap, $start, $end - $start) = '';
	$self-> selection(0,0);
	$self-> text( $cap) unless $self-> {readOnly};
}

sub cut
{
	my $self = $_[0];
	my ( $start, $end) = $self-> selection_strpos;
	return if $start == $end;

	my $cap = $self-> text;
	my $del = substr( $cap, $start, $end - $start);
	substr( $cap, $start, $end - $start) = '';
	$self-> selection(0,0);
	$self-> text( $cap) unless $self-> {readOnly};
	$::application-> Clipboard-> text( $del) unless $self-> {writeOnly};
}


sub x2offset
{
	my ( $self, $x) = @_;

 	$x -= $self-> {atDrawX} + $self-> {borderWidth} + 1;
	my $fc = $self->{firstChar};
	return $fc if $x <= 0;
	return $fc + $self-> {n_clusters} if $x >= $self-> {lineWidth};
	return $fc + $self-> {glyphs}-> x2cluster( $self, $x, $fc);
}

sub offset2strpos
{
	my $self = shift;
	my $l  = $self->{n_clusters};
	my @p  = @_;
	my $bd = $self->{glyphs}->indexes;
	my @ret    =  map {
		 ($_ <   0) ? 0 :
		(($_ >= $l) ? $l - 1 : $bd->[$_])
	} @p;
	return $#ret ? @ret : $ret[0];
}

sub char_at
{
	my ( $self, $at ) = @_;
	return undef if $at < 0 || $at >= $self->{n_clusters};
	my ($f, $l) = $self-> {glyphs}-> cluster2range($at);
	return $l ? substr( $self->text, $f, 1) : '';
}

sub has_selection { $_[0]->{selStart} != $_[0]->{selEnd} }

sub selection_strpos
{
	my $self = shift;
	return (0,0) if $self->{selStart} == $self->{selEnd};
	return ($self->{selTextStart}, $self->{selTextEnd});
}

sub on_mousedown
{
	my ( $self, $btn, $mod, $x, $y) = @_;
	return if defined $self-> {mouseTransaction};

	if ( $btn == mb::Middle) {
		my $cp = $::application-> bring('Primary');
		return unless $cp;
		return if $self-> {readOnly};

		my $cap = $self-> text;
		my ( $start, $end) = $self-> selection;
		($start, $end) = ( $self-> charOffset, $self-> charOffset) if $start == $end;
		my $s = $cp-> text;
		return if !defined($s) or length( $s) == 0;

		my ($p_start, $p_end) = $self-> selection_strpos;
		substr( $cap, $p_start, $p_end - $p_start) = $s;
		$self-> selection(0,0);
		$self-> text( $cap);
		$self-> charOffset( $start + length( $s));
		$self-> clear_event;
		return;
	} elsif ( $btn == mb::Right) {
		return;
	}

	my ( $start, $end) = $self-> selection_strpos;
	my $ofs = $self-> x2offset( $x);
	if (
		$start != $end && 
		! $self-> {writeOnly} && 
		!$self->{drag_transaction} && 
		!$self->{drop_transaction} && 
		$ofs >= $start && $ofs < $end 
	) {
		$self-> {drag_transaction} = 1;
		my $act = $self-> begin_drag(
			text       => substr( $self->text, $start, $end - $start),
			actions    => dnd::Copy|( $self->{readOnly} ? 0 : dnd::Move),
			self_aware => 0,
		);
		$self-> {drag_transaction} = 0;
		if ( $act == dnd::Move && !$self->{readOnly}) {
			my $cap = $self->text;
			substr( $cap, $start, $end - $start) = '';
			$self-> selection(0,0);
			$self-> text( $cap);
			$self-> charOffset( $start );
		}
		$self-> selection(0,0) if $act < 0;
	} else {
		$self-> {mouseTransaction} = 1;
		$self-> selection(0,0);
		$self-> charOffset( $ofs );
		$self-> {anchor} = $self-> charOffset;
		$self-> capture(1);
		$self-> clear_event;
	}
}

sub new_offset
{
	my ( $self, $ofs) = @_;
	$self-> {autoAdjustDisabled} = 1;
	$self-> charOffset( $ofs);
	$self-> selection( $self-> {anchor}, $self-> charOffset);
	delete $self-> {autoAdjustDisabled};
}

sub on_mousemove
{
	my ( $self, $mod, $x, $y) = @_;
	$self-> clear_event;
	return unless defined $self-> {mouseTransaction};
	my $border = $self-> {borderWidth};
	my $width  = $self-> width;
	if (( $x >= $border + 1) && ( $x <= $width - $border - 1))
	{
		$self-> new_offset( $self-> x2offset( $x));
		$self-> scroll_timer_stop;
		return;
	}

	my $firstAct = ! $self-> scroll_timer_active;
	$self-> scroll_timer_start if $firstAct;
	return unless $self-> scroll_timer_semaphore;

	$self-> scroll_timer_semaphore(0);
	if ( $firstAct) {
		if ( $x <= $border + $self-> {atDrawX}) {
			$self-> new_offset( $self-> firstChar);
		} else {
			$x = $width - $border if $x > $width - $border;
			$self-> new_offset( $self-> x2offset( $x));
		}
	} else {
		$self-> {autoAdjustDisabled} = 1;
		my $delta = 1;
		my $fw = $self-> {font_width};
		$delta = ($width - $border * 2)/($fw*6) if $width - $border * 2 > $fw * 6;
		$delta = int( $delta);
		my $nSel = $self-> charOffset + $delta * ( $x <= $border ? -1 : 1);
		$nSel = 0 if $nSel < 0;

		$self-> lock;
		$self-> selection( $self-> {anchor}, $nSel);

		my $newFc  = $self-> firstChar + $delta * ( $x <= $border ? -1 : 1);
		my $caplen = $self->{n_clusters};
		$newFc = $caplen - $delta if $newFc + $delta > $caplen;

		$self-> firstChar ( $newFc);
		$self-> charOffset( $nSel);
		$self-> unlock;

		delete $self-> {autoAdjustDisabled};
	}
}

sub on_mouseup
{
	my ( $self, $btn, $mod, $x, $y) = @_;

	return unless defined $self-> {mouseTransaction};

	delete $self-> {mouseTransaction};
	$self-> scroll_timer_stop;
	$self-> capture(0);

	return if $self-> {writeOnly};

	my $cp = $::application-> bring('Primary');
	return unless $cp;

	my ( $start, $end) = $self-> selection_strpos;
	$cp-> text(substr( $self-> text, $start, $end - $start)) if $start != $end;
}

sub on_size
{
	my $self = $_[0];
	$self-> reset;
	$self-> firstChar( $self-> firstChar) if $self->{alignment} != ta::Left;
}

sub on_fontchanged
{
	my $self = shift;

	my $font = $self-> font;
	$self-> {font_height} = $font-> height;
	$self-> {font_width} = $font-> width;

	$self-> check_auto_size;
	$self-> reset;
}

sub set_alignment
{
	my ( $self, $align) = @_;

	$align = ta::Left if
		$align != ta::Left &&
		$align != ta::Right &&
		$align != ta::Center;
	$self-> {alignment} = $align;

	$self-> reset;
	$self-> repaint;
}

sub set_border_width
{
	my ( $self, $width) = @_;

	$width = 0 if $width < 0;
	$self-> {borderWidth} = $width;

	$self-> check_auto_size;
	$self-> reset;
	$self-> repaint;
}

# see how many clusters can be displayed
# so that $last is the last one
sub fit_clusters_back
{
	my ( $self, $last ) = @_;
	$last //= $self->{n_clusters} - 1;
	$last = $self->{n_clusters} - 1 if $last > $self->{n_clusters} - 1;
	return 0 if $last <= 0;

	my $w   = $self-> width - ( $self->borderWidth + 1) * 2;
	my $sub = $self->{glyphs}->get_sub( 0, $last + 1)->reverse;
	return $sub-> sub_text_wrap( $self, 0, undef, $w, tw::ReturnFirstLineLength);
}

sub set_char_offset
{
	my ( $self, $offset) = @_;

	my $l   = $self->{n_clusters};
	$offset = $l if $offset > $l;
	$offset = 0 if $offset < 0;
	return if $self-> {charOffset} == $offset;

	$self->{glyphs}->cursor2offset($offset, $self->textDirection);

	$self-> push_undo_action( 'charOffset', $offset) unless $self->has_undo_action('charOffset');

	my $border = $self-> {borderWidth};
	$self-> {charOffset} = $offset;
	my $w = $self-> width - ( $border + 1) * 2;
	my $fc = $self-> {firstChar};
	if ( $fc > $offset) {
		$self-> firstChar($offset);
	} else {
		my $len = $offset - $fc;
		my $gap = ($len > 0) ? $self->{glyphs}-> get_sub_width($self, $fc, $len) : 0;
		if ( $gap > $w ) {
			$self-> firstChar($offset - $self-> fit_clusters_back($offset));
		} else {
			$self-> reset_cursor;
		}
	}
}

sub set_max_len
{
	my ( $self, $len) = @_;

	my $cap = $self-> text;
	$len = -1 if $len < 0;
	$self-> {maxLen} = $len;
	$self-> text( substr( $cap, 0, $len)) if $len >= 0 and length($cap) > $len;
}

sub set_first_char
{
	my ( $self, $pos) = @_;

	my $l = $self-> {n_clusters};
	$pos = $l if $pos > $l;
	$pos = 0 if $pos < 0;
	$pos = 0 if
		( $pos > 0 ) &&
		( $self-> {alignment} != ta::Left) &&
		( $self-> {glyphs}-> get_width($self) <= $self-> width - $self-> {borderWidth} * 2 - 2);
	my $ofc = $self-> {firstChar};
	if ( $pos > 0 && $self-> textDirection ) {
		my $back = $self-> fit_clusters_back;
		$pos = $self->{n_clusters} - $back if $back + $pos > $self->{n_clusters};
	}
	return if $self-> {firstChar} == $pos;
	$self-> push_undo_action( 'firstChar', $pos);
	$self-> {firstChar} = $pos;
	$self-> reset;
	$self-> repaint;
}

sub set_write_only
{
	my ( $self, $wo) = @_;
	return if $wo == $self-> {writeOnly};

	$self-> {writeOnly} = $wo;
	$self-> text( $self-> text);
}

sub set_password_char
{
	my ( $self, $pc) = @_;
	return if $pc eq $self-> {passwordChar};

	$self-> {passwordChar} = $pc;
	$self-> text( $self-> text) if $self-> {writeOnly};
}

sub set_insert_mode
{
	my ( $self, $insert) = @_;
	my $oi = $self-> {insertMode};
	$self-> {insertMode} = $insert;
	if ($oi != $insert) {
		$self-> reset;
		$self-> push_undo_action( 'insertMode', $oi);
	}
	$::application-> insertMode( $insert);
}

sub set_selection
{
	my ( $self, $start, $end) = @_;

	my $l = $self->{n_clusters};
	my ( $ostart, $oend) = $self-> selection;
	my $onsel = $ostart == $oend;
	$end   = $l if $end   < 0;
	$start = $l if $start < 0;
	( $start, $end) = ( $end, $start) if $start > $end;
	$start = $l if $start > $l;
	$end   = $l if $end   > $l;
	$start = $end if $start > $end;
	my $old_chunks = $self->{selChunks} // [];
	$self-> {selStart} = $start;
	$self-> {selEnd} = $end;
	$self-> {selChunks} = [];
	$self-> push_group_undo_action('selection', $ostart, $oend);
	return if $start == $end && $onsel;

	my $new_chunks;
	if ( $start != $end ) {
		if ( $start == 0 && $end == $l ) {
			# select all
			$self->{selChunks} = [ 0, $self->{n_clusters} ];
			$self->{selTextStart} = 0;
			$self->{selTextEnd}   = length($self->text);
		} else {
			my ( $s, $e ) = $self-> {glyphs}-> selection2range($start, $end - 1);
			$self->{selTextStart} = $s;
			$self->{selTextEnd}   = $e + 1;
			$self->{selChunks} = $self->{glyphs}->selection_chunks($s,$e);
		}
		$new_chunks = $self->{selChunks};
	} else {
		$new_chunks = [ $self->{n_clusters} ];
		$self->{selTextStart} = $self->{selTextEnd} = 0;
	}

	my $ooffset = $self-> charOffset;
	$self-> charOffset($self->{glyphs}->index2cluster($end))
		if ( $start != $end) && !defined $self-> {autoAdjustDisabled};
	return if ( $start == $ostart && $end == $oend);

	$self-> reset;

	my $border = $self-> {borderWidth};
	my @size = $self-> size;
	my @r = ( $self->{atDrawX} + $border + 2, $self->{atDrawX} + $border + 2 );
	my @invalid_rects;
	my $fc = $self->{firstChar};
	$self-> begin_paint_info;
	$self-> {glyphs}-> selection_walk(
		$self-> {glyphs}-> selection_diff( $old_chunks, $new_chunks ),
		$self->{firstChar}, $self->{n_clusters},
		sub {
			my ( $offset, $length, $changed ) = @_;
			my $dx = $self->{glyphs}->get_sub_width($self, $fc + $offset, $length );
			$r[1] += $dx;
			push @invalid_rects, [ $r[0] - 1, $border + 1, $r[1], $size[1]-$border-1 ]
				if $changed;
			$r[0] = $r[1];
		}
	);
	$self-> end_paint_info;

	$self->invalidate_rect(@$_) for @invalid_rects;
}

sub on_enable  { $_[0]-> repaint; }
sub on_disable { $_[0]-> repaint; }

sub on_leave
{
	my @s = $_[0]-> selection;
	$_[0]-> repaint if $s[0] != $s[1];
}

sub on_enter
{
	my $self = $_[0];

	$self-> insertMode( $::application-> insertMode);

	if ( $self-> {autoSelect}) {
		my @s = $self-> selection;
		$self-> {autoAdjustDisabled} = 1;
		$self-> select_all;
		$self-> {autoAdjustDisabled} = undef;
		my @s2 = $self-> selection;
		$self-> repaint if $s[0] == $s2[0] and $s[1] == $s2[1];
	} else {
		my @s = $self-> selection;
		$self-> repaint if $s[0] != $s[1];
	}
}

sub on_dragbegin
{
	my $self = shift;
	$self->{drop_transaction} = 1;
}

sub on_dragend
{
	my ( $self, $clipboard, $action, $modmap, $x, $y, $ref) = @_;
	$self->{drop_transaction} = 0;
	return unless $clipboard;
	my $cap = $clipboard->text;
	$self->text($cap) if defined $cap;
}

sub select_all { $_[0]-> selection(0,-1); }

sub autoHeight
{
	return $_[0]-> {autoHeight} unless $#_;
	$_[0]-> {autoHeight} = $_[1];
	$_[0]-> check_auto_size;
}

sub textDirection
{
	return $_[0]-> {textDirection} unless $#_;
	my ( $self, $td ) = @_;
	$self-> {textDirection} = $td;
	$self-> text( $self-> text );
	$self-> alignment( $td ? ta::Right : ta::Left );
}

sub edit_text
{
	my ($self, $text) = @_;
	$self-> begin_undo_group;
	$self-> push_undo_action( 'edit_text', $self->text);
	$self-> text($text);
	$self-> end_undo_group;
}

sub autoSelect    {($#_)?($_[0]-> {autoSelect}    = $_[1])                :return $_[0]-> {autoSelect}   }
sub autoTab       {($#_)?($_[0]-> {autoTab}       = $_[1])                :return $_[0]-> {autoTab}      }
sub readOnly      {($#_)?($_[0]-> {readOnly }     = $_[1])                :return $_[0]-> {readOnly }    }
sub wordDelimiters{($#_)?($_[0]-> {wordDelimiters}= $_[1])                :return $_[0]-> {wordDelimiters}}
sub alignment     {($#_)?($_[0]-> set_alignment(    $_[1]))               :return $_[0]-> {alignment}    }
sub borderWidth   {($#_)?($_[0]-> set_border_width( $_[1]))               :return $_[0]-> {borderWidth}  }
sub charOffset    {($#_)?($_[0]-> set_char_offset(  $_[1]))               :return $_[0]-> {charOffset}   }
sub maxLen        {($#_)?($_[0]-> set_max_len  (    $_[1]))               :return $_[0]-> {maxLen   }    }
sub firstChar     {($#_)?($_[0]-> set_first_char(   $_[1]))               :return $_[0]-> {firstChar}    }
sub writeOnly     {($#_)?($_[0]-> set_write_only(   $_[1]))               :return $_[0]-> {writeOnly}    }
sub passwordChar  {($#_)?($_[0]-> set_password_char($_[1]))               :return $_[0]-> {passwordChar} }
sub insertMode    {($#_)?($_[0]-> set_insert_mode  (    $_[1]))           :return $_[0]-> {insertMode}   }
sub selection     {($#_)? $_[0]-> set_selection   ($_[1], $_[2]) : return ($_[0]-> {selStart},$_[0]-> {selEnd})}
sub selStart      {($#_)? $_[0]-> set_selection   ($_[1], $_[0]-> {selEnd}): return $_[0]-> {'selStart'}}
sub selEnd        {($#_)? $_[0]-> set_selection   ($_[0]-> {'selStart'}, $_[1]):return $_[0]-> {'selEnd'}}
sub selText    {
	my( $f, $t) = $_[0]->selection_strpos;
	($#_) ? do {
	my $x = $_[ 0]-> text;
	substr( $x, $f, $t - $f) = $_[ 1];
	$_[0]-> text( $x);
	$_[0]-> set_selection( $f, $f + length $_[ 1]);
	} : return substr( $_[ 0]-> text, $f, $t - $f);
}

1;

=pod

=head1 NAME

Prima::InputLine - standard input line widget

=head2 SYNOPSIS

	use Prima qw(InputLine Application);
	Prima::InputLine-> new( text => 'Hello world!');
	run Prima;

=for podview <img src="inputline.gif" cut=1>

=for html <p><img src="https://raw.githubusercontent.com/dk/Prima/master/pod/Prima/inputline.gif">

=head1 DESCRIPTION

The class provides basic functionality of an input line,
including hidden input, read-only state, selection, and
clipboard operations. The input line text data is
contained in L<text> property.

=head1 API

=head2 Events

=over

=item Change

The notification is called when the L<text> property is changed, either
interactively or as a result of direct call.

=back

=head2 Properties

=over

=item alignment INTEGER

One of the following C<ta::> constants, defining the text alignment:

	ta::Left
	ta::Right
	ta::Center

Default value: C<ta::Left>

=item autoHeight BOOLEAN

If 1, adjusts the height of the widget automatically when its font changes.

Default value: 1

=item autoSelect BOOLEAN

If 1, all the text is selected when the widget becomes focused.

Default value: 1

=item autoTab BOOLEAN

If 1, the keyboard C<kb::Left> and C<kb::Right> commands, if received
when the cursor is at the beginning or at the end of text, and cannot be
mover farther, not processed. The result of this is that the default handler
moves focus to a neighbor widget, in a way as if the Tab key
was pressed.

Default value: 0

=item borderWidth INTEGER

Width of 3d-shade border around the widget.

Default value: 2

=item charOffset INTEGER

Selects the position of the cursor in clusters starting from
the beginning of visual text.

=item firstChar

Selects the first visible cluster of text

=item insertMode BOOLEAN

Governs the typing mode - if 1, the typed text is inserted, if 0, the text overwrites
the old text. When C<insertMode> is 0, the cursor shape is thick and covers the whole
character; when 1, it is of default width.

Default toggle key: Insert

=item maxLen INTEGER

The maximal length of the text, that can be stored into L<text> or typed by the user.

Default value: 256

=item passwordChar CHARACTER

A character to be shown instead of the text letters when L<writeOnly> property value is 1.

Default value: C<'*'>

=item readOnly BOOLEAN

If 1, the text cannot be edited by the user.

Default value: 0

=item selection START, END

Two integers, specifying the beginning and the end of the selected text, in
clusters. A case with no selection is when START equals END.

=item selStart INTEGER

Selects the start of text selection.

=item selEnd INTEGER

Selects the end of text selection.

=item textDirection BOOLEAN

If set, indicates RTL text input.

=item wordDelimiters STRING

Contains string of character that are used for locating a word break.
Default STRING value consists of punctuation marks, space and tab characters,
and C<\xff> character.

=item writeOnly BOOLEAN

If 1, the input is not shown but mapped to L<passwordChar> characters.
Useful for a password entry.

Default value: 0

=back

=head2 Methods

=over

=item copy

Copies the selected text, if any, to the clipboard.

Default key: Ctrl+Insert

=item cut

Cuts the selected text into the clipboard.

Default key: Shift+Delete

=item delete

Removes the selected text.

Default key: Delete

=item paste

Copies text from the clipboard and inserts it in the cursor position.

Default key: Shift+Insert

=item select_all

Selects all text

=back

=head2 Bi-directional input and output

When working on bidirectional texts, or text represented by complex script
shaping, methods C<firstChar>, C<charOffset>, C<selection> etc cannot be used
to calculate text offsets f.ex. via C<substr>. Note that these values are in
clusters, not in characters (see L<Prima::Drawable::Glyphs> for the
description>. Also, selection ranges of bidi text become not straighforward.
Use the following methods whenever text manipulations are needed:

=over

=item char_at OFFSET

Returns character at OFFSET

=item selection_strpos

Returns range of characters covered by the selection.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

L<Prima>, L<Prima::Widget>, F<examples/edit.pl>.

=cut
