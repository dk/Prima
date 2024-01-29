use strict;
use warnings;
use Test::More;
use Prima::sys::Test;
use Prima::Application;


my $w;
my $z;

{
	no warnings 'redefine';
	sub curry
	{
		my $cb = shift;
		my $ok = $cb->(@_);
		return $ok if $ok;
		diag('font: [', $w->font->name, ']');
		return 0;
	}
	my $is_deeply = \&is_deeply;
	my $ok = \&ok;
	my $is = \&is;
	*is_deeply = sub { curry($is_deeply, @_) };
	*ok        = sub ($;$)  { curry($ok, @_) };
	*is        = sub ($$;$) { curry($is, @_) };
}


my %opt;
my %glyphs;
my $high_unicode_char;

@ARGV = map {
	m/^font=(.*)$/ ? ( do {
		$opt{font} = $1;
		()
	}) : $_
} @ARGV;

my $bad_fonts = sub { qr/$_[0]/ }->( join('|', (
	'Iosevka.*Curly Slab',
)));

sub xtr($)
{
	my $xtr = shift;

	$xtr =~ tr[A-Z][\N{U+5d0}-\N{U+5e8}]; # hebrew
	# RTL(|/) ligates to %, with ZWJ (fribidi) or without (harfbuzz)
	$xtr =~ tr[|/%0][\x{627}\x{644}\x{fefb}\x{feff}]; 
	$xtr =~ tr[+-][\x{200d}\x{200c}];
	$xtr =~ s[\^][$high_unicode_char]g if defined $high_unicode_char;

	return $xtr;
}

sub glyphs($)
{
        my $str = xtr(shift);
        my %g;
        for my $c ( split //, $str ) {
	        my $k = $w-> text_shape($c, polyfont => 0);
                return unless $k;
                $g{$c} = $k->glyphs->[0];
        }
        return %glyphs = %g;
}

sub no_glyphs($)
{
        my $str = xtr(shift);
        my %g;
        for my $c ( split //, $str ) {
                $g{$c} = ord($c);
        }
        return %glyphs = %g;
}

sub glyphs_fully_resolved 
{
	return 0 unless scalar keys %glyphs;
	return 0 == scalar grep { !$_ } values %glyphs;
}

