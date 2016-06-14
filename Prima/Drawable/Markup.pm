package Prima::Drawable::Markup;

use strict;
use warnings;
use Prima;
our %ESCAPES;

=head1 NAME

Prima::Markup - Allow markup in Prima Widgets

=head1 SYNOPSIS

    use Prima qw(Application Buttons Markup);
    my $markup_button = Prima::MarkupButton->create(...);

=head1 DESCRIPTION

C<Prima::Markup> adds the ability to recognize POD-like markup to Prima widgets.
Currently supported markup sequences are C<B> (bold text), C<I> (italic text),
C<U> (underlined text), C<E> (escape sequences), C<F> (change font), C<S>
(change font size), C<N> (change font encoding), and C<C> (change color).

The C<F> sequence is used as follows: C<FE<lt>n|textE<gt>>, where C<n> is a
0-based index into the C<fontPalette>.

The C<S> sequence is used as follows: C<SE<lt>n|textE<gt>>, where C<n> is the
font size.  The font size may optionally be preceded by C<+> or C<->, in which
case it is relative to the current font size.

The C<N> sequence is used as follows: C<NE<lt>n|textE<gt>>, where C<n> is a
0-based index into the C<encodings>.

The C<C> sequence is used as follows: C<CE<lt>c|textE<gt>>, where C<c> is a color
in any form accepted by Prima, including the C<cl> constants (C<Black> C<Blue>
C<Green> C<Cyan> C<Red> C<Magenta> C<Brown> C<LightGray> C<DarkGray> C<LightBlue>
C<LightGreen> C<LightCyan> C<LightRed> C<LightMagenta> C<Yellow> C<White> C<Gray>).

The methods C<text_out> and C<get_text_width> are affected by C<Prima::Markup>.
C<text_out> will write formatted text to the canvas, and C<get_text_width> will
return the width of the formatted text.  B<NOTE>: These methods do not save state
between calls, so your markup cannot span lines (since each line is drawn or
measured with a separate call).

=cut

sub new {
	my ($class, $text, @rest) = @_;
	return bless {
		text => $text,
		@rest,
	}, $class;
}

sub clone
{
	my ( $self, @opt) = @_;
	my %opt = ( %$self, @opt);
	my $text = delete $opt{text};
	return ref($self)->new( $text, %opt );
}

