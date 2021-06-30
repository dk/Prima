package Prima::Bidi;

use strict;
use warnings;
use base 'Exporter';
use Prima;
our $available;
our $enabled = 0;
our $failure_text;
our $language;
our $default_direction_rtl;

language($ENV{LANG} // Prima::Application->get_system_info->{guiLanguage});

our @methods = qw(
	language

	paragraph
	map
	revmap
	visual
	selection_map
	selection_diff
	selection_walk
	selection_chunks
	edit_insert
	edit_delete
	map_find
);

our @EXPORT_OK = ( qw(
	is_bidi is_complex
), map { "bidi_$_" } @methods);
{ local $_; eval "sub bidi_$_ { shift; goto &$_ }" for @methods; }

enabled(1);

sub import
{
	my $package = shift;
	my @other;
	for my $p ( @_ ) {
		if ( $p eq ':require' ) {
			my $error = enabled(1);
			die $error if $error;
		} elsif ( $p eq ':methods') {
			$package->export_to_level(1, __PACKAGE__, qw(is_bidi is_complex), map { "bidi_$_" } @methods);
		} else {
			push @other, $p;
		}
	}
	if ( @other ) {
		@_ = ($package, @other);
		goto &Exporter::import;
	}
}

sub enabled
{
	return $enabled unless @_;
	return $enabled = 0 unless $_[0];
	unless ( defined $available ) {
		eval "use Text::Bidi::Paragraph; use Text::Bidi::Constants;";
		if ( $@ ) {
			$failure_text = "Bi-directional text services not available: $@\n";
			$available = $enabled = 0;
			return $failure_text;
		}
		$available = 1;
	}
	return ( $enabled = $available ) ? undef : $failure_text;
}

sub language
{
	return $language unless @_;
	$language = shift;

	$default_direction_rtl = ( $language =~ /^(
		ar| # arabic
		dv| # divehi
		fa| # persian (farsi)
		ha| # hausa
		he| # hebrew
		iw| # hebrew (old code)
		ji| # yiddish (old code)
		ps| # pashto, pushto
		ur| # urdu
		yi  # yiddish
	)/x ? 1 : 0);
}

sub visual { scalar paragraph(@_) }

sub paragraph
{
	my ( $text, $rtl, $flags ) = @_;
	my $p = Text::Bidi::Paragraph->new( $text,
		defined($rtl) ? (
			dir => $rtl ? $Text::Bidi::Par::RTL : $Text::Bidi::Par::LTR
		) : ()
	);
	my $off = 0;
	my $width = $p->len;
	my @text;
	while ( $off < $width ) {
		my $v = $p->visual($off, $width, $flags);
		my $l = length($v);
		$off += $l;
		push @text, $v;
	}
	return ($p, join("\n", @text));
}

sub map_find
{
	my ($map, $index) = @_;
	for ( my $i = 0; $i < @$map; $i++) {
		return $i if $map->[$i] == $index;
	}
	return undef;
}

sub _par
{
	my ( $text, @opt ) = @_;
	my $p = Text::Bidi::Paragraph->new( $text, @opt );
	my $off = 0;
	my $width = $p->len;
	while ( $off < $width ) {
		my $v = $p->visual($off, $width);
		my $l = length($v);
		$off += $l;
	}
	return $p;
}

sub map { _par(@_)->map }

sub revmap
{
	my $map = shift;
	return $map unless ref $map;
	my @newmap = (0) x @$map;
	for (my $i = 0; $i < @$map; $i++) { $newmap[ $map->[$i] ] = $i }
	return \@newmap;
}

sub selection_map
{
	my $text = shift;
	return is_bidi($text) ? _par($text)->map : length($text);
}

sub selection_chunks
{
	my ( $map, $start, $end, $offset ) = @_;
	$offset //= 0;
	my @selection_map;
	return [0] if $start > $end || $offset > $end;
	unless ( ref $map ) {
		for ( my $i = $offset; $i < $map; $i++) {
			push @selection_map, ( $i >= $start && $i <= $end ) ? 1 : 0;
		}
		return _map2chunks( \@selection_map );
	}

	return [0] if $offset > @$map;
	$start = 0      if $start < 0;
	$end   = 0      if $end   < 0;
	$start = $#$map if $start > $#$map;
	$end   = $#$map if $end   > $#$map;
	my ($text_start, $text_end) = @$map[$start, $end];
	($text_start, $text_end) = ($text_end, $text_start) if $text_start > $text_end;
	for ( my $i = $offset; $i < @$map; $i++) {
		push @selection_map, ($map->[$i] >= $text_start && $map->[$i] <= $text_end) ? 1 : 0;
	}
	# warn "$start:$end > $text_start:$text_end > @selection_map\n";

	return _map2chunks( \@selection_map );
}

sub _map2chunks
{
	my $selection_map = shift;

	my @chunks;
	push @chunks, 0 if $selection_map->[0];
	my $last_selected = -1;
	for my $selected ( @$selection_map ) {
		if ( $selected == $last_selected ) {
			$chunks[-1]++;
		} else {
			push @chunks, 1;
			$last_selected = $selected;
		}
	}

	return \@chunks;
}

sub _chunks2map
{
	my @map;
	selection_walk( shift, 0, undef, sub {
		my ( $offset, $length, $selected ) = @_;
		push @map, ($selected) x $length;
	});
	return \@map;
}

sub selection_diff
{
	my ( $old, $new) = map { _chunks2map($_) } @_;
	my @diff;
	my $max = ( @$old > @$new ) ? @$old : @$new;
	for ( my $i = 0; $i < $max; $i++) {
		$diff[$i] = (($old->[$i] // 0) == ($new->[$i] // 0) ) ? 0 : 1 ;
	}
	return _map2chunks( \@diff );
}

sub selection_walk
{
	my ( $selection_chunks, $from, $to, $sub ) = @_;
	my ( $ptr, $selected ) = (0,1);
	for my $chunk ( @$selection_chunks ) {
		$selected = not $selected;
		my $offset = $ptr - $from;
		$ptr += $chunk;
		my $length = $chunk;
		if ( $offset < 0 ) {
			$length += $offset;
			$offset = 0;
		}
		if ( defined $to ) {
			last if $offset >= $to;
			$length = $from - $offset if $offset + $length > $to;
		}
		next if $length <= 0;
		$sub->( $offset, $length, $selected );
	}
}

sub is_bidi        { $_[-1] =~ m/[\p{bc=R}\p{bc=AL}\p{bc=AN}\p{bc=RLE}\p{bc=RLO}]/ }

eval 'sub is_complex { $_[-1] =~ m/[' .
	join('', map { sprintf q(\\x{%04x}-\\x{%04x}), @$_ } (
		[0x0300,0x036f], # Diacritical
		[0x0590,0x05ff], # Hebrew
		[0x0600,0x06ef], # Arabic
		[0x06f0,0x06f9], # Persian
		[0x06fa,0x06ff], # Arabic
		[0x0700,0x074f], # Syriac
		[0x0750,0x077f], # Arabic
		[0x0780,0x07bf], # Thaana
		[0x0900,0x097f], # Devanagari
		[0x0980,0x09ff], # Bengali
		[0x0a00,0x0a7f], # Gurmukhi
		[0x0a80,0x0aff], # Gujarati
		[0x0b00,0x0b7f], # Oriya
		[0x0b80,0x0bff], # Tamil
		[0x0c00,0x0c7f], # Telugu
		[0x0c80,0x0cff], # Kannada
		[0x0d00,0x0d7f], # Malayalam
		[0x0d80,0x0dff], # Sinhala
		[0x0e00,0x0e7f], # Thai
		[0x0e80,0x0eff], # Lao
		[0x0f00,0x0fff], # Tibetan
		[0x1100,0x11ff], # Hangul
		[0x1800,0x18af], # Mongolian
		[0x1cd0,0x1cff], # Devanagari
		[0x1dc0,0x1dff], # Diacritical
		[0x2800,0x28ff], # Braille
		[0x3130,0x318f], # Hangul
		[0x3200,0x321f], # Hangul
		[0x3260,0x327f], # Hangul
		[0xa840,0xa87f], # Phags_pa
		[0xa8e0,0xa8ff], # Devanagari
		[0xa960,0xa97f], # Hangul
		[0xac00,0xd7a3], # Hangul
		[0xd7b0,0xd7ff], # Hangul
		[0xfb1d,0xfb4f], # Hebrew
		[0xfb50,0xfdff], # Arabic
		[0xfe70,0xfeff], # Arabic
		[0xffa0,0xffdf], # Hangul
	)) . ']/x};';

sub is_strong($)   { $_[-1] =~ m/^[\p{bc=L}\p{bc=R}\p{bc=AL}]$/ }
sub is_weak($)     { $_[-1] =~ m/^[\p{bc=EN}\p{bc=ES}\p{bc=ET}\p{bc=AN}\p{bc=CS}\p{bc=NSM}\p{bc=BN}]$/ }
sub is_neutral($)  { $_[-1] =~ m/^[\p{bc=B}\p{bc=S}\p{bc=WS}\p{bc=ON}]$/ }
sub is_explicit($) { $_[-1] =~ m/^[\p{bc=LRE}\p{bc=LRO}\p{bc=RLE}\p{bc=RLO}\p{bc=PDF}\p{bc=LRI}\p{bc=RLI}\p{bc=FSI}\p{bc=PDI}]$/ }
sub is_rtl($)      { $_[-1] =~ m/^[\p{bc=R}\p{bc=AL}\p{bc=RLE}\p{bc=RLO}\p{bc=RLI}]$/ }
sub is_ltr($)      { $_[-1] =~ m/^[\p{bc=L}\p{bc=LRE}\p{bc=LRO}\p{bc=LRI}]$/ }

sub edit_insert
{
	my ( $src_p, $visual_pos, $new_str ) = @_;

	return $visual_pos, 0 unless length $new_str;

	unless ($src_p) {
		# empty string or non-bidi
		my $rtl = $enabled && is_bidi($new_str);
		return $visual_pos, $rtl ? 0 : length($new_str);
	}

	my $new_p     = _par($new_str);
	my $map       = $src_p->map;
	my $t         = $src_p->types;
	my $new_map   = $new_p->map;
	my $new_types = $new_p->types;
	my $new_type  = 0;
	my $limit     = $#$map;

	my ($tl,$tr,$pl,$pr);
	if ( $visual_pos > 0 ) {
		$tl = $t->[$pl = $map->[$visual_pos - 1]];
	}
	if ( $visual_pos <= $limit ) {
		$tr = $t->[$pr = $map->[$visual_pos]];
	}

	# Cursor between two strongs
	if ( defined($tl) && defined($tr) && is_strong $tl && is_strong $tr ) {
		if ( !is_rtl $tl ) {
			if ( !is_rtl $tr ) {
				# normal LTR
				return $pl + 1, 1;
			} else {
				# Cursor between R and L - the rightmost wins
				return ( $pl > $pr ) ?
					($pl + 1, 1) :
					($pr + 1, 0);
			}
		} else {
			if ( is_rtl $tr) {
				# normal RTL
				return $pr + 1, 0;
			} else {
				# Cursor between L and R - the leftmost wins
				return ( $pl < $pr ) ?
					($pl + 1, 1) :
					($pr + 1, 0);
			}
		}
	}

	# Cursor next to a strong
	if ( defined($tl) && is_strong $tl) {
		return (!is_rtl $tl) ?
			# LTR append
			( $pl + 1, 1 ) :
			# RTL prepend
			( $pl, 0 );
	}
	if ( defined($tr) && is_strong $tr) {
		return (!is_rtl $tr) ?
			# LTR prepend
			( $pr, 0 ) :
			# RTL append
			( $pr + 1, 0 );
	}

	# find out dominant directions by scanning to the first strong at each direction, if any
	if ( defined $tl ) {
		my $vp = $visual_pos - 1;
		$vp-- while $vp >= 0 && !is_strong($tl = $t->[$map->[$vp]]);
		# right to a weak, adjacent to a strong LTR further right
		return $pl + 1, 1 if is_strong $tl && !is_rtl $tl;
	}
	if ( defined $tr ) {
		my $vp = $visual_pos;
		$vp++ while $vp < $limit && !is_strong($tr = $t->[$map->[$vp]]);
		# left to a weak, adjacent to a strong LTR further left
		return $pr, 0     if is_strong $tr && !is_rtl $tr;
	}

	# cursor at the end
	return !is_rtl $tr ? (0, 0) : ($limit + 1, 0) unless defined $tl;
	return !is_rtl $tl ? ($limit + 1, 1) : (0, 0) unless defined $tr;

	# XXX this is too complex, give up
	return $visual_pos, is_bidi($new_str) ? 0 : length($new_str);
}

sub edit_delete
{
	my ( $p, $visual_pos, $backspace ) = @_;

	# non-bidi compatibility
	unless (ref $p) {
		if ( $backspace ) {
			return ($visual_pos > 0) ? (1, $visual_pos - 1, -1) : (0, 0, 0);
		} else {
			return ($visual_pos < $p) ? (1, $visual_pos, 0) : (0, 0, 0);
		}
	}

	my $map   = $p->map;
	my $t     = $p->types;
	my $limit = $#$map;

	my ($il,$ir,$l,$r,$pl,$pr) = (0,0,0,0,$limit,0);

	if ( $visual_pos > 0 ) {
		$il = $t->[$map->[$pl = $visual_pos - 1]];
		$pl-- while !is_strong($l = $t->[$map->[$pl]]) && $pl > 0;
	}
	if ( $visual_pos <= $limit ) {
		$ir = $t->[$map->[$pr = $visual_pos]];
		$pr++ while !is_strong($r = $t->[$map->[$pr]]) && $pr < $limit;
	}

	#warn "il: ", (is_strong($il) ? 'strong' : 'weak'), ' ', (is_rtl($il) ? 'rtl' : 'ltr'), " at ", $visual_pos - 1, "\n";
	#warn "ir: ", (is_strong($ir) ? 'strong' : 'weak'), ' ', (is_rtl($ir) ? 'rtl' : 'ltr'), " at ", $visual_pos, "\n";
	#warn "l: ", (is_strong($l) ? 'strong' : 'weak'), ' ', (is_rtl($l) ? 'rtl' : 'ltr'), " at $pl\n";
	#warn "r: ", (is_strong($r) ? 'strong' : 'weak'), ' ', (is_rtl($r) ? 'rtl' : 'ltr'), " at $pr\n";
	#warn "vp: $visual_pos, limit: $limit\n";

	if ( $backspace ) {
		# strong ltr immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0       && is_strong $il && !is_rtl $il;
		# strong rtl immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_strong $ir && is_rtl $ir;
		# any ltr immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0 && !is_rtl $il;
		# any rtl on right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_rtl $r;
		# any rtl on left, kill greedy leftmost
		if ($visual_pos > 0 && is_rtl $l) {
			my $L = $l;
			$pl-- while !(is_strong($L = $t->[$map->[$pl]]) && !is_rtl $L) && $pl > 0;
			return 1, $map->[$pl], $pl - $visual_pos
		}
	} else {
		# strong ltr immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && is_strong $il && !is_rtl $il;
		# strong rtl immediately left, kill immediately left
		return 1, $map->[$visual_pos - 1], 0  if $visual_pos > 0       && is_strong $ir && is_rtl $ir;
		# any ltr immediately right, kill immediately right
		return 1, $map->[$visual_pos], 0      if $visual_pos <= $limit && !is_rtl $ir;
		# any rtl on left, kill immediately left
		return 1, $map->[$visual_pos - 1], -1 if $visual_pos > 0 && is_rtl $l;
		# any ltr on right, kill greedy rightmost
		if ($visual_pos <= $limit && !is_rtl $r) {
			my $R = $r;
			$pr-- while !(is_strong($R = $t->[$map->[$pr]]) && is_rtl $R) && $pr > 0;
			return 1, $map->[$pr], $pr - $visual_pos
		}
	}

	# nothing
	return 0, 0, 0;
}

sub debug_str
{
	return unless $enabled;
	my $str = shift;
	my $p = _par($str);
	my $t = $p->types;
	my $b = $p->bd;
	for ( my $i = 0; $i < length($str); $i++) {
		my $chr = ord( substr( $str, $i, 1));
		my $typ = $t->[$i];
		my $tn  = $b->get_bidi_type_name($typ);
		my @mas;
		no strict 'refs';
		for my $name ( keys %Text::Bidi::Mask::) {
			my $value = ${"Text::Bidi::Mask::$name"};
			next unless $typ & $value;
			push @mas, $name;
		}
		printf("$i: %03x: %06x / %s\n", $chr, $typ, join(',', @mas));
	}
}

1;

=pod

=head1 NAME

Prima::Bidi - helper routines for bi-directional text input and output

=head1 SYNOPSIS

=encoding utf-8

=for latex-makedoc header
\usepackage{amsmath,amssymb}
\DeclareFontFamily{U}{rcjhbltx}{}
\DeclareFontShape{U}{rcjhbltx}{m}{n}{<->rcjhbltx}{}
\DeclareSymbolFont{hebrewletters}{U}{rcjhbltx}{m}{n}
\DeclareMathSymbol{\alef}{\mathord}{hebrewletters}{39}
\DeclareMathSymbol{\pe}{\mathord}{hebrewletters}{112}
\DeclareMathSymbol{\samekh}{\mathord}{hebrewletters}{115}

=begin latex-makedoc

=begin latex

\begin{tt}
~ ~\\
\hspace*{1.5em}use Prima::Bidi qw(:enable is\_bidi);\\
\hspace*{1.5em}\$bidi\_text = "'$\alef\pe\samekh123$'";\\
\hspace*{1.5em}say Prima::Bidi::visual( \$bidi\_text ) if is\_bidi(\$bidi\_text);\\
 \\
\hspace*{1.5em}'123$\samekh\pe\alef$'\\
\end{tt}

=end latex

=end latex-makedoc

=for latex-makedoc cut

   use Prima::Bidi qw(:enable is_bidi);
   $bidi_text = "'אפס123'";
   say Prima::Bidi::visual( $bidi_text ) if is_bidi($bidi_text);

   '123ספא'

=for latex-makedoc cut

or same, for classes

   use Prima::Bidi qw(:methods);
   say $self->bidi_visual( $bidi_text ) if $self-> is_bidi($bidi_text);

=head1 API

The API follows closely L<Text::Bidi> api, with view to serve as a loose set of
helper routines for input and output in Prima widgets. It also makes use of
installations without C<Text::Bidi> safe. Exports set of C<bidi_XXX> names,
available as method calls.

=over

=item is_bidi $TEXT

Returns boolean flags whether the text contains any bidirectional characters.

=item bidi_map $TEXT, ...

Shortcut for C<< Text::Bidi::Paragraph->new($TEXT, ...)->map >>.

Returns a set of integer indices, showing placement of where in original bidi
text a corresponding character can be found, i.e. C<$map[0]> contains the index of
the character to be displayed leftmost, etc.

This function could be useful f.ex. for translating screen position to text position.

See L<Text::Bidi::Paragraph/map> for more.

=item bidi_paragraph $TEXT, $RTL, $FLAGS

Returns a C<Text::Bidi::Paragraph(dir => $RTL)> object together with result of
call to C<visual($FLAGS)>.

=item bidi_revmap $MAP

Returns an inverse array of result of C<map>, i.e. showing where, if any, a
bidi character is to be displayed in visual text, so that f.ex. C<$revmap[0]>
contains visual position of character #0, etc.

=item bidi_edit_delete $PARAGRAPH, $VISUAL_POSITION, $DELETE

Handles bidirectional deletion, emulating user hitting a backspace (C<$DELETE =
0>) or delete (C<$DELETE = 1>) key at C<$VISUAL_POSITION> in text represented
by a C<Text::Bidi::Paragraph> object.

Returns three integers, showing 1) how many characters are to be deleted, 2) at
which text offset, and 3) how many characters to the right the cursor has to
move.