sub gmap($) { [ @glyphs{ split //, $_[0] } ] }

sub r { map { $_ | to::RTL } @_ }
sub R { reverse r @_ }

sub comp
{
	my ( $got, $exp, $name, $hexy, $text) = @_;

	if ( !$got && !$exp) { # undef and 0 are same, whatever
		ok(1, $name);
		return;
	}
	goto FAIL unless
		((ref($got) // '') eq 'ARRAY') &&
		((ref($exp) // '') eq 'ARRAY') &&
		@$got == @$exp;

	for ( my $i = 0; $i < @$got; $i++) {
		goto FAIL if ($got->[$i] // '<undef>') ne ($exp->[$i] // '<undef>');
	}
	ok(1, $name);
	return;

FAIL:
	ok(0, "$name {$text}");
	$got ||= ['<undef>'];
	$exp ||= ['<undef>'];
	$exp = [ map { defined($_) ? $_ : '<undef>' } @$exp ];
	$got = [ map { defined($_) ? $_ : '<undef>' } @$got ];
	if ( $hexy ) {
		@$got = map { /^\d+$/ ? (sprintf q(%x), $_) : $_ } @$got;
		@$exp = map { /^\d+$/ ? (sprintf q(%x), $_) : $_ } @$exp;
	} else {
		$_ = '-' . ($_ & ~to::RTL) for grep { /^\d+$/ && $_ & to::RTL } @$got;
		$_ = '-' . ($_ & ~to::RTL) for grep { /^\d+$/ && $_ & to::RTL } @$exp;
	}
	diag(sprintf("got [@$got], expected [@$exp] font=%s", $w->font->name));
}

sub t2
{
	my ( $text, $glyphs, $indexes, $name, %opt) = @_;

	my $orig_text   = $text;
	my $orig_glyphs = $glyphs . '#';
	$text   = xtr $text;
	$glyphs = xtr $glyphs;
	$text =~ tr
		[<>=]
		#[\x{2067}\x{2066}\x{2069}]
		[\x{202B}\x{202a}\x{202c}]
		;

	$z = $w-> text_shape($text, %opt, polyfont => 0);
	return ok(0, "$name (undefined)") unless defined $z;
	return ok(0, "$name (unnecessary, retval=0)") unless $z;
	comp($z->glyphs, gmap $glyphs, "$name (glyphs)", 1, $orig_text);
	if ( defined $indexes ) {
		comp($z->indexes, $indexes, "$name (indexes)", 0, $_[0]);
		return;
	}

	my %rev = reverse %glyphs;
	my $v = join '',
		map {
			my $ofs  = $_ & ~to::RTL;
			my $char = sprintf("(%x)",$_);
		AGAIN:
			if ( $ofs >= 0 && $ofs < length($orig_text)) {
				$char = substr($orig_text, $ofs, 1);
				if ( $char =~ /[\<\>\=]/ ) {
					$ofs++;
					goto AGAIN;
				}
				if ($_ & to::RTL) {
					$char = "(+$char)" if $char !~ /[A-Z\/\|\%0\+\-\.\s\?\`\<\>\^]/;
				} else {
					$char = "(-$char)" if $char !~ /[a-z\+\-\.\s\"\d\^]/;
				}
			} elsif ( $ofs == length($orig_text)) {
				$char = '#';
			}
			$char
		}
		@{$z->indexes // []};
	unless (is($v, $orig_glyphs, "$name (indexes)")) {
		my $got = $z->indexes // ['<undef>'];
		$got = [ map { defined($_) ? $_ : '<undef>' } @$got ];
		$_ = '-' . ($_ & ~to::RTL) for grep { /^\d+$/ && $_ & to::RTL } @$got;
		diag("got indexes: [@$got]");
	}
}

sub t
{
	my ( $text, $glyphs, $name, %opt) = @_;
	t2($text, $glyphs, undef, $name, %opt);
}

sub find_char
{
	my ($font, $char) = @_;
	$w->font($font);
	my @r = @{ $w->get_font_ranges };
	my $found = 0;
	my @chars = map { ord } split '', $char;
	for ( my $i = 0; $i < @r; $i += 2 ) {
		my ( $l, $r ) = @r[$i, $i+1];
		for my $c ( @chars ) {
			$found++ if $l <= $c && $r >= $c;
		}
		last if $found == @chars;
	}
	return $found == @chars;
}

sub find_high_unicode_char
{
	my ($font) = @_;
	$w->font($font);
	my @r = @{ $w->get_font_ranges };
	my @range;
	my $found;
	for ( my $i = 0; $i < @r; $i += 2 ) {
		my ( $l, $r ) = @r[$i, $i+1];
		next unless $r >= 0x10000;
		$l = 0x10000 if $l < 0x10000;
		push @range, $l .. $r;
		return \@range;
	}
	return undef;
}

sub find_high_unicode_font
{
	my $c = find_high_unicode_char($w->font);
	return $c if defined $c;
	return undef if defined $opt{font};

	my @f = @{$::application->fonts};
	for my $f ( @f ) {
		next unless $f->{vector};
		next if $f->{name} =~ $bad_fonts;
		$c = find_high_unicode_char($f);
		return $c if defined $c;
	}
	return undef;
}

# try to find font with given letters
# aim at highest standard, ie ttf/xft + scaling + bidi fonts
sub find_vector_font
{
	my $find_char = shift;
	return 1 if find_char($w->font, $find_char);
	return 0 if defined $opt{font};

	my $got_rtl;
	my $found;
	my @f = @{$::application->fonts};

	# fontconfig fonts
	for my $f ( @f ) {
		next unless $f->{vector};
		next unless $f->{name} =~ /^[A-Z]/;
		next if $f->{name} =~ $bad_fonts;
		next unless find_char($f, $find_char);
		$found = $f;
		$got_rtl = 1;
		goto FOUND;
	}

FOUND:
	$w->font->name($found->{name}) if $found;

	return $got_rtl;
}

sub find_glyphs
{
	my ($font, $glyphs) = @_;
	$w->font($font);
	my $null = $w->text_shape(chr($w->font->defaultChar), polyfont => 0);
	$null = $null ? $null->glyphs->[0] : 0;

	my $z = $w->text_shape($glyphs, polyfont => 0);
	return 0 unless $z;
	return !grep { $_ == $null || $_ == 0 } @{$z->glyphs};
}

# a font may have glyphs but won't know how to ligate them, i.e. glyph found but script not found
sub find_shaping_font
{
	my $glyphs = shift;
	return 1 if find_glyphs($w->font, $glyphs);
	return 0 if defined $opt{font};

	my $got_rtl;
	my $found;
	my @f = @{$::application->fonts};

	# fontconfig fonts
	for my $f ( @f ) {
		next unless $f->{vector};
		next unless $f->{name} =~ /^[A-Z]/;
		next unless find_glyphs($f, $glyphs);
		$found = $f;
		$got_rtl = 1;
		goto FOUND;
	}

FOUND:
	$w->font->name($found->{name}) if $found;

	return $got_rtl;
}

sub check_noshape_nofribidi
{
	t('12', '12', 'ltr');
	t('12ABC', '12CBA', 'rtl in ltr');
	t('>AB', 'BA', 'bidi');

	$glyphs{"\0"} = 0;
	t2('12ABC', "\0"x5, [0,1,R(2..4),5], 'null shaping', level => ts::None);
}

sub check_noreorder
{
	t2('12ABC', '12CBA', [0,1,R(2..4),5], 'reorder glyphs',   level => ts::Glyphs, reorder => 1);
	t2('12ABC', '12ABC', [0,1,r(2..4),5], 'noreorder glyphs', level => ts::Glyphs, reorder => 0);
	t2('12ABC', '12CBA', [0,1,R(2..4),5], 'reorder full',     level => ts::Full,   reorder => 1);
	t2('12ABC', '12ABC', [0,1,r(2..4),5], 'noreorder full',   level => ts::Full,   reorder => 0);
}

# very minimal support for bidi and X11 core fonts only
sub test_minimal
{
	ok(1, "test minimal");
	no_glyphs '12ABC';
	check_noshape_nofribidi();
}

# very minimal support for bidi with xft but no harfbuzz
sub test_glyph_mapping
{
	ok(1, "test glyph mapping without bidi");

        SKIP: {
                glyphs "12ABC";
		skip("text shaping is not available", 1) unless glyphs_fully_resolved;
		check_noshape_nofribidi();
		check_noreorder();
        }
}

sub check_proper_bidi
{
	# http://unicode.org/reports/tr9/tr9-22.html
	SKIP : {
		glyphs ' ACDEIMNORUYSacdeghimnrs.?"`';
    		skip("not enough glyphs for proper bidi test", 1) unless glyphs_fully_resolved;
		t(
			'car means CAR.',
			'car means RAC.', 
			'example 1');
		t(
			'<car MEANS CAR.=',
			'.RAC SNAEM car',
			'example 2');
		t(
			'he said "<car MEANS CAR=."',
			'he said "RAC SNAEM car."',
			'example 3');
		t(
			'DID YOU SAY `>he said "<car MEANS CAR="=`?',
			'?`he said "RAC SNAEM car"` YAS UOY DID',
			'example 4',
			rtl => 1); # XXX not needed for autodetect
	}
}

sub test_fribidi
{
	ok(1, "test bidi");
	SKIP: {
		glyphs "12ABC|/%0";
		skip("text shaping is not available", 1) unless glyphs_fully_resolved;

		check_noshape_nofribidi();
		check_noreorder();
		t('12ABC', 'CBA12', 'rtl in rtl', rtl => 1);
		t2('/|', '%0', [R(0,1),2], 'arabic ligation with ZW nobreaker');
		t('|/', '/|', 'no arabic ligation');

		check_proper_bidi();
	}
}

sub test_shaping
{
	my ($found) = @_;
	ok(1, "test shaping");

	SKIP: {
		skip("no vector fonts", 1) unless $found;

               	glyphs "12ABC";
		skip("text shaping is not available", 1) unless glyphs_fully_resolved;
		check_noshape_nofribidi();
		check_noreorder();

		my $z = $w->text_shape('12', polyfont => 0);
		ok((4 == @{$z->positions // []}), "positions are okay");
		ok((2 == @{$z->advances  // []}), "advances are okay");

		$z = $w->text_shape('12', level => ts::Glyphs, polyfont => 0); 
		is_deeply($z->indexes, [0,1,2], "glyph mapper indexes are okay");
		ok((0 == @{$z->positions // []}), "glyph mapper positions are okay");
		ok((0 == @{$z->advances  // []}), "glyph mapper advances are okay");

		$z = $w->text_shape('12', level => ts::Glyphs, advances => 1, polyfont => 0); 
		ok((4 == @{$z->positions // []}), "glyph mapper positions w/advances are okay");
		ok((2 == @{$z->advances  // []}), "glyph mapper advances a w/advances are okay");

		if ( $opt{fribidi} ) {
			t('12ABC', 'CBA12', 'rtl in rtl', rtl => 1);
		}

		SKIP: {
                	glyphs "|-/%";
			skip("arabic shaping is not available", 1) unless glyphs_fully_resolved;
			t('|/', '/|', 'no arabic ligation');
			t2('/|', '%', [r(0),2], 'arabic ligation');
			if ( $opt{fribidi} ) {
				t('/-|', '|-/', 'arabic non-ligation');
				check_proper_bidi();
			}
		}

		SKIP: {
			skip("no devanagari font", 1) unless find_vector_font("\x{924}");
			my $z = $w-> text_shape("\x{924}\x{94d}\x{928}", polyfont => 0);
			ok( $z && scalar(grep {$_} @{$z->glyphs}), 'devanagari shaping');
		}

		SKIP: {
 			skip("no khmer font", 1) unless find_vector_font("\x{179f}");
			my $z = $w-> text_shape("\x{179f}\x{17b9}\x{1784}\x{17d2}", polyfont => 0);
			ok( $z && scalar(grep {$_} @{$z->glyphs}), 'khmer shaping');
		}
	}
}

sub test_bytes
{
	ok(1, "bytes");

	my $k = $w-> text_shape("A\x{fe}", level => ts::Bytes, polyfont => 0);
	is( 2, scalar(@{$k->glyphs}), "two bytes mapped to two glyphs");
	is_deeply( $k->indexes, [0,1,2], "two bytes index array");
}

sub test_tabs
{
	my $k = $w-> text_shape(" \t", replace_tabs => 8 );
	SKIP: {
		my $xg = $k->glyphs;
		is( $xg->[0], $xg->[1], "tabs replaced");
		my $xa = $k->advances;
		skip("no advances", 1) unless $k;
		is( $xa->[0] * 8, $xa->[1], "tabs replaced with correct width");
	}
}

sub test_high_unicode
{
	ok(1, "high unicode");

	my $k = $w-> text_shape("\x{10FF00}" x 2, polyfont => 0);
	is_deeply( $k->glyphs, [0,0], "unresolvable glyphs");

	SKIP: {
		my $chars = find_high_unicode_font;
		skip("no fonts with characters above 0x10000", 1) unless $chars && @$chars;
		#splice(@$chars, 256); # win32 reports empty glyphs as available, but surely in 256 should be at least one valid glyph

		my $char;
		%glyphs = ();
        	for my $c (@$chars) {
		        my $k = $w-> text_shape(chr($c), polyfont => 0);
        	        next unless $k && $k->glyphs->[0];
			$high_unicode_char = chr($char = $c); # as ^
        	        $glyphs{$high_unicode_char} = $k->glyphs->[0];
			last;
        	}
		skip("text shaping is not available", 1) unless defined $char;
		t("^^", "^^", sprintf("found char U+%x in " . $w->font->name . " as glyph %x", $char, $glyphs{$high_unicode_char}));
	}
}

sub test_glyphs_wrap
{ SKIP: {
	skip("no font at all", 1) unless find_shaping_font( "12");
	$w->font->size(12);
	my $z = $w-> text_shape('12', advances => 1, polyfont => 0);
	is( 2, scalar( @{ $z->glyphs // [] }), "text '12' resolved to 2 glyphs");

	my ($tw) = @{ $z->advances // [ $w->get_text_width('1') ] };

	my $r = $w-> text_wrap( $z, 0, tw::BreakSingle );
	is_deeply( $r, [], "wrap that doesn't fit");

	$r = $w-> text_wrap( $z, 0, tw::ReturnFirstLineLength );
	is( $r, 1, "tw::ReturnFirstLineLength");

	$r = $w-> text_wrap( $z, 0, tw::ReturnChunks );
	is_deeply( $r, [0,1,1,1], "tw::ReturnChunks");

	$r = $w-> text_wrap( $z, 0, 0 );
	is( scalar(@$r), 2, "wrap: split to 2 pieces");
	is_deeply( $r->[0]->glyphs, [ $z->glyphs->[0] ], "glyphs 1");
	is_deeply( $r->[1]->glyphs, [ $z->glyphs->[1] ], "glyphs 2");
	is_deeply( $r->[0]->indexes, [ $z->indexes->[0], length('12') ], "indexes 1");
	is_deeply( $r->[1]->indexes, [ $z->indexes->[1], length('12') ], "indexes 2");
	if ( $z-> advances ) {
		is( $r->[0]->advances->[0], $z->advances->[0], "advances 1");
		is( $r->[1]->advances->[0], $z->advances->[1], "advances 2");
		is_deeply( $r->[0]->positions, [ @{$z->positions}[0,1] ], "positions 1");
		is_deeply( $r->[1]->positions, [ @{$z->positions}[2,3] ], "positions 2");
	}

	$r = $w-> text_wrap( $z, 1_000_000, 0 );
	is_deeply($r->[0], $z, "quick clone");

	SKIP: { if ( $opt{shaping} ) {
		skip("no arabic font", 1) unless find_shaping_font( xtr('|/%'));
		$w->font->size(12);
		glyphs "|/%";
		skip("arabic glyphs are not available", 1) unless glyphs_fully_resolved;
		$z = $w-> text_shape(xtr('|/|'), polyfont => 0); # 2 glyphs, | and /|, visually /| is on the left
		skip("arabic shaping is not available for font ".$w->font->name, 1)
			unless $z && $z->glyphs && 2 == @{ $z->glyphs };
		$r = $w-> text_wrap($z, 0, tw::ReturnChunks);
		is_deeply($r, [0,1 , 1,1], "ligature wrap, chunks");
		$r = $w-> text_wrap($z, 0, 0);
		is_deeply($r->[0]->glyphs, [$glyphs{xtr '%'}], 'ligature wrap, left glyphs');
		is_deeply($r->[0]->indexes, [r(1),length('|/|')], 'ligature wrap, left indexes');
		is_deeply($r->[1]->glyphs, [$glyphs{xtr '|'}], 'ligature wrap, right glyphs');
		is_deeply($r->[1]->indexes, [r(0),length('|/|')], 'ligature wrap, right indexes');

		$z = $w-> text_wrap_shape(xtr('/|') . "\n" . xtr('/|') . "~p",
			undef,
			options => tw::CalcMnemonic|tw::NewLineBreak|tw::CollapseTilde,
			rtl => 1
		);
		is( $z->[-1]->{tildeLine}, 1, "tilde is at line 1");
		is( $z->[-1]->{tildePos}, 2, "'p' is at position 2");
	}}
}}

sub test_combining { SKIP: {
	skip("no combining without shaping", 1) unless $opt{shaping};
	skip("no extended latin font", 1) unless find_shaping_font( "f\x{100}\x{300}");
	my $xp;
	if ( $^O =~ /win32/i) {
		my $info = $::application->get_system_info;
		$xp = 1 if $info->{release} < 6;
	}

	# A with a dash on top combined with an acute
	# acute must be combined with no advance
	$w->font->size(12);
	my $z = $w-> text_shape( "\x{100}\x{300}", polyfont => 0 )->advances;
	if ( !$z && $w->font->name ne $::application->get_default_font->{name} ) {
		$w->font->set( %{ $::application-> get_default_font}, size => 12 );
		$z = $w-> text_shape( "\x{100}\x{300}", polyfont => 0 )->advances;
		skip($w->font->name . " does not create advances table", 1) unless $z; 
	}
	ok( $z->[0] != 0, "'A' has non-zero advance");
	if ( $xp ) {
		if ($z->[1] == 0 ) {
			ok( 1, "joined 'acute' has zero advance");
		} else {
			skip("This XP is bad at combining, skip ", 1);
		}
	} else {
		ok( $z->[1] == 0, "joined 'acute' has zero advance");
	}

	# ff may be a ligature, but that's not essential -
	# the main interest here to see that ZWNJ is indeed ZW
	$z = $w-> text_shape( "f\x{200c}f", polyfont => 0 )->advances;
	ok( $z->[0] != 0, "'f' has non-zero advance");
	ok( $z->[1] == 0, "ZWNJ has zero advance");
}}

sub dump_bitmap
{
	my ( $text, $i ) = @_;
	diag("Bitmap dump $text " . $i->width . "x" . $i->height);
	my ($x,$y) = $i->size;
	for my $Y ( 1..$y) {
		my $str = '';
		for my $X ( 1..$x) {
			my $px = $i->pixel($X-1, $y-$Y);
			$str .= ($px ? '*' : ' ');
		}
		diag($str);
	}
}

sub test_drawing
{ SKIP: {
	glyphs "12";
	skip("glyph drawing is not available", 1) unless glyphs_fully_resolved;

	$w-> backColor(cl::Black);
	$w-> color(cl::White);
	$w-> font-> set( height => 25, style => fs::Underlined );
	$w-> clear;
	$w-> text_out( "12", 5, 5 );
	my $i = $w->image;
	$i->type(im::Byte);
	my $sum1 = $i->sum;
	skip("text drawing on bitmap is not available", 1) unless $sum1;

	my $z = $w-> text_shape('12', polyfont => 0);
	skip("shaping is not available", 1) unless $z;

	$w-> clear;
	$w-> text_out( $z, 5, 5 );
	$i = $w->image;
	$i->type(im::Byte);
	my $sum2 = $i->sum;
	is($sum2, $sum1, "glyphs plotting");

	$w-> clear;
	$w-> text_out( $z->glyphs, 5, 5 );
	$i = $w->image;
	$i->type(im::Byte);
	$sum2 = $i->sum;
	is($sum2, $sum1, "glyphs plotting, terse version");

	$w-> clear;
	$w-> font-> set( height => 25, style => fs::Underlined, direction => -10 );
	$w-> text_out( "12", 5, 5 );
	$i = $w->image;
	$i->type(im::Byte);
	$sum1 = $i->sum;
	my $data1 = $i;

	$z = $w-> text_shape('12', polyfont => 0, level => ts::Glyphs);
	$w-> clear;
	$w-> text_out( $z, 5, 5 );
	$i = $w->image;
	$i->type(im::Byte);
	$sum2 = $i->sum;
	is($sum2, $sum1, "glyphs plotting 45 degrees");
	if ( $sum2 ne $sum1 ) {
		dump_bitmap('1', $data1);
		dump_bitmap('2', $i);
	}
}}

sub run_test
{
	my $unix = shift;

	$w = Prima::DeviceBitmap-> create( type => dbt::Pixmap, width => 32, height => 32);
	if (defined $opt{font}) {
		$w->font->set( name => $opt{font}, encoding => "");
	} elsif ( $^O =~ /win32/i && $w->font->name !~ /arial|courier|times|segoe|verdana|tahoma|consolas/i) {
		$w->font->name('Arial'); # we're testing shaping implementation, not that all possible fonts
		                         # can be properly shaped
	}
	my $found = find_vector_font(xtr('A'));

	my $z = $w-> text_shape( "1", polyfont => 0 );
	plan skip_all => "Shaping is not available" if defined $z && $z eq '0';

	$opt{fribidi} = 1 if Prima::Application->get_system_value(sv::FriBidi);
	if ( $unix ) {
		%opt = (%opt, map { $_ => 1 } split ' ', Prima::Application->sys_action('shaper'));
		if ( $opt{harfbuzz} && $opt{xft}) {
			$opt{shaping} = 1;
			test_shaping($found, $opt{fribidi});
		} elsif ( $opt{fribidi}) {
			test_fribidi;
		} elsif ( $opt{xft}) {
			test_glyph_mapping;
		} else {
			test_minimal;
		}
	} else {
		$opt{shaping} = 1;
		test_shaping($found, $opt{fribidi});
	}
	test_bytes;
	test_tabs;
	test_high_unicode;
	test_drawing;
	test_glyphs_wrap;
	test_combining;
}

if ( Prima::Application-> get_system_info->{apc} == apc::Unix ) {
	if ( @ARGV ) {
		run_test(1);
	} else {
		my %options = Prima::options();
		my @opt = grep { m/^no-(fribidi|harfbuzz|xft)$/ } sort keys %options;
		my @xargv;
		push @xargv, "font=$opt{font}" if defined $opt{font};
		for ( my $i = 0; $i < 2 ** @opt; $i++) {
			my @xopt = map { "--$_" } @opt[ grep { $i & (1 << $_) } 0..$#opt ];
			my @inc  = map { "-I$_" } @INC;
			for ( split "\n", `$^X @inc $0 @xopt @xargv TEST 2>&1`) {
				if (m/^(ok|not ok)\s+\d+(.*)/) {
					my ( $ok, $info ) = ( $1 eq 'ok', $2);
					if ( $info =~ /# skip (.*)/) {
						SKIP: { skip("(@xopt) $1", 1) };
					} else {
						ok($ok, "(@xopt) $info");
					}
				} elsif ( m/# SKIP (.*)/) {
					SKIP: { skip("(@xopt) $1", 1) };
				} elsif ( !m/^\d+\.\.\d+/) {
					warn "$_\n";
				}
			}
		}
	}
} else {
	run_test(0);
}

done_testing;


