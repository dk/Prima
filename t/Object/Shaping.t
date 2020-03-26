use strict;
use warnings;

use Test::More;
use Prima::Test;
use Data::Dumper;

my $w;
my $z;

my %glyphs;

sub xtr($)
{
	my $xtr = shift;
	# RTL(JI) ligates to Y
	$xtr =~ tr/ABRCIJY/\x{629}\x{630}\x{630}\x{631}\x{627}\x{644}\x{fefb}/;
	return $xtr;
}

sub glyphs($)
{
        my $str = xtr(shift);
        my %g;
        for my $c ( split //, $str ) {
	        my $k = $w-> text_shape($c);
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

sub gmap($) { [ @glyphs{ split //, $_[0] } ] }

sub r { map { $_ | 0x8000 } @_ }

sub comp
{
	my ( $got, $exp, $name) = @_;

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
	ok(0, $name);
	$got ||= ['<undef>'];
	$exp ||= ['<undef>'];
	$exp = [ map { defined($_) ? $_ : '<undef>' } @$exp ];
	$got = [ map { defined($_) ? $_ : '<undef>' } @$got ];
	$_ = '-' . ($_ & 0x7fff) for grep { /^\d+$/ && $_ & 0x8000 } @$got;
	$_ = '-' . ($_ & 0x7fff) for grep { /^\d+$/ && $_ & 0x8000 } @$exp;
	diag(sprintf("got [@$got], expected [@$exp]"));
}

sub t
{
	my ( $text, $glyphs, $clusters, $name) = @_;

	$text   = xtr $text;
	$glyphs = xtr $glyphs;
	$text =~ tr/<>=/\x{2067}\x{2066}\x{2069}/;

	$z = $w-> text_shape($text);
	comp($z->glyphs, gmap $glyphs, "$name (glyphs)");
	comp($z->clusters, $clusters, "$name (clusters)");
}

# very minimal support for bidi and X11 core fonts only
sub test_minimal
{
	ok(1, "test minimal");
	no_glyphs '12ABC';

	t('12', '12', [0,1], 'ltr');
	t('12ABC', '12CBA', [0,1,r(4,3,2)], 'rtl');
	t('>AB', 'BA', [r(2,1)], 'bidi');
}

# very minimal support for bidi with xft but no harfbuzz
sub test_glyph_mapping
{
	ok(1, "test glyph mapping without bidi");

        SKIP: {
                glyphs "12ABC";
		skip("text shaping is not available", 1) unless keys %glyphs;

		t('12', '12', [0,1], 'ltr');
		t('12ABC', '12CBA', [0,1,r(4,3,2)], 'rtl');
		t('>AB', 'BA', [r(2,1)], 'bidi');
        }
}

sub test_fribidi
{
	# 627 644
	ok(1, "test bidi");
}

sub test_shaping
{
	my ($found, $with_bidi) = @_;
	ok(1, "test shaping");

	SKIP: {
		skip("no vector fonts", 1) unless $found;
                glyphs "12ABCIJY";
		skip("text shaping is not available", 1) unless keys %glyphs;

		t('12', '12', [0,1], 'eur num ltr');
		t('AB', 'BA', [r(1,0)], 'pure rtl');
		t('12ABC', '12CBA', [0,1,r(4,3,2)], 'simple rtl');
		t('12>ABC', '12CBA', [0,1,r(5,4,3)], 'simple rtl + bidi');
		t('IJ', 'JI', [r(1,0)], 'no ligation');
#		t('JI', 'Y', [r(0)], 'ligation'); XXX requires explicit lang
	}
}

sub run_test
{
	my $unix = shift;

	$w = Prima::DeviceBitmap-> create( type => dbt::Pixmap, width => 32, height => 32);
	my $found = find_font();

	my $z = $w-> text_shape( "1" );
	plan skip_all => "Shaping is not available" if defined $z && $z eq '0';

	if ( $unix ) {
		my %opt = map { $_ => 1 } split ' ', Prima::Application->sys_action('shaper');
		if ( $opt{harfbuzz} && $opt{xft}) {
			test_shaping($found, $opt{fribidi});
		} elsif ( $opt{fribidi}) {
			test_fribidi;
		} elsif ( $opt{xft}) {
			test_glyph_mapping;
		} else {
			test_minimal;
		}
	} else {
		test_shaping($found);
	}
}

sub can_rtl
{
	$w->font(shift);
	my @r = @{ $w->get_font_ranges };
	my $arabic = 0x631;
	my $found_arabic;
	for ( my $i = 0; $i < @r; $i += 2 ) {
		my ( $l, $r ) = @r[$i, $i+1];
		$found_arabic = 1, last if $l <= $arabic && $r >= $arabic;
	}
	return $found_arabic;
}

# try to find font with arabic and hebrew letters
# aim at highest standard, ie ttf/xft + scaling + bidi fonts
sub find_font
{
	return 1 if can_rtl($w->font);

	my $got_rtl;
	my $found;
	my @f = @{$w->fonts};

	# fontconfig fonts
	for my $f ( @f ) {
		next unless $f->{vector};
		next unless $f->{name} =~ /^[A-Z]/;
		next unless can_rtl($f);
		$found = $f;
		$got_rtl = 1;
		goto FOUND;
	}

FOUND:
	$w->font->name($found->{name}) if $found;

	return $got_rtl;
}

if ( Prima::Application-> get_system_info->{apc} == apc::Unix ) {
	if ( @ARGV ) {
		run_test(1);
	} else {
		my %options = Prima::options();
		my @opt = grep { m/^no-(fribidi|harfbuzz|xft)$/ } sort keys %options;
		for ( my $i = 0; $i < 2 ** @opt; $i++) {
			my @xopt = map { "--$_" } @opt[ grep { $i & (1 << $_) } 0..$#opt ];
			my @inc  = map { "-I$_" } @INC;
			for ( split "\n", `$^X @inc $0 @xopt TEST 2>&1`) {
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


