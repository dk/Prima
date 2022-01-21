use strict;
use warnings;
use utf8;

use Test::More;
use Prima::sys::Test;
use Prima::Application;
use Prima::Drawable::Glyphs;

my $utf8_line = "line\x{2028}line";
my @r;
@r = @{$::application-> text_wrap( $utf8_line, 1000, tw::NewLineBreak)};
is( scalar @r, 2, "wrap utf8 text");
ok( @r, "wrap utf8 text"  );
is( $r[0], $r[1], "wrap utf8 text" );

sub glyph
{
	my ($glyphs, $indexes, $advances, $positions) = @_;
	my $g = Prima::Drawable::Glyphs->new_empty;
	@{$g->[0]} = @$glyphs;
	for ( @$indexes ) {
		$_ = -$_ | to::RTL if $_ < 0 || $_ eq '-0';
	}
	@{$g->[1]} = @$indexes;
	$g->[2] = $g->new_array('advances');
	$g->[3] = $g->new_array('positions');
	@{ $g->[2] } = @$advances;
	@{ $g->[3] } = @$positions if $positions;
	return $g;
}

sub wrap
{
	my $sv = $::application-> text_wrap( @_ );
	return ref($sv) ? @$sv : $sv;
}

sub test
{
	my ( $id, $t, $w, $fl, $g, $len, $chunks, $text, $glyphs, %opt) = @_;
	my $ti = $opt{tabs} // -1;
	my $first = $opt{first} // 0;
	my $tlen  = $opt{len} // -1;
	@r = wrap( $t, $w, $fl | tw::ReturnFirstLineLength, $ti, $first, $tlen, $g);
	is_deeply(\@r, [$len], "$id: length");
	@r = wrap( $t, $w, $fl | tw::ReturnChunks, $ti, $first, $tlen, $g);
	diag("WANTED: @$chunks\nGOT:@r\n") unless is_deeply(\@r, $chunks, "$id: chunks");
	@r = wrap( $t, $w, $fl, $ti, $first, $tlen, $g);
	delete @{$r[-1]}{qw(tildeStart tildeEnd)}
		if $fl && $r[-1] && ref($r[-1]) eq 'HASH';
	is_deeply(\@r, $text, "$id: text");
	@r = wrap( $t, $w, $fl | tw::ReturnGlyphs, $ti, $first, $tlen, $g);
	is( scalar(@r), scalar(@$glyphs), "$id: ".scalar(@$glyphs) . " glyphs");
	delete @{$r[-1]}{qw(tildeStart tildeEnd)}
		if $fl && $r[-1] && ref($r[-1]) eq 'HASH';
	for ( 0 .. $#$glyphs ) {
		next if is_deeply($r[$_], $glyphs->[$_], "$id: G($_)");
		next unless $r[$_] and ref($r[$_]) eq 'Prima::Drawable::Glyphs';
		diag("WANTED: \n" . $glyphs->[$_]->_debug(1));
		diag("GOT: \n" . $r[$_]->_debug(1));
	}
}

my $all = 0;

test( "U1G1 U1G1",
        # 4     0 5        6
	"\x{444} \x{555}\x{666}",
	40,
	tw::SpaceBreak,
	glyph(
		[4,0,5,6], [0,1,2,3,4], [10,11,12,13]
	),

	1,
	[0,1,2,2],
	["\x{444}","\x{555}\x{666}"],
	[
		glyph([4], [0,4], [10]),
		glyph([5,6], [2,3,4], [12,13]),
	],
) if $all;

test( "U1G1 U2G1",
        # 4        5   0 6             8
	"\x{444}\x{555} \x{666}\x{777}\x{888}\x{999}",
	40,
	tw::SpaceBreak,
	glyph(
		[4,5,0,6,8], [0,1,2,3,5,7], [10,11,12,13,14]
	),

	2,
	[0,2,3,4],
	["\x{444}\x{555}","\x{666}\x{777}\x{888}\x{999}"],
	[
		glyph([4,5], [0,1,7], [10,11]),
		glyph([6,8], [3,5,7], [13,14]),
	],
) if $all;

test( "U3G2 U2G2",
        # 1+2                    4+5               0   7+8          9+a
	"\x{111}\x{222}\x{333}\x{444}\x{555}\x{666} \x{777}\x{888}\x{999}\x{aaa}",
	60,
	tw::SpaceBreak,
	glyph(
		[1,2,4,5,0,7,8,9,10],
		[0,0,3,3,6,7,7,9,9,11],
		[10,10,11,11,12,13,13,14,14],
	),

	6,
	[0,6,7,4],
	["\x{111}\x{222}\x{333}\x{444}\x{555}\x{666}", "\x{777}\x{888}\x{999}\x{aaa}"],
	[
		glyph([1,2,4,5], [0,0,3,3,11], [10,10,11,11]),
		glyph([7,8,9,10], [7,7,9,9,11], [13,13,14,14]),
	],
) if $all;

test("tabs",
        # 4     0   5    0  6            0
	"\x{444}\t\x{555}\t\x{666}\x{777}\t",
	170,
	tw::WordBreak | tw::ExpandTabs | tw::CalcTabs,
	glyph(
		[4,0,5,0,6,0], [0,1,2,3,4,6,7], [4,10,5,10,6,10]
	),

	3,
	[0,3,4,3],
	["\x{444}        \x{555}","\x{666}\x{777}        "],
	[
		glyph([4,0,5], [0,1,2,7], [4,80,5]),
		glyph([6,0], [4,6,7], [6,80]),
	],
	tabs => 8,
) if $all;

