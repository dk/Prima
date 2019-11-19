use strict;
use warnings;
use lib "blib/arch", ".";
use Prima;

my $i = Prima::Icon->new(
	size => [32, 3],
	type => 1,
	conversion => ict::None,
	autoMasking => am::None,
	fillPattern => [(0xF6) x 8],
	linePattern => "\4\1\2\1",
	rop => rop::alpha(rop::SrcCopy, 255),
	rop2 => rop::CopyPut,	
);
$i->clear;
$i->region(Prima::Region->new( box => [ 1, 1, 30, 1 ]));

$i->line(0,1,32,1);
$i->type(im::BW);

my $d = $i->data;
$d =~ s/(.)/sprintf("%02x", ord($1))/ge;
print "$d\n";
