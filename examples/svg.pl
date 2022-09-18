use strict;
use warnings;
use lib '.';
use Prima::Drawable::SVG;
use Prima qw(Application);

my $selector = '.a .b';
$selector =~ m/^((?:\.\w+(?:\s+|$))+)$/;
die "'$1'";

my $p = Prima::Drawable::SVG::Parser->new;
my $r = $p->parse(<<SVG);
<svg x=10 y="10%">
<ellipse cx="50%" cy="50%" rx="20" ry="50" />
</svg>
SVG
#<rect x="0" y="0" width="90%" height="80%" />
#use Data::Dumper; print STDERR Dumper(	$r);

my $w = Prima::MainWindow->new(onPaint => sub {
	$_[1]->clear;
	$r->draw($_[1]);
});

run Prima;