test( "-U1G1 -U1G1",
        # 4     0 5        6
	"\x{444} \x{555}\x{666}",
	40,
	tw::WordBreak,
	glyph(
		[6,5,0,4], [-3,-2,-1,'-0',4], [10,11,12,13]
	),

	1,
	[0,1,2,2],
	["\x{444}","\x{555}\x{666}"],
	[
 		glyph([4], ['-0',4], [13]),
		glyph([6,5], [-3,-2,4], [10,11]),
	],
) if $all;

test( "-U1G1 -U2G1",
        # 4        5   0 6             8
	"\x{444}\x{555} \x{666}\x{777}\x{888}\x{999}",
	40,
	tw::WordBreak,
	glyph(
		[8,6,0,5,4], [-5,-3,2,-1,'-0',7], [10,11,12,13,14]
	),

	2,
	[0,2,3,4],
	["\x{444}\x{555}","\x{666}\x{777}\x{888}\x{999}"],
	[
		glyph([5,4], [-1,'-0',7], [13,14]),
		glyph([8,6], [-5,-3,7], [10,11]),
	],
) if $all;

test( "-U3G2 -U2G2",
        # 1+2                    4+5               0   7+8          9+a
	"\x{111}\x{222}\x{333}\x{444}\x{555}\x{666} \x{777}\x{888}\x{999}\x{aaa}",
	60,
	tw::WordBreak,
	glyph(
		[10,9,8,7,0,5,4,2,1],
		[-9,-9,-7,-7,-6,-3,-3,'-0','-0',,11],
		[10,10,11,11,12,13,13,14,14],
	),

	6,
	[0,6,7,4],
	["\x{111}\x{222}\x{333}\x{444}\x{555}\x{666}", "\x{777}\x{888}\x{999}\x{aaa}"],
	[
		glyph([5,4,2,1], [-3,-3,'-0','-0',11], [13,13,14,14]),
		glyph([10,9,8,7], [-9,-9,-7,-7,11], [10,10,11,11]),
	],
) if $all;

test("tilde",
	"1 \x{111}~\x{222}",
	-1,
	tw::CollapseTilde | tw::CalcMnemonic | tw::SpaceBreak,
	glyph([1,2,3,0,4],[0..5],[10..14]),

	1,
	[0,1,2,3],
	["1","\x{111}\x{222}", {tildePos => 1, tildeLine => 1, tildeChar => "\x{222}"}],
	[
		glyph([1],[0,5],[10]),
		glyph([3,0,4],[2,3,4,5],[12,13,14]),
		{tildePos => 1, tildeLine => 1, tildeChar => "\x{222}"}
	]
) if $all;

test( "-U3G2/4 -U2G2/8",
        # 1+2                    4+5               0   7+8          9+a
	"\x{111}\x{222}\x{333}\x{444}\x{555}\x{666} \x{777}\x{888}\x{999}\x{aaa}",
	40,
	tw::WordBreak,
	glyph(
		[10,9,8,7,0,5,4,2,1],
		[-9,-9,-7,-7,-6,-3,-3,'-0','-0',11],
		[10,10,11,11,12,13,13,14,14],
	),

	3,
	[3,3,7,2],
	["\x{444}\x{555}\x{666}", "\x{777}\x{888}"],
	[
		glyph([5,4], [-3,-3,11], [13,13]),
		glyph([8,7], [-7,-7,11], [11,11]),
	],
	first => 3,
	len   => 6,
) if $all;

test( "-U3G2/4 -U2G2/8 + tilde",
        # 1+2                    4+5               0   7+8          9+a
	"\x{111}\x{222}\x{333}\x{444}\x{555}\x{666}~\x{777}\x{888}\x{999}\x{aaa}",
	-1,
	tw::CollapseTilde | tw::CalcMnemonic,
	glyph(
		[10,9,8,7,0,5,4,2,1],
		[-9,-9,-7,-7,-6,-3,-3,'-0','-0',11],
		[10,10,11,11,12,13,13,14,14],
	),

	6,
	[3,6],
	[
		"\x{444}\x{555}\x{666}\x{777}\x{888}",
		{tildePos => 6, tildeLine => 0, tildeChar => "\x{777}"}
	],
	[
		glyph([8,7,5,4], [-7,-7,-3,-3,11], [11,11,13,13]),
		{tildePos => 6, tildeLine => 0, tildeChar => "\x{777}"}
	],
	first => 3,
	len   => 6,
) if $all;

$all = 1;

SKIP: {
	skip "libthai not used", 1
		unless $::application->get_system_value(sv::LibThai);
	test("thai breaks",
		"สวัสดีชาวโลก",
		80,
		tw::WordBreak,
		glyph( [(1) x 12], [0..12], [(10) x 12]),

		6,
		[0,6,6,6],
		["สวัสดี","ชาวโลก"],
		[
			glyph( [(1) x 6], [0..5,12],  [(10) x 6]),
			glyph( [(1) x 6], [6..11,12], [(10) x 6]),
		],
	) if $all;
}

done_testing;