C<$PARAGRAPH> can be a non-object, in which case the text is considered to be non-bidi.

=item bidi_edit_insert $PARAGRAPH, $VISUAL_POSITION, $NEW_STRING

Handles typing of bidirectional text C<$NEW_STRING>, inside an existing
C<$PARAGRAPH> represented by a C<Text::Bidi::Paragraph> object, where cursor is
a C<$VISUAL_POSITION>.

C<$PARAGRAPH> can be a non-object, in which case the text is considered to be non-bidi.

=item bidi_map_find $MAP, $INDEX

Searches thround C<$MAP> (returned by C<bidi_map>) for integer C<$INDEX>, returns
its position if found.

=item bidi_selection_chunks $MAP, $START, $END, $OFFSET = 0

Calculates a set of chunks of texts, that, given a text selection from
positions C<$START> to C<$END>, represent each either a set of selected and non-selected
visual characters. The text is represented by a result of C<bidi_map>.

Returns array of integers, RLE-encoding the chunks, where the first integer
signifies number of non-selected characters to display, the second - number
of selected characters, the third the non-selected again, etc. If the first
character belongs to the selected chunk, the first integer in the result is set
to 0.

C<$MAP> can be also an integer length of text (i.e. shortcut for an identity
array (0,1,2,3...) in which case the text is considered to be non-bidi, and
selection result will contain max 3 chunks).

