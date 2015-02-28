# $Id$

unless ( $] >= 5.006 && 
	$::application-> get_system_value( sv::CanUTF8_Output)
) {
	print "1..1 support\n";
	skip;
	return 1;
}

print "1..2 support,wrap utf8 text\n";
ok(1);

my $utf8_line;
eval '$utf8_line="line\\x{2028}line"';
my @r = @{$::application-> text_wrap( $utf8_line, 1000, tw::NewLineBreak)};
ok( 2 == @r && $r[0] eq $r[1]);

1;
