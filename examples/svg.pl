use strict;
use warnings;
use lib '.';
use Prima::Drawable::SVG;
use Prima qw(Application);


my $p = Prima::Drawable::SVG::Parser->new;
my $r = $p->parse(<<SVG);
<svg x=20 y="20%">
<ellipse cx="10%" rx="20" ry="50" />
</svg>
SVG
use Data::Dumper; print STDERR Dumper(	$r);

my $w = Prima::MainWindow->new(onPaint => sub {
	$_[1]->clear;
	$r->draw($_[1], 100, 100);
});

run Prima;