sub walk 
{
	my ( $self, %opt ) = @_;
	my $text = $self-> {text};
	my (@font_stack, @color_stack, @cmd_stack, @delim_stack, $escape);
	my @tokens = split /([A-Z]<(?:<+\s+)?)/, $text;
	my ( $font_changed, $color_changed) = ( 0,0 );
	my %font  = %{ $opt{default_font} || { encoding => '', size => 0, style => 0 } };
	my $color = $opt{default_color} || 0;

	my $apply = sub {
		if ( $color_changed ) {
			$color_changed = 0;
			$opt{color}->( $color ) if $opt{color};
		}
		if ( $font_changed ) {
			$font_changed = 0;
			$opt{font}->( \%font ) if $opt{font};
		}
	};

	while ( @tokens ) {
		my $token = shift @tokens;
		## Look for the beginning of a sequence
		if ( $token=~/^([A-Z])(<(?:<+\s+)?)$/ ) {
			## Push a new sequence onto the stack of those "in-progress"
			my ($cmd, $ldelim) = ($1, $2);
			$ldelim =~ s/\s+$//, (my $rdelim = $ldelim) =~ tr/</>/;
			push @cmd_stack, $cmd;
			push @delim_stack, $rdelim;

			if ($cmd eq 'C') {
				(shift @tokens)=~/^([^|]+)\|(.*)$/;
				unshift @tokens, $2;

				my $c = $1;
				if ( $c =~ /^[0-9a-f]{6}$/ ) {
					$c = hex $c;
				} elsif ( $c =~ /^cl\:\:(.+)$/ && exists($cl::{$1})) {
					$c = &{$cl::{$1}}();
				} elsif ( $c !~ /^\d+$/) {
					warn "Bad color: $c";
					$cmd_stack[-1] = '0';
					next;
				}
				push @color_stack, $color;
				$color = $c;
				$color_changed++;
			}
			elsif ($cmd eq 'F') {
				(shift @tokens)=~/^([^|]+)\|(.*)$/;
				unshift @tokens, $2;
				my $f = $1;
				if ( $f !~ /^\d+$/) {
					warn "Bad fond id: $f";
					$cmd_stack[-1] = '0';
					next;
				}
				$font_changed++;
				push @font_stack, { %font };
				delete @font{qw(width height pitch)};
				%font = ( %font, %{ $self-> fontPalette->[$f] // {} } );
			}
			elsif ($cmd eq 'S') {
				(shift @tokens)=~/^([^|]+)\|(.*)$/;
				unshift @tokens, $2;
				my $s = $1;
				if ($s =~ /^([+-]?)(\d+)$/) {
					$font_changed++;
					push @font_stack, { %font };
					delete @font{qw(width height)};
					if ( $1 eq '+' ) {
						$font{size} += $2;
					} elsif ( $1 eq '-') {
						$font{size} -= $2;
					} else {
						$font{size} = $2;
					}
				} else {
					warn "Bad font size: $s";
					$cmd_stack[-1] = '0';
					next;
				}
			}
			elsif ($cmd eq 'N') {
				(shift @tokens)=~/^([^|]+)\|(.*)$/;
				unshift @tokens, $2;
				my $n = $1;
				unless ($n =~ /^(\d+)$/) {
					warn "Bad encoding id: $n";
					$cmd_stack[-1] = '0';
					next;
				}
				$font_changed++;
				push @font_stack, { %font };
				%font = ( %font, encoding => $self-> encodings->[$n] // $font{encoding} );
			}
			elsif ($cmd eq 'I') {
				$font_changed++;
				push @font_stack, { %font };
				$font{style} |= fs::Italic;
			}
			elsif ($cmd eq 'B') {
				$font_changed++;
				push @font_stack, { %font };
				$font{style} |= fs::Bold;
			}
			elsif ($cmd eq 'U') {
				$font_changed++;
				push @font_stack, { %font };
				$font{style} |= fs::Underlined;
			}
			elsif ($cmd eq 'E') {
				$escape = 1;
			}
		} # end of if block for open sequence
		## Look for sequence ending
		else {
			## Make sure we match the right kind of closing delimiter
			my ($seq_end, $post_seq) = ("", "");
			my $dlm;
			if ( $dlm = $delim_stack[$#delim_stack] and
			       (
			        ($dlm eq '>' and $token=~/\A(.*?)(\>)/s) or 
				  ($dlm ne '>' and $token=~/\A(.*?)(\s{1,}$dlm)/s)
			       )
                        ) {
				my $t = $escape ? escape($self,$1) : $1;
				my $use_encoding;
				if (ref($t) eq 'ARRAY') {
					$font_changed++;
					push @font_stack, { %font };
					%font = ( %font, encoding => $font{encoding} );
					$t = chr($t->[1]);
					$use_encoding++;
				}

				$apply->();

				$opt{text}->($t) if length($t) && $opt{text};

				if ( $use_encoding ) {
					%font = %{ pop @font_stack };
					$font_changed++;
				}

				my $rest = substr($token, length($1) + length($2));
				length($rest) and unshift @tokens, $rest;

				my $cmd = pop @cmd_stack;
				if ($cmd eq 'C') {
					$color = pop @color_stack;
					$color_changed++;
				}
				elsif ($cmd =~ /^[FSNIBU]$/) {
					%font = %{ pop @font_stack };
					$font_changed++;
				}
				elsif ($cmd eq 'E') {
					$escape = 0;
				}
				next;
			} # end of if block for close sequence
			else { # if we get here, we're non-escaped text
				$apply->();
				$opt{text}->($token) if length($token) && $opt{text};
			}
		} # end of else block after if block for open sequence
	} # end of while loop
	$apply->();
}

my $RAD = 57.29577951;

sub text_out
{
	my ($self, $canvas, $x, $y) = @_;
	$self-> walk(
		default_font => $canvas-> font,
		text => sub { 
			my $t = shift;
			$canvas->text_out($t, $x, $y );
			my $width = $canvas-> get_text_width($t);
			if ( $canvas-> font-> direction != 0.0 ) {
				$x += $width * cos( $canvas-> font-> direction / $RAD); 
				$y += $width * sin( $canvas-> font-> direction / $RAD); 
			} else {
				$x += $width;
			}
		},
		color => sub { $canvas-> color( shift ) },
		font  => sub { $canvas-> font( shift ); },
	);
}

sub get_text_width_with_overhangs
{
	my ($self, $canvas) = @_;
	my $first_a_width = 0;
	my $last_c_width = 0;
	my $width = 0;
	$self-> walk(
		default_font => $canvas-> font,
		text => sub { 
			my $t = shift;
			if ( !defined $first_a_width) {
				my $char = substr( $t, 0, 1 );
				( $first_a_width ) = @{ $canvas->get_font_abc(ord($char), ord($char), utf8::is_utf8($t)) };
			}
			my $char = substr( $t, -1, 1 );
			( undef, undef, $last_c_width ) = @{ $canvas->get_font_abc(ord($char), ord($char), utf8::is_utf8($t)) };
			$width += $canvas-> get_text_width($t);
		},
		font  => sub { $canvas-> font( shift ) },
	);
	$first_a_width = ( $first_a_width < 0 ) ? -$first_a_width : 0;
	$last_c_width  = ( $last_c_width  < 0 ) ? -$last_c_width : 0;
	return ($width, $first_a_width, $last_c_width);
}

sub get_text_width
{
	my ( $self, $canvas, $add_overhangs) = @_;
	if ( $add_overhangs ) {
		my ( $width, $a, $c) = $self-> get_text_width_with_overhangs($canvas);
		return $width + $a + $c;
	}
	my $width = 0;
	$self-> walk(
		default_font => $canvas-> font,
		text => sub { $width += $canvas-> get_text_width(shift) },
		font => sub { $canvas-> font( shift ) },
	);
	return $width;
}

sub text_wrap
{
# XXX this is not even close. For serious text wrapping look at Prima::TextView
	my ( $self, $canvas, $width, $opt, $indent) = @_;
	my $ret = $canvas-> text_wrap( $self-> {text}, $width, $opt, $indent);
	unless ( $opt & tw::ReturnChunks ) {
		for my $t ( @$ret ) {
			next if ref $t;
			$t = $self-> clone( text => $t );
		}
	}
	return $ret;
}

sub get_text_box
{
	my ( $self, $canvas, $text) = @_;
	my ( $w, $a, $c) = $self-> get_text_width_with_overhangs($canvas);

	my ( $fa, $fd ) = ( $canvas->font->ascent - 1, $canvas->font->descent );
	
	my @ret = (
		-$a,      $fa,
		-$a,     -$fd,
		$w - $c,  $fa,
		$w - $c, -$fd,
		$w, 0
	);
	unless ( $canvas-> textOutBaseline) {
		$ret[$_] += $fd for (1,3,5,7,9);
	}
	if ( $canvas-> font-> direction != 0.0 ) {
		my $s = sin( $canvas-> font-> direction / $RAD );
		my $c = cos( $canvas-> font-> direction / $RAD );
		my $i;
		for ( $i = 0; $i < 10; $i+=2) {
			my ( $x, $y) = @ret[$i,$i+1];
			$ret[$i]   = $x * $c - $y * $s;
			$ret[$i+1] = $x * $s + $y * $c;
		}
	}
	return \@ret;
}

=head1 PROPERTIES

The following properties are used:

=over 4

=item fontPalette([@fontPalette])

Gets or sets the font palette to be used for C<F> sequences within this widget.
Each element of the array should be a hashref suitable for setting a font.  This
method can also accept an arrayref instead of an array.  If called in list
context, returns an array; if called in scalar context, returns an arrayref.

=cut

sub fontPalette {
	my ($self) = shift;
	if (@_) {
		my @p = @_;
		if (ref($_[0]) eq 'ARRAY') {
			@p = @{ $_[0] };
		}
		$self->{fontPalette} = [ @p ];
	}
	if ($self->{fontPalette}) {
		return @{ $self->{fontPalette} } if (wantarray);
		return $self->{fontPalette};
	}
	return wantarray ? () : [];
}

=item $widget->encodings([@encodings])

Gets or sets the encodings array to be used within C<N> (and possibly C<E>) sequences
within this widget.  See the L<Prima::Application> method C<fonts> for information on
valid values for encodings.  This method can also accept an arrayref instead of an array.
If called in list context, returns an array; if called in scalar context, returns an
arrayref.

=cut

sub encodings {
	my $self = shift;
	if (@_) {
		my @e = @_;
		if (ref($_[0]) eq 'ARRAY') {
			@e = @{ $_[0] };
		}
		$self->{encodings} = [ @e ];
		$self->font->encoding($e[0]);	# set font to index 0
	}
	if ($self->{encodings}) {
		return @{ $self->{encodings} } if (wantarray);
		return $self->{encodings};
	}
	return wantarray ? () : [];
}

=item $widget->escapes([%escapes])

Gets or sets the custom escapes hash for a widget.  The keys in the hash should be the
escape sequences, and the values can be a scalar, a coderef, or an arrayref.  If a
scalar, the scalar will be inserted into the text.  If a coderef, the return value will
be inserted into the text (this is useful for inserting the current date and/or time,
for example).  If an arrayref, the first element is an index into C<encodings> and the
second is a position within that encoding (the current font must be valid for the encoding,
or the character will not display properly).  This method can also accept a hashref
instead of a hash.  If called in list context, returns a hash; if called in scalar context,
returns a hashref.

When an C<E> sequence is encountered, the code first checks the custom escapes hash, then
checks the default escapes hash (which was lifted directly from L<Pod::Text>).

Numeric escapes are automatically converted to the appropriate character; no checking of
either escapes hash is performed.  It accepts either decimal or hexadecimal numbers.

The default escapes are:

    'amp'       =>    '&',      # ampersand
    'lt'        =>    '<',      # left chevron, less-than
    'gt'        =>    '>',      # right chevron, greater-than
    'quot'      =>    '"',      # double quote
                                 
    "Aacute"    =>    "\xC1",   # capital A, acute accent
    "aacute"    =>    "\xE1",   # small a, acute accent
    "Acirc"     =>    "\xC2",   # capital A, circumflex accent
    "acirc"     =>    "\xE2",   # small a, circumflex accent
    "AElig"     =>    "\xC6",   # capital AE diphthong (ligature)
    "aelig"     =>    "\xE6",   # small ae diphthong (ligature)
    "Agrave"    =>    "\xC0",   # capital A, grave accent
    "agrave"    =>    "\xE0",   # small a, grave accent
    "Aring"     =>    "\xC5",   # capital A, ring
    "aring"     =>    "\xE5",   # small a, ring
    "Atilde"    =>    "\xC3",   # capital A, tilde
    "atilde"    =>    "\xE3",   # small a, tilde
    "Auml"      =>    "\xC4",   # capital A, dieresis or umlaut mark
    "auml"      =>    "\xE4",   # small a, dieresis or umlaut mark
    "Ccedil"    =>    "\xC7",   # capital C, cedilla
    "ccedil"    =>    "\xE7",   # small c, cedilla
    "Eacute"    =>    "\xC9",   # capital E, acute accent
    "eacute"    =>    "\xE9",   # small e, acute accent
    "Ecirc"     =>    "\xCA",   # capital E, circumflex accent
    "ecirc"     =>    "\xEA",   # small e, circumflex accent
    "Egrave"    =>    "\xC8",   # capital E, grave accent
    "egrave"    =>    "\xE8",   # small e, grave accent
    "ETH"       =>    "\xD0",   # capital Eth, Icelandic
    "eth"       =>    "\xF0",   # small eth, Icelandic
    "Euml"      =>    "\xCB",   # capital E, dieresis or umlaut mark
    "euml"      =>    "\xEB",   # small e, dieresis or umlaut mark
    "Iacute"    =>    "\xCD",   # capital I, acute accent
    "iacute"    =>    "\xED",   # small i, acute accent
    "Icirc"     =>    "\xCE",   # capital I, circumflex accent
    "icirc"     =>    "\xEE",   # small i, circumflex accent
    "Igrave"    =>    "\xCD",   # capital I, grave accent
    "igrave"    =>    "\xED",   # small i, grave accent
    "Iuml"      =>    "\xCF",   # capital I, dieresis or umlaut mark
    "iuml"      =>    "\xEF",   # small i, dieresis or umlaut mark
    "Ntilde"    =>    "\xD1",   # capital N, tilde
    "ntilde"    =>    "\xF1",   # small n, tilde
    "Oacute"    =>    "\xD3",   # capital O, acute accent
    "oacute"    =>    "\xF3",   # small o, acute accent
    "Ocirc"     =>    "\xD4",   # capital O, circumflex accent
    "ocirc"     =>    "\xF4",   # small o, circumflex accent
    "Ograve"    =>    "\xD2",   # capital O, grave accent
    "ograve"    =>    "\xF2",   # small o, grave accent
    "Oslash"    =>    "\xD8",   # capital O, slash
    "oslash"    =>    "\xF8",   # small o, slash
    "Otilde"    =>    "\xD5",   # capital O, tilde
    "otilde"    =>    "\xF5",   # small o, tilde
    "Ouml"      =>    "\xD6",   # capital O, dieresis or umlaut mark
    "ouml"      =>    "\xF6",   # small o, dieresis or umlaut mark
    "szlig"     =>    "\xDF",   # small sharp s, German (sz ligature)
    "THORN"     =>    "\xDE",   # capital THORN, Icelandic
    "thorn"     =>    "\xFE",   # small thorn, Icelandic
    "Uacute"    =>    "\xDA",   # capital U, acute accent
    "uacute"    =>    "\xFA",   # small u, acute accent
    "Ucirc"     =>    "\xDB",   # capital U, circumflex accent
    "ucirc"     =>    "\xFB",   # small u, circumflex accent
    "Ugrave"    =>    "\xD9",   # capital U, grave accent
    "ugrave"    =>    "\xF9",   # small u, grave accent
    "Uuml"      =>    "\xDC",   # capital U, dieresis or umlaut mark
    "uuml"      =>    "\xFC",   # small u, dieresis or umlaut mark
    "Yacute"    =>    "\xDD",   # capital Y, acute accent
    "yacute"    =>    "\xFD",   # small y, acute accent
    "yuml"      =>    "\xFF",   # small y, dieresis or umlaut mark
                                  
    "lchevron"  =>    "\xAB",   # left chevron (double less than) laquo
    "rchevron"  =>    "\xBB",   # right chevron (double greater than) raquo

    "iexcl"  =>   "\xA1",   # inverted exclamation mark
    "cent"   =>   "\xA2",   # cent sign
    "pound"  =>   "\xA3",   # (UK) pound sign
    "curren" =>   "\xA4",   # currency sign
    "yen"    =>   "\xA5",   # yen sign
    "brvbar" =>   "\xA6",   # broken vertical bar
    "sect"   =>   "\xA7",   # section sign
    "uml"    =>   "\xA8",   # diaresis
    "copy"   =>   "\xA9",   # Copyright symbol
    "ordf"   =>   "\xAA",   # feminine ordinal indicator
    "laquo"  =>   "\xAB",   # left pointing double angle quotation mark
    "not"    =>   "\xAC",   # not sign
    "shy"    =>   "\xAD",   # soft hyphen
    "reg"    =>   "\xAE",   # registered trademark
    "macr"   =>   "\xAF",   # macron, overline
    "deg"    =>   "\xB0",   # degree sign
    "plusmn" =>   "\xB1",   # plus-minus sign
    "sup2"   =>   "\xB2",   # superscript 2
    "sup3"   =>   "\xB3",   # superscript 3
    "acute"  =>   "\xB4",   # acute accent
    "micro"  =>   "\xB5",   # micro sign
    "para"   =>   "\xB6",   # pilcrow sign = paragraph sign
    "middot" =>   "\xB7",   # middle dot = Georgian comma
    "cedil"  =>   "\xB8",   # cedilla
    "sup1"   =>   "\xB9",   # superscript 1
    "ordm"   =>   "\xBA",   # masculine ordinal indicator
    "raquo"  =>   "\xBB",   # right pointing double angle quotation mark
    "frac14" =>   "\xBC",   # vulgar fraction one quarter
    "frac12" =>   "\xBD",   # vulgar fraction one half
    "frac34" =>   "\xBE",   # vulgar fraction three quarters
    "iquest" =>   "\xBF",   # inverted question mark
    "times"  =>   "\xD7",   # multiplication sign
    "divide" =>   "\xF7",   # division sign

=cut

sub escapes {
	my $self = shift;
	my %e = @_;
	if (ref($_[0]) eq 'HASH') {
		%e = %{ $_[0] };
	}
	$self->{'escapes'} = { %e };

	if (wantarray) {
		return %{ $self->{'escapes'} }
	}
	return $self->{'escapes'};
}

sub escape {
	my ($self, $esc) = @_;
	my $result = $self->{'escapes'}{$esc};
	if (ref($result) eq 'ARRAY') {
		return $result;
	}
	if (ref($result) eq 'CODE') {
		return &$result;
	}
	return undef if ref($result);	# no other refs valid

	return chr($esc) if ($esc=~/^\d+$/);
	return chr(hex $esc) if ($esc=~s/^x([A-Fa-f0-9]+)$/$1/);
	return $result if (exists $self->{'escapes'}{$esc});
	return $ESCAPES{$esc};
}

# stolen from Pod::Text
%ESCAPES = (
    'amp'       =>    '&',      # ampersand
    'lt'        =>    '<',      # left chevron, less-than
    'gt'        =>    '>',      # right chevron, greater-than
    'quot'      =>    '"',      # double quote
                                 
    "Aacute"    =>    "\xC1",   # capital A, acute accent
    "aacute"    =>    "\xE1",   # small a, acute accent
    "Acirc"     =>    "\xC2",   # capital A, circumflex accent
    "acirc"     =>    "\xE2",   # small a, circumflex accent
    "AElig"     =>    "\xC6",   # capital AE diphthong (ligature)
    "aelig"     =>    "\xE6",   # small ae diphthong (ligature)
    "Agrave"    =>    "\xC0",   # capital A, grave accent
    "agrave"    =>    "\xE0",   # small a, grave accent
    "Aring"     =>    "\xC5",   # capital A, ring
    "aring"     =>    "\xE5",   # small a, ring
    "Atilde"    =>    "\xC3",   # capital A, tilde
    "atilde"    =>    "\xE3",   # small a, tilde
    "Auml"      =>    "\xC4",   # capital A, dieresis or umlaut mark
    "auml"      =>    "\xE4",   # small a, dieresis or umlaut mark
    "Ccedil"    =>    "\xC7",   # capital C, cedilla
    "ccedil"    =>    "\xE7",   # small c, cedilla
    "Eacute"    =>    "\xC9",   # capital E, acute accent
    "eacute"    =>    "\xE9",   # small e, acute accent
    "Ecirc"     =>    "\xCA",   # capital E, circumflex accent
    "ecirc"     =>    "\xEA",   # small e, circumflex accent
    "Egrave"    =>    "\xC8",   # capital E, grave accent
    "egrave"    =>    "\xE8",   # small e, grave accent
    "ETH"       =>    "\xD0",   # capital Eth, Icelandic
    "eth"       =>    "\xF0",   # small eth, Icelandic
    "Euml"      =>    "\xCB",   # capital E, dieresis or umlaut mark
    "euml"      =>    "\xEB",   # small e, dieresis or umlaut mark
    "Iacute"    =>    "\xCD",   # capital I, acute accent
    "iacute"    =>    "\xED",   # small i, acute accent
    "Icirc"     =>    "\xCE",   # capital I, circumflex accent
    "icirc"     =>    "\xEE",   # small i, circumflex accent
    "Igrave"    =>    "\xCD",   # capital I, grave accent
    "igrave"    =>    "\xED",   # small i, grave accent
    "Iuml"      =>    "\xCF",   # capital I, dieresis or umlaut mark
    "iuml"      =>    "\xEF",   # small i, dieresis or umlaut mark
    "Ntilde"    =>    "\xD1",   # capital N, tilde
    "ntilde"    =>    "\xF1",   # small n, tilde
    "Oacute"    =>    "\xD3",   # capital O, acute accent
    "oacute"    =>    "\xF3",   # small o, acute accent
    "Ocirc"     =>    "\xD4",   # capital O, circumflex accent
    "ocirc"     =>    "\xF4",   # small o, circumflex accent
    "Ograve"    =>    "\xD2",   # capital O, grave accent
    "ograve"    =>    "\xF2",   # small o, grave accent
    "Oslash"    =>    "\xD8",   # capital O, slash
    "oslash"    =>    "\xF8",   # small o, slash
    "Otilde"    =>    "\xD5",   # capital O, tilde
    "otilde"    =>    "\xF5",   # small o, tilde
    "Ouml"      =>    "\xD6",   # capital O, dieresis or umlaut mark
    "ouml"      =>    "\xF6",   # small o, dieresis or umlaut mark
    "szlig"     =>    "\xDF",   # small sharp s, German (sz ligature)
    "THORN"     =>    "\xDE",   # capital THORN, Icelandic
    "thorn"     =>    "\xFE",   # small thorn, Icelandic
    "Uacute"    =>    "\xDA",   # capital U, acute accent
    "uacute"    =>    "\xFA",   # small u, acute accent
    "Ucirc"     =>    "\xDB",   # capital U, circumflex accent
    "ucirc"     =>    "\xFB",   # small u, circumflex accent
    "Ugrave"    =>    "\xD9",   # capital U, grave accent
    "ugrave"    =>    "\xF9",   # small u, grave accent
    "Uuml"      =>    "\xDC",   # capital U, dieresis or umlaut mark
    "uuml"      =>    "\xFC",   # small u, dieresis or umlaut mark
    "Yacute"    =>    "\xDD",   # capital Y, acute accent
    "yacute"    =>    "\xFD",   # small y, acute accent
    "yuml"      =>    "\xFF",   # small y, dieresis or umlaut mark
                                  
    "lchevron"  =>    "\xAB",   # left chevron (double less than) laquo
    "rchevron"  =>    "\xBB",   # right chevron (double greater than) raquo

    "iexcl"  =>   "\xA1",   # inverted exclamation mark
    "cent"   =>   "\xA2",   # cent sign
    "pound"  =>   "\xA3",   # (UK) pound sign
    "curren" =>   "\xA4",   # currency sign
    "yen"    =>   "\xA5",   # yen sign
    "brvbar" =>   "\xA6",   # broken vertical bar
    "sect"   =>   "\xA7",   # section sign
    "uml"    =>   "\xA8",   # diaresis
    "copy"   =>   "\xA9",   # Copyright symbol
    "ordf"   =>   "\xAA",   # feminine ordinal indicator
    "laquo"  =>   "\xAB",   # left pointing double angle quotation mark
    "not"    =>   "\xAC",   # not sign
    "shy"    =>   "\xAD",   # soft hyphen
    "reg"    =>   "\xAE",   # registered trademark
    "macr"   =>   "\xAF",   # macron, overline
    "deg"    =>   "\xB0",   # degree sign
    "plusmn" =>   "\xB1",   # plus-minus sign
    "sup2"   =>   "\xB2",   # superscript 2
    "sup3"   =>   "\xB3",   # superscript 3
    "acute"  =>   "\xB4",   # acute accent
    "micro"  =>   "\xB5",   # micro sign
    "para"   =>   "\xB6",   # pilcrow sign = paragraph sign
    "middot" =>   "\xB7",   # middle dot = Georgian comma
    "cedil"  =>   "\xB8",   # cedilla
    "sup1"   =>   "\xB9",   # superscript 1
    "ordm"   =>   "\xBA",   # masculine ordinal indicator
    "raquo"  =>   "\xBB",   # right pointing double angle quotation mark
    "frac14" =>   "\xBC",   # vulgar fraction one quarter
    "frac12" =>   "\xBD",   # vulgar fraction one half
    "frac34" =>   "\xBE",   # vulgar fraction three quarters
    "iquest" =>   "\xBF",   # inverted question mark
    "times"  =>   "\xD7",   # multiplication sign
    "divide" =>   "\xF7",   # division sign
);

=back

=head1 BUGS

Text wrapping with C<text_wrap> is barely supported. For serious text wrapping,
look at L<Prima::TextView> and L<Prima::PodView>.

=head1 COPYRIGHT

Copyright 2003 Teo Sankaro

Portions of this code were copied or adapted from L<Pod::Text> by Russ Allbery
and L<Pod::Parser> by Brad Appleton, both of which derive from the original
C<Pod::Text> by Tom Christianson.

You may redistribute and/or modify this module under the same terms as Perl itself.
(Although a credit would be nice.)

=head1 AUTHOR

Teo Sankaro, E<lt>teo_sankaro@hotmail.comE<gt>.

=cut

1;