C<$OFFSET> may be greater that 0, but less than C<$START>, if that information
is not needed.

Example: consider embedded number in a bidi text. For the sake of clarity I'll use
latin characters here. For example, we have a text scalar containing these characters:

   ABC123

where I<ABC> is right-to-left text, and which, when rendered on screen, should be
displayed as

   123CBA

(and C<$MAP> will be (3,4,5,2,1,0) ).

Next, the user clicks the mouse between A and B (in text offset 1), drags the
mouse then to the left, and finally stops between characters 2 and 3 (text
offset 4). The resulting selection then should not be, as one might naively expect,
this:

   123CBA
   __^^^_

but this instead:

   123CBA
   ^^_^^_

because the next character after C is 1, and the I<range> of the selected sub-text is from
characters 1 to 4.

In this case, the result of call to C<bidi_selection_chunks( $MAP, 1, 4 )> will be C<0,2,1,2,1> .

=item bidi_selection_diff $OLD, $NEW

Given set of two chunk lists, in format as returned by C<bidi_selection_chunks>, calculates
the list of chunks affected by the selection change. Can be used for efficient repaints when
the user interactively changes text selection, to redraw only the changed regions.

=item bidi_selection_map $TEXT

Same as C<bidi_map>, except when C<$TEXT> is not bidi, returns just the length of
it. Such format can be used to pass the result further to
C<bidi_selection_chunks> efficiently where operations are performed on a
non-bidi text.

=item bidi_selection_walk $CHUNKS, $FROM, $TO = length, $SUB

Walks the selection chunks array, returned by C<bidi_selection_chunks>, between
C<$FROM> and C<$TO> visual positions, and for each chunk calls the provided
C<< $SUB->($offset, $length, $selected) >>, where each call contains 2 integers to
chunk offset and length, and a boolean flag whether the chunk is selected or
not.

Can be also used on a result of C<bidi_selection_diff>, in which case
C<$selected> flag is irrelevant.

=item bidi_visual $TEXT, $RTL, $FLAGS

Same as C<bidi_paragraph> but returns only the rendered text, omitting the
paragraph object.

=back

=head1 AUTHOR

Dmitry Karasik, E<lt>dmitry@karasik.eu.orgE<gt>.

=head1 SEE ALSO

F<examples/bidi.pl>, L<Text::Bidi>

=cut
